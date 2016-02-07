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

cLuaInterpreter::cLuaInterpreter(string scriptname):
	mScriptName(scriptname)
{
	mL = luaL_newstate();
}

cLuaInterpreter::~cLuaInterpreter()
{
	char *args[] = { NULL };

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

	lua_setglobal(mL, "VH");
	int status = luaL_dofile(mL, (char*)mScriptName.c_str());

	if (status) {
		const char *error = luaL_checkstring(mL, 1);
		ReportLuaError(error);
		return false;
	}

	lua_pushstring(mL, LUA_VERSION);
	lua_setglobal(mL, "_PLUGINVERSION");
	lua_pushstring(mL, HUB_VERSION_VERS);
	lua_setglobal(mL, "_HUBVERSION");
	return true;
}

void cLuaInterpreter::Load()
{
	char *args[] = {
		(char*)mScriptName.c_str(), // set first argument to script name, could be useful for path detection
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

void cLuaInterpreter::RegisterFunction(const char *fncname, int (*fncptr)(lua_State*))
{
	lua_pushstring(mL, fncname);
	lua_pushcfunction(mL, fncptr);
	lua_rawset(mL, -3);
}

bool cLuaInterpreter::CallFunction(const char *func, char *args[], cConnDC *conn)
{
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

		lua_pop(mL, 1);
		lua_remove(mL, base); // remove _TRACEBACK
		return ret;
	}

	return true;
}

	}; // namespace nLuaPlugin
}; // namespace nVerliHub
