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

#include "cmysqlworker.h"
#include "casyncsocketserver.h"
#include "cconfmysql.h"
#include "cquery.h"
#include <cstring>
#include <future>
#include <memory>
#include <chrono>

#ifndef CR_SERVER_GONE_ERROR // note: from mysql/errmsg.h
	#define CR_SERVER_GONE_ERROR 2006
#endif

#ifndef CR_SERVER_LOST
	#define CR_SERVER_LOST 2013
#endif

using namespace std;

namespace nVerliHub {
	namespace nMySQL {

static constexpr size_t sMaxJobs = 10000; // note: queued queries, when database is slow or down more writes are dropped, they are statistics only
static constexpr long long sStopWait = 5000; // note: milliseconds that hub stop waits for queued writes, then rest is dropped

static long long SteadyMS()
{
	return (long long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

cMySQLWorker::cMySQLWorker(nSocket::cAsyncSocketServer &serv, cMySQL &main, const string &host, const string &user, const string &pass, const string &data, const string &charset, bool tls, const string &sock):
	cObj("cMySQLWorker"),
	mServ(serv),
	mMain(main),
	mHost(host),
	mUser(user),
	mPass(pass),
	mData(data),
	mCharset(charset),
	mTLS(tls),
	mSock(sock)
{
	mPool.SetDrain(true); // note: queued writes are done before hub stops, at most sStopWait, see Expired()
	mPool.SetOnStop([this] { mStopUntil = SteadyMS() + sStopWait; });
	mPool.SetThreadHooks(
		[] { mysql_thread_init(); },
		[this] {
			Disconnect();
			mysql_thread_end();
		}
	);

	mMain.mBeforeQuery = [this] { Flush(); }; // note: reads on main connection wait for queued writes, see Flush()
}

cMySQLWorker::~cMySQLWorker()
{
	mMain.mBeforeQuery = nullptr; // note: main connection can live longer
	mPool.Stop();
}

void cMySQLWorker::Flush()
{
	if (!mPool.IsRunning() || mPool.IsIdle()) // note: nothing queued or running, almost always
		return;

	auto done = std::make_shared<std::promise<void>>();
	std::future<void> wait = done->get_future();

	if (!mPool.Post([done]() -> nThread::cWorkerPool::tResult { // note: single thread, so this runs after every query queued before
		done->set_value();
		return nullptr;
	}))
		return;

	wait.wait_for(std::chrono::seconds(3)); // note: database is slow or down, main thread does not freeze for it, query runs anyway, so nothing is lost and nothing is logged
}

bool cMySQLWorker::Start()
{
	return (mPool.IsRunning() || mServ.StartPool(mPool, 1, sMaxJobs)); // note: works only while main loop is running
}

bool cMySQLWorker::Expired() const
{
	const long long until = mStopUntil;
	return (until && (SteadyMS() > until)); // note: hub is stopping and its wait time is over
}

void cMySQLWorker::Lost(bool drop, const string &err)
{
	if (drop)
		mDropped++;
	else
		mFailed++;

	if (err.size())
		mLastErr = err;

	Report();
}

void cMySQLWorker::Report()
{
	if (!mDropped && !mFailed)
		return;

	const std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();

	if ((now - mLastLostLog) < std::chrono::minutes(1)) // note: dont flood log while database is slow or down, rest is counted and logged later
		return;

	if (ErrLog(0)) { // note: same level as lost database connection, admin must see it, cause can also be full disk on database server
		LogStream() << "Database thread lost queries, failed: " << mFailed << ", dropped because queue is full: " << mDropped;

		if (mLastErr.size())
			LogStream() << ", last error: " << mLastErr;

		LogStream() << endl;
	}

	mDropped = 0;
	mFailed = 0;
	mLastErr.clear();
	mLastLostLog = now;
}

bool cMySQLWorker::Write(const string &query)
{
	if (!Start()) { // note: thread is not available, for example main loop is not running yet, then main connection is used
		cQuery main(mMain);
		main.OStream() << query;
		main.Query();
		main.Clear();
		return false;
	}

	const bool queued = mPool.Post([this, query]() -> nThread::cWorkerPool::tResult {
		string err;

		if (Expired()) // note: dropped
			return nullptr;

		if (Execute(query, err))
			return nullptr;

		return [this, err] { Lost(false, err); }; // note: query failed also after retry, counted on main thread
	});

	if (queued)
		return true;

	Lost(true, ""); // note: queue is full, main connection is not used, it would freeze main thread when database is slow
	return false;
}

bool cMySQLWorker::Select(const string &query, tSelectDone done)
{
	if (!done || !Start())
		return false;

	const bool queued = mPool.Post([this, query, done]() -> nThread::cWorkerPool::tResult {
		string err;
		tRows rows;
		bool ok = Execute(query, err);

		if (ok) {
			MYSQL_RES *res = mysql_store_result(mHandle);

			if (res) {
				const unsigned int cols = mysql_num_fields(res);
				MYSQL_ROW row;

				while ((row = mysql_fetch_row(res))) {
					const unsigned long *lens = mysql_fetch_lengths(res);
					tRow item;
					item.reserve(cols);

					for (unsigned int col = 0; col < cols; col++)
						item.emplace_back(row[col] ? string(row[col], lens[col]) : string());

					rows.push_back(std::move(item));
				}

				mysql_free_result(res);

			} else if (mysql_field_count(mHandle)) { // note: query should have returned rows
				ok = false;
				err = mysql_error(mHandle);
			}
		}

		return [this, err, ok, rows = std::move(rows), done] { // note: runs on main thread
			if (!ok) // note: failed also after retry
				Lost(false, err);

			done(ok, rows);
		};
	});

	if (!queued) // note: queue is full, caller gets failed result and tries again later, main connection is not used, it would freeze main thread when database is slow
		done(false, tRows());

	return true;
}

bool cMySQLWorker::Connect(string &err)
{
	if (mHandle)
		return true;

	mHandle = mysql_init(NULL);

	if (!mHandle) {
		err = "Unable to initialize MySQL structure";
		return false;
	}

	unsigned int tout = 10; // note: seconds, dont hold up hub stop forever when server does not answer
	mysql_options(mHandle, MYSQL_OPT_CONNECT_TIMEOUT, &tout);
	mysql_options(mHandle, MYSQL_OPT_READ_TIMEOUT, &tout);
	mysql_options(mHandle, MYSQL_OPT_WRITE_TIMEOUT, &tout);

	if (mCharset.size()) // note: same as main connection
		mysql_options(mHandle, MYSQL_SET_CHARSET_NAME, mCharset.c_str());
	else if (strcmp(DEFAULT_CHARSET, "") != 0)
		mysql_options(mHandle, MYSQL_SET_CHARSET_NAME, DEFAULT_CHARSET);

	if (!cMySQL::RealConnect(mHandle, mHost, mUser, mPass, mData, mTLS, mSock)) { // note: same host and port handling as main connection
		err = mysql_error(mHandle);
		Disconnect();
		return false;
	}

	return true;
}

void cMySQLWorker::Disconnect()
{
	if (mHandle) {
		mysql_close(mHandle);
		mHandle = NULL;
	}
}

bool cMySQLWorker::Execute(const string &query, string &err)
{
	for (int attempt = 0; attempt < 2; attempt++) {
		if (!Connect(err))
			return false;

		if (mysql_real_query(mHandle, query.data(), query.size()) == 0)
			return true;

		const unsigned int code = mysql_errno(mHandle);
		err = mysql_error(mHandle);

		if (code == CR_SERVER_LOST) { // note: lost during query, server may have done it already, so it is not sent again, same as main connection, next query reconnects
			Disconnect();
			return false;
		}

		if (code != CR_SERVER_GONE_ERROR) // note: only query that was not sent is worth another try
			return false;

		Disconnect(); // note: reconnect and try once more
	}

	return false;
}

	}; // namespace nMySQL
}; // namespace nVerliHub
