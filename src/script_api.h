/*
	Copyright (C) 2003-2005 Daniel Muller, dan at verliba dot cz
	Copyright (C) 2006-2012 Verlihub Team, devs at verlihub-project dot org
	Copyright (C) 2013-2014 RoLex, webmaster at feardc dot net

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

#ifndef _WIN32
#define __int64 long long
#endif
#include <map>
#include <string>

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
	bool CloseConnection(char *nick);
	bool CloseConnectionNice(char *nick);
	bool StopHub(int code);
	char* GetUserCC(char *nick);

	#if HAVE_LIBGEOIP
		string GetIPCC(const string);
		string GetIPCN(const string);
	#endif

	char* GetMyINFO(char *nick);
	int GetUserClass(char *nick);
	char* GetUserHost(char *nick);
	char* GetUserIP(char *nick);
	bool Ban(char *, const string, const string, unsigned, unsigned);
	bool ParseCommand(char *nick, char *cmd, int pm);
	bool KickUser(char *op, char *nick, char *reason);
	bool SetConfig(char *config_name, char *var, char *val);
	int GetConfig(char *config_name, char *var, char *val, int size);
	char* GetVHCfgDir();
	bool GetTempRights(char *nick, map<string,int> &rights);
	bool AddRegUser(char *nick, int uClass, char *passwd, char* op);
	bool DelRegUser(char *nick);
	bool ScriptCommand(string, string, string, string);
	int CheckBotNick(const string &nick);
	bool CheckDataPipe(const string &data);

	extern "C" {
		int GetUsersCount();
		char* GetNickList();
	}

	__int64 GetTotalShareSize();
};
