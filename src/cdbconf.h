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

#ifndef NCONFIGCDBCONF_H
#define NCONFIGCDBCONF_H

#include "cconfigfile.h"

namespace nVerliHub {
	namespace nConfig {

class cDBConf: public cConfigFile
{
public:
	cDBConf(const string &);
	~cDBConf();

	string db_host;
	string db_user;
	string db_pass;
	string db_data;
	string db_charset;
	string config_name;
	string locale;
	string mmdb_path;
	//bool allow_exec;
	//bool allow_exec_mod;
};

	}; // namespace nConfig
};// namespace nVerliHub

#endif
