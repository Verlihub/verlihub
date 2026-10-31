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

#include "cmysql.h"
#include "cconfmysql.h"
#include "i18n.h"
#include <string.h>
#include <stdlib.h>

namespace nVerliHub {

	namespace nMySQL {

cMySQL::cMySQL(string &host, string &user, string &pass, string &data, string &charset, bool tls, const string &sock, bool info):
	cObj("cMySQL"),
	mDBName(data),
	mDBHost(host),
	mDBUser(user),
	mDBPass(pass),
	mDBChar(charset),
	mDBTLS(tls),
	mDBSock(sock),
	mInfo(info),
	mDBHandle(NULL),
	mLost(false),
	mFailed(0)
{
	Init();

	if (!Connect()) {
		Close();
		throw "Exiting due to unavailable MySQL connection";
	}
}

cMySQL::~cMySQL()
{
	if (mDBHandle)
		Close();
}

void cMySQL::Init()
{
	if (mysql_library_init(0, NULL, NULL))
		throw "Unable to initialize MySQL library";

	mDBHandle = mysql_init(NULL);

	if (!mDBHandle)
		throw "Unable to initialize MySQL structure";
}

void cMySQL::Close()
{
	if (mDBHandle) {
		if (mInfo && Log(0))
			LogStream() << "Closing MySQL connection" << endl;

		mysql_close(mDBHandle);
		mDBHandle = NULL;
	}

	mysql_library_end();
}

bool cMySQL::Connect(bool quiet)
{
	if (!quiet && mInfo && Log(0))
		LogStream() << "Connecting to MySQL/MariaDB server " << mDBUser << " @ " << mDBHost << " / " << mDBName << " using charset " << ((mDBChar.size()) ? mDBChar : ((strcmp(DEFAULT_CHARSET, "") != 0) ? DEFAULT_CHARSET : "<default>")) << endl;

	if (mDBChar.size()) // note: no automatic reconnect of client library, it is deprecated, see Reconnect()
		mysql_options(mDBHandle, MYSQL_SET_CHARSET_NAME, mDBChar.c_str());
	else if (strcmp(DEFAULT_CHARSET, "") != 0)
		mysql_options(mDBHandle, MYSQL_SET_CHARSET_NAME, DEFAULT_CHARSET);

	unsigned int wait = 10; // note: seconds for read and write, main thread waits for every query, so database that does not answer must not hold hub forever, same as database thread
	mysql_options(mDBHandle, MYSQL_OPT_READ_TIMEOUT, &wait);
	mysql_options(mDBHandle, MYSQL_OPT_WRITE_TIMEOUT, &wait);

	if (!RealConnect(mDBHandle, mDBHost, mDBUser, mDBPass, mDBName, mDBTLS, mDBSock)) {
		if (!quiet)
			Error(0, "Failed connecting to MySQL/MariaDB server: "); // note: error appends mysql error text

		return false;
	}

	return true;
}

bool cMySQL::Reconnect()
{
	const std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();

	if (mLost && ((now - mLastTry) < std::chrono::seconds(5))) // note: database is still down, dont block main loop on every query
		return false;

	if (!mLost) { // note: first time, log and opchat get it only when reconnect fails, next attempts are quiet
		mLostSince = now;
		mLastLog = now;
		mFailed = 0;
		mNotice = _("Lost connection to database server, hub reconnects by itself, until then registered users can't login and commands that use database fail.");
	}

	mLost = true;
	mLastTry = now;

	if (mDBHandle) // note: library stays initialized, only handle is made again
		mysql_close(mDBHandle);

	mDBHandle = mysql_init(NULL);

	if (!mDBHandle) {
		if (ErrLog(0))
			LogStream() << "Unable to initialize MySQL structure" << endl;

		return false;
	}

	unsigned int timeout = 3; // note: main thread waits for it
	mysql_options(mDBHandle, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);

	if (!Connect(true)) { // note: same charset, tls, port and socket as on start
		mFailed++;

		if (mFailed == 1) { // note: connection is lost only when reconnect failed
			mLastLog = now;

			if (ErrLog(0))
				LogStream() << "Lost connection to MySQL/MariaDB server, reconnect failed: " << mysql_error(mDBHandle) << endl;

		} else if ((now - mLastLog) >= std::chrono::minutes(1)) { // note: dont flood log while database is down
			mLastLog = now;

			if (ErrLog(0))
				LogStream() << "Still no connection to MySQL/MariaDB server after " << mFailed << " attempts: " << mysql_error(mDBHandle) << endl;
		}

		return false;
	}

	mLost = false;
	const long secs = long(std::chrono::duration_cast<std::chrono::seconds>(now - mLostSince).count());

	if (mFailed) { // note: same log as lost connection, else reader thinks it never came back, notice replaces lost notice when it was not sent yet, both are not needed then
		if (ErrLog(0))
			LogStream() << "Reconnected to MySQL/MariaDB server after " << secs << " seconds" << endl;

		mNotice = string(autosprintf(_("Connection to database server is back after %ld seconds."), secs));
	} else { // note: first attempt worked, nothing was lost, so no log and no notice
		mNotice.clear();
	}

	return true;
}

bool cMySQL::TakeNotice(string &text)
{
	if (mNotice.empty())
		return false;

	text.swap(mNotice);
	mNotice.clear();
	return true;
}

void cMySQL::SplitHost(const string &addr, string &host, unsigned int &port)
{
	host = addr;
	port = 0; // note: default port
	const size_t pos = addr.find(':');

	if ((pos == string::npos) || (addr.find(':', pos + 1) != string::npos)) // note: no port, or more colons like in ipv6 address
		return;

	const string num = addr.substr(pos + 1);

	if (num.empty() || (num.size() > 5) || (num.find_first_not_of("0123456789") != string::npos)) // note: not a port, whole value stays host and connection reports it
		return;

	const unsigned long val = strtoul(num.c_str(), NULL, 10);

	if (!val || (val > 65535))
		return;

	host = addr.substr(0, pos);
	port = val;
}

bool cMySQL::RealConnect(MYSQL *handle, const string &addr, const string &user, const string &pass, const string &data, bool tls, const string &sock)
{
	string host;
	unsigned int port;
	SplitHost(addr, host, port);

#if defined(MARIADB_PACKAGE_VERSION) // note: mariadb connector/c 3.4 and later requires tls by default, so not required must be set too
	my_bool enforce = (tls ? 1 : 0);
	mysql_options(handle, MYSQL_OPT_SSL_ENFORCE, &enforce);

	if (!tls) {
		my_bool verify = 0;
		mysql_options(handle, MYSQL_OPT_SSL_VERIFY_SERVER_CERT, &verify);
	}
#elif defined(LIBMYSQL_VERSION_ID) && (LIBMYSQL_VERSION_ID >= 50711) // note: mysql client uses tls when server has it, but does not require it
	if (tls) {
		unsigned int mode = SSL_MODE_REQUIRED;
		mysql_options(handle, MYSQL_OPT_SSL_MODE, &mode);
	}
#endif

	if (port) { // note: with localhost client uses local socket and ignores port, so force tcp when port is given
		unsigned int proto = MYSQL_PROTOCOL_TCP;
		mysql_options(handle, MYSQL_OPT_PROTOCOL, &proto);
	}

	const char *path = ((!port && sock.size()) ? sock.c_str() : NULL); // note: client library uses socket only for localhost without port, else default path of library
	return (mysql_real_connect(handle, host.c_str(), user.c_str(), pass.c_str(), (data.size() ? data.c_str() : NULL), port, path, 0) != NULL); // note: empty database is used by installation to check account only
}

void cMySQL::Error(int level, const string &text)
{
	if (ErrLog(level))
		LogStream() << text << (mDBHandle ? mysql_error(mDBHandle) : "Unknown error") << endl;
}

	}; // namepspace nMySQL
}; // namespace nVerliHub
