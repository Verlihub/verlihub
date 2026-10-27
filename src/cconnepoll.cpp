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

#include "cconnepoll.h"

#if USE_EPOLL

#include <cerrno>
#include <cstring>
#include <unistd.h>

namespace nVerliHub {
	using namespace nEnums;

	namespace nSocket {

static const uint32_t sInputEvents = (EPOLLIN | EPOLLPRI);
static const size_t sMaxEvents = 4096; // note: more ready sockets are reported in next step, epoll is level triggered

cConnEpoll::cConnEpoll():
	mEpoll(epoll_create1(EPOLL_CLOEXEC))
{
	if (mEpoll < 0)
		throw "Unable to create epoll connection chooser";

	mWant.reserve(20480); // note: same as poll chooser
	mRev.reserve(20480);
	mState.reserve(20480);
	mEvents.resize(sMaxEvents);
}

cConnEpoll::~cConnEpoll()
{
	if (mEpoll >= 0)
		::close(mEpoll);
}

bool cConnEpoll::AddConn(cConnBase *conn)
{
	if (!cConnChoose::AddConn(conn))
		return false;

	if (size_t(mLastSock) >= mWant.size()) { // note: grow like poll chooser
		const size_t size = mLastSock + (mLastSock / 4) + 1;
		mWant.resize(size, 0);
		mRev.resize(size, 0);
		mState.resize(size, 0);
	}

	return true; // note: socket is registered in epoll when first event is requested
}

void cConnEpoll::Apply(tSocket sock, uint32_t want)
{
	const uint32_t was = mWant[sock];
	mWant[sock] = want;

	if (!want) { // note: nothing left, remove from epoll like poll chooser resets its entry
		if (mState[sock] & eES_ADDED) {
			epoll_ctl(mEpoll, EPOLL_CTL_DEL, sock, NULL);
			mState[sock] &= ~eES_ADDED;
		}

		mRev[sock] = 0; // note: dont report old events of this socket anymore
		return;
	}

	if ((want == was) && (mState[sock] & eES_ADDED))
		return;

	struct epoll_event ev;
	memset(&ev, 0, sizeof(ev));
	ev.events = want;
	ev.data.fd = sock;

	if (mState[sock] & eES_ADDED) {
		if (epoll_ctl(mEpoll, EPOLL_CTL_MOD, sock, &ev) == 0)
			return;

		if (errno != ENOENT) // note: socket was closed and reused without us knowing, add it again
			return;
	}

	if (epoll_ctl(mEpoll, EPOLL_CTL_ADD, sock, &ev) == 0) {
		mState[sock] |= eES_ADDED;
		return;
	}

	if ((errno == EEXIST) && (epoll_ctl(mEpoll, EPOLL_CTL_MOD, sock, &ev) == 0)) // note: should not happen, but keep epoll in sync with what we want
		mState[sock] |= eES_ADDED;
}

void cConnEpoll::OptIn(tSocket sock, tChEvent mask)
{
	if (!Valid(sock))
		return;

	if (mask & eCC_CLOSE) { // note: same as poll chooser, close clears requested events and waits to be reported
		Apply(sock, 0);
		mState[sock] &= ~eES_ERROR;

		if (!(mState[sock] & eES_CLOSING)) {
			mState[sock] |= eES_CLOSING;
			mClosing.push_back(sock);
		}

		return;
	}

	uint32_t want = mWant[sock];

	if (mask & eCC_INPUT)
		want |= sInputEvents;

	if (mask & eCC_OUTPUT)
		want |= EPOLLOUT;

	if (mask & eCC_ERROR) {
		want |= (EPOLLERR | EPOLLHUP);
		mState[sock] |= eES_ERROR;
	}

	if (want)
		mState[sock] &= ~eES_CLOSING; // note: requested events again, like poll chooser, it is not closing anymore

	Apply(sock, want);
}

void cConnEpoll::OptOut(tSocket sock, tChEvent mask)
{
	if (!Valid(sock))
		return;

	uint32_t want = mWant[sock];

	if (mask & eCC_INPUT)
		want &= ~sInputEvents;

	if (mask & eCC_OUTPUT)
		want &= ~uint32_t(EPOLLOUT);

	if (mask & eCC_ERROR) {
		want &= ~uint32_t(EPOLLERR | EPOLLHUP);
		mState[sock] &= ~eES_ERROR;
	}

	if (!want) // note: nothing left, like poll chooser reset, this also ends closing state
		mState[sock] &= ~(eES_CLOSING | eES_ERROR);

	Apply(sock, want);
}

int cConnEpoll::OptGet(tSocket sock)
{
	if (!Valid(sock))
		return 0;

	if (mState[sock] & eES_CLOSING)
		return eCC_CLOSE;

	int mask = 0;
	const uint32_t want = mWant[sock];

	if (want & sInputEvents)
		mask |= eCC_INPUT;

	if (want & EPOLLOUT)
		mask |= eCC_OUTPUT;

	if (mState[sock] & eES_ERROR)
		mask |= eCC_ERROR;

	return mask;
}

int cConnEpoll::RevGet(tSocket sock)
{
	if (!Valid(sock))
		return 0;

	int mask = 0;
	const uint32_t rev = mRev[sock];

	if (mState[sock] & eES_CLOSING)
		mask |= eCC_CLOSE;

	if (rev & sInputEvents)
		mask |= eCC_INPUT;

	if (rev & EPOLLOUT)
		mask |= eCC_OUTPUT;

	if (rev & (EPOLLERR | EPOLLHUP))
		mask |= eCC_ERROR;

	return mask;
}

bool cConnEpoll::RevTest(tSocket sock)
{
	if (!Valid(sock))
		return false;

	return (mRev[sock] || (mState[sock] & eES_CLOSING));
}

int cConnEpoll::Choose(nUtils::cTime &tmout)
{
	for (const tSocket sock: mReady) { // note: events of previous step are done
		if (Valid(sock))
			mRev[sock] = 0;
	}

	mReady.clear();
	size_t keep = 0;

	for (size_t i = 0; i < mClosing.size(); i++) { // note: closing sockets are reported without waiting, socket itself has no event for that
		const tSocket sock = mClosing[i];

		if (Valid(sock) && (mState[sock] & eES_CLOSING)) {
			mClosing[keep++] = sock;
			mReady.push_back(sock);
		}
	}

	mClosing.resize(keep);
	const int wait = (mReady.size() ? 0 : ((int)tmout.MiliSec() + 1)); // note: same timeout as poll chooser
	int res;

	do {
		res = epoll_wait(mEpoll, mEvents.data(), (int)mEvents.size(), wait);
	} while ((res < 0) && (errno == EINTR));

	for (int i = 0; i < res; i++) {
		const tSocket sock = mEvents[i].data.fd;

		if (!Valid(sock) || !(mState[sock] & eES_ADDED)) // note: removed meanwhile
			continue;

		if (!mRev[sock] && !(mState[sock] & eES_CLOSING)) // note: closing socket is already in list
			mReady.push_back(sock);

		mRev[sock] = mEvents[i].events;
	}

	return mReady.size();
}

	}; // namespace nSocket
}; // namespace nVerliHub

#endif // USE_EPOLL
