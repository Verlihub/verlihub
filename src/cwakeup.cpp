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

#include "cwakeup.h"
#include "csocketapi.h"

#ifndef _WIN32
	#include <cerrno>
	#include <fcntl.h>
	#include <unistd.h>
#endif

namespace nVerliHub {
	namespace nSocket {

#ifndef _WIN32

static bool SetNonBlockExec(int fd)
{
	const int flags = fcntl(fd, F_GETFL, 0);

	if ((flags < 0) || (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0))
		return false;

	return (fcntl(fd, F_SETFD, FD_CLOEXEC) == 0);
}

#endif

cWakeup::~cWakeup()
{
	Close();
}

bool cWakeup::Open()
{
	if (IsOpen())
		return true;

#ifdef _WIN32
	tSocket fds[2]; // note: windows can not wait on pipe together with sockets, socket pair is used there

	if (!SockPair(fds))
		return false;
#else
	int fds[2];

	if (pipe(fds) != 0)
		return false;

	if (!SetNonBlockExec(fds[0]) || !SetNonBlockExec(fds[1])) {
		::close(fds[0]);
		::close(fds[1]);
		return false;
	}
#endif

	mRead = fds[0];
	mWrite = fds[1];
	mPending = false;
	mBroken = false;
	mBrokenTold = false;
	return true;
}

void cWakeup::Close()
{
	if (mRead != INVALID_SOCKET) {
	#ifdef _WIN32
		SockClose(mRead);
	#else
		::close(mRead);
	#endif
		mRead = INVALID_SOCKET;
	}

	if (mWrite != INVALID_SOCKET) {
	#ifdef _WIN32
		SockClose(mWrite);
	#else
		::close(mWrite);
	#endif
		mWrite = INVALID_SOCKET;
	}

	mPending = false;
}

void cWakeup::Signal()
{
	if ((mWrite == INVALID_SOCKET) || mPending.exchange(true)) // note: already signaled and not cleared yet
		return;

	const char byte = 1;

#ifdef _WIN32
	send(mWrite, &byte, 1, 0); // note: would block means buffer is full, which means main loop will wake up anyway
#else
	ssize_t res;

	do {
		res = ::write(mWrite, &byte, 1);
	} while ((res < 0) && (errno == EINTR));

	// note: eagain means pipe is full, which means main loop will wake up anyway
#endif
}

void cWakeup::Clear()
{
	if (mRead != INVALID_SOCKET) {
		char buf[64];

	#ifdef _WIN32
		int res;

		while ((res = recv(mRead, buf, sizeof(buf), 0)) > 0)
		{}

		if ((res == 0) || ((res < 0) && !SockTryAgain())) // note: other end is closed or socket has error
			mBroken = true;
	#else
		ssize_t res;

		do {
			res = ::read(mRead, buf, sizeof(buf));
		} while ((res > 0) || ((res < 0) && (errno == EINTR)));

		if ((res == 0) || ((res < 0) && !SockTryAgain())) // note: write end is closed or pipe has error
			mBroken = true;
	#endif
	}

	/*
		note: clear flag only after draining, otherwise byte written by signal in between is drained while flag stays set, and all next signals are lost
		signal that comes before this line is not lost, because caller processes everything right after this call
	*/
	mPending = false;
}

	}; // namespace nSocket
}; // namespace nVerliHub
