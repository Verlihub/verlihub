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

#ifndef PROXY_TYPES_H
#define PROXY_TYPES_H

#include <stdbool.h>

typedef struct VH_ProxyConfig {
	const char* HubAddr;
	const char* HubNetwork;
	const char* Hosts; // space separated
	const char* Cert;
	const char* Key;
	const char* CertOrg;
	const char* CertMail;
	const char* CertHost;
	bool LogErrors;
	int Wait; // ms
	int Buffer; // kb
	int MinVer;
	bool NoSendIP;
} VH_ProxyConfig;

#endif // PROXY_TYPES_H
