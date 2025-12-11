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

package main

import (
	"flag"
	"log"
	"strings"
	"time"

	"github.com/verlihub/tls-proxy/proxy"
)

var (
	fHost = flag.String("host", ":411", "Space separated list of hosts to listen on")
	fWait = flag.Duration("wait", 600 * time.Millisecond, "Time to wait to detect the protocol")
	fHub = flag.String("hub", "127.0.0.1:411", "Hub address to connect to")
	fHubNet = flag.String("net", "tcp4", "Network type: tcp4, tcp6, tcp, unix")
	fIP = flag.Bool("ip", true, "Send client IP")
	fLog = flag.Bool("log", false, "Enable error logging")
	fCert = flag.String("cert", "hub.crt", "Public .crt file")
	fKey = flag.String("key", "hub.key", "Private .key file")
	fVer = flag.Int("ver", 2, "Minimum required TLS version") // 0 - 1.0, 1 - 1.1, 2 - 1.2, 3 - 1.3
	fBuf = flag.Int("buf", 10, "Buffer size in KB")
	fCertOrg = flag.String("cert-org", "Verlihub", "Certificate organisation")
	fCertMail = flag.String("cert-mail", "verlihub@localhost", "Certificate email")
	fCertHost = flag.String("cert-host", "localhost", "Certificate hostname")
)

func main() {
	flag.Parse()

	if err := run(); err != nil {
		log.Fatal(err)
	}
}

func run() error {
	p, err := proxy.New(proxy.Config {
		HubAddr: *fHub,
		HubNetwork: *fHubNet,
		Hosts: strings.Split(*fHost, " "),
		Cert: *fCert,
		Key: *fKey,
		CertOrg: *fCertOrg,
		CertMail: *fCertMail,
		CertHost: *fCertHost,
		LogErrors: *fLog,
		Wait: *fWait,
		Buffer: *fBuf,
		MinVer: *fVer,
		NoSendIP: !*fIP,
	})

	if err != nil {
		return err
	}

	err = p.Run()

	if err != nil {
		return err
	}

	defer p.Close()
	p.Wait()
	return nil
}

// end of file
