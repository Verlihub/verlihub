/*
	Copyright (C) 2019-2021 Dexo, dexo at verlihub dot net
	Copyright (C) 2019-2026 Verlihub Team, info at verlihub dot net

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

package proxy

import (
	"bytes"
	"crypto/tls"
	"crypto/x509"
	"encoding/pem"
	"io"
	"io/ioutil"
	"log"
	"strings"
	"strconv"
	"net"
	"os"
	"os/signal"
	"syscall"
	//"runtime"
	"sync"
	"time"

	"github.com/verlihub/tls-proxy/utils"
)

func init() {
	log.SetFlags(log.Lshortfile | log.Ldate | log.Ltime)
	log.SetOutput(os.Stdout)
	//runtime.GOMAXPROCS(1)
}

type Config struct {
	HubAddr string
	HubNetwork string // verlihub supports only tcp4
	Hosts []string
	Cert string
	Key string
	CertOrg string
	CertMail string
	CertHost string
	LogErrors bool
	Wait time.Duration
	Buffer int
	MinVer int
	NoSendIP bool // must be enabled in verlihub
}

func (p *Proxy) SetBuf(val int) {
	log.Println("Setting buffer config to:", strconv.Itoa(val))
	p.c.Buffer = val
}

func (p *Proxy) SetWait(val int) {
	log.Println("Setting wait config to:", strconv.Itoa(val))
	p.c.Wait = time.Duration(val) * time.Millisecond
}

func (p *Proxy) SetLog(val bool) {
	log.Println("Setting log config to:", strconv.FormatBool(val))
	p.c.LogErrors = val
}

type Proxy struct {
	c Config
	tls *tls.Config
	wg sync.WaitGroup
	lis []net.Listener
}

func New(c Config) (*Proxy, error) {
	if c.HubAddr == "" {
		c.HubNetwork = "tcp4"
		c.HubAddr = "127.0.0.1:411"

	} else if c.HubNetwork == "" {
		c.HubNetwork = "tcp4"
	}

	if c.Buffer == 0 {
		c.Buffer = 10
	}

	p := &Proxy{c: c}

	if c.Cert == "" {
		c.Cert = "hub.crt"
	}

	if c.Key == "" {
		c.Key = "hub.key"
	}

	var need = false

	if _, err := os.Stat(c.Cert); os.IsNotExist(err) {
		log.Println("Certificate not found:", c.Cert)
		need = true

	} else if _, err := os.Stat(c.Key); os.IsNotExist(err) {
		log.Println("Key not found:", c.Key)
		need = true

	} else {
		log.Println("Validating certificate:", c.Cert)
		data, err := ioutil.ReadFile(c.Cert)

		if err != nil {
			log.Println("Error reading file:", err.Error())
			need = true

		} else {
			block, _ := pem.Decode([]byte(data))

			if block == nil {
				log.Println("Error decoding PEM data")
				need = true

			} else {
				cert, err := x509.ParseCertificate(block.Bytes)

				if err != nil {
					log.Println("Error parsing certificate:", err.Error())
					need = true

				} else if time.Now().After(cert.NotAfter) { // Add(time.Day * 7)
					log.Println("Certificate has expired:", cert.NotAfter.String())
					need = true

				} else {
					log.Println("Certificate is valid:", cert.NotAfter.String())
				}
			}
		}
	}

	if need {
		if c.CertHost == "" {
			c.CertHost = "localhost"
		}

		if c.CertOrg == "" {
			c.CertOrg = "Verlihub"
		}

		if c.CertMail == "" {
			c.CertMail = "verlihub@localhost"
		}

		log.Println("Generating certificates:", c.Cert, c.Key)
		err := certs.MakeCerts(c.Cert, c.Key, c.CertHost, c.CertOrg, c.CertMail)

		if err != nil {
			return nil, err
		}
	}

	log.Println("Using certificates:", c.Cert, c.Key)
	var err error
	var minver uint16

	switch c.MinVer {
		case 0:
			minver = tls.VersionTLS10
		case 1:
			minver = tls.VersionTLS11
		case 2:
			minver = tls.VersionTLS12
		default:
			minver = tls.VersionTLS13
	}

	tlsConfig := &tls.Config{NextProtos: []string{"nmdc"}, MinVersion: minver}
	tlsConfig.Certificates = make([]tls.Certificate, 1)
	tlsConfig.Certificates[0], err = tls.LoadX509KeyPair(c.Cert, c.Key)

	if err != nil {
		return nil, err
	}

	p.tls = tlsConfig
	return p, nil
}

func (p *Proxy) Wait() {
	p.wg.Wait()
}

func (p *Proxy) Close() error {
	log.Println("Stopping TLS proxy")

	for _, l := range p.lis {
		err := l.Close()

		if err != nil {
			log.Println(err)
		}
	}

	p.lis = nil
	return nil
}

func (p *Proxy) Run() error {
	signal.Ignore(syscall.SIGPIPE)
	log.Println("Starting TLS proxy:", strings.Join(p.c.Hosts[:], " "), "->", p.c.HubAddr)

	for _, host := range p.c.Hosts {
		l, err := net.Listen("tcp4", host)

		if err != nil {
			_ = p.Close()
			return err
		}

		p.lis = append(p.lis, l)
	}

	for i, l := range p.lis {
		p.wg.Add(1)
		l := l
		log.Println("Proxying", p.c.Hosts[i], "->", p.c.HubAddr)

		go func() {
			defer p.wg.Done()
			p.acceptOn(l)
		}()
	}

	//p.Wait()
	return nil
}

func (p *Proxy) acceptOn(l net.Listener) {
	for {
		c, err := l.Accept()

		if err != nil {
			if p.c.LogErrors {
				log.Println(err)
			}

			continue
		}

		go func() {
			err := p.serve(c)

			if err != nil && err != io.EOF {
				if p.c.LogErrors {
					log.Println(c.RemoteAddr(), err)
				}
			}
		}()
	}
}

type timeoutErr interface {
	Timeout() bool
}

func (p *Proxy) serve(c net.Conn) error {
	defer func() {
		_ = c.Close()
	}()

	buf := make([]byte, 1024)
	i := copy(buf, "$MyIP ")
	i += copy(buf[i:], c.RemoteAddr().(*net.TCPAddr).IP.String())
	i += copy(buf[i:], " 0.0|")

	if p.tls == nil || p.c.Wait <= 0 { // no auto detection
		return p.writeAndStream(buf[:i], c, i)
	}

	err := c.SetReadDeadline(time.Now().Add(p.c.Wait))

	if err != nil {
		return err
	}

	n, err := c.Read(buf[i:])
	_ = c.SetReadDeadline(time.Time{})

	if e, ok := err.(timeoutErr); ok && e.Timeout() { // has to be plain nmdc
		return p.writeAndStream(buf[:i], c, i)
	}

	if err != nil {
		return err
	}

	buf = buf[:i+n]

	if n >= 2 && buf[i] == 0x16 && buf[i+1] == 0x03 {
		tlsBuf := buf[i:]
		c = &multiReadConn{r: io.MultiReader(bytes.NewReader(tlsBuf), c), Conn: c}
		tc := tls.Server(c, p.tls) // tls handshake
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
		return p.writeAndStream(buf, tc, i)
	}

	return p.writeAndStream(buf, c, i)
}

type multiReadConn struct {
	r io.Reader
	net.Conn
}

func (c *multiReadConn) Read(b []byte) (int, error) {
	return c.r.Read(b)
}

func (p *Proxy) dialHub() (net.Conn, error) {
	return net.Dial(p.c.HubNetwork, p.c.HubAddr)
}

func (p *Proxy) writeAndStream(b []byte, c io.ReadWriteCloser, i int) error {
	h, err := p.dialHub()

	if err != nil {
		return err
	}

	defer h.Close()

	if p.c.NoSendIP {
		b = b[i:]
	}

	if len(b) > 0 {
		_, err = h.Write(b)

		if err != nil {
			return err
		}
	}

	return p.stream(c, h)
}

func (p *Proxy) stream(c, h io.ReadWriteCloser) error {
	closeBoth := func() {
		_ = c.Close()
		_ = h.Close()
	}

	defer closeBoth()

	go func() {
		defer closeBoth()
		_, _ = p.copyBuffer(h, c)
	}()

	_, _ = p.copyBuffer(c, h)
	return nil
}

// this was copied from io package and modified to add instrumentation
func (p *Proxy) copyBuffer(dst io.Writer, src io.Reader) (written int64, err error) {
	size := p.c.Buffer * 1024

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

			if nw < 0 || nr < nw {
				nw = 0

				if ew == nil {
					ew = io.ErrClosedPipe // note: fake
				}
			}

			written += int64(nw)

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
