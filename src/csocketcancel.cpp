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

#include "csocketcancel.h"
#include "csocketapi.h" // note: shutdown and SHUT_RDWR

using namespace std;

namespace nVerliHub {
	namespace nSocket {

void cSocketCancel::Cancel()
{
	lock_guard<mutex> lock(mLock);
	mCancel = true;

	if (mSock != INVALID_SOCKET) // note: socket is still open, job detaches it under same lock before closing
		::shutdown(mSock, SHUT_RDWR);
}

void cSocketCancel::Reset()
{
	lock_guard<mutex> lock(mLock);
	mCancel = false;
	mSock = INVALID_SOCKET;
}

bool cSocketCancel::Attach(tSocket sock)
{
	lock_guard<mutex> lock(mLock);

	if (mCancel)
		return false;

	mSock = sock;
	return true;
}

void cSocketCancel::Detach()
{
	lock_guard<mutex> lock(mLock);
	mSock = INVALID_SOCKET;
}

	}; // namespace nSocket
}; // namespace nVerliHub
