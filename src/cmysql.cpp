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

#include "cmysql.h"
#include "cconfmysql.h"
#include "i18n.h"
#include <string.h>

namespace nVerliHub {

	namespace nMySQL {

cMySQL::cMySQL(string &host, string &user, string &pass, string &data, string &charset):
	cObj("cMySQL"),
	mDBName(data),
	mDBHandle(NULL)
{
	Init();

	if (!Connect(host, user, pass, data, charset)) {
		Close();
		throw "Exiting due to unavailable MySQL connection";
	}
}

cMySQL::~cMySQL()
{
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
		if (Log(0))
			LogStream() << "Closing MySQL connection" << endl;

		mysql_close(mDBHandle);
		mDBHandle = NULL;
	}

	mysql_library_end();
}

bool cMySQL::Connect(string &host, string &user, string &pass, string &data, string &charset)
{
	if (Log(0))
		LogStream() << "Connecting to MySQL server " << user << " @ " << host << " / " << data << " using charset " << ((charset.size()) ? charset : ((strcmp(DEFAULT_CHARSET, "") != 0) ? DEFAULT_CHARSET : "<default>")) << endl;

	bool yes = true;
	mysql_options(mDBHandle, MYSQL_OPT_RECONNECT, &yes);

	if (charset.size())
		mysql_options(mDBHandle, MYSQL_SET_CHARSET_NAME, charset.c_str());
	else if (strcmp(DEFAULT_CHARSET, "") != 0)
		mysql_options(mDBHandle, MYSQL_SET_CHARSET_NAME, DEFAULT_CHARSET);

	if (!mysql_real_connect(mDBHandle, host.c_str(), user.c_str(), pass.c_str(), data.c_str(), 0, NULL, 0)) {
		Error(0, mysql_error(mDBHandle));
		return false;
	}

	return true;
}

void cMySQL::Error(int level, const string &text)
{
	if (ErrLog(level))
		LogStream() << text << ": " << (mDBHandle ? mysql_error(mDBHandle) : "Unknown error") << endl;
}

	}; // namepspace nMySQL
}; // namespace nVerliHub
