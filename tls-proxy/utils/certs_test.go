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

// $ go test

package certs

import (
	"testing"
	"fmt"
	"os"
)

func TestCerts(t *testing.T) {
	var cert = "./test.crt"
	var key = "./test.key"
	var host = "localhost"
	var org = "Verlihub"
	var mail = "verlihub@localhost"

	hash, err := MakeCerts(cert, key, host, org, mail)

	if err != nil {
		t.Error(err)
	}

	os.Remove(cert)
	os.Remove(key)

	fmt.Println("Test passed with keyprint:", hash)
}

// end of file
