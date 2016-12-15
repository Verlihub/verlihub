/*
	Copyright (C) 2003-2005 Daniel Muller, dan at verliba dot cz
	Copyright (C) 2006-2017 Verlihub Team, info at verlihub dot net

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
#include "mysql_version.h"
#include "i18n.h"

namespace nVerliHub {

	namespace nMySQL {

cMySQL::cMySQL(): cObj("cMySQL")
{
	Init();
}

cMySQL::cMySQL(string &host, string &user, string &pass, string &data, string &charset): cObj("cMySQL"), mDBName(data)
{
	Init();

	if (!Connect(host, user, pass, data, charset))
		throw "MySQL connection error";
}

cMySQL::~cMySQL()
{
	mysql_close(mDBHandle);
}

void cMySQL::Init()
{
	mDBHandle = NULL;
	mDBHandle = mysql_init(mDBHandle);

	if (!mDBHandle)
		Error(0, _("Can't initiate MySQL structure"));
}

bool cMySQL::Connect(string &host, string &user, string &pass, string &data, string &charset)
{
	if (Log(1))
		LogStream() << "Connecting to MySQL server " << user << " @ " << host << " / " << data << " using charset " << ((charset.size()) ? charset : DEFAULT_CHARSET) << endl;

	mysql_options(mDBHandle, MYSQL_OPT_COMPRESS, 0);

	#if MYSQL_VERSION_ID >= 50000
		mysql_options(mDBHandle, MYSQL_OPT_RECONNECT, "true");
	#endif

	if (charset.size())
		mysql_options(mDBHandle, MYSQL_SET_CHARSET_NAME, charset.c_str());

	if (!mysql_real_connect(mDBHandle, host.c_str(), user.c_str(), pass.c_str(), data.c_str(), 0, 0, 0)) {
		Error(1, _("Connection to MySQL server failed"));
		return false;
	}

	return true;
}

void cMySQL::Error(int level, string text)
{
	if (ErrLog(level))
		LogStream() << text << ": " << mysql_error(mDBHandle) << endl;
}

	}; // namepspace nMySQL
}; // namespace nVerliHub
