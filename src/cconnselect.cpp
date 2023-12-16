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

#include "cconnselect.h"

#if ! HAVE_SYS_POLL_H

namespace nVerliHub {
	using namespace nEnums;

	namespace nSocket {

cConnSelect::cConnSelect()
{}

cConnSelect::~cConnSelect()
{
	tFDs::iterator it;
	sChooseRes *FD;
	tSocket sock;
	for (it = mFDs.begin(); it != mFDs.end(); )
	{
		FD = *it;
		++it;

		if (FD)
		{
			sock = FD->mSock;
			mFDs.RemoveByHash(sock);
			delete FD;
			FD = NULL;
		}
	}
}

void cConnSelect::OptIn(tSocket sock, tChEvent mask)
{
	if( mask & eCC_INPUT  ) mReadFS.Set(sock);
	if( mask & eCC_OUTPUT ) mWriteFS.Set(sock);
	if( mask & eCC_ERROR  ) mExceptFS.Set(sock);
	if( mask & eCC_CLOSE  ) mCloseFS.Set(sock);
	sChooseRes *FD = mFDs.GetByHash(sock);
	if (!FD)
	{
		FD = new sChooseRes;
		FD->mSock = sock;
		FD->mEvent = mask;
		mFDs.AddWithHash(FD , sock );
	}
	else
	{
		FD->mEvent |= mask;
	}

}

void cConnSelect::OptOut(tSocket sock, tChEvent mask)
{
	if( mask & eCC_INPUT  ) mReadFS.Clr(sock);
	if( mask & eCC_OUTPUT ) mWriteFS.Clr(sock);
	if( mask & eCC_ERROR  ) mExceptFS.Clr(sock);
	if( mask & eCC_CLOSE  ) mCloseFS.Clr(sock);
	sChooseRes *FD = mFDs.GetByHash(sock);
	if(FD)
	{
		FD->mEvent -= (FD->mEvent & mask);
		if(!FD->mEvent)
		{
			mFDs.RemoveByHash(sock);
			delete FD;
			FD = NULL;
		}
	}
}

int cConnSelect::OptGet( tSocket sock )
{
	int mask = 0;
	if( mReadFS.IsSet(sock) ) mask |= eCC_INPUT;
	if( mWriteFS.IsSet(sock)) mask |= eCC_OUTPUT;
	if( mExceptFS.IsSet(sock))mask |= eCC_ERROR;
	if( mCloseFS.IsSet(sock) )mask |= eCC_CLOSE;
	return mask;
}

int cConnSelect::RevGet( tSocket sock )
{
	int mask = 0;
	if( mResReadFS.IsSet(sock) ) mask |= eCC_INPUT;
	if( mResWriteFS.IsSet(sock)) mask |= eCC_OUTPUT;
	if( mResExceptFS.IsSet(sock))mask |= eCC_ERROR;
	if( mCloseFS.IsSet(sock)    )mask |= eCC_CLOSE;
	return mask;
}

bool cConnSelect::RevTest( tSocket sock )
{
	if( mResWriteFS.IsSet(sock)) return true;
	if( mResReadFS.IsSet(sock) ) return true;
	if( mResExceptFS.IsSet(sock))return true;
	if( mCloseFS.IsSet(sock)    )return true; // note that's not an error
	return false;
}

int cConnSelect::Select( cTime &tmout )
{
	mResReadFS = mReadFS;
	mResWriteFS = mWriteFS;
	mResExceptFS = mExceptFS;
	int size = mLastSock;//mFDs.size();
	int ret = select(size, &mResReadFS, &mResWriteFS, &mResExceptFS, (timeval *)(&tmout));
	if( ret == SOCKET_ERROR ) return -1;
	ClearRevents();
	FDSet2HashRevents(mResReadFS, eCC_INPUT);
	FDSet2HashRevents(mResWriteFS, eCC_OUTPUT);
	FDSet2HashRevents(mResExceptFS, eCC_ERROR);
	FDSet2HashRevents(mCloseFS, eCC_CLOSE);
	return ret;
}

void cConnSelect::ClearRevents(void)
{
	tFDs::iterator it;
	for (it = mFDs.begin(); it != mFDs.end(); ++ it)
		if(*it) (*it)->mRevent = 0;
}


void cConnSelect::FDSet2HashRevents(sFDSet &fdset, unsigned mask)
{
	unsigned i;
	tSocket sock;
	/*
	#ifdef _WIN32
	for(i = 0; i < fdset.fd_count; i++)
	{
		sock = fdset.fd_array[i];
		sChooseRes *FD = mFDs.GetByHash(sock);
		if (!FD)
		{
			FD = new sChooseRes;
			FD->mSock = sock;
			FD->mEvent = 0;
			FD->mRevent = mask;
			mFDs.AddWithHash(FD , sock );
		}
		else
		{
			FD->mRevent |= mask;
		}
	}
	#else
	*/
	for(i = 0; i < FD_SETSIZE; i++)
	{
		sock = i;
		if (FD_ISSET(sock,&fdset))
		{
			sChooseRes *FD = mFDs.GetByHash(sock);
			if (!FD)
			{
				FD = new sChooseRes;
				FD->mSock = sock;
				FD->mEvent = 0;
				FD->mRevent = mask;
				mFDs.AddWithHash(FD , sock );
			}
			else
			{
				FD->mRevent |= mask;
			}
		}
	}
	//#endif
}

	}; // namespace nSocket
}; // namespace nVerliHub

#endif
