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
#include <vector>

#define VH_TABLE_NAME "VH"

using namespace std;
namespace nVerliHub {
	namespace nLuaPlugin {

class cLuaInterpreter
{
public:
	cLuaInterpreter(string, string);
	~cLuaInterpreter();

	bool Init();
	void ReportLuaError(const char*);
	bool CallFunction(const char*, const char *[], cConnDC *conn = NULL);
	void RegisterFunction(const char *func, int (*ptr)(lua_State*));
	void VHPushString(const char *name, const char *val, bool update = false);
	void Load();

	string mConfigName;
	string mScriptName;

	struct mScriptBot {
		const char *uNick;
		const char *uMyINFO;
		int uShare;
		int uClass;
	};

	typedef vector<mScriptBot *> tvBot;
	tvBot botList;

	void addBot(const char *Nick, const char *MyINFO, int Share, int Class) {
		bool add = true;

		for (unsigned int i = 0; i < botList.size(); i++) {
			if (strcmp(botList[i]->uNick, Nick) == 0) {
				add = false;
				break;
			}
		}

		if (add) {
			mScriptBot *item = new mScriptBot;
			item->uNick = Nick;
			item->uMyINFO = MyINFO;
			item->uShare = Share;
			item->uClass = Class;
			botList.push_back(item);
		}
	}

	void editBot(const char *Nick, const char *MyINFO, int Share, int Class) {
		mScriptBot *bot = NULL;

		for (unsigned int i = 0; i < botList.size(); i++) {
			if (strcmp(botList[i]->uNick, Nick) == 0) {
				bot = botList[i];
				break;
			}
		}

		if (bot) {
			// dont need to set nick
			bot->uMyINFO = MyINFO;
			bot->uShare = Share;
			bot->uClass = Class;
		}
	}

	void delBot(const char *Nick) {
		for (unsigned int i = 0; i < botList.size(); i++) {
			if (strcmp(botList[i]->uNick, Nick) == 0) {
				botList.erase(botList.begin() + i);
				break;
			}
		}
	}

	void clean() {
		tvBot::iterator it;

		for (it = botList.begin(); it != botList.end(); ++it) {
			if (*it) {
				delete (*it);
				(*it) = NULL;
			}
		}

		botList.clear();
	}

	lua_State *mL;
};

	}; // namespace nLuaPlugin
}; // namespace nVerliHub

#endif
