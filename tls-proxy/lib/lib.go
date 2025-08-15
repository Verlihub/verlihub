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

/*
#include "dcproxy_types.h"
*/

import "C"

import (
	"strings"
	"time"
	"unsafe"

	"github.com/verlihub/tls-proxy/dcproxy"
)

var lastErr error

func setLastErr(err error) {
	lastErr = err
}

//export DCLastError
func DCLastError() *C.char {
	if lastErr == nil {
		return nil
	}

	e := lastErr.Error()
	return C.CString(e)
}

var curProxy *dcproxy.Proxy

//export NewDCProxyConfig
func NewDCProxyConfig() *C.DCProxyConfig {
	const sz = C.size_t(unsafe.Sizeof(C.DCProxyConfig{}))
	c := (*C.DCProxyConfig)(C.malloc(sz))
	*c = C.DCProxyConfig{} // zero memory
	return c
}

//export DCProxyStart
func DCProxyStart(conf *C.DCProxyConfig) C.int {
	c := dcproxy.Config{
		HubAddr: C.GoString(conf.HubAddr),
		HubNetwork: C.GoString(conf.HubNetwork),
		Hosts: strings.Split(C.GoString(conf.Hosts), ","),
		Cert: C.GoString(conf.Cert),
		Key: C.GoString(conf.Key),
		CertOrg: C.GoString(conf.CertOrg),
		CertHost: C.GoString(conf.CertHost),
		PProf: C.GoString(conf.PProf),
		Metrics: C.GoString(conf.Metrics),
		LogErrors: bool(conf.LogErrors),
		Wait: time.Duration(conf.Wait) * time.Millisecond,
		Buffer: int(conf.Buffer),
		MinVer: int(conf.MinVer),
		NoSendIP: bool(conf.NoSendIP),
	}

	p, err := dcproxy.New(c)

	if err != nil {
		setLastErr(err)
		return 0
	}

	err = p.Run()

	if err != nil {
		setLastErr(err)
		return 0
	}

	curProxy = p
	return 1
}

//export DCProxyStop
func DCProxyStop() {
	if curProxy == nil {
		return
	}

	_ = curProxy.Close()
	curProxy = nil
}

func main() {}

// end of file
