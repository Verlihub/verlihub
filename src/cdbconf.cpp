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

#include "cdbconf.h"

namespace nVerliHub {
	namespace nConfig {
cDBConf::cDBConf(const string &file): cConfigFile(file, false)
{
	msLogLevel = 1;
	Add("db_host", db_host, string("localhost"));
	Add("db_user", db_user, string("verlihub"));
	Add("db_pass", db_pass, string(""));
	Add("db_data", db_data, string("verlihub"));
	Add("db_charset", db_charset, string(""));
	Add("config_name", config_name, string("config"));
	Add("locale", locale, string(""));
	Load();
}

cDBConf::~cDBConf()
{}

	}; // namespace nConfig
}; // namespace nVerliHub
