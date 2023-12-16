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

#include "cconnchoose.h"
#ifndef NSERVERCCONNPOLL_H
#define NSERVERCCONNPOLL_H
#if !USE_SELECT
#include <sys/poll.h>
#if !defined _SYS_POLL_H_ && !defined _SYS_POLL_H && !defined pollfd && !defined _POLL_EMUL_H_ && !defined _POLL_H
/** the poll file descriptor structure (where not defined) */
struct pollfd {
   int fd;           /* file descriptor */
   short events;     /* requested events */
   short revents;    /* returned events */
};
#endif

#include <vector>
using std::vector;

namespace nVerliHub {
	namespace nSocket {

/**
polling connection chooser

@author Daniel Muller
*/
class cConnPoll : public cConnChoose
{
public:
	cConnPoll();
	~cConnPoll();

	/** Calls the poll function to determine non-blocking socket
	  * \sa cConnChoose::Choose
	  */
	virtual int Choose(nUtils::cTime &tmout)
	{
		return this->poll((int)tmout.MiliSec());
	}

	/**
	* Register the connection for the given I/O operation.
	* @param conn The connection.
	* @param event Bitwise OR list of I/O operation.
	*/
	virtual void OptIn(tSocket socket, nEnums::tChEvent event);
	using cConnChoose::OptIn;
	/**
	* Unregister the connection for the given I/O operations.
	* @param conn The connection.
	* @param event Bitwise OR list of I/O operation.
	*/
	virtual void OptOut(tSocket socket, nEnums::tChEvent event);
	using cConnChoose::OptOut;
	/**
	* Unregister the connection for the given I/O operation.
	* @param conn The connection.
	* @param event Bitwise OR list of I/O operation.
	*/
	virtual int OptGet(tSocket socket);
	using cConnChoose::OptGet;

	/// @see cConnChoose::RevGet

	virtual int RevGet(tSocket socket);

	virtual bool RevTest(tSocket socket);

//  	void OptOut(cConnBase *conn, nEnums::tChEvent event) { cConnChoose::OptOut(conn,event); };
//  	void OptIn(cConnBase *conn, nEnums::tChEvent event) { cConnChoose::OptIn(conn,event); };
//  	void OptGet(cConnBase *conn) { cConnChoose::OptGet(conn); };

	/**
	* Add new connection to be handled by connection manager.
	* @param conn The connection.
	* @return True if connection is added; otherwise false.
	*/
	virtual bool AddConn(cConnBase *conn);

	/**
	  * Wrapper for pollfd structure. It provides constructor and reset method
	  * @author Daniel Muller
	*/
	struct cPollfd: public pollfd
	{
		cPollfd()
		{
			reset();
		}
		void reset()
		{
			fd=-1;
			events=revents=0;
		}
	};

	virtual bool RevTest(cPollfd &);

	int poll(int wp_sec);
	typedef vector<cPollfd> tFDArray;

	cPollfd &FD(int sock)
	{
		return mFDs[sock];
	}

protected:
	tFDArray mFDs;

	const int mBlockSize;
};

	}; // namespace nSocket
}; // namespace nVerliHub

#endif

#endif
