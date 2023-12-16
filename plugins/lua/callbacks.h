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

#ifndef CALLBACKS_H
#define CALLBACKS_H

extern "C"
{
    #include <lua.h>
}

#include "cpilua.h"
#include "cluainterpreter.h"

namespace nVerliHub {
	int _SendToUser(lua_State *L);
	int _SendToClass(lua_State *L);
	int _SendToAll(lua_State *L);
	int _SendToActive(lua_State *L);
	int _SendToActiveClass(lua_State *L);
	int _SendToPassive(lua_State *L);
	int _SendToPassiveClass(lua_State *L);
	int _SendPMToAll(lua_State *L);
	int _SendToChat(lua_State *L);
	int _SendToOpChat(lua_State *L);
	int _Disconnect(lua_State *L);
	int _StopHub(lua_State *L);
	int _GetMyINFO(lua_State *L);
	int _GetUserCC(lua_State *L);
	int _GetUserCN(lua_State *L);
	int _GetUserCity(lua_State *L);
	int _GetIPCC(lua_State *L);
	int _GetIPCN(lua_State *L);
	int _GetIPCity(lua_State *L);
	int _GetIPASN(lua_State *L);
	int _GetUserGeoIP(lua_State *L);
	int _GetHostGeoIP(lua_State *L);
	int _GetVHCfgDir(lua_State *L);
	int _GetUpTime(lua_State *L);
	int _GetServFreq(lua_State *L);
	int _RegBot(lua_State *L);
	int _UnRegBot(lua_State *L);
	int _EditBot(lua_State *L);
	int _IsBot(lua_State *L);
	int _GetHubIp(lua_State *L);
	int _GetHubSecAlias(lua_State *L);
	int _SetConfig(lua_State *L);
	int _GetConfig(lua_State *L);
	int _GetUserClass(lua_State *L);
	int _GetUserHost(lua_State *L);
	int _GetUserIP(lua_State *L);
	int _SetUserIP(lua_State *L);
	int _SetMyINFOFlag(lua_State *L);
	int _UnsetMyINFOFlag(lua_State *L);
	int _IsSecConn(lua_State *L);
	int _GetTLSVer(lua_State *L);
	int _IsUserOnline(lua_State *L);
	int _GetUserSupports(lua_State *L);
	int _GetUserHubURL(lua_State *L);
	int _GetUserExtJSON(lua_State *L);
	int _GetUserVersion(lua_State *L);
	int _InUserSupports(lua_State *L);
	int _GetNickList(lua_State *L);
	int _GetOPList(lua_State *L);
	int _GetBotList(lua_State *L);
	int _GetLuaBots(lua_State *L);
	int _PassTempBan(lua_State *L);
	int _Ban(lua_State *L);
	int _KickUser(lua_State *L);
	int _KickRedirUser(lua_State *L);
	int _DelNickTempBan(lua_State *L);
	int _DelIPTempBan(lua_State *L);
	int _ReportUser(lua_State *L);
	int _ParseCommand(lua_State *L);
	int _SQLQuery(lua_State *L);
	int _SQLFetch(lua_State *L);
	int _SQLFree(lua_State *L);
	int _GetUsersCount(lua_State *L);
	int _GetTotalShareSize(lua_State *L);
	int _GetTempRights(lua_State *L);
	int _SetTempRights(lua_State *L);
	int _AddChatUser(lua_State *L);
	int _DelChatUser(lua_State *L);
	int _IsChatUser(lua_State *L);
	int _AddRegUser(lua_State *L);
	int _DelRegUser(lua_State *L);
	int _SetRegClass(lua_State *L);
	int _GetTopic(lua_State *L);
	int _SetTopic(lua_State *L);
	int _ScriptCommand(lua_State *L);

	nLuaPlugin::cLuaInterpreter *FindLua(lua_State *L);
	void luaerror(lua_State *L, const char *errstr);
	string IntToStr(int val);
	int StrToInt(string const &str);
}; // namespace nVerliHub

#endif
