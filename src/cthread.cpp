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

#include <string>
#include <unistd.h>
#include "cthread.h"

using namespace std;

namespace nVerliHub {
	namespace nThread {

cThread::cThread()
{
	mRun = 0;
	mThread = 0;
	mStop = 1;
}

cThread::~cThread()
{
	void *status;
	mStop = 1;

	if (mRun == 1)
		pthread_join(mThread, &status);
}

/*
bool cThread::Stopped() const
{
	return (mStop == 1) || (mRun == 0);
}
*/

int cThread::Start()
{
	if (mRun || !mStop)
		return -1;

	mStop = 0;
	return pthread_create(&mThread, 0, (tThreadFunc)ThreadFunc, this);
}

int cThread::Stop(bool BeHard)
{
	void *status;

	if (!mRun || mStop)
		return -1;

	mStop = 1;

	if (BeHard)
		pthread_join(mThread, &status);

	return 0;
}

void *cThread::ThreadFunc(void *obj)
{
	cThread *This = (cThread*)obj;
	This->mRun = 1;

	while (!This->mStop) { // infinite loop, can be stopped only by deleting instance
		if (This->HasSomethingToDo())
			This->DoSomething();
		else
			usleep(1000);
	}

	This->mRun = 0;
	return obj;
}

	}; // namespace nThread
}; // namespace nVerliHub
