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

	Please see http://www.gnu.org/licenses/ for a copy
	of the GNU General Public License.
*/

#include "cconnpoll.h"
#include <cerrno>

#if !USE_SELECT

#if HAVE_UNISTD_H
	#include <unistd.h>
#endif

namespace nVerliHub {
	using namespace nEnums;

	namespace nSocket {

cConnPoll::cConnPoll()
{
	mFDs.reserve(20480); // idea: number depends on open files limit for process
}

cConnPoll::~cConnPoll()
{}

void cConnPoll::OptIn(tSocket sock, tChEvent mask)
{
	if (tSocket(mFDs.size()) <= sock)
		return;

 	unsigned event = FD(sock).events;

	if (!event && mask)
		FD(sock).fd = sock;

	if (mask & eCC_CLOSE) {
		FD(sock).events = 0;

	} else {
		if (mask & eCC_INPUT)
			event = POLLIN; // note: no urgent data, hub does not read it

		if (mask & eCC_OUTPUT)
			event |= POLLOUT;

		if (mask & eCC_ERROR)
			event |= POLLERR | POLLHUP | POLLNVAL;

		FD(sock).events |= event;
	}
}

void cConnPoll::OptOut(tSocket sock, tChEvent mask)
{
	if (tSocket(mFDs.size()) <= sock)
		return;

 	unsigned event = ~(0u);

	if (mask & eCC_INPUT)
		event = ~unsigned(POLLIN);

	if (mask & eCC_OUTPUT)
		event &= ~unsigned(POLLOUT);

	if (mask & eCC_ERROR)
		event &= ~unsigned(POLLERR | POLLHUP | POLLNVAL);

	if (!(FD(sock).events &= event))
		FD(sock).reset(); // nothing left
}

int cConnPoll::OptGet(tSocket sock)
{
	int mask = 0;

	if (tSocket(mFDs.size()) <= sock)
		return mask;

 	unsigned event = FD(sock).events;

	if (!event && (FD(sock).fd == sock)) {
		mask = eCC_CLOSE;

	} else {
		if (event & POLLIN)
			mask |= eCC_INPUT;

		if (event & POLLOUT)
			mask |= eCC_OUTPUT;

		if (event & (POLLERR | POLLHUP | POLLNVAL))
			mask |= eCC_ERROR;
	}

	return mask;
}

int cConnPoll::RevGet(tSocket sock)
{
	int mask = 0;

	if (tSocket(mFDs.size()) <= sock)
		return mask;

	cPollfd &theFD = FD(sock);
 	unsigned event = theFD.revents;

	if (!theFD.events && (theFD.fd == sock))
		mask = eCC_CLOSE;

	if (event & POLLIN)
		mask |= eCC_INPUT;

	if (event & POLLOUT)
		mask |= eCC_OUTPUT;

	if (event & (POLLERR | POLLHUP | POLLNVAL))
		mask |= eCC_ERROR;

	return mask;
}

bool cConnPoll::RevTest(cPollfd &theFD)
{
 	if (theFD.fd == INVALID_SOCKET)
		return false;

	if (!theFD.events)
		return true;

 	unsigned event = theFD.revents;

 	if (!event)
		return false;

	if (event & POLLOUT)
		return true;

	if (event & POLLIN)
		return true;

	if (event & (POLLERR | POLLHUP | POLLNVAL))
		return true;

	return false;
}

bool cConnPoll::RevTest(tSocket sock)
{
	if (tSocket(mFDs.size()) <= sock)
		return false;

	cPollfd &theFD = FD(sock);
	return RevTest(theFD);
}

int cConnPoll::poll(int wp_sec)
{
//#if !defined _WIN32
	// note: all sockets in one poll call with one timeout for all
	size_t count = mFDs.size();

	if (count > size_t(mLastSock + 1)) // note: entries above last socket are never used
		count = mLastSock + 1;

	if (!count)
		return 0;

	int ret;

	do {
		ret = ::poll(mFDs.data(), count, wp_sec + 1);
	} while ((ret < 0) && (errno == EINTR));

	return ((ret < 0) ? 0 : ret); // note: interrupted call is repeated, other errors return no events
/*
#else
	return 0;
#endif
*/
}

bool cConnPoll::AddConn(cConnBase *conn)
{
	if (!cConnChoose::AddConn(conn))
		return false;

	if (mLastSock >= (tSocket)mFDs.size()) // 2
		mFDs.resize(mLastSock + (mLastSock / 4) + 1); // note: plus one, so small socket number fits too

	return true;
}

	}; // namepsace nSocket
}; // namespace nVerliHub

#endif
