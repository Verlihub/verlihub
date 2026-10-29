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

#ifndef NSOCKETCSOCKETAPI_H
#define NSOCKETCSOCKETAPI_H

/*
	small portable layer over posix sockets and windows sockets, only what verlihub needs
	include this instead of system socket headers
*/

#include "cconnbase.h"
#include <string>

#ifdef _WIN32
	#include <winsock2.h>
	#include <ws2tcpip.h>

	#ifndef SHUT_RDWR
		#define SHUT_RDWR SD_BOTH
	#endif

	#ifndef MSG_DONTWAIT
		#define MSG_DONTWAIT 0 // note: sockets are non blocking anyway
	#endif
#else
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <netdb.h>
	#include <poll.h>
#endif

#ifndef MSG_NOSIGNAL
	#define MSG_NOSIGNAL 0 // note: windows has no sigpipe at all
#endif

namespace nVerliHub {
	namespace nSocket {

// prepare and release socket library, windows needs it once before any socket is used
bool SockStartup();
void SockCleanup();

// socket that can be used, zero is also treated as no socket, like everywhere in verlihub
inline bool SockValid(tSocket sock)
{
	return ((sock != INVALID_SOCKET) && (sock != 0));
}

// error of last failed socket call and its text
int SockError();
std::string SockErrorText(int err);

// last socket call failed only because it would block or was interrupted, so it can be tried again
bool SockTryAgain();

// last socket call was interrupted by signal, note: on blocking socket with timeout, would block means timeout, not try again
bool SockInterrupted();

// error means timeout or refused connection, err is from SockError(), used to tell scripts why request failed
bool SockIsTimeout(int err);
bool SockIsRefused(int err);

int SockClose(tSocket sock);
bool SockNonBlock(tSocket sock, bool on = true);

/*
	connect of non blocking socket was started and is in progress, then wait for it with SockWaitConnect()
	wait returns 1 when connect ended, then SockConnectError() tells result, 0 when msec expired, -1 on error
	note: windows poll does not report failed connect on older versions, so select is used there
*/
bool SockInProgress();
int SockWaitConnect(tSocket sock, int msec);
int SockConnectError(tSocket sock);
int SockSetOpt(tSocket sock, int level, int name, const void *val, int len);

// receive or send timeout, windows takes milliseconds, posix takes timeval
int SockSetTimeout(tSocket sock, int name, unsigned int msec);

// poll few sockets, windows has it as WSAPoll
int SockPoll(struct pollfd *fds, unsigned long count, int msec);

#ifdef _WIN32
	// connected pair of local sockets, windows can not wait on pipe together with sockets
	bool SockPair(tSocket pair[2]);
#endif

	}; // namespace nSocket
}; // namespace nVerliHub

#endif
