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
	on windows socket pair is used, pipe is not selectable there, broken pair gives events all the time, so it is marked broken and main loop stops waiting on it
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

		// main thread only, call before processing whatever was signaled, so no signal is lost, also finds broken read end
		void Clear();

		// main thread only, read end is closed or has error, signals do not wake up anymore
		bool IsBroken() const
		{
			return mBroken;
		}

		void SetBroken()
		{
			mBroken = true;
		}

		// main thread only, true once after it is broken, for log
		bool BrokenNews()
		{
			if (!mBroken || mBrokenTold)
				return false;

			mBrokenTold = true;
			return true;
		}

		operator tSocket() const override
		{
			return mRead;
		}

	private:
		tSocket mRead = INVALID_SOCKET;
		tSocket mWrite = INVALID_SOCKET;
		std::atomic<bool> mPending{false};
		bool mBroken = false;
		bool mBrokenTold = false;
};

	}; // namespace nSocket
}; // namespace nVerliHub

#endif
