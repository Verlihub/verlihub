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

#include "csocketapi.h"
#include <cstring>

#ifndef _WIN32
	#include <cerrno>
	#include <fcntl.h>
	#include <unistd.h>
	#include <sys/time.h>
#endif

using namespace std;

namespace nVerliHub {
	namespace nSocket {

#ifdef _WIN32

bool SockStartup()
{
	WSADATA data;
	return (WSAStartup(MAKEWORD(2, 2), &data) == 0);
}

void SockCleanup()
{
	WSACleanup();
}

int SockError()
{
	return WSAGetLastError();
}

string SockErrorText(int err)
{
	char buf[256];
	const DWORD len = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, err, 0, buf, sizeof(buf), NULL);
	string res(buf, len);

	while (res.size() && ((res.back() == '\n') || (res.back() == '\r') || (res.back() == ' ') || (res.back() == '.'))) // note: windows ends message with dot and new line
		res.pop_back();

	return (res.size() ? res : ("Socket error " + to_string(err)));
}

bool SockTryAgain()
{
	const int err = WSAGetLastError();
	return ((err == WSAEWOULDBLOCK) || (err == WSAEINTR));
}

bool SockInterrupted()
{
	return (WSAGetLastError() == WSAEINTR);
}

int SockClose(tSocket sock)
{
	return closesocket(sock);
}

bool SockNonBlock(tSocket sock, bool on)
{
	u_long val = (on ? 1 : 0);
	return (ioctlsocket(sock, FIONBIO, &val) == 0);
}

bool SockIsTimeout(int err)
{
	return ((err == WSAETIMEDOUT) || (err == WSAEWOULDBLOCK)); // note: receive timeout on blocking socket is timed out
}

bool SockIsRefused(int err)
{
	return (err == WSAECONNREFUSED);
}

bool SockInProgress()
{
	return (WSAGetLastError() == WSAEWOULDBLOCK);
}

int SockWaitConnect(tSocket sock, int msec)
{
	fd_set wset, eset;
	FD_ZERO(&wset);
	FD_ZERO(&eset);
	FD_SET(sock, &wset);
	FD_SET(sock, &eset); // note: failed connect is reported as exception
	struct timeval val;
	val.tv_sec = msec / 1000;
	val.tv_usec = (msec % 1000) * 1000;
	const int res = select(0, NULL, &wset, &eset, &val); // note: first argument is ignored on windows
	return ((res > 0) ? 1 : res);
}

int SockConnectError(tSocket sock)
{
	int err = 0;
	int len = sizeof(err);

	if (getsockopt(sock, SOL_SOCKET, SO_ERROR, (char*)&err, &len) != 0)
		return WSAGetLastError();

	return err;
}

int SockSetOpt(tSocket sock, int level, int name, const void *val, int len)
{
	return setsockopt(sock, level, name, (const char*)val, len); // note: windows takes char pointer
}

int SockSetTimeout(tSocket sock, int name, unsigned int msec)
{
	const DWORD val = msec;
	return setsockopt(sock, SOL_SOCKET, name, (const char*)&val, sizeof(val));
}

int SockPoll(struct pollfd *fds, unsigned long count, int msec)
{
	return WSAPoll(fds, count, msec);
}

bool SockPair(tSocket pair[2])
{
	// note: listen on local address, connect to it and accept, then check that we accepted our own connection
	pair[0] = pair[1] = INVALID_SOCKET;
	tSocket listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (listener == INVALID_SOCKET)
		return false;

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	addr.sin_port = 0; // note: any free port
	int len = sizeof(addr);
	bool good = (bind(listener, (struct sockaddr*)&addr, sizeof(addr)) == 0) && (getsockname(listener, (struct sockaddr*)&addr, &len) == 0) && (listen(listener, 1) == 0);

	if (good) {
		pair[1] = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		good = (pair[1] != INVALID_SOCKET) && (connect(pair[1], (struct sockaddr*)&addr, sizeof(addr)) == 0);
	}

	if (good) {
		pair[0] = accept(listener, NULL, NULL);
		good = (pair[0] != INVALID_SOCKET);
	}

	if (good) { // note: make sure nobody else connected to our port meanwhile
		struct sockaddr_in local, peer;
		int llen = sizeof(local), plen = sizeof(peer);
		good = (getsockname(pair[1], (struct sockaddr*)&local, &llen) == 0) && (getpeername(pair[0], (struct sockaddr*)&peer, &plen) == 0) && (local.sin_port == peer.sin_port) && (local.sin_addr.s_addr == peer.sin_addr.s_addr);
	}

	if (good) { // note: wakeup byte must not wait in nagle buffer
		const int yes = 1;
		setsockopt(pair[1], IPPROTO_TCP, TCP_NODELAY, (const char*)&yes, sizeof(yes));
		good = SockNonBlock(pair[0]) && SockNonBlock(pair[1]);
	}

	closesocket(listener);

	if (!good) {
		if (pair[0] != INVALID_SOCKET)
			closesocket(pair[0]);

		if (pair[1] != INVALID_SOCKET)
			closesocket(pair[1]);

		pair[0] = pair[1] = INVALID_SOCKET;
	}

	return good;
}

#else

bool SockStartup()
{
	return true;
}

void SockCleanup()
{}

int SockError()
{
	return errno;
}

string SockErrorText(int err)
{
	return strerror(err);
}

bool SockTryAgain()
{
	return ((errno == EAGAIN) || (errno == EWOULDBLOCK) || (errno == EINTR));
}

bool SockInterrupted()
{
	return (errno == EINTR);
}

int SockClose(tSocket sock)
{
	return ::close(sock);
}

bool SockNonBlock(tSocket sock, bool on)
{
	const int flags = fcntl(sock, F_GETFL, 0);
	return ((flags >= 0) && (fcntl(sock, F_SETFL, (on ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK))) == 0));
}

bool SockIsTimeout(int err)
{
	return ((err == EAGAIN) || (err == EWOULDBLOCK) || (err == ETIMEDOUT)); // note: receive timeout on blocking socket is would block
}

bool SockIsRefused(int err)
{
	return (err == ECONNREFUSED);
}

bool SockInProgress()
{
	return (errno == EINPROGRESS);
}

int SockWaitConnect(tSocket sock, int msec)
{
	struct pollfd fd;
	fd.fd = sock;
	fd.events = POLLOUT;
	fd.revents = 0;
	const int res = ::poll(&fd, 1, msec);
	return ((res > 0) ? 1 : res);
}

int SockConnectError(tSocket sock)
{
	int err = 0;
	socklen_t len = sizeof(err);

	if (getsockopt(sock, SOL_SOCKET, SO_ERROR, &err, &len) != 0)
		return errno;

	return err;
}

int SockSetOpt(tSocket sock, int level, int name, const void *val, int len)
{
	return setsockopt(sock, level, name, val, len);
}

int SockSetTimeout(tSocket sock, int name, unsigned int msec)
{
	struct timeval val;
	val.tv_sec = msec / 1000;
	val.tv_usec = (msec % 1000) * 1000;
	return setsockopt(sock, SOL_SOCKET, name, &val, sizeof(val));
}

int SockPoll(struct pollfd *fds, unsigned long count, int msec)
{
	return ::poll(fds, count, msec);
}

#endif

	}; // namespace nSocket
}; // namespace nVerliHub
