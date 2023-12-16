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

#ifndef NSERVERCCONNCHOOSE_H
#define NSERVERCCONNCHOOSE_H
#ifdef HAVE_CONFIG_H
	#include <config.h>
#endif // HAVE_CONFIG_H

#ifndef __extension__
	#  define __extension__
#endif // __extension__

#ifndef TEMP_FAILURE_RETRY
	#define TEMP_FAILURE_RETRY(expression) expression
#endif // TEMP_FAILURE_RETRY

#ifndef TEMP_FAILURE_RETRY
	//#if !defined _WIN32
		#define TEMP_FAILURE_RETRY(expression) \
		(__extension__ ({ long int __result;\
		while ((__result = (long int) (expression)) == -1L && errno == EINTR){}; __result; }))
	/*
	#else // _WIN32
		#define TEMP_FAILURE_RETRY(expression) while ((long int) (expression) == -1L && errno == EINTR){}
	#endif // _WIN32
	*/
#endif

/*
#ifdef _WIN32
	#define USE_SELECT 1
#else
*/
	#if HAVE_SYS_POLL_H
		#define USE_SELECT 0
	#else
		#define USE_SELECT 1
	#endif
//#endif // _WIN32

#include "ctime.h"
#include "cconnbase.h"

//#ifndef _WIN32
	#ifndef USE_OLD_CONNLIST
		#include <vector>
		using std::vector;
	#else
		#include <map>
		#include "tchashlistmap.h"
		using std::map;
	#endif // USE_OLD_CONNLIST
/*
#else
	#include "thasharray.h"
#endif // _WIN32
*/

namespace nVerliHub {
	using namespace nUtils;
	namespace nEnums {
		enum  tChEvent
		{
			eCC_INPUT = 1 << 0,
			eCC_OUTPUT= 1 << 1,
			eCC_ERROR = 1 << 2,
			eCC_CLOSE = 1 << 3,
			eCC_ALL   = eCC_INPUT | eCC_OUTPUT | eCC_ERROR
		};
	};

	namespace nSocket {

/**
 * This class is an interface for polling an selecting connection and it provides the following features:
 * - Connections managment
 * - Register and unregister connection depending on I/O operation (read, write)
 * - Random access to a connection by socket id
 * @author Daniel Muller

both:
-----
- quickly (un)register connections
<= connections have SockDescriptor
- fast iteration over resulting connections
=> random access for connection (by the number of sock)
=> useage of a map
- result iterator must be persitent against deletion of other connections
=> useage of a linked list
- quiclky add/remove connections
=> random access by connection pointer


poll:
-----
- keep pollfd structures in "few" array(s) (in order not to call poll too many times)
=> useage of vector(s) of pollfds

select:
-------

*/

class cConnChoose
{
public:
	cConnChoose();
	virtual ~cConnChoose();

	//#ifndef _WIN32
	#ifdef USE_OLD_CONNLIST
	typedef tcHashListMap <cConnBase* , tSocket> tConnList;
	#else
	typedef vector<cConnBase*> tConnList;
	#endif
	/*
	#else
	typedef tHashArray<cConnBase*> tConnList;
	#endif
	*/

	/**
	* Add new connection to be handled by connection manager.
	* @param conn The connection.
	* @return True if connection is added; otherwise false.
	*/
	virtual bool AddConn(cConnBase *);
	virtual bool DelConn(cConnBase *);
	virtual bool HasConn(cConnBase *);

	virtual cConnBase * operator[] (tSocket sock);

	tSocket operator[] (cConnBase *conn);

	virtual int Choose(cTime &) = 0;

	/**
	* Register the connection for the given I/O operations.
	* @param conn The connection.
	* @param event Bitwise OR list of I/O operation.
	*/
	virtual void OptIn(cConnBase *conn, nEnums::tChEvent event);

	/**
	* Unregister the connection for the given I/O operations.
	* @param conn The connection.
	* @param event Bitwise OR list of I/O operation.
	*/
	virtual void OptOut(cConnBase *conn, nEnums::tChEvent event);

	/**
	* Return I/O operations for the given connection.
	* @param conn The connection.
	* @return Bitwise OR list of I/O operation.
	* @see OptIn(cConnBase *conn, tChEvent events)
	*/
	int OptGet(cConnBase *conn);

	int RevGet(cConnBase *conn);

	bool RevTest(cConnBase *conn);

