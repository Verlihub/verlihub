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

	Please see http://www.gnu.org/licenses/ for a copy
	of the GNU General Public License.
*/

#ifndef CMYSQL_H
#define CMYSQL_H

/*
#if ! defined _WIN32
//#include <mysql/mysql.h>
#else
typedef unsigned int SOCKET;
#endif
*/

#include <mysql/mysql.h>
//#include <mysql/errmsg.h>

#include "cobj.h"
#include <chrono>
#include <functional>

#ifndef CR_SERVER_GONE_ERROR // note: from mysql/errmsg.h, connection is gone before query is sent
	#define CR_SERVER_GONE_ERROR 2006
#endif

#ifndef CR_SERVER_LOST // note: connection is lost during query, query can be done already
	#define CR_SERVER_LOST 2013
#endif

namespace nVerliHub {
/**
   Mysql utilities
   Contains classes that encapsulate mysql structures,  and ease their usage
*/
	namespace nMySQL {

/**
a class encapsulating operations with mysql conenction

@author Daniel Muller
*/
class cMySQL: public cObj
{
	friend class cQuery;
	public:
		cMySQL(string &host, string &user, string &pass, string &data, string &charset, bool tls = false, const string &sock = "");
		~cMySQL();
		void Init();
		bool Connect(bool quiet = false);
		void Close();

		string GetDBName()
		{
			return mDBName;
		}

		// split db_host into host and port, port is 0 when not given
		static void SplitHost(const string &addr, string &host, unsigned int &port);

		// connect to server, same for main connection and database worker, tls requires encrypted connection, sock is local socket path or empty for default
		static bool RealConnect(MYSQL *handle, const string &addr, const string &user, const string &pass, const string &data, bool tls, const string &sock = "");

		// connect again after lost connection, for example database server restart, at most once in few seconds and with short timeout, because main connection runs on main thread
		bool Reconnect();

		// message for operators when connection was lost or is back, it is made inside of query, so hub sends it later from its timer
		bool TakeNotice(string &text);

		void Error(int level, const string& text);

		// called before every query on this connection, database thread sets it, so main connection first waits for queued writes
		std::function<void()> mBeforeQuery;

	private:
		string mDBName, mDBHost, mDBUser, mDBPass, mDBChar;
		bool mDBTLS;
		string mDBSock;
		MYSQL *mDBHandle;
		bool mLost; // note: connection is lost and not yet back
		unsigned long mFailed; // note: failed reconnect attempts since connection was lost
		std::chrono::steady_clock::time_point mLostSince, mLastTry, mLastLog; // note: when connection was lost, last reconnect attempt and last log line about it
		string mNotice; // note: waiting message for operators
};

	}; // namespace nMySQL
}; // namespace nVerliHub

#endif
