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

#include "src/cserverdc.h"
#include "src/script_api.h"
#include "callbacks.h"
#include "cluainterpreter.h"
#include "cpilua.h"
#include <iostream>
#include <stdlib.h>
#include <string>
#include "src/i18n.h"

using namespace std;

namespace nVerliHub {
	using namespace nSocket;
	namespace nLuaPlugin {

cLuaInterpreter::cLuaInterpreter(const string& configname, const string& scriptname):
	mConfigName(configname),
	mScriptName(scriptname)
{
	mL = luaL_newstate();
}

cLuaInterpreter::~cLuaInterpreter()
{
	if (mL)
		lua_close(mL);

	mL = NULL;
	clean();
}

bool cLuaInterpreter::Init()
{
	luaL_openlibs(mL);
	lua_newtable(mL);

	RegisterFunction("SendDataToUser", &_SendToUser); // backward compatibility
	RegisterFunction("SendToUser", &_SendToUser);
	RegisterFunction("SendDataToAll", &_SendToClass); // backward compatibility
	RegisterFunction("SendToClass", &_SendToClass);
	RegisterFunction("SendToAll", &_SendToAll);
	RegisterFunction("SendToActive", &_SendToActive);
	RegisterFunction("SendToActiveClass", &_SendToActiveClass);
	RegisterFunction("SendToPassive", &_SendToPassive);
	RegisterFunction("SendToPassiveClass", &_SendToPassiveClass);
	RegisterFunction("SendPMToAll", &_SendPMToAll);
	RegisterFunction("SendToChat", &_SendToChat);
	RegisterFunction("SendToOpChat", &_SendToOpChat);
	RegisterFunction("CloseConnection", &_Disconnect); // backward compatibility
	RegisterFunction("Disconnect", &_Disconnect);
	RegisterFunction("DisconnectByName", &_Disconnect); // backward compatibility
	RegisterFunction("StopHub", &_StopHub);
	RegisterFunction("GetUserCC", &_GetUserCC);
	RegisterFunction("GetUserCN", &_GetUserCN);
	RegisterFunction("GetUserCity", &_GetUserCity);
	RegisterFunction("GetIPCC", &_GetIPCC);
	RegisterFunction("GetIPCN", &_GetIPCN);
	RegisterFunction("GetIPCity", &_GetIPCity);
	RegisterFunction("GetIPASN", &_GetIPASN);
	RegisterFunction("GetUserGeoIP", &_GetUserGeoIP);
	RegisterFunction("GetHostGeoIP", &_GetHostGeoIP);
	RegisterFunction("GetMyINFO", &_GetMyINFO);
	RegisterFunction("GetUpTime", &_GetUpTime);
	RegisterFunction("GetServFreq", &_GetServFreq);
	RegisterFunction("RegBot", &_RegBot);
	RegisterFunction("AddRobot", &_RegBot); // backward compatibility
	RegisterFunction("UnRegBot", &_UnRegBot);
	RegisterFunction("DelRobot", &_UnRegBot); // backward compatibility
	RegisterFunction("EditBot", &_EditBot);
	RegisterFunction("IsBot", &_IsBot);
	RegisterFunction("GetHubIp", &_GetHubIp);
	RegisterFunction("GetHubSecAlias", &_GetHubSecAlias);
	RegisterFunction("AddChatUser", &_AddChatUser);
	RegisterFunction("DelChatUser", &_DelChatUser);
	RegisterFunction("IsChatUser", &_IsChatUser);
	RegisterFunction("AddRegUser", &_AddRegUser);
	RegisterFunction("DelRegUser", &_DelRegUser);
	RegisterFunction("SetRegClass", &_SetRegClass);
	RegisterFunction("GetUserClass", &_GetUserClass);
	RegisterFunction("GetUserHost", &_GetUserHost);
	RegisterFunction("GetUserIP", &_GetUserIP);
	RegisterFunction("SetUserIP", &_SetUserIP);
	RegisterFunction("SetMyINFOFlag", &_SetMyINFOFlag);
	RegisterFunction("UnsetMyINFOFlag", &_UnsetMyINFOFlag);
	RegisterFunction("IsSecConn", &_IsSecConn);
	RegisterFunction("GetTLSVer", &_GetTLSVer);
	RegisterFunction("IsUserOnline", &_IsUserOnline);
	RegisterFunction("GetUserSupports", &_GetUserSupports);
	RegisterFunction("GetUserHubURL", &_GetUserHubURL);
	RegisterFunction("GetUserExtJSON", &_GetUserExtJSON);
	RegisterFunction("GetUserVersion", &_GetUserVersion);
	RegisterFunction("InUserSupports", &_InUserSupports);
	RegisterFunction("PassTempBan", &_PassTempBan);
	RegisterFunction("Ban", &_Ban);
	RegisterFunction("KickUser", &_KickUser);
	RegisterFunction("KickRedirUser", &_KickRedirUser);
	RegisterFunction("DelNickTempBan", &_DelNickTempBan);
	RegisterFunction("DelIPTempBan", &_DelIPTempBan);
	RegisterFunction("ReportUser", &_ReportUser);
	RegisterFunction("ParseCommand", &_ParseCommand);
	RegisterFunction("SetConfig", &_SetConfig);
	RegisterFunction("GetConfig", &_GetConfig);
	RegisterFunction("SQLQuery", &_SQLQuery);
	RegisterFunction("SQLFetch", &_SQLFetch);
	RegisterFunction("SQLFree", &_SQLFree);
	RegisterFunction("GetUsersCount", &_GetUsersCount);
	RegisterFunction("GetTotalShareSize", &_GetTotalShareSize);
	RegisterFunction("GetNickList", &_GetNickList);
	RegisterFunction("GetOPList", &_GetOPList);
	RegisterFunction("GetBotList", &_GetBotList);
	RegisterFunction("GetLuaBots", &_GetLuaBots);
	RegisterFunction("GetBots", &_GetLuaBots); // backward compatibility
	RegisterFunction("GetTempRights", &_GetTempRights);
	RegisterFunction("SetTempRights", &_SetTempRights);
	RegisterFunction("GetVHCfgDir", &_GetVHCfgDir);
	RegisterFunction("GetTopic", &_GetTopic);
	RegisterFunction("SetTopic", &_SetTopic);
	RegisterFunction("ScriptCommand", &_ScriptCommand);

	cServerDC *serv = cServerDC::sCurrentServer;

	if (serv) {
		VHPushString("HubSec", serv->mC.hub_security.c_str());
		VHPushString("OpChat", serv->mC.opchat_name.c_str());
	}

	VHPushString("HubVer", HUB_VERSION_VERS);
	VHPushString("PlugVer", LUA_PI_VERSION);
	VHPushString("ConfName", mConfigName.c_str());
	VHPushString("ScriptName", mScriptName.c_str());

	lua_setglobal(mL, VH_TABLE_NAME);

	if (luaL_dofile(mL, mScriptName.c_str())) {
		const char *error = lua_tostring(mL, -1);
		ReportLuaError(error);
		return false;
	}

	return true;
}

void cLuaInterpreter::Load()
{
	const char *args[] = {
		mScriptName.c_str(), // set first argument to script name, could be useful for path detection
		NULL
	};

	CallFunction("Main", args); // call Main() first

	/*
		if (!CallFunction("Main", args))
			UnloadScript(); // todo
	*/
}

void cLuaInterpreter::ReportLuaError(const char *error)
{
	if (!cpiLua::me || (cpiLua::me->log_level == 0))
		return;

	cServerDC *serv = cServerDC::sCurrentServer;

	if (!serv)
		return;

	string toall(_("Lua error"));
	toall.append(": ");

	if (error)
		toall.append(error);
	else
		toall.append(_("Unknown error"));

	string start, end;
	serv->mP.Create_PMForBroadcast(start, end, serv->mC.opchat_name, serv->mC.opchat_name, toall);
	serv->SendToAllWithNick(start, end, cpiLua::me->err_class, eUC_MASTER); // use err_class here
	cpiLua::me->ReportLuaError(toall); // also write to err file
}

void cLuaInterpreter::RegisterFunction(const char *func, int (*ptr)(lua_State*))
{
	lua_pushstring(mL, func);
	lua_pushcfunction(mL, ptr);
	lua_rawset(mL, -3);
}

void cLuaInterpreter::VHPushString(const char *name, const char *val, bool update)
{
	int pos = -3;

	if (update)
		lua_getglobal(mL, VH_TABLE_NAME);
	else
		pos = lua_gettop(mL);

	lua_pushlstring(mL, name, strlen(name));
	lua_pushstring(mL, val);

	if (update)
		lua_settable(mL, pos);
	else
		lua_rawset(mL, pos);
}

bool cLuaInterpreter::CallFunction(const char *func, const char *args[], cConnDC *conn)
{
	lua_settop(mL, 0);
	lua_getglobal(mL, func);

	if (lua_isnil(mL, -1)) {
		lua_pop(mL, 1);
		return true;
	}

	int pos = 0;

	while (args[pos] != NULL) {
		lua_pushstring(mL, args[pos]);
		pos++;
	}

	if (lua_pcall(mL, pos, 1, 0)) {
		const char *err = lua_tostring(mL, -1);
		ReportLuaError(err);
		lua_pop(mL, 1);
		return true;
	}

	bool ret = true;

	if (lua_istable(mL, -1)) {
		/*
			new style, advanced table return

			table index = 1, type = string
			value: data = protocol message to send
			value: empty = dont send anything

			table index = 2, type = boolean
			value: 0 = discard
			value: 1 = dont discard

			table index = 3, type = boolean
			value: 0 = disconnect user
			value: 1 = dont disconnect
		*/

		string data;
		lua_pushnil(mL);

		while (lua_next(mL, -2) != 0) {
			lua_pushvalue(mL, -2); // -1 is now key and -2 is value, pop 2 below

			if (lua_isnumber(mL, -1)) { // table keys must not be named
				pos = (int)lua_tonumber(mL, -1);

				if (pos == 1) { // the message
					if (lua_isstring(mL, -2) && conn) { // value at index 1 must be a string, connection is required
						data = lua_tostring(mL, -2);

						if (data.size())
							conn->Send(data, false); // send data, script must add the ending pipe
					}

				} else if (pos == 2) { // discard flag
					if (lua_isnumber(mL, -2)) { // value at index 2 must be a boolean
						if ((int)lua_tonumber(mL, -2) == 0)
							ret = false;

					} else { // accept boolean and nil
						if ((int)lua_toboolean(mL, -2) == 0)
							ret = false;
					}

				} else if (pos == 3) { // disconnect flag
					if (conn) { // connection is required
						if (lua_isnumber(mL, -2)) { // value at index 3 must be a boolean
							if ((int)lua_tonumber(mL, -2) == 0) {
								conn->CloseNow(); // disconnect user
								ret = false; // automatically discard due disconnect
							}

						} else { // accept boolean and nil
							if ((int)lua_toboolean(mL, -2) == 0) {
								conn->CloseNow(); // disconnect user
								ret = false; // automatically discard due disconnect
							}
						}
					}
				}
			}

			lua_pop(mL, 2);
		}

	} else if (lua_isnumber(mL, -1)) {
		/*
			old school, simple boolean return for backward compatibility

			type = boolean
			value: 0 = discard
			value: 1 = dont discard
		*/

		if ((int)lua_tonumber(mL, -1) == 0)
			ret = false;
	}

	lua_pop(mL, 1);
	return ret;
}

	}; // namespace nLuaPlugin
}; // namespace nVerliHub
