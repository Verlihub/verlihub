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

#ifndef NTHREADSCTHREAD_H
#define NTHREADSCTHREAD_H
#include "cmutex.h"
#include "cobj.h"

namespace nVerliHub {
	namespace nThread {
/**
@author Daniel Muller
*/
class cThread : public cMutex
{
public:
	cThread();
	virtual ~cThread();

	/** start thread */
	int Start();
	/** stop it */
	int Stop( bool bHard = true );
	/** is it stopped ? */
	//bool Stopped() const;

	/** sleep N nanoseconds */
	//void NanoSleep( unsigned int N);

	/** */
	typedef void * (*tThreadFunc)(void*);
	static void* ThreadFunc(void *);

	/** */
	virtual bool HasSomethingToDo() = 0;
	virtual void DoSomething() = 0;

protected:
	/** */
	int mRun;
	/** */
	pthread_t mThread;
	/** */
	int mStop;
};
	}; // namespace nThread
}; // namespace nVerliHub

#endif
