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

package certs

import (
	"crypto/sha256"
	"crypto/ecdsa"
	"crypto/ed25519"
	"crypto/elliptic"
	"crypto/rand"
	"crypto/rsa"
	"crypto/x509"
	"crypto/x509/pkix"
	"encoding/pem"
	"encoding/base32"
	"math/big"
	"net"
	"os"
	"io"
	"io/ioutil"
	"time"
	"strings"
	"errors"
)

var base32enc = base32.StdEncoding.WithPadding(base32.NoPadding)

func fromBytes(data []byte) string {
	const pref = "SHA256/"
	hash := sha256.Sum256(data)
	out := make([]byte, len(pref) + base32enc.EncodedLen(len(hash)))
	copy(out, pref)
	base32enc.Encode(out[len(pref):], hash[:])
	return string(out)
}

func fromFile(read io.Reader) ([]string, error) {
	hash, err := ioutil.ReadAll(read)

	if err != nil {
		return nil, err
	}

	var out []string

	for {
		data, rest := pem.Decode(hash)

		if data == nil {
			break
		}

		hash = rest

		if data.Type != "CERTIFICATE" {
			continue
		}

		out = append(out, fromBytes(data.Bytes))
	}

	if len(out) == 0 {
		return nil, errors.New("Error decoding PEM data")
	}

	return out, nil
}

func keyType(priv any) any {
	switch key := priv.(type) {
		case *rsa.PrivateKey:
			return &key.PublicKey
		case *ecdsa.PrivateKey:
			return &key.PublicKey
		case ed25519.PrivateKey:
			return key.Public().(ed25519.PublicKey)
		default:
			return nil
	}
}

func MakeCerts(cert, key, host, org, mail string) ([]string, error) {
	if len(cert) == 0 {
		return nil, errors.New("Missing certificate file name")
	}

	if len(key) == 0 {
		return nil, errors.New("Missing private key file name")
	}

	if len(host) == 0 {
		return nil, errors.New("Missing required host name")
	}

	var curve = "" // defaults
	var bits = 2048
	var ised = false
	var isca = false
	var valid = 5 * 365 * 24 * time.Hour // 5 years

	var priv any
	var err error

	switch curve {
		case "":
			if ised {
				_, priv, err = ed25519.GenerateKey(rand.Reader)
			} else {
				priv, err = rsa.GenerateKey(rand.Reader, bits)
			}
		case "P224":
			priv, err = ecdsa.GenerateKey(elliptic.P224(), rand.Reader)
		case "P256":
			priv, err = ecdsa.GenerateKey(elliptic.P256(), rand.Reader)
		case "P384":
			priv, err = ecdsa.GenerateKey(elliptic.P384(), rand.Reader)
		case "P521":
			priv, err = ecdsa.GenerateKey(elliptic.P521(), rand.Reader)
		default:
			return nil, errors.New("Unknown elliptic curve: " + curve)
	}

	if err != nil {
		return nil, errors.New("Failed to generate private key: " + err.Error())
	}

	limit := new(big.Int).Lsh(big.NewInt(1), 128)
	serial, err := rand.Int(rand.Reader, limit)

	if err != nil {
		return nil, errors.New("Failed to generate serial number: " + err.Error())
	}

	usage := x509.KeyUsageDigitalSignature

	if _, isrsa := priv.(*rsa.PrivateKey); isrsa {
		usage |= x509.KeyUsageKeyEncipherment
	}

	templ := x509.Certificate{
		SerialNumber: serial,
		Subject: pkix.Name{CommonName: org, Organization: []string{org}},
		EmailAddresses: []string{mail},
		NotBefore: time.Now(),
		NotAfter: time.Now().Add(valid),
		KeyUsage: usage,
		ExtKeyUsage: []x509.ExtKeyUsage{x509.ExtKeyUsageServerAuth},
		BasicConstraintsValid: true,
	}

	hosts := strings.Split(host, ",")

	for _, dns := range hosts {
		if ip := net.ParseIP(dns); ip != nil {
			templ.IPAddresses = append(templ.IPAddresses, ip)
		} else {
			templ.DNSNames = append(templ.DNSNames, dns)
		}
	}

	if isca {
		templ.IsCA = true
		templ.KeyUsage |= x509.KeyUsageCertSign
	}

	der, err := x509.CreateCertificate(rand.Reader, &templ, &templ, keyType(priv), priv)

	if err != nil {
		return nil, errors.New("Failed to create certificate: " + err.Error())
	}

	certfile, err := os.Create(cert)

	if err != nil {
		return nil, errors.New("Failed to open " + cert + " for writing: " + err.Error())
	}

	if err := pem.Encode(certfile, &pem.Block{Type: "CERTIFICATE", Bytes: der}); err != nil {
		certfile.Close()
		return nil, errors.New("Failed to write to " + cert + ": " + err.Error())
	}

	certfile.Seek(0, io.SeekStart)
	var read io.Reader = certfile
	hash, err := fromFile(read)

	if err != nil {
		certfile.Close()
		return nil, err
	}

	if err := certfile.Close(); err != nil {
		return nil, errors.New("Failed to close " + cert + ": " + err.Error())
	}

	keyfile, err := os.OpenFile(key, os.O_WRONLY | os.O_CREATE | os.O_TRUNC, 0600)

	if err != nil {
		return nil, errors.New("Failed to open " + key + " for writing: " + err.Error())
	}

	pk, err := x509.MarshalPKCS8PrivateKey(priv)

	if err != nil {
		keyfile.Close()
		return nil, errors.New("Failed to marshal private key: " + err.Error())
	}

	if err := pem.Encode(keyfile, &pem.Block{Type: "PRIVATE KEY", Bytes: pk}); err != nil {
		keyfile.Close()
		return nil, errors.New("Failed to write to " + key + ": " + err.Error())
	}

	if err := keyfile.Close(); err != nil {
		return nil, errors.New("Failed to close " + key + ": " + err.Error())
	}

	return hash, nil
}

// end of file
