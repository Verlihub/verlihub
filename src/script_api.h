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

#ifndef SCRIPT_API_H
#define SCRIPT_API_H

//#ifndef _WIN32
	#define __int64 long long
//#endif

#include <map>
#include <string>
#include "cvhplugin.h"

using namespace std;

namespace nVerliHub {
	bool SendDataToUser(const char *data, const char *nick, bool delay = false);
	bool SendToClass(const char *data, int min_class = 0, int max_class = 10, bool delay = false);
	bool SendToAll(const char *data, bool delay = false);
	bool SendToActive(const char *data, bool delay = false);
	bool SendToActiveClass(const char *data, int min_class = 0, int max_class = 10, bool delay = false);
	bool SendToPassive(const char *data, bool delay = false);
	bool SendToPassiveClass(const char *data, int min_class = 0, int max_class = 10, bool delay = false);
	bool SendPMToAll(const char *data, const char *from, int min_class = 0, int max_class = 10);
	bool SendToChat(const char *nick, const char *text, int min_class = 0, int max_class = 10);
	bool SendToOpChat(const char *data, const char *nick = NULL);
	bool CloseConnection(const char *nick, long delay = 0);
	bool StopHub(int code, int delay);
	string GetIPCC(const char *ip);
	string GetIPCN(const char *ip);
	string GetIPCity(const char *ip, const char *db = NULL);
	const char* GetMyINFO(const char *nick);
	int GetUserClass(const char *nick);
	const char* GetUserHost(const char *nick);
	const char* GetUserIP(const char *nick);
	bool SetUserIP(const char *nick, const char *ip);
	bool SetMyINFOFlag(const char *nick, const unsigned int flag);
	bool UnsetMyINFOFlag(const char *nick, const unsigned int flag);
	bool Ban(const char *, const string &, const string &, unsigned, unsigned);
	bool KickUser(const char *oper, const char *nick, const char *why, const char *note_op = NULL, const char *note_usr = NULL, bool hide = false);
	bool DeleteNickTempBan(const char *nick);
	bool DeleteIPTempBan(const char *ip);
	bool ParseCommand(const char *nick, const char *cmd, int pm);
	bool SetConfig(const char *conf, const char *var, const char *val);
	char* GetConfig(const char *conf, const char *var, const char *def = NULL);
	const char* GetVHCfgDir();
	//bool GetTempRights(const char *nick, map<string,int> &rights);
	bool AddRegUser(const char *nick, int uclass, const char *pass, const char* op);
	bool DelRegUser(const char *nick);
	bool SetRegClass(const char *nick, int clas);
	bool ScriptCommand(string *cmd, string *data, string *plug, string *script, bool inst = false);
	int CheckBotNick(const string &nick);
	bool CheckDataPipe(const string &data);

	extern "C"
	{
		int GetUsersCount();
		const char* GetNickList();
	}

	unsigned __int64 GetTotalShareSize();
};

#endif
