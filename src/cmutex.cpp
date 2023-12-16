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

#include "cmutex.h"
#include <iostream>
#include <string.h>

using namespace std;
namespace nVerliHub {
	namespace nThread {

cMutex::cMutex()
{
	if ( (mLastError=pthread_mutex_init(&mMutex,NULL)) != 0 )
		PrintError(__FUNCTION__);
}

cMutex::~cMutex()
{
	if ((mLastError=pthread_mutex_destroy(&mMutex)) != 0 )
		PrintError(__FUNCTION__);
}

int cMutex::Lock()
{
	if ((mLastError=pthread_mutex_lock(&mMutex)) != 0 )
		PrintError(__FUNCTION__);
	return 0;
}

int cMutex::UnLock()
{
	if ((mLastError=pthread_mutex_unlock(&mMutex)) != 0 )
		PrintError(__FUNCTION__);
	return 0;
}

bool cMutex::TryLock()
{
	return (mLastError=pthread_mutex_trylock(&mMutex)) == 0;
}


void cMutex::ClearError()
{
	mLastError = 0;
}

const char* cMutex::GetError()
{
	return strerror(mLastError);
}

void cMutex::PrintError(const char *function)
{
	cerr << "Mutex error in " << function << " : " << GetError() << endl;
}

	}; // namespace nUtils
}; // namespace nVerliHub
