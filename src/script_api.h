/*
	Copyright (C) 2003-2005 Daniel Muller, dan at verliba dot cz
	Copyright (C) 2006-2016 Verlihub Team, info at verlihub dot net

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

#ifndef _WIN32
#define __int64 long long
#endif
#include <map>
#include <string>
#include "cvhplugin.h"

using namespace std;

namespace nVerliHub {
	bool SendDataToUser(char *data, char *nick);
	bool SendToClass(char *data, int min_class, int max_class);
	bool SendToAll(char *data);
	bool SendToActive(char *data);
	bool SendToActiveClass(char *data, int min_class, int max_class);
	bool SendToPassive(char *data);
	bool SendToPassiveClass(char *data, int min_class, int max_class);
	bool SendPMToAll(char *data, char *from, int min_class, int max_class);
	bool SendToOpChat(char *data);
	bool CloseConnection(char *nick, long delay = 0);
	bool StopHub(int code, unsigned delay);
	char* GetUserCC(char *nick);

	#ifdef HAVE_LIBGEOIP
		const string GetIPCC(const char *ip);
		const string GetIPCN(const char *ip);
	#endif

	const char* GetMyINFO(char *nick);
	int GetUserClass(char *nick);
	const char* GetUserHost(char *nick);
	const char* GetUserIP(char *nick);
	bool Ban(char *, const string &, const string &, unsigned, unsigned);
	bool ParseCommand(char *nick, char *cmd, int pm);
	bool KickUser(char *opnick, char *nick, char *reason);
	bool SetConfig(const char *config_name, const char *var, const char *val);
	int GetConfig(char *config_name, char *var, char *val, int size);
	char* GetVHCfgDir();
	bool GetTempRights(char *nick, map<string,int> &rights);
	bool AddRegUser(char *nick, int uclass, char *pass, char* op);
	bool DelRegUser(char *nick);
	bool ScriptCommand(string *cmd, string *data, string *plug, string *script);
	bool ScriptQuery(string *cmd, string *data, string *recipient, string *sender, ScriptResponses *responses);
	int CheckBotNick(const string &nick);
	bool CheckDataPipe(const string &data);

	extern "C" {
		int GetUsersCount();
		const char* GetNickList();
	}

	unsigned __int64 GetTotalShareSize();
};

#endif
