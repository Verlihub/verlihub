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

#ifndef NSERVERCCONNEPOLL_H
#define NSERVERCCONNEPOLL_H

#include "cconnchoose.h"

#if USE_EPOLL

#include <sys/epoll.h>
#include <cstdint>
#include <vector>

namespace nVerliHub {
	namespace nSocket {

/*
	epoll connection chooser, cost of one step depends on number of active connections, not on number of all connections
	level triggered, so it behaves same as poll chooser: what is not read or written now is reported again in next step

	note: close is not a socket event, connection that is closed with CloseNow() is kept in own list and reported in next step
	note: when connection is deleted, its pending events are cleared, so new connection with reused socket does not get them
*/

class cConnEpoll: public cConnChoose
{
	public:
		cConnEpoll();
		~cConnEpoll();

		cConnEpoll(const cConnEpoll&) = delete;
		cConnEpoll& operator=(const cConnEpoll&) = delete;

		virtual int Choose(nUtils::cTime &tmout);

		virtual bool AddConn(cConnBase *conn);

		virtual void OptIn(tSocket sock, nEnums::tChEvent mask);
		using cConnChoose::OptIn;
		virtual void OptOut(tSocket sock, nEnums::tChEvent mask);
		using cConnChoose::OptOut;
		virtual int OptGet(tSocket sock);
		using cConnChoose::OptGet;
		virtual int RevGet(tSocket sock);
		virtual bool RevTest(tSocket sock);

		// iterates only over sockets that have events or are closing
		struct iterator
		{
			cConnEpoll *mChoose = NULL;
			size_t mPos = 0;
			sChooseRes mRes;

			iterator() = default;

			iterator(cConnEpoll *choose, size_t pos):
				mChoose(choose),
				mPos(pos)
			{
				Skip();
			}

			iterator &operator++()
			{
				mPos++;
				Skip();
				return *this;
			}

			sChooseRes &operator*()
			{
				mRes.mSock = mChoose->mReady[mPos];
				mRes.mEvent = mChoose->OptGet(mRes.mSock);
				mRes.mRevent = mChoose->RevGet(mRes.mSock);
				mRes.mConn = mChoose->operator[](mRes.mSock);
				return mRes;
			}

			bool operator!=(const iterator &it) const
			{
				return (mPos != it.mPos);
			}

			bool operator==(const iterator &it) const
			{
				return (mPos == it.mPos);
			}

			private:
				void Skip() // note: connection might be deleted after step started
				{
					while (mChoose && (mPos < mChoose->mReady.size()) && !mChoose->RevTest(mChoose->mReady[mPos]))
						mPos++;
				}
		};

		iterator begin()
		{
			return iterator(this, 0);
		}

		iterator end()
		{
			return iterator(this, mReady.size());
		}

	private:
		enum tState // note: socket state flags
		{
			eES_ADDED = 1 << 0, // registered in epoll
			eES_CLOSING = 1 << 1, // closed with CloseNow(), waits to be reported
			eES_ERROR = 1 << 2 // error events were requested, they are always reported by epoll anyway
		};

		bool Valid(tSocket sock) const
		{
			return ((sock >= 0) && (size_t(sock) < mWant.size()));
		}

		void Apply(tSocket sock, uint32_t want); // note: update epoll registration

		int mEpoll;
		std::vector<uint32_t> mWant; // note: by socket, requested epoll events
		std::vector<uint32_t> mRev; // note: by socket, events of current step
		std::vector<unsigned char> mState; // note: by socket, tState flags
		std::vector<tSocket> mReady; // note: sockets of current step, iterated by iterator
		std::vector<tSocket> mClosing; // note: sockets waiting to report close
		std::vector<struct epoll_event> mEvents; // note: buffer for epoll_wait results
};

	}; // namespace nSocket
}; // namespace nVerliHub

#endif // USE_EPOLL
#endif
