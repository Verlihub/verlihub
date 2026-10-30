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

	Please see https://www.gnu.org/licenses/ for a copy
	of the GNU General Public License.
*/

#ifndef NMYSQLCMYSQLWORKER_H
#define NMYSQLCMYSQLWORKER_H

#include "cmysql.h"
#include "cworkerpool.h"
#include "cobj.h"
#include <functional>
#include <string>
#include <vector>
#include <atomic>
#include <chrono>

namespace nVerliHub {
	namespace nSocket {
		class cAsyncSocketServer;
	};

	namespace nMySQL {

/*
	database thread with its own mysql connection, so main thread does not wait for writes it does not need result of
	it is also used to read big tables in background, result comes back on main thread
	single thread, so queries run in same order as they were queued, on hub stop all queued queries are still done

	note: connection is opened, used and closed only on its own thread, mysql connection can not be shared between threads
	note: query that can not be queued runs on main connection instead, so nothing is lost when thread is not running
*/

class cMySQLWorker: public cObj
{
	public:
		using tRow = std::vector<std::string>; // note: null value is empty string
		using tRows = std::vector<tRow>;
		using tSelectDone = std::function<void(bool ok, const tRows &rows)>;

		cMySQLWorker(nSocket::cAsyncSocketServer &serv, cMySQL &main, const string &host, const string &user, const string &pass, const string &data, const string &charset, bool tls = false, const string &sock = "");
		~cMySQLWorker();

		cMySQLWorker(const cMySQLWorker&) = delete;
		cMySQLWorker& operator=(const cMySQLWorker&) = delete;

		// queue query that returns nothing, it runs on main connection when thread is not available, it is dropped when queue is full, returns true when queued
		bool Write(const string &query);

		// queue select, done is called on main thread, returns false when thread is not available, caller must then do it by itself, when queue is full done gets failed result at once
		bool Select(const string &query, tSelectDone done);

		/*
			main thread waits until queries queued before are done, at most few seconds, returns at once when nothing is queued
			note: every query on main connection calls it first, so read after write sees that write, same order as on single thread, for example login statistics in script
		*/
		void Flush();

		// log of lost queries, failed after retry or dropped because queue is full, at most once a minute, hub timer calls it, so last count is not left unlogged
		void Report();

	private:
		bool Start();
		bool Expired() const;
		void Lost(bool drop, const string &err);

		// worker thread only
		bool Connect(string &err);
		void Disconnect();
		bool Execute(const string &query, string &err);

		nSocket::cAsyncSocketServer &mServ;
		cMySQL &mMain;
		string mHost, mUser, mPass, mData, mCharset;
		bool mTLS;
		string mSock;
		MYSQL *mHandle = NULL; // note: used only on worker thread
		std::atomic<long long> mStopUntil{0}; // note: set on main thread when hub stops, read on worker thread
		unsigned long mDropped = 0; // note: main thread only, same for next three
		unsigned long mFailed = 0;
		string mLastErr;
		std::chrono::steady_clock::time_point mLastLostLog;
		nThread::cWorkerPool mPool; // note: declared last, so its thread is stopped before other members are destroyed
};

	}; // namespace nMySQL
}; // namespace nVerliHub

#endif
