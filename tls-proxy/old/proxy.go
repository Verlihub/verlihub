/*
	Copyright (C) 2019-2021 Dexo, dexo at verlihub dot net
	Copyright (C) 2019-2025 Verlihub Team, info at verlihub dot net

	This is free software; You can redistribute it
	and modify it under the terms of the GNU General
	Public License as published by the Free Software
	Foundation, either version 3 of the license, or at
	your option any later version.

	It is distributed in the hope that it will be
	useful, but without any warranty, without even the
	implied warranty of merchantability or fitness for
	a particular purpose. See the GNU General Public
	License for more details.

	Please see https://www.gnu.org/licenses/ for a copy
	of the GNU General Public License.
*/

// TLS Proxy 0.0.3.0

package main

import (
	"C"
	"bytes"
	"crypto/tls"
	"flag"
	"io"
	"log"
	"net"
	"strings"
	"sync"
	"time"
)

var (
	fHost = flag.String("host", ":411", "Comma separated list of hosts to listen on")
	fWait = flag.Duration("wait", 650*time.Millisecond, "Time to wait to detect the protocol")
	fHub = flag.String("hub", "127.0.0.1:411", "Hub address to connect to")
	fIP = flag.Bool("ip", true, "Send client IP")
	fLog = flag.Bool("log", false, "Enable connection logging")
	fCert = flag.String("cert", "hub.crt", "TLS .crt file")
	fKey = flag.String("key", "hub.key", "TLS .key file")
	fVer = flag.Int("ver", 2, "Minimum required TLS version") // 0 - 1.0, 1 - 1.2, 2 - 1.2, 3 - 1.3
	fBuf = flag.Int("buf", 10, "Buffer size in KB")
)

func main() {
	flag.Parse()

	if err := run(); err != nil {
		log.Fatal(err)
	}
}

var tlsConfig *tls.Config

func run() error {
	if *fCert != "" && *fKey != "" {
		log.Println("Using certificates:", *fCert, *fKey)
		var err error
		var minver uint16

		switch *fVer {
			case 0:
				minver = tls.VersionTLS10
			case 1:
				minver = tls.VersionTLS11
			case 2:
				minver = tls.VersionTLS12
			default:
				minver = tls.VersionTLS13
		}

		tlsConfig = &tls.Config{NextProtos: []string{"nmdc"}, MinVersion: minver}
		tlsConfig.Certificates = make([]tls.Certificate, 1)
		tlsConfig.Certificates[0], err = tls.LoadX509KeyPair(*fCert, *fKey)

		if err != nil {
			return err
		}

	} else {
		log.Println("No certificates, TLS disabled")
	}

	hosts := strings.Split(*fHost, ",")
	var lis []net.Listener

	defer func() {
		for _, l := range lis {
			_ = l.Close()
		}
	}()

	for _, host := range hosts {
		l, err := net.Listen("tcp4", host)

		if err != nil {
			return err
		}

		lis = append(lis, l)
	}

	var wg sync.WaitGroup

	for i, l := range lis {
		wg.Add(1)
		l := l
		log.Println("Proxying", hosts[i], "to", *fHub)

		go func() {
			defer wg.Done()
			acceptOn(l)
		}()
	}

	wg.Wait()
	return nil
}

func acceptOn(l net.Listener) {
	for {
		c, err := l.Accept()

		if err != nil {
			if *fLog {
				log.Println(err)
			}

			continue
		}

		go func() {
			err := serve(c)

			if err != nil && err != io.EOF {
				if *fLog {
					log.Println(c.RemoteAddr(), err)
				}
			}
		}()
	}
}

type timeoutErr interface {
	Timeout() bool
}

func serve(c net.Conn) error {
	defer func() {
		_ = c.Close()
	}()

	buf := make([]byte, 1024)
	i := copy(buf, "$MyIP ")
	i += copy(buf[i:], c.RemoteAddr().(*net.TCPAddr).IP.String())
	i += copy(buf[i:], " 0.0|")

	if tlsConfig == nil || *fWait <= 0 { // no auto detection
		return writeAndStream(buf[:i], c, i)
	}

	start := time.Now()
	err := c.SetReadDeadline(start.Add(*fWait))

	if err != nil {
		return err
	}

	n, err := c.Read(buf[i:])
	_ = c.SetReadDeadline(time.Time{})

	if e, ok := err.(timeoutErr); ok && e.Timeout() { // has to be plain nmdc
		return writeAndStream(buf[:i], c, i)
	}

	if err != nil {
		return err
	}

	buf = buf[:i+n]

	if n >= 2 && buf[i] == 0x16 && buf[i+1] == 0x03 {
		tlsBuf := buf[i:]
		c = &multiReadConn{r: io.MultiReader(bytes.NewReader(tlsBuf), c), Conn: c}
		tc := tls.Server(c, tlsConfig) // tls handshake
		defer tc.Close()
		err = tc.Handshake()

		if err != nil {
			return err
		}

		buf[i-4] = '1' // set version
		state := tc.ConnectionState()

		switch state.Version {
			case tls.VersionTLS13:
				buf[i-2] = '3'
			case tls.VersionTLS12:
				buf[i-2] = '2'
			case tls.VersionTLS11:
				buf[i-2] = '1'
		}

		buf = buf[:i]
		return writeAndStream(buf, tc, i)
	}

	return writeAndStream(buf, c, i)
}

type multiReadConn struct {
	r io.Reader
	net.Conn
}

func (c *multiReadConn) Read(b []byte) (int, error) {
	return c.r.Read(b)
}

func dialHub() (net.Conn, error) {
	return net.Dial("tcp4", *fHub)
}

func writeAndStream(p []byte, c io.ReadWriteCloser, i int) error {
	h, err := dialHub()

	if err != nil {
		return err
	}

	defer h.Close()

	if !*fIP {
		p = p[i:]
	}

	if len(p) > 0 {
		_, err = h.Write(p)

		if err != nil {
			return err
		}
	}

	return stream(c, h)
}

func stream(c, h io.ReadWriteCloser) error {
	closeBoth := func() {
		_ = c.Close()
		_ = h.Close()
	}

	defer closeBoth()

	go func() {
		defer closeBoth()
		_, _ = copyBuffer(h, c)
	}()

	_, _ = copyBuffer(c, h)
	return nil
}

// this was copied from io package and modified to add instrumentation
func copyBuffer(dst io.Writer, src io.Reader) (written int64, err error) {
	size := *fBuf * 1024

	if l, ok := src.(*io.LimitedReader); ok && int64(size) > l.N {
		if l.N < 1 {
			size = 1
		} else {
			size = int(l.N)
		}
	}

	buf := make([]byte, size)

	for {
		nr, er := src.Read(buf)

		if nr > 0 {
			nw, ew := dst.Write(buf[0:nr])

			if nw > 0 {
				written += int64(nw)
			}

			if ew != nil {
				err = ew
				break
			}

			if nr != nw {
				err = io.ErrShortWrite
				break
			}
		}

		if er != nil {
			if er != io.EOF {
				err = er
			}

			break
		}
	}

	return written, err
}

// end of file