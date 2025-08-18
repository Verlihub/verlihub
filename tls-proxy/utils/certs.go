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

package certs

import (
	"crypto/rand"
	"crypto/rsa"
	"crypto/x509"
	"crypto/x509/pkix"
	"encoding/pem"
	"errors"
	"fmt"
	"math/big"
	"net"
	"time"
)

func generateCerts(host, org, mail string) (cert, key []byte, _ error) {
	// generate a new key-pair
	rootKey, err := rsa.GenerateKey(rand.Reader, 2048)

	if err != nil {
		return nil, nil, err
	}

	rootCertTmpl, err := certTemplate(org)

	if err != nil {
		return nil, nil, err
	}

	// describe what the certificate will be used for
	rootCertTmpl.IsCA = true
	rootCertTmpl.KeyUsage = x509.KeyUsageCertSign | x509.KeyUsageDigitalSignature
	rootCertTmpl.ExtKeyUsage = []x509.ExtKeyUsage{x509.ExtKeyUsageServerAuth, x509.ExtKeyUsageClientAuth}

	if ip := net.ParseIP(host); ip != nil {
		rootCertTmpl.IPAddresses = []net.IP{ip}
	} else {
		rootCertTmpl.DNSNames = []string{host}
	}

	rootCertTmpl.EmailAddresses = []string{mail}
	_, rootCertPEM, err := createCert(rootCertTmpl, rootCertTmpl, &rootKey.PublicKey, rootKey)

	if err != nil {
		return nil, nil, fmt.Errorf("Error creating certificate: %v", err)
	}

	// pem encode the private key
	rootKeyPEM := pem.EncodeToMemory(&pem.Block{
		Type: "RSA PRIVATE KEY", Bytes: x509.MarshalPKCS1PrivateKey(rootKey),
	})

	return rootCertPEM, rootKeyPEM, nil
}

// helper function to create a cert template with a serial number and other required fields
func certTemplate(org string) (*x509.Certificate, error) {
	// generate a random serial number, a real cert authority would have some logic behind this
	serialNumberLimit := new(big.Int).Lsh(big.NewInt(1), 128)
	serialNumber, err := rand.Int(rand.Reader, serialNumberLimit)

	if err != nil {
		return nil, errors.New("Failed to generate serial number: " + err.Error())
	}

	tmpl := x509.Certificate{
		SerialNumber: serialNumber,
		Subject: pkix.Name{Organization: []string{org}},
		SignatureAlgorithm: x509.SHA256WithRSA,
		NotBefore: time.Now(),
		NotAfter: time.Now().Add(time.Hour * 24 * 356 * 5), // 5 years
		BasicConstraintsValid: true,
	}

	return &tmpl, nil
}

func createCert(template, parent *x509.Certificate, pub interface{}, parentPriv interface{}) (cert *x509.Certificate, certPEM []byte, err error) {
	certDER, err := x509.CreateCertificate(rand.Reader, template, parent, pub, parentPriv)

	if err != nil {
		return
	}

	// parse the resulting certificate so we can use it again
	cert, err = x509.ParseCertificate(certDER)

	if err != nil {
		return
	}

	// pem encode the certificate, this is a standard tls encoding
	b := pem.Block{Type: "CERTIFICATE", Bytes: certDER}
	certPEM = pem.EncodeToMemory(&b)
	return
}

// end of file
