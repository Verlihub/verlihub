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

#ifndef NTHREADSCWORKERTHREAD_H
#define NTHREADSCWORKERTHREAD_H
#include "cthread.h"
#include "cthreadwork.h"

namespace nVerliHub {
	namespace nThread {

/**
a thread designed to do some work of type cThreadWork

@author Daniel Muller
*/
class cWorkerThread : public cThread
{
public:
	cWorkerThread();
	virtual ~cWorkerThread();

	/**
	 * \brief Queue some work
	 * \return fals when no more work can be queud
	 * */
	virtual bool AddWork(cThreadWork *);

	/**
	 * \brief to know if there is work to do
	 *
	 * thread asks itself if not, it sleeps, or also other thread may ask this, to make it run
	 * */
	virtual bool HasSomethingToDo();

	/**
	 * \bried Does a single piece of work
	 *
	 * and when it's done, it removes the work
	 * */
	virtual void DoSomething();

protected:
	cThreadWork * mWork;
};

	}; // namespace nThread
}; // namespace nVerliHub

#endif
