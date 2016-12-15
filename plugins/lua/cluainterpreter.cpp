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

cLuaInterpreter::cLuaInterpreter(string configname, string scriptname):
	mConfigName(configname),
	mScriptName(scriptname)
{
	mL = luaL_newstate();
}

cLuaInterpreter::~cLuaInterpreter()
{
	const char *args[] = { NULL };

	if (mL) {
		CallFunction("UnLoad", args);
		lua_close(mL);
	}

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

	#ifdef HAVE_LIBGEOIP
		RegisterFunction("GetIPCC", &_GetIPCC);
		RegisterFunction("GetIPCN", &_GetIPCN);
		RegisterFunction("GetIPCity", &_GetIPCity);
		RegisterFunction("GetUserGeoIP", &_GetUserGeoIP);
		RegisterFunction("GetHostGeoIP", &_GetHostGeoIP);
	#endif

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
	RegisterFunction("GetUserClass", &_GetUserClass);
	RegisterFunction("GetUserHost", &_GetUserHost);
	RegisterFunction("GetUserIP", &_GetUserIP);
	RegisterFunction("IsUserOnline", &_IsUserOnline);
	RegisterFunction("GetUserSupports", &_GetUserSupports);
	RegisterFunction("GetUserHubURL", &_GetUserHubURL);
	RegisterFunction("GetUserExtJSON", &_GetUserExtJSON);
	RegisterFunction("GetUserVersion", &_GetUserVersion);
	RegisterFunction("InUserSupports", &_InUserSupports);
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
	RegisterFunction("ScriptQuery", &_ScriptQuery);
	cServerDC *serv = cServerDC::sCurrentServer;

	if (serv) {
		VHPushString("HubSec", serv->mC.hub_security.c_str());
		VHPushString("OpChat", serv->mC.opchat_name.c_str());
		VHPushString("HubVer", HUB_VERSION_VERS);
		VHPushString("PlugVer", LUA_PI_VERSION);
		VHPushString("ConfName", mConfigName.c_str());
		VHPushString("ScriptName", mScriptName.c_str());
	}

	lua_setglobal(mL, VH_TABLE_NAME);
	int status = luaL_dofile(mL, mScriptName.c_str());

	if (status) {
		const char *error = luaL_checkstring(mL, 1);
		ReportLuaError(error);
		return false;
	}

	lua_pushstring(mL, LUA_PI_VERSION);
	lua_setglobal(mL, "_PLUGINVERSION");
	lua_pushstring(mL, HUB_VERSION_VERS);
	lua_setglobal(mL, "_HUBVERSION");
	lua_pushstring(mL, mScriptName.c_str());
	lua_setglobal(mL, "_SCRIPTFILE");
	const char *path = mScriptName.c_str();

	for (int i = strlen(path) - 2; i >= 0; i--) {
		if ((path[i] == '/') || (path[i] == '\\')) {
			path = &path[i + 1];
			break;
		}
	}

	lua_pushstring(mL, path); // these two globals are to be set by the script, but should have sane defaults
	lua_setglobal(mL, "_SCRIPTNAME");
	lua_pushstring(mL, "0.0.0");
	lua_setglobal(mL, "_SCRIPTVERSION");
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
	if (cpiLua::me && (cpiLua::me->log_level > 0)) {
		cServerDC *serv = cServerDC::sCurrentServer;

		if (serv) {
			string toall = _("Lua error");
			toall.append(": ");

			if (error)
				toall.append(error);
			else
				toall.append(_("Unknown error"));

			string start, end;
			serv->mP.Create_PMForBroadcast(start, end, serv->mC.opchat_name, serv->mC.opchat_name, toall);
			serv->SendToAllWithNick(start, end, cpiLua::me->err_class, eUC_MASTER); // use err_class here
		}
	}
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
	ScriptResponses *responses = NULL;

	if (!strcmp(func, "VH_OnScriptQuery")) {
		const char *recipient = args[2];
		if (strlen(recipient) && strcmp(recipient, "lua") && strcmp(recipient, mScriptName.c_str()))
			return true;
		responses = (ScriptResponses *)conn;
		conn = NULL;
	}

	lua_settop(mL, 0);
	int base = lua_gettop(mL);
	lua_pushliteral(mL, "_TRACEBACK");

	#if defined LUA_GLOBALSINDEX
		lua_rawget(mL, LUA_GLOBALSINDEX);
	#else
		lua_pushglobaltable(mL);
	#endif

	lua_insert(mL, base);
	lua_getglobal(mL, func);

	if (lua_isnil(mL, -1)) { // function dont exist
		lua_pop(mL, -1); // remove nil value
		lua_remove(mL, base); // remove _TRACEBACK
	} else {
		int i = 0;

		while (args[i] != NULL) {
			lua_pushstring(mL, args[i]);
			i++;
		}

		int result = lua_pcall(mL, i, 1, base);

		if (result) {
			const char *error = lua_tostring(mL, -1);
			ReportLuaError(error);
			lua_pop(mL, 1);
			lua_remove(mL, base); // remove _TRACEBACK
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

			i = lua_gettop(mL);
			lua_pushnil(mL);

			while (lua_next(mL, i) != 0) {
				if (lua_isnumber(mL, -2)) { // table keys must not be named
					int key = (int)lua_tonumber(mL, -2);

					if (key == 1) { // message?
						if (lua_isstring(mL, -1) && conn) { // value at index 1 must be a string, connection is required
							string data = lua_tostring(mL, -1);

							if (data.size())
								conn->Send(data, false); // send data, script must add the ending pipe
						}
					} else if (key == 2) { // discard?
						if (lua_isnumber(mL, -1)) { // value at index 2 must be a boolean
							if ((int)lua_tonumber(mL, -1) == 0)
								ret = false;
						} else { // accept boolean and nil
							if ((int)lua_toboolean(mL, -1) == 0)
								ret = false;
						}
					} else if (key == 3) { // disconnect?
						if (conn) { // connection is required
							if (lua_isnumber(mL, -1)) { // value at index 3 must be a boolean
								if ((int)lua_tonumber(mL, -1) == 0) {
									conn->CloseNow(); // disconnect user
									ret = false; // automatically discard due disconnect
								}
							} else { // accept boolean and nil
								if ((int)lua_toboolean(mL, -1) == 0) {
									conn->CloseNow(); // disconnect user
									ret = false; // automatically discard due disconnect
								}
							}
						}
					}
				}

				lua_pop(mL, 1);
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
		//} else { // accept boolean and nil, same as above
			//if ((int)lua_toboolean(mL, -1) == 0)
				//ret = false;

		}
		if (!strcmp(func, "VH_OnScriptQuery") && responses) {
			const char *command = args[0];
			const char *answer = NULL;
			const char *sender = mScriptName.c_str();
			bool to_pop = false;

			if (lua_isstring(mL, -1)) answer = lua_tostring(mL, -1);
			if (!answer || !strlen(answer)) {
				if (!strcmp(command, "_get_script_file")) {
					answer = mScriptName.c_str();
				} else if (!strcmp(command, "_get_script_version")) {
					lua_getglobal(mL, "_SCRIPTVERSION");
					if (lua_isstring(mL, -1)) answer = lua_tostring(mL, -1);
					to_pop = true;
				} else if (!strcmp(command, "_get_script_name")) {
					lua_getglobal(mL, "_SCRIPTNAME");
					if (lua_isstring(mL, -1)) answer = lua_tostring(mL, -1);
					to_pop = true;
				}
			}
			if (answer) responses->push_back(ScriptResponse(answer, sender));
			if (to_pop) lua_pop(mL, 1);
		}

		lua_pop(mL, 1);
		lua_remove(mL, base); // remove _TRACEBACK
		return ret;
	}

	return true;
}

	}; // namespace nLuaPlugin
}; // namespace nVerliHub
