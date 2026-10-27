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

#ifndef NSOCKETCWAKEUP_H
#define NSOCKETCWAKEUP_H

#include "cconnbase.h"
#include <atomic>

namespace nVerliHub {
	namespace nSocket {

/*
	wakes up main loop from other threads, registered in connection chooser like any other connection
	any thread calls Signal(), main loop sees input on read end and calls Clear() before processing
	multiple signals before Clear() are merged into one, so pipe never fills up

	todo: pipe is not selectable on windows, use socket pair there
*/

class cWakeup : public cConnBase
{
	public:
		cWakeup() = default;
		~cWakeup();

		cWakeup(const cWakeup&) = delete;
		cWakeup& operator=(const cWakeup&) = delete;

		bool Open();
		void Close();

		bool IsOpen() const
		{
			return (mRead != INVALID_SOCKET);
		}

		// thread safe, can be called from any thread
		void Signal();

		// main thread only, call before processing whatever was signaled, so no signal is lost
		void Clear();

		operator tSocket() const override
		{
			return mRead;
		}

	private:
		tSocket mRead = INVALID_SOCKET;
		tSocket mWrite = INVALID_SOCKET;
		std::atomic<bool> mPending{false};
};

	}; // namespace nSocket
}; // namespace nVerliHub

#endif
