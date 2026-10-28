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

#ifndef CR_SERVER_GONE_ERROR // note: from mysql/errmsg.h
	#define CR_SERVER_GONE_ERROR 2006
#endif

#ifndef CR_SERVER_LOST
	#define CR_SERVER_LOST 2013
#endif

using namespace std;

namespace nVerliHub {
	namespace nMySQL {

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
	mPool.SetDrain(true); // note: queued writes are done before hub stops
	mPool.SetThreadHooks(
		[] { mysql_thread_init(); },
		[this] {
			Disconnect();
			mysql_thread_end();
		}
	);
}

cMySQLWorker::~cMySQLWorker()
{
	mPool.Stop();
}

bool cMySQLWorker::Start()
{
	return (mPool.IsRunning() || mServ.StartPool(mPool, 1)); // note: works only while main loop is running
}

bool cMySQLWorker::Write(const string &query)
{
	const bool queued = Start() && mPool.Post([this, query]() -> nThread::cWorkerPool::tResult {
		string err;

		if (Execute(query, err))
			return nullptr;

		return [this, query, err] { // note: log on main thread
			if (ErrLog(1))
				LogStream() << "Query failed on database thread: " << err << " :: " << query << endl;
		};
	});

	if (queued)
		return true;

	cQuery main(mMain); // note: thread is not available, run it on main connection like before
	main.OStream() << query;
	main.Query();
	main.Clear();
	return false;
}

bool cMySQLWorker::Select(const string &query, tSelectDone done)
{
	if (!done || !Start())
		return false;

	return mPool.Post([this, query, done]() -> nThread::cWorkerPool::tResult {
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

		return [this, query, err, ok, rows = std::move(rows), done] { // note: runs on main thread
			if (!ok && ErrLog(1))
				LogStream() << "Query failed on database thread: " << err << " :: " << query << endl;

			done(ok, rows);
		};
	});
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

		if ((code != CR_SERVER_GONE_ERROR) && (code != CR_SERVER_LOST)) // note: only lost connection is worth another try
			return false;

		Disconnect(); // note: reconnect and try once more
	}

	return false;
}

	}; // namespace nMySQL
}; // namespace nVerliHub
