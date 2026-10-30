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

static const uint32_t sInputEvents = EPOLLIN; // note: no urgent data, hub does not read it, normal receive would find nothing
static const size_t sMaxEvents = 4096; // note: more ready sockets are reported in next step, epoll is level triggered

cConnEpoll::cConnEpoll():
#ifdef _WIN32
	mEpoll(epoll_create1(0)) // note: wepoll takes no flags
#else
	mEpoll(epoll_create1(EPOLL_CLOEXEC))
#endif
{
#ifdef _WIN32
	if (!mEpoll)
#else
	if (mEpoll < 0)
#endif
		throw "Unable to create epoll connection chooser";

	mSocks.reserve(20480); // note: same as poll chooser
	mEvents.resize(sMaxEvents);
}

cConnEpoll::~cConnEpoll()
{
#ifdef _WIN32
	if (mEpoll)
		epoll_close(mEpoll);
#else
	if (mEpoll >= 0)
		::close(mEpoll);
#endif
}

bool cConnEpoll::AddConn(cConnBase *conn)
{
	if (!cConnChoose::AddConn(conn))
		return false;

#ifdef _WIN32
	mSocks[(tSocket)(*conn)] = sSock(); // note: handle can be reused by new socket, it starts clean
#else
	if (size_t(mLastSock) >= mSocks.size()) // note: grow like poll chooser
		mSocks.resize(mLastSock + (mLastSock / 4) + 1);
#endif

	return true; // note: socket is registered in epoll when first event is requested
}

bool cConnEpoll::DelConn(cConnBase *conn)
{
	if (!conn || !cConnChoose::DelConn(conn)) // note: this opts out of all events, which removes socket from epoll
		return false;

#ifdef _WIN32
	mSocks.erase((tSocket)(*conn)); // note: unknown socket is skipped by iterator, also when it is still in ready or closing list
#endif

	return true;
}

void cConnEpoll::Apply(tSocket sock, sSock &item, uint32_t want)
{
	const uint32_t was = item.mWant;
	item.mWant = want;

	if (!want) { // note: nothing left, remove from epoll like poll chooser resets its entry
		if (item.mState & eES_ADDED) {
			epoll_ctl(mEpoll, EPOLL_CTL_DEL, sock, NULL);
			item.mState &= ~eES_ADDED;
		}

		item.mRev = 0; // note: dont report old events of this socket anymore
		return;
	}

	if ((want == was) && (item.mState & eES_ADDED))
		return;

	struct epoll_event ev;
	memset(&ev, 0, sizeof(ev));
	ev.events = want;
	ev.data.u64 = (uint64_t)sock; // note: windows socket does not fit into fd field

	if (item.mState & eES_ADDED) {
		if (epoll_ctl(mEpoll, EPOLL_CTL_MOD, sock, &ev) == 0)
			return;

		if (errno != ENOENT) // note: socket was closed and reused without us knowing, add it again
			return;
	}

	if (epoll_ctl(mEpoll, EPOLL_CTL_ADD, sock, &ev) == 0) {
		item.mState |= eES_ADDED;
		return;
	}

	if ((errno == EEXIST) && (epoll_ctl(mEpoll, EPOLL_CTL_MOD, sock, &ev) == 0)) // note: should not happen, but keep epoll in sync with what we want
		item.mState |= eES_ADDED;
}

void cConnEpoll::OptIn(tSocket sock, tChEvent mask)
{
	sSock *item = Get(sock);

	if (!item)
		return;

	if (mask & eCC_CLOSE) { // note: same as poll chooser, close clears requested events and waits to be reported
		Apply(sock, *item, 0);
		item->mState &= ~eES_ERROR;

		if (!(item->mState & eES_CLOSING)) {
			item->mState |= eES_CLOSING;
			mClosing.push_back(sock);
		}

		return;
	}

	uint32_t want = item->mWant;

	if (mask & eCC_INPUT)
		want |= sInputEvents;

	if (mask & eCC_OUTPUT)
		want |= EPOLLOUT;

	if (mask & eCC_ERROR) {
		want |= (EPOLLERR | EPOLLHUP);
		item->mState |= eES_ERROR;
	}

	if (want)
		item->mState &= ~eES_CLOSING; // note: requested events again, like poll chooser, it is not closing anymore

	Apply(sock, *item, want);
}

void cConnEpoll::OptOut(tSocket sock, tChEvent mask)
{
	sSock *item = Get(sock);

	if (!item)
		return;

	uint32_t want = item->mWant;

	if (mask & eCC_INPUT)
		want &= ~sInputEvents;

	if (mask & eCC_OUTPUT)
		want &= ~uint32_t(EPOLLOUT);

	if (mask & eCC_ERROR) {
		want &= ~uint32_t(EPOLLERR | EPOLLHUP);
		item->mState &= ~eES_ERROR;
	}

	if (!want) // note: nothing left, like poll chooser reset, this also ends closing state
		item->mState &= ~(eES_CLOSING | eES_ERROR);

	Apply(sock, *item, want);
}

int cConnEpoll::OptGet(tSocket sock)
{
	const sSock *item = Get(sock);

	if (!item)
		return 0;

	if (item->mState & eES_CLOSING)
		return eCC_CLOSE;

	int mask = 0;

	if (item->mWant & sInputEvents)
		mask |= eCC_INPUT;

	if (item->mWant & EPOLLOUT)
		mask |= eCC_OUTPUT;

	if (item->mState & eES_ERROR)
		mask |= eCC_ERROR;

	return mask;
}

int cConnEpoll::RevGet(tSocket sock)
{
	const sSock *item = Get(sock);

	if (!item)
		return 0;

	int mask = 0;

	if (item->mState & eES_CLOSING)
		mask |= eCC_CLOSE;

	if (item->mRev & sInputEvents)
		mask |= eCC_INPUT;

	if (item->mRev & EPOLLOUT)
		mask |= eCC_OUTPUT;

	if (item->mRev & (EPOLLERR | EPOLLHUP))
		mask |= eCC_ERROR;

	return mask;
}

bool cConnEpoll::RevTest(tSocket sock)
{
	const sSock *item = Get(sock);

	if (!item)
		return false;

	return (item->mRev || (item->mState & eES_CLOSING));
}

int cConnEpoll::Choose(nUtils::cTime &tmout)
{
	for (const tSocket sock: mReady) { // note: events of previous step are done
		sSock *item = Get(sock);

		if (item)
			item->mRev = 0;
	}

	mReady.clear();
	size_t keep = 0;

	for (size_t i = 0; i < mClosing.size(); i++) { // note: closing sockets are reported without waiting, socket itself has no event for that
		const tSocket sock = mClosing[i];
		const sSock *item = Get(sock);

		if (item && (item->mState & eES_CLOSING)) {
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
		const tSocket sock = (tSocket)mEvents[i].data.u64;
		sSock *item = Get(sock);

		if (!item || !(item->mState & eES_ADDED)) // note: removed meanwhile
			continue;

		if (!item->mRev && !(item->mState & eES_CLOSING)) // note: closing socket is already in list
			mReady.push_back(sock);

		item->mRev = mEvents[i].events;
	}

	return mReady.size();
}

	}; // namespace nSocket
}; // namespace nVerliHub

#endif // USE_EPOLL