	virtual void OptIn (tSocket, nEnums::tChEvent) = 0;
	virtual void OptOut(tSocket, nEnums::tChEvent) = 0;
	virtual int OptGet(tSocket) = 0;
	virtual int RevGet(tSocket) = 0;
	virtual bool RevTest(tSocket) = 0;

	/**
	  Choose Result, returned by iterator,
	  describing what was to be chosen and what has been chosen, gives also a pointer to the class stored in the chooser
	*/
	struct sChooseRes
	{
		tSocket mSock;
		int mEvent;
		int mRevent;
		cConnBase *mConn;

		sChooseRes():
			mSock(0),
			mEvent(nEnums::eCC_INPUT),
			mRevent(nEnums::eCC_INPUT),
			mConn(NULL)
		{}
	};

	#if 1
	/** A Choose result iterator, probably OS dependant, select does not work with it yet
	*/
	struct iterator
	{
		tSocket *mEnd;
		cConnChoose *mChoose;
		sChooseRes mRes;

		iterator(cConnChoose *ch, tSocket *end) :
			mEnd(end), mChoose(ch){}

		iterator():mEnd(0),mChoose(0){};

		iterator &operator++()
		{
			while( (++mRes.mSock <= *mEnd) && !(mChoose->RevTest(mRes.mSock)) ){
			}
			return *this;
		}

		sChooseRes & operator*()
		{
			mRes.mEvent = mChoose->OptGet(mRes.mSock);
			mRes.mRevent = mChoose->RevGet(mRes.mSock);
			mRes.mConn  = mChoose->operator[](mRes.mSock);
			return mRes;
		}

		bool operator!=(const iterator &it) const
		{
			return mRes.mSock != it.mRes.mSock;
		}

		bool operator==(const iterator &it) const
		{
			return mRes.mSock == it.mRes.mSock;
		}

		iterator &operator=(const iterator &it)
		{
			mRes.mSock = it.mRes.mSock;
			mEnd = it.mEnd;
			mChoose = it.mChoose;
			return *this;
		}

		private:
			iterator(const iterator &it);
	};

	iterator &begin()
	{
		static iterator sBegin(this, &mLastSock);
		sBegin.mRes.mSock = 0;
		if( !RevTest(tSocket(0)) ) ++sBegin;
		return sBegin;
	}

	iterator &end()
	{
		static iterator sEnd(this, &mLastSock);
		sEnd.mRes.mSock = mLastSock + 1;
		return sEnd;
	}
	#else


	//friend struct iterator;
	struct iterator
	{
		tConnList::iterator mIterator;
		//tConnList::iterator mNextIterator;
		cConnChoose *mChoose;
		sChooseRes mRes;

		iterator():mChoose(NULL){}

		iterator(tConnList::iterator Iterator, cConnChoose *Choose):
  			mIterator(Iterator),mChoose(Choose){}

		iterator &operator++(){

			for ( 	++mIterator;// = mNextIterator;
				(mIterator != mChoose->mConnList.end()) && !(mChoose->RevTest((*mIterator)->operator tSocket()));
				++mIterator)// = mNextIterator)
			{
				//++mNextIterator;
			}
			return *this;
		};

		sChooseRes & operator*()
		{
			mRes.mSock = (*mIterator)->operator tSocket();
			mRes.mEvent = mChoose->OptGet(mRes.mSock);
			mRes.mRevent = mChoose->RevGet(mRes.mSock);
			mRes.mConn  = *mIterator;
			return mRes;
		}

		bool operator!=(const iterator &it) const
		{
			return mIterator != it.mIterator;
		}

		bool operator==(const iterator &it) const
		{
			return mIterator == it.mIterator;
		}

		iterator &operator=(const iterator &it)
		{
			mIterator = it.mIterator;
			//mNextIterator = mIterator;
			//++mNextIterator;
			mChoose = it.mChoose;
			return *this;
		}
	};

	iterator &begin()
	{
		sBegin = iterator(mConnList.begin(),this);
		if(!RevTest((*(mConnList.begin()))->operator tSocket()))
			++sBegin;
		return sBegin;
	}

	iterator &end()
	{
		sEnd= iterator(mConnList.end(),this);
		return sEnd;
	}
	#endif //0

	tConnList mConnList;

protected:
	tSocket	mLastSock;
	static iterator sBegin;
	static iterator sEnd;
};
	}; // namespace nSocket
}; // namespace nVerliHub

#endif
