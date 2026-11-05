/*
	Copyright (C) 2003-2005 Daniel Muller, dan at verliba dot cz
	Copyright (C) 2006-2026 Verlihub Team, info at verlihub dot net

	Verlihub is free software; You can redistribute it
	and modify it under the terms of the GNU General
	Public License as published by the Free Software
	Foundation, either version 3 of the license, or at
	your option any later version.

	Verlihub is distributed in the hope that it will be
	useful, but without any warranty, without even the
	implied warranty of merchantability or fitness for
	a particular purpose. See the GNU General Public
	License for more details.

	Please see https://www.gnu.org/licenses/ for a copy
	of the GNU General Public License.
*/

#include "cporttest.h"
#include "csocketapi.h"
#include <string.h>

using namespace std;

namespace nVerliHub {
	namespace nSocket {

static bool MakeAddr(const string &addr, unsigned int port, struct sockaddr_in &dest)
{
	memset(&dest, 0, sizeof(dest));
	dest.sin_family = AF_INET;
	dest.sin_port = htons((unsigned short)port);
	return inet_pton(AF_INET, addr.c_str(), &dest.sin_addr) == 1;
}

static bool BindSource(tSocket sock, const string &bind)
{
	if (bind.empty())
		return true;

	struct sockaddr_in src;
	return MakeAddr(bind, 0, src) && (::bind(sock, (struct sockaddr*)&src, sizeof(src)) == 0);
}

bool PortTestTCP(const string &addr, unsigned int port, const string &bind, int msec)
{
	struct sockaddr_in dest;

	if (!MakeAddr(addr, port, dest))
		return false;

	const tSocket sock = socket(AF_INET, SOCK_STREAM, 0);

	if (!SockValid(sock))
		return false;

	bool good = false;

	if (SockNonBlock(sock) && BindSource(sock, bind)) {
		if (connect(sock, (struct sockaddr*)&dest, sizeof(dest)) == 0)
			good = true;
		else if (SockInProgress() && (SockWaitConnect(sock, msec) == 1))
			good = (SockConnectError(sock) == 0);
	}

	SockClose(sock);
	return good;
}

void PortTestUDP(const string &addr, unsigned int port, const string &bind, const string &text)
{
	struct sockaddr_in dest;

	if (!MakeAddr(addr, port, dest))
		return;

	const tSocket sock = socket(AF_INET, SOCK_DGRAM, 0);

	if (!SockValid(sock))
		return;

	if (BindSource(sock, bind)) {
		for (int count = 0; count < 2; count++) // note: one can be lost
			sendto(sock, text.c_str(), (int)text.size(), 0, (struct sockaddr*)&dest, sizeof(dest));
	}

	SockClose(sock);
}

	}; // namespace nSocket
}; // namespace nVerliHub
