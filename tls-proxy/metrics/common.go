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

package metrics

type Counter interface {
	Add(v float64)
}

type Gauge interface {
	Counter
}

type Observer interface {
	Observe(v float64)
}

var (
	_ Counter = noop{}
	_ Gauge = noop{}
	_ Observer = noop{}
)

type noop struct{}

func (noop) Add(v float64) {}
func (noop) Observe(v float64) {}

func ListenAndServe(addr string) error {
	return listenAndServe(addr)
}

var (
	ConnAccepted Counter = noop{}
	ConnError Counter = noop{}
	ConnOpen Gauge = noop{}
	ConnInsecure Counter = noop{}
	ConnOpenInsecure Gauge = noop{}
	ConnTLS Counter = noop{}
	ConnOpenTLS Gauge = noop{}
	ConnTLSHandshake Observer = noop{}
	ConnRx Counter = noop{}
	ConnTx Counter = noop{}
)

// end of file
