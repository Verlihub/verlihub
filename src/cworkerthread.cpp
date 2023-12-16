/*
	Copyright (C) 2003-2005 Daniel Muller, dan at verliba dot cz
	Copyright (C) 2006-2024 Verlihub Team, info at verlihub dot net

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

#include "cworkerthread.h"

namespace nVerliHub {
	namespace nThread {

cWorkerThread::cWorkerThread():
	mWork(NULL)
{}

cWorkerThread::~cWorkerThread()
{
	Stop(true);

	if (mWork) {
		delete mWork;
		mWork = NULL;
	}
}

bool cWorkerThread::AddWork(cThreadWork *theWork)
{
	bool Result = false;

	if (TryLock()) {
		if (!mWork) {
			mWork = theWork;
			Result = true;
		}

		UnLock();
	}

	if (Result)
		Start();

	return Result;
}

bool cWorkerThread::HasSomethingToDo()
{
	return mWork != NULL;
}

/*!
    \fn nThreads::cWorkerThread::Thread(cObj *)
*/
void cWorkerThread::DoSomething()
{
	if (mWork != NULL) {
		mWork->DoTheWork();
		delete mWork;
		mWork = NULL;
	}
}

	}; // namespace nThread
}; // namespace nVerliHub
