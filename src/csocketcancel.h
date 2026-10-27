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

#ifndef NSOCKETCSOCKETCANCEL_H
#define NSOCKETCSOCKETCANCEL_H

#include "cconnbase.h"
#include <atomic>
#include <mutex>

namespace nVerliHub {
	namespace nSocket {

/*
	lets main thread abort blocking socket work of job that runs on other thread
	job attaches socket it is working with, Cancel() sets flag and shuts down that socket, so blocked connect, send or receive return at once
	job must detach socket before closing it, otherwise main thread could shut down another connection that reused same descriptor

	note: host name resolving can not be aborted this way, it ends when resolver times out
*/

class cSocketCancel
{
	public:
		cSocketCancel() = default;

		cSocketCancel(const cSocketCancel&) = delete;
		cSocketCancel& operator=(const cSocketCancel&) = delete;

		// any thread, usually main thread when stopping
		void Cancel();

		// main thread, before new job is started, there must be no running job using this
		void Reset();

		bool IsCancelled() const
		{
			return mCancel;
		}

		// job thread, returns false when already cancelled, then socket must not be used
		bool Attach(tSocket sock);

		// job thread, must be called before socket is closed
		void Detach();

	private:
		std::mutex mLock;
		tSocket mSock = INVALID_SOCKET;
		std::atomic<bool> mCancel{false};
};

	}; // namespace nSocket
}; // namespace nVerliHub

#endif
