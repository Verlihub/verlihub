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

// +build metrics

package metrics

import (
	"net/http"

	"github.com/prometheus/client_golang/prometheus"
	"github.com/prometheus/client_golang/prometheus/promauto"
	"github.com/prometheus/client_golang/prometheus/promhttp"
)

func listenAndServe(addr string) error {
	return http.ListenAndServe(addr, promhttp.Handler())
}

func init() {
	ConnAccepted = promauto.NewCounter(prometheus.CounterOpts{
		Name: "dc_conn_accepted",
		Help: "Total number of accepted connections",
	})

	ConnError = promauto.NewCounter(prometheus.CounterOpts{
		Name: "dc_conn_error",
		Help: "Total number of connections failed with an error",
	})

	ConnOpen = promauto.NewGauge(prometheus.GaugeOpts{
		Name: "dc_conn_open",
		Help: "Number of open connections",
	})

	ConnInsecure = promauto.NewCounter(prometheus.CounterOpts{
		Name: "dc_conn_insecure",
		Help: "Total number of insecure connections",
	})

	ConnOpenInsecure = promauto.NewGauge(prometheus.GaugeOpts{
		Name: "dc_conn_insecure_open",
		Help: "Number of open insecure connections",
	})

	ConnTLS = promauto.NewCounter(prometheus.CounterOpts{
		Name: "dc_conn_tls",
		Help: "Total number of TLS connections",
	})

	ConnOpenTLS = promauto.NewGauge(prometheus.GaugeOpts{
		Name: "dc_conn_tls_open",
		Help: "Number of open TLS connections",
	})

	ConnTLSHandshake = promauto.NewHistogram(prometheus.HistogramOpts{
		Name: "dc_conn_tls_handshake_sec",
		Help: "Time spent on TLS handshake",
	})

	ConnRx = promauto.NewCounter(prometheus.CounterOpts{
		Name: "dc_conn_rx_bytes",
		Help: "Total bytes received from client",
	})

	ConnTx = promauto.NewCounter(prometheus.CounterOpts{
		Name: "dc_conn_tx_bytes",
		Help: "Total bytes sent to client",
	})
}

// end of file
