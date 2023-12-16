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

#ifndef NSCRIPTSCLUAINTERPRETER_H
#define NSCRIPTSCLUAINTERPRETER_H

extern "C"
{
	#include "lua.h"
	#include "lualib.h"
	#include "lauxlib.h"
}

#include "src/cconndc.h"
#include <cstring>
#include <string>
#include <iostream>
#include <map>

#define VH_TABLE_NAME "VH"

using namespace std;

namespace nVerliHub {
	namespace nLuaPlugin {

class cLuaInterpreter
{
public:
	cLuaInterpreter(const string&, const string&);
	~cLuaInterpreter();

	bool Init();
	void ReportLuaError(const char*);
	bool CallFunction(const char*, const char *[], cConnDC *conn = NULL);
	void RegisterFunction(const char *func, int (*ptr)(lua_State*));
	void VHPushString(const char *name, const char *val, bool update = false);
	void Load();

	string mConfigName;
	string mScriptName;

	struct mScriptBot
	{
		string uMyINFO;
		int uShare;
		int uClass;
	};

	typedef map<string, mScriptBot> tvBot;
	tvBot botList;

	void addBot(const char *nick, const char *info, int shar, int clas)
	{
		mScriptBot &item = botList[nick];
		item.uMyINFO = info;
		item.uShare = shar;
		item.uClass = clas;
	}

	void editBot(const char *nick, const char *info, int shar, int clas)
	{
		tvBot::iterator it = botList.find(nick);

		if (it != botList.end()) {
			it->second.uMyINFO = info;
			it->second.uShare = shar;
			it->second.uClass = clas;
		}
	}

	void delBot(const char *nick)
	{
		botList.erase(nick);
	}

	void clean()
	{
		botList.clear();
	}

	lua_State *mL;
};

	}; // namespace nLuaPlugin
}; // namespace nVerliHub

#endif
