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

#ifndef NSERVERCCONNSELECT_H
#define NSERVERCCONNSELECT_H

#include "cconnchoose.h"

#if USE_SELECT
/*
#if defined _WIN32
#include "Winsock2.h"
#else
*/
#include <sys/select.h>
//#endif

#include "thasharray.h"

namespace nVerliHub {
	namespace nSocket {

/**
selecting connection chooser

@author Daniel Muller
*/

class cConnSelect : public cConnChoose
{
public:
	cConnSelect();
	~cConnSelect();

	/** \brief Perform the choose operation
	  *
	  * determine which among the sunscribed connections will not block in specified IO operations
	  * \param tmout timeout for the select operation
	  * \sa OptIn
	  */
	virtual int Choose(cTime &tmout){ return this->Select(tmout); };

	/**
	* Register the connection for the given I/O operation.
	* @param conn The connection.
	* @param event Bitwise OR list of I/O operation.
	*/
	virtual void OptIn(tSocket sock, tChEvent events);

	/**
	* Unregister the connection for the given I/O operation.
	* @param conn The connection.
	* @param event Bitwise OR list of I/O operation.
	*/
	virtual void OptOut(tSocket, tChEvent);

	/**
	* Return I/O operations for the given connection.
	* @param conn The connection.
	* @return Bitwise OR list of I/O operation.
	* @see OptIn(tSocket sock, tChEvent events)
	*/
	virtual int OptGet(tSocket sock);
	/// \brief get the result after Choose operation
	virtual int RevGet(tSocket sock);
	/// \brief test wheather the choose result is non-nul
	virtual bool RevTest(tSocket sock);

	/**
	* Wrapper for fd_set structure. It provides constructor and = operator.
	* @author Daniel Muller
	*/
	struct sFDSet : public fd_set
	{
		sFDSet(){ FD_ZERO(this);}
		sFDSet & operator=(const sFDSet &set)
		{
			/*
			#ifdef _WIN32
			fd_count=set.fd_count;
			memcpy(&fd_array,&(set.fd_array), sizeof (fd_array));
			#else
			*/
			memcpy(&fds_bits,&(set.fds_bits), sizeof(fds_bits));
			//#endif
			return *this;
		}
		bool IsSet(tSocket sock){ return FD_ISSET(sock, this) != 0;}
		void Clr(tSocket sock){ FD_CLR(sock, this);}
		void Set(tSocket sock){ FD_SET(sock, this);}
	};

	void ClearRevents();
	void FDSet2HashRevents(sFDSet &fdset, unsigned mask);

	typedef tHashArray<sChooseRes *> tFDs;
	struct iterator;

	iterator begin() { return iterator(this, mFDs.begin()); }

	iterator end() { return iterator(this, mFDs.end()); }

	struct iterator
	{
		cConnSelect *mSel;
		tFDs::iterator mIT;

		iterator():
			mSel(NULL)
		{}

		iterator(cConnSelect *sel, tFDs::iterator it) : mSel(sel), mIT(it)
		{}

		bool operator != (const iterator &it)
		{
			return mIT != it.mIT;
		}

		sChooseRes & operator *()
		{
			(*mIT)->mConn = (*mSel)[(*mIT)->mSock];
			return *(*mIT);
		}

		bool Advance()
		{
			while (! (++mIT).IsEnd() && !(*mIT)->mRevent){}
			return !mIT.IsEnd();
		}

		iterator & operator ++(){ Advance(); return *this;}

		private:

		iterator(const iterator &it);

		iterator &operator=(const iterator &it)
		{
			mSel= it.mSel;
			mIT = it.mIT;
			return *this;
		}
	};

protected:
	int Select(cTime &tmout);
	// select settings
	sFDSet mReadFS;
	sFDSet mWriteFS;
	sFDSet mExceptFS;
	sFDSet mCloseFS;

	// select results
	sFDSet mResReadFS;
	sFDSet mResWriteFS;
	sFDSet mResExceptFS;

	tFDs mFDs;
};


	}; // namespace nSocket
}; // namespace nVerliHub

#endif
#endif
