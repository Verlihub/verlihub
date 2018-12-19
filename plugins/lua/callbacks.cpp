/*
	Copyright (C) 2003-2005 Daniel Muller, dan at verliba dot cz
	Copyright (C) 2006-2018 Verlihub Team, info at verlihub dot net

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

#define ERR_EMPT "Empty parameter"
#define ERR_PARAM "Wrong parameters"
#define ERR_CALL "Call error"
#define ERR_SERV "Error getting server"
#define ERR_LUA "Error getting Lua"
#define ERR_PLUG "Error getting plugin"
#define ERR_CLASS "Invalid class number"
#define ERR_QUERY "Query is not ready"

extern "C"
{
	#include <lua.h>
	#include <lualib.h>
	#include <lauxlib.h>
}

#include "cpilua.h"
#include "callbacks.h"
#include "src/cserverdc.h"
#include "src/cconndc.h"
#include "src/cuser.h"
#include "src/script_api.h"
#include <iostream>
#include <sstream>
#include <string>

using namespace std;

namespace nVerliHub {
	using namespace nSocket;
	using namespace nEnums;
	using namespace nLuaPlugin;

cServerDC* GetCurrentVerlihub()
{
	return (cServerDC*)cServerDC::sCurrentServer;
}

int _SendToUser(lua_State *L)
{
	int args = lua_gettop(L) - 1;

	if (args < 2) {
		luaL_error(L, "Error calling VH:SendToUser, expected atleast 2 arguments but got %d.", args);
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}

	if (!lua_isstring(L, 2) || !lua_isstring(L, 3) || ((args >= 3) && !lua_isnumber(L, 4))) {
		luaerror(L, ERR_PARAM);
		return 2;
	}

	string data = lua_tostring(L, 2), nick = lua_tostring(L, 3);

	if (data.empty() || nick.empty()) {
		luaerror(L, ERR_EMPT);
		return 2;
	}

	bool delay = false;

	if (args >= 3)
		delay = (int(lua_tonumber(L, 4)) > 0);

	if (!SendDataToUser(data.c_str(), nick.c_str(), delay)) {
		luaerror(L, ERR_CALL);
		return 2;
	}

	lua_pushboolean(L, 1);
	lua_pushnil(L);
	return 2;
}

int _SendToClass(lua_State *L)
{
	int args = lua_gettop(L) - 1;

	if (args < 1) {
		luaL_error(L, "Error calling VH:SendToClass, expected atleast 1 argument but got %d.", args);
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}

	if (!lua_isstring(L, 2) || ((args >= 2) && !lua_isnumber(L, 3)) || ((args >= 3) && !lua_isnumber(L, 4)) || ((args >= 4) && !lua_isnumber(L, 5))) {
		luaerror(L, ERR_PARAM);
		return 2;
	}

	string data = lua_tostring(L, 2);

	if (data.empty()) {
		luaerror(L, ERR_EMPT);
		return 2;
	}

	int min_class = eUC_NORMUSER;

	if (args >= 2) {
		min_class = int(lua_tonumber(L, 3));

		if ((min_class < eUC_PINGER) || (min_class > eUC_MASTER) || ((min_class > eUC_ADMIN) && (min_class < eUC_MASTER))) {
			luaerror(L, ERR_CLASS);
			return 2;
		}
	}

	int max_class = eUC_MASTER;

	if (args >= 3) {
		max_class = int(lua_tonumber(L, 4));

		if ((max_class < eUC_PINGER) || (max_class > eUC_MASTER) || ((max_class > eUC_ADMIN) && (max_class < eUC_MASTER))) {
			luaerror(L, ERR_CLASS);
			return 2;
		}
	}

	bool delay = false;

	if (args >= 4)
		delay = (int(lua_tonumber(L, 5)) > 0);

	if (!SendToClass(data.c_str(), min_class, max_class, delay)) {
		luaerror(L, ERR_CALL);
		return 2;
	}

	lua_pushboolean(L, 1);
	lua_pushnil(L);
	return 2;
}

int _SendToAll(lua_State *L)
{
	int args = lua_gettop(L) - 1;

	if (args < 1) {
		luaL_error(L, "Error calling VH:SendToAll, expected atleast 1 argument but got %d.", args);
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}

	if (!lua_isstring(L, 2) || ((args >= 2) && !lua_isnumber(L, 3))) {
		luaerror(L, ERR_PARAM);
		return 2;
	}

	string data = lua_tostring(L, 2);

	if (data.empty()) {
		luaerror(L, ERR_EMPT);
		return 2;
	}

	bool delay = false;

	if (args >= 2)
		delay = (int(lua_tonumber(L, 3)) > 0);

	if (!SendToAll(data.c_str(), delay)) {
		luaerror(L, ERR_CALL);
		return 2;
	}

	lua_pushboolean(L, 1);
	lua_pushnil(L);
	return 2;
}

int _SendToActive(lua_State *L)
{
	int args = lua_gettop(L) - 1;

	if (args < 1) {
		luaL_error(L, "Error calling VH:SendToActive, expected atleast 1 argument but got %d.", args);
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}

	if (!lua_isstring(L, 2) || ((args >= 2) && !lua_isnumber(L, 3))) {
		luaerror(L, ERR_PARAM);
		return 2;
	}

	string data = lua_tostring(L, 2);

	if (data.empty()) {
		luaerror(L, ERR_EMPT);
		return 2;
	}

	bool delay = false;

	if (args >= 2)
		delay = (int(lua_tonumber(L, 3)) > 0);

	if (!SendToActive(data.c_str(), delay)) {
		luaerror(L, ERR_CALL);
		return 2;
	}

	lua_pushboolean(L, 1);
	lua_pushnil(L);
	return 2;
}

int _SendToActiveClass(lua_State *L)
{
	int args = lua_gettop(L) - 1;

	if (args < 1) {
		luaL_error(L, "Error calling VH:SendToActiveClass, expected atleast 1 argument but got %d.", args);
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}

	if (!lua_isstring(L, 2) || ((args >= 2) && !lua_isnumber(L, 3)) || ((args >= 3) && !lua_isnumber(L, 4)) || ((args >= 4) && !lua_isnumber(L, 5))) {
		luaerror(L, ERR_PARAM);
		return 2;
	}

	string data = lua_tostring(L, 2);

	if (data.empty()) {
		luaerror(L, ERR_EMPT);
		return 2;
	}

	int min_class = eUC_NORMUSER;

	if (args >= 2) {
		min_class = int(lua_tonumber(L, 3));

		if ((min_class < eUC_PINGER) || (min_class > eUC_MASTER) || ((min_class > eUC_ADMIN) && (min_class < eUC_MASTER))) {
			luaerror(L, ERR_CLASS);
			return 2;
		}
	}

	int max_class = eUC_MASTER;

	if (args >= 3) {
		max_class = int(lua_tonumber(L, 4));

		if ((max_class < eUC_PINGER) || (max_class > eUC_MASTER) || ((max_class > eUC_ADMIN) && (max_class < eUC_MASTER))) {
			luaerror(L, ERR_CLASS);
			return 2;
		}
	}

	bool delay = false;

	if (args >= 4)
		delay = (int(lua_tonumber(L, 5)) > 0);

	if (!SendToActiveClass(data.c_str(), min_class, max_class, delay)) {
		luaerror(L, ERR_CALL);
		return 2;
	}

	lua_pushboolean(L, 1);
	lua_pushnil(L);
	return 2;
}

int _SendToPassive(lua_State *L)
{
	int args = lua_gettop(L) - 1;

	if (args < 1) {
		luaL_error(L, "Error calling VH:SendToPassive, expected atleast 1 argument but got %d.", args);
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}

	if (!lua_isstring(L, 2) || ((args >= 2) && !lua_isnumber(L, 3))) {
		luaerror(L, ERR_PARAM);
		return 2;
	}

	string data = lua_tostring(L, 2);

	if (data.empty()) {
		luaerror(L, ERR_EMPT);
		return 2;
	}

	bool delay = false;

	if (args >= 2)
		delay = (int(lua_tonumber(L, 3)) > 0);

	if (!SendToPassive(data.c_str(), delay)) {
		luaerror(L, ERR_CALL);
		return 2;
	}

	lua_pushboolean(L, 1);
	lua_pushnil(L);
	return 2;
}

int _SendToPassiveClass(lua_State *L)
{
	int args = lua_gettop(L) - 1;

	if (args < 1) {
		luaL_error(L, "Error calling VH:SendToPassiveClass, expected atleast 1 argument but got %d.", args);
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}

	if (!lua_isstring(L, 2) || ((args >= 2) && !lua_isnumber(L, 3)) || ((args >= 3) && !lua_isnumber(L, 4)) || ((args >= 4) && !lua_isnumber(L, 5))) {
		luaerror(L, ERR_PARAM);
		return 2;
	}

	string data = lua_tostring(L, 2);

	if (data.empty()) {
		luaerror(L, ERR_EMPT);
		return 2;
	}

	int min_class = eUC_NORMUSER;

	if (args >= 2) {
		min_class = int(lua_tonumber(L, 3));

		if ((min_class < eUC_PINGER) || (min_class > eUC_MASTER) || ((min_class > eUC_ADMIN) && (min_class < eUC_MASTER))) {
			luaerror(L, ERR_CLASS);
			return 2;
		}
	}

	int max_class = eUC_MASTER;

	if (args >= 3) {
		max_class = int(lua_tonumber(L, 4));

		if ((max_class < eUC_PINGER) || (max_class > eUC_MASTER) || ((max_class > eUC_ADMIN) && (max_class < eUC_MASTER))) {
			luaerror(L, ERR_CLASS);
			return 2;
		}
	}

	bool delay = false;

	if (args >= 4)
		delay = (int(lua_tonumber(L, 5)) > 0);

	if (!SendToPassiveClass(data.c_str(), min_class, max_class, delay)) {
		luaerror(L, ERR_CALL);
		return 2;
	}

	lua_pushboolean(L, 1);
	lua_pushnil(L);
	return 2;
}

int _SendPMToAll(lua_State *L)
{
	int args = lua_gettop(L) - 1;

	if (args < 2) {
		luaL_error(L, "Error calling VH:SendPMToAll, expected atleast 2 argument but got %d.", args);
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}

	if (!lua_isstring(L, 2) || !lua_isstring(L, 3) || ((args >= 3) && !lua_isnumber(L, 4)) || ((args >= 4) && !lua_isnumber(L, 5))) {
		luaerror(L, ERR_PARAM);
		return 2;
	}

	string data = lua_tostring(L, 2), from = lua_tostring(L, 3); // todo: make from nick optional, use hub security nick

	if (data.empty() || from.empty()) {
		luaerror(L, ERR_EMPT);
		return 2;
	}

	int min_class = eUC_NORMUSER;

	if (args >= 3) {
		min_class = int(lua_tonumber(L, 4));

		if ((min_class < eUC_PINGER) || (min_class > eUC_MASTER) || ((min_class > eUC_ADMIN) && (min_class < eUC_MASTER))) {
			luaerror(L, ERR_CLASS);
			return 2;
		}
	}

	int max_class = eUC_MASTER;

	if (args >= 4) {
		max_class = int(lua_tonumber(L, 5));

		if ((max_class < eUC_PINGER) || (max_class > eUC_MASTER) || ((max_class > eUC_ADMIN) && (max_class < eUC_MASTER))) {
			luaerror(L, ERR_CLASS);
			return 2;
		}
	}

	if (!SendPMToAll(data.c_str(), from.c_str(), min_class, max_class)) {
		luaerror(L, ERR_CALL);
		return 2;
	}

	lua_pushboolean(L, 1);
	lua_pushnil(L);
	return 2;
}

int _SendToChat(lua_State *L)
{
	int args = lua_gettop(L) - 1;

	if (args < 2) {
		luaL_error(L, "Error calling VH:SendToChat, expected atleast 2 arguments but got %d.", args);
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}

	if (!lua_isstring(L, 2) || !lua_isstring(L, 3) || ((args >= 4) && (!lua_isnumber(L, 4) || !lua_isnumber(L, 5)))) {
		luaerror(L, ERR_PARAM);
		return 2;
	}

	string nick = lua_tostring(L, 2), text = lua_tostring(L, 3);

	if (nick.empty() || text.empty()) {
		luaerror(L, ERR_EMPT);
		return 2;
	}

	int min_class = eUC_NORMUSER, max_class = eUC_MASTER;

	if (args >= 4) {
		min_class = int(lua_tonumber(L, 4));
		max_class = int(lua_tonumber(L, 5));

		if ((min_class < eUC_PINGER) || (min_class > eUC_MASTER) || ((min_class > eUC_ADMIN) && (min_class < eUC_MASTER)) || (max_class < eUC_PINGER) || (max_class > eUC_MASTER) || ((max_class > eUC_ADMIN) && (max_class < eUC_MASTER))) {
			luaerror(L, ERR_CLASS);
			return 2;
		}
	}

	if (!SendToChat(nick.c_str(), text.c_str(), min_class, max_class)) {
		luaerror(L, ERR_CALL);
		return 2;
	}

	lua_pushboolean(L, 1);
	lua_pushnil(L);
	return 2;
}

int _SendToOpChat(lua_State *L)
{
	int args = lua_gettop(L) - 1;

	if (args < 1) {
		luaL_error(L, "Error calling VH:SendToOpChat, expected atleast 1 argument but got %d.", args);
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}

	if (!lua_isstring(L, 2) || ((args >= 2) && !lua_isstring(L, 3))) {
		luaerror(L, ERR_PARAM);
		return 2;
	}

	string data = lua_tostring(L, 2);

	if (data.empty()) {
		luaerror(L, ERR_EMPT);
		return 2;
	}

	string nick;

	if (args >= 2)
		nick = lua_tostring(L, 3);

	if (!SendToOpChat(data.c_str(), (nick.size() ? nick.c_str() : NULL))) {
		luaerror(L, ERR_CALL);
		return 2;
	}

	lua_pushboolean(L, 1);
	lua_pushnil(L);
	return 2;
}

int _Disconnect(lua_State *L)
{
	int args = lua_gettop(L) - 1;

	if (args < 1) {
		luaL_error(L, "Error calling VH:Disconnect, expected atleast 1 argument but got %d.", args);
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}

	if (!lua_isstring(L, 2) || ((args >= 2) && !lua_isnumber(L, 3))) {
		luaerror(L, ERR_PARAM);
		return 2;
	}

	string nick = lua_tostring(L, 2);
	long delay = 0;

	if (args >= 2)
		delay = (long)lua_tonumber(L, 3);

	if (!CloseConnection(nick.c_str(), delay)) {
		luaerror(L, ERR_CALL);
		return 2;
	}

	lua_pushboolean(L, 1);
	lua_pushnil(L);
	return 2;
}

int _StopHub(lua_State *L)
{
	int args = lua_gettop(L) - 1;

	if (args < 1) {
		luaL_error(L, "Error calling VH:StopHub, expected atleast 1 argument but got %d.", args);
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}

	if (!lua_isnumber(L, 2) || ((args >= 2) && !lua_isnumber(L, 3))) {
		luaerror(L, ERR_PARAM);
		return 2;
	}

	int code = (int)lua_tonumber(L, 2);
	int delay = 0;

	if (args >= 2)
		delay = (int)lua_tonumber(L, 3);

	if (!StopHub(code, delay)) {
		luaerror(L, ERR_CALL);
		return 2;
	}

	lua_pushboolean(L, 1);
	lua_pushnil(L);
	return 2;
}

int _GetMyINFO(lua_State *L)
{
	string nick;
	const char *myinfo;
	int result = 1;

	if(lua_gettop(L) == 2) {
		if(!lua_isstring(L, 2)) {
			luaerror(L, ERR_PARAM);
			return 2;
		}
		nick = lua_tostring(L, 2);
		myinfo = GetMyINFO(nick.c_str());
		if(strlen(myinfo) < 1)
		{
			result = 0;
			myinfo = "User not found";
		}
		lua_pushboolean(L, result);
		lua_pushstring(L, myinfo);
		return 2;
	} else {
		luaL_error(L, "Error calling VH:GetMyINFO; expected 1 argument but got %d", lua_gettop(L) - 1);
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}
}

int _GetUserCC(lua_State *L)
{
	if (lua_gettop(L) < 2) {
		luaL_error(L, "Error calling VH:GetUserCC, expected 1 argument but got %d.", lua_gettop(L) - 1);
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}

	cServerDC *serv = GetCurrentVerlihub();

	if (!serv) {
		luaerror(L, ERR_SERV);
		return 2;
	}

	if (!lua_isstring(L, 2)) {
		luaerror(L, ERR_PARAM);
		return 2;
	}

	string nick = lua_tostring(L, 2);
	cUser *user = serv->mUserList.GetUserByNick(nick);

	if (!user || !user->mxConn) {
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}

	nick = user->mxConn->GetGeoCC(); // country code
	lua_pushboolean(L, 1);
	lua_pushstring(L, nick.c_str());
	return 2;
}

int _GetUserCN(lua_State *L)
{
	if (lua_gettop(L) < 2) {
		luaL_error(L, "Error calling VH:GetUserCN, expected 1 argument but got %d.", lua_gettop(L) - 1);
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}

	cServerDC *serv = GetCurrentVerlihub();

	if (!serv) {
		luaerror(L, ERR_SERV);
		return 2;
	}

	if (!lua_isstring(L, 2)) {
		luaerror(L, ERR_PARAM);
		return 2;
	}

	string nick = lua_tostring(L, 2);
	cUser *user = serv->mUserList.GetUserByNick(nick);

	if (!user || !user->mxConn) {
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}

	nick = user->mxConn->GetGeoCN(); // country name
	lua_pushboolean(L, 1);
	lua_pushstring(L, nick.c_str());
	return 2;
}

int _GetUserCity(lua_State *L)
{
	if (lua_gettop(L) < 2) {
		luaL_error(L, "Error calling VH:GetUserCity, expected 1 argument but got %d.", lua_gettop(L) - 1);
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}

	cServerDC *serv = GetCurrentVerlihub();

	if (!serv) {
		luaerror(L, ERR_SERV);
		return 2;
	}

	if (!lua_isstring(L, 2)) {
		luaerror(L, ERR_PARAM);
		return 2;
	}

	string nick = lua_tostring(L, 2);
	cUser *user = serv->mUserList.GetUserByNick(nick);

	if (!user || !user->mxConn) {
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}

	nick = user->mxConn->GetGeoCI(); // city name
	lua_pushboolean(L, 1);
	lua_pushstring(L, nick.c_str());
	return 2;
}

int _GetIPCC(lua_State *L)
{
	if (lua_gettop(L) < 2) {
		luaL_error(L, "Error calling VH:GetIPCC, expected 1 argument but got %d.", lua_gettop(L) - 1);
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}

	if (!lua_isstring(L, 2)) {
		luaerror(L, ERR_PARAM);
		return 2;
	}

	string ip = lua_tostring(L, 2);
	const string cc = GetIPCC(ip.c_str());

	if (cc.size()) {
		lua_pushboolean(L, 1);
		lua_pushstring(L, cc.c_str());
	} else {
		lua_pushboolean(L, 0);
		lua_pushnil(L);
	}

	return 2;
}

int _GetIPCN(lua_State *L)
{
	if (lua_gettop(L) < 2) {
		luaL_error(L, "Error calling VH:GetIPCN, expected 1 argument but got %d.", lua_gettop(L) - 1);
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}

	if (!lua_isstring(L, 2)) {
		luaerror(L, ERR_PARAM);
		return 2;
	}

	string ip = lua_tostring(L, 2);
	const string cn = GetIPCN(ip.c_str());

	if (cn.size()) {
		lua_pushboolean(L, 1);
		lua_pushstring(L, cn.c_str());
	} else {
		lua_pushboolean(L, 0);
		lua_pushnil(L);
	}

	return 2;
}

int _GetIPCity(lua_State *L)
{
	if (lua_gettop(L) < 2) {
		luaL_error(L, "Error calling VH:GetIPCity, expected atleast 1 argument but got %d.", lua_gettop(L) - 1);
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}

	cServerDC *serv = GetCurrentVerlihub();

	if (!serv) {
		luaerror(L, ERR_SERV);
		return 2;
	}

	if (!lua_isstring(L, 2)) {
		luaerror(L, ERR_PARAM);
		return 2;
	}

	string db;

	if (lua_gettop(L) > 2) {
		if (!lua_isstring(L, 3)) {
			luaerror(L, ERR_PARAM);
			return 2;
		} else {
			db = lua_tostring(L, 3);
		}
	}

	string ip = lua_tostring(L, 2);
	const string ci = GetIPCity(ip.c_str(), db.c_str()); // use script api, it has new optimizations

	if (ci.size()) {
		lua_pushboolean(L, 1);
		lua_pushstring(L, ci.c_str());
	} else {
		lua_pushboolean(L, 0);
		lua_pushnil(L);
	}

	return 2;
}

int _GetIPASN(lua_State *L)
{
	int args = lua_gettop(L) - 1;

	if (args < 1) {
		luaL_error(L, "Error calling VH:GetIPASN, expected atleast 1 argument but got %d.", args);
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}

	cServerDC *serv = GetCurrentVerlihub();

	if (!serv) {
		luaerror(L, ERR_SERV);
		return 2;
	}

	if (!lua_isstring(L, 2) || ((args > 1) && !lua_isstring(L, 3))) {
		luaerror(L, ERR_PARAM);
		return 2;
	}

	string ip = lua_tostring(L, 2), db, asn;

	if (args > 1)
		db = lua_tostring(L, 3);

	if (serv->mMaxMindDB->GetASN(asn, ip, db)) {
		lua_pushboolean(L, 1);
		lua_pushstring(L, asn.c_str());
	} else {
		lua_pushboolean(L, 0);
		lua_pushnil(L);
	}

	return 2;
}

int _GetUserGeoIP(lua_State *L)
{
	if (lua_gettop(L) < 2) {
		luaL_error(L, "Error calling VH:GetUserGeoIP, expected atleast 1 argument but got %d.", lua_gettop(L) - 1);
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}

	cServerDC *serv = GetCurrentVerlihub();

	if (!serv) {
		luaerror(L, ERR_SERV);
		return 2;
	}

	if (!lua_isstring(L, 2)) {
		luaerror(L, ERR_PARAM);
		return 2;
	}

	string nick = lua_tostring(L, 2);
	string db;

	if (lua_gettop(L) > 2) {
		if (!lua_isstring(L, 3)) {
			luaerror(L, ERR_PARAM);
			return 2;
		} else
			db = lua_tostring(L, 3);
	}

	cUser *usr = serv->mUserList.GetUserByNick(nick);

	if (!usr || !usr->mxConn) {
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}

	string geo_host, geo_ran_lo, geo_ran_hi, geo_cc, geo_ccc, geo_cn, geo_reg_code, geo_reg_name, geo_tz, geo_cont, geo_city, geo_post;
	double geo_lat, geo_lon;
	unsigned short geo_met, geo_area;

	if (serv->mMaxMindDB->GetGeoIP(geo_host, geo_ran_lo, geo_ran_hi, geo_cc, geo_ccc, geo_cn, geo_reg_code, geo_reg_name, geo_tz, geo_cont, geo_city, geo_post, geo_lat, geo_lon, geo_met, geo_area, usr->mxConn->AddrIP(), db)) {
		lua_pushboolean(L, 1);
		lua_newtable(L);
		int x = lua_gettop(L);

		lua_pushliteral(L, "host");
		if (geo_host.empty()) { lua_pushnil(L); } else { lua_pushstring(L, geo_host.c_str()); }
		lua_rawset(L, x);

		lua_pushliteral(L, "range_low");
		if (geo_ran_lo.empty()) { lua_pushnil(L); } else { lua_pushstring(L, geo_ran_lo.c_str()); }
		lua_rawset(L, x);

		lua_pushliteral(L, "range_high");
		if (geo_ran_hi.empty()) { lua_pushnil(L); } else { lua_pushstring(L, geo_ran_hi.c_str()); }
		lua_rawset(L, x);

		lua_pushliteral(L, "country_code");
		if (geo_cc.empty()) { lua_pushnil(L); } else { lua_pushstring(L, geo_cc.c_str()); }
		lua_rawset(L, x);

		lua_pushliteral(L, "country_code_xxx");
		if (geo_ccc.empty()) { lua_pushnil(L); } else { lua_pushstring(L, geo_ccc.c_str()); }
		lua_rawset(L, x);

		lua_pushliteral(L, "country");
		if (geo_cn.empty()) { lua_pushnil(L); } else { lua_pushstring(L, geo_cn.c_str()); }
		lua_rawset(L, x);

		lua_pushliteral(L, "region_code");
		if (geo_reg_code.empty()) { lua_pushnil(L); } else { lua_pushstring(L, geo_reg_code.c_str()); }
		lua_rawset(L, x);

		lua_pushliteral(L, "region");
		if (geo_reg_name.empty()) { lua_pushnil(L); } else { lua_pushstring(L, geo_reg_name.c_str()); }
		lua_rawset(L, x);

		lua_pushliteral(L, "time_zone");
		if (geo_tz.empty()) { lua_pushnil(L); } else { lua_pushstring(L, geo_tz.c_str()); }
		lua_rawset(L, x);

		lua_pushliteral(L, "continent_code");
		if (geo_cont.empty()) { lua_pushnil(L); } else { lua_pushstring(L, geo_cont.c_str()); }
		lua_rawset(L, x);

		string cont;

		if (geo_cont == "AF") {
			cont = "Africa";
		} else if (geo_cont == "AS") {
			cont = "Asia";
		} else if (geo_cont == "EU") {
			cont = "Europe";
		} else if (geo_cont == "NA") {
			cont = "North America";
		} else if (geo_cont == "SA") {
			cont = "South America";
		} else if (geo_cont == "OC") {
			cont = "Oceania";
		} else if (geo_cont == "AN") {
			cont = "Antarctica";
		}

		lua_pushliteral(L, "continent");
		if (cont.empty()) { lua_pushnil(L); } else { lua_pushstring(L, cont.c_str()); }
		lua_rawset(L, x);

		lua_pushliteral(L, "city");
		if (geo_city.empty()) { lua_pushnil(L); } else { lua_pushstring(L, geo_city.c_str()); }
		lua_rawset(L, x);

		lua_pushliteral(L, "postal_code");
		if (geo_post.empty()) { lua_pushnil(L); } else { lua_pushstring(L, geo_post.c_str()); }
		lua_rawset(L, x);

		lua_pushliteral(L, "latitude");
		lua_pushnumber(L, (double)geo_lat);
		lua_rawset(L, x);

		lua_pushliteral(L, "longitude");
		lua_pushnumber(L, (double)geo_lon);
		lua_rawset(L, x);

		lua_pushliteral(L, "metro_code");
		lua_pushnumber(L, (unsigned short)geo_met);
		lua_rawset(L, x);

		lua_pushliteral(L, "area_code");
		lua_pushnumber(L, (unsigned short)geo_area);
		lua_rawset(L, x);
	} else {
		lua_pushboolean(L, 0);
		lua_newtable(L);
		int x = lua_gettop(L);

		lua_pushliteral(L, "host");
		lua_pushnil(L);
		lua_rawset(L, x);

		lua_pushliteral(L, "range_low");
		lua_pushnil(L);
		lua_rawset(L, x);

		lua_pushliteral(L, "range_high");
		lua_pushnil(L);
		lua_rawset(L, x);

		lua_pushliteral(L, "country_code");
		lua_pushnil(L);
		lua_rawset(L, x);

		lua_pushliteral(L, "country_code_xxx");
		lua_pushnil(L);
		lua_rawset(L, x);

		lua_pushliteral(L, "country");
		lua_pushnil(L);
		lua_rawset(L, x);

		lua_pushliteral(L, "region_code");
		lua_pushnil(L);
		lua_rawset(L, x);

		lua_pushliteral(L, "region");
		lua_pushnil(L);
		lua_rawset(L, x);

		lua_pushliteral(L, "time_zone");
		lua_pushnil(L);
		lua_rawset(L, x);

		lua_pushliteral(L, "continent_code");
		lua_pushnil(L);
		lua_rawset(L, x);

		lua_pushliteral(L, "city");
		lua_pushnil(L);
		lua_rawset(L, x);

		lua_pushliteral(L, "postal_code");
		lua_pushnil(L);
		lua_rawset(L, x);

		lua_pushliteral(L, "latitude");
		lua_pushnil(L);
		lua_rawset(L, x);

		lua_pushliteral(L, "longitude");
		lua_pushnil(L);
		lua_rawset(L, x);

		lua_pushliteral(L, "metro_code");
		lua_pushnil(L);
		lua_rawset(L, x);

		lua_pushliteral(L, "area_code");
		lua_pushnil(L);
		lua_rawset(L, x);
	}

	return 2;
}

int _GetHostGeoIP(lua_State *L)
{
	if (lua_gettop(L) < 2) {
		luaL_error(L, "Error calling VH:GetHostGeoIP, expected atleast 1 argument but got %d.", lua_gettop(L) - 1);
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}

	cServerDC *serv = GetCurrentVerlihub();

	if (!serv) {
		luaerror(L, ERR_SERV);
		return 2;
	}

	if (!lua_isstring(L, 2)) {
		luaerror(L, ERR_PARAM);
		return 2;
	}

	string host = lua_tostring(L, 2);
	string db;

	if (lua_gettop(L) > 2) {
		if (!lua_isstring(L, 3)) {
			luaerror(L, ERR_PARAM);
			return 2;
		} else
			db = lua_tostring(L, 3);
	}

	string geo_host, geo_ran_lo, geo_ran_hi, geo_cc, geo_ccc, geo_cn, geo_reg_code, geo_reg_name, geo_tz, geo_cont, geo_city, geo_post;
	double geo_lat, geo_lon;
	unsigned short geo_met, geo_area;

	if (serv->mMaxMindDB->GetGeoIP(geo_host, geo_ran_lo, geo_ran_hi, geo_cc, geo_ccc, geo_cn, geo_reg_code, geo_reg_name, geo_tz, geo_cont, geo_city, geo_post, geo_lat, geo_lon, geo_met, geo_area, host, db)) {
		lua_pushboolean(L, 1);
		lua_newtable(L);
		int x = lua_gettop(L);

		lua_pushliteral(L, "host");
		if (geo_host.empty()) { lua_pushnil(L); } else { lua_pushstring(L, geo_host.c_str()); }
		lua_rawset(L, x);

		lua_pushliteral(L, "range_low");
		if (geo_ran_lo.empty()) { lua_pushnil(L); } else { lua_pushstring(L, geo_ran_lo.c_str()); }
		lua_rawset(L, x);

		lua_pushliteral(L, "range_high");
		if (geo_ran_hi.empty()) { lua_pushnil(L); } else { lua_pushstring(L, geo_ran_hi.c_str()); }
		lua_rawset(L, x);

		lua_pushliteral(L, "country_code");
		if (geo_cc.empty()) { lua_pushnil(L); } else { lua_pushstring(L, geo_cc.c_str()); }
		lua_rawset(L, x);

		lua_pushliteral(L, "country_code_xxx");
		if (geo_ccc.empty()) { lua_pushnil(L); } else { lua_pushstring(L, geo_ccc.c_str()); }
		lua_rawset(L, x);

		lua_pushliteral(L, "country");
		if (geo_cn.empty()) { lua_pushnil(L); } else { lua_pushstring(L, geo_cn.c_str()); }
		lua_rawset(L, x);

		lua_pushliteral(L, "region_code");
		if (geo_reg_code.empty()) { lua_pushnil(L); } else { lua_pushstring(L, geo_reg_code.c_str()); }
		lua_rawset(L, x);

		lua_pushliteral(L, "region");
		if (geo_reg_name.empty()) { lua_pushnil(L); } else { lua_pushstring(L, geo_reg_name.c_str()); }
		lua_rawset(L, x);

		lua_pushliteral(L, "time_zone");
		if (geo_tz.empty()) { lua_pushnil(L); } else { lua_pushstring(L, geo_tz.c_str()); }
		lua_rawset(L, x);

		lua_pushliteral(L, "continent_code");
		if (geo_cont.empty()) { lua_pushnil(L); } else { lua_pushstring(L, geo_cont.c_str()); }
		lua_rawset(L, x);

		string cont;

		if (geo_cont == "AF") {
			cont = "Africa";
		} else if (geo_cont == "AS") {
			cont = "Asia";
		} else if (geo_cont == "EU") {
			cont = "Europe";
		} else if (geo_cont == "NA") {
			cont = "North America";
		} else if (geo_cont == "SA") {
			cont = "South America";
		} else if (geo_cont == "OC") {
			cont = "Oceania";
		} else if (geo_cont == "AN") {
			cont = "Antarctica";
		}

		lua_pushliteral(L, "continent");
		if (cont.empty()) { lua_pushnil(L); } else { lua_pushstring(L, cont.c_str()); }
		lua_rawset(L, x);

		lua_pushliteral(L, "city");
		if (geo_city.empty()) { lua_pushnil(L); } else { lua_pushstring(L, geo_city.c_str()); }
		lua_rawset(L, x);

		lua_pushliteral(L, "postal_code");
		if (geo_post.empty()) { lua_pushnil(L); } else { lua_pushstring(L, geo_post.c_str()); }
		lua_rawset(L, x);

		lua_pushliteral(L, "latitude");
		lua_pushnumber(L, (double)geo_lat);
		lua_rawset(L, x);

		lua_pushliteral(L, "longitude");
		lua_pushnumber(L, (double)geo_lon);
		lua_rawset(L, x);

		lua_pushliteral(L, "metro_code");
		lua_pushnumber(L, (unsigned short)geo_met);
		lua_rawset(L, x);

		lua_pushliteral(L, "area_code");
		lua_pushnumber(L, (unsigned short)geo_area);
		lua_rawset(L, x);
	} else {
		lua_pushboolean(L, 0);
		lua_newtable(L);
		int x = lua_gettop(L);

		lua_pushliteral(L, "host");
		lua_pushnil(L);
		lua_rawset(L, x);

		lua_pushliteral(L, "range_low");
		lua_pushnil(L);
		lua_rawset(L, x);

		lua_pushliteral(L, "range_high");
		lua_pushnil(L);
		lua_rawset(L, x);

		lua_pushliteral(L, "country_code");
		lua_pushnil(L);
		lua_rawset(L, x);

		lua_pushliteral(L, "country_code_xxx");
		lua_pushnil(L);
		lua_rawset(L, x);

		lua_pushliteral(L, "country");
		lua_pushnil(L);
		lua_rawset(L, x);

		lua_pushliteral(L, "region_code");
		lua_pushnil(L);
		lua_rawset(L, x);

		lua_pushliteral(L, "region");
		lua_pushnil(L);
		lua_rawset(L, x);

		lua_pushliteral(L, "time_zone");
		lua_pushnil(L);
		lua_rawset(L, x);

		lua_pushliteral(L, "continent_code");
		lua_pushnil(L);
		lua_rawset(L, x);

		lua_pushliteral(L, "city");
		lua_pushnil(L);
		lua_rawset(L, x);

		lua_pushliteral(L, "postal_code");
		lua_pushnil(L);
		lua_rawset(L, x);

		lua_pushliteral(L, "latitude");
		lua_pushnil(L);
		lua_rawset(L, x);

		lua_pushliteral(L, "longitude");
		lua_pushnil(L);
		lua_rawset(L, x);

		lua_pushliteral(L, "metro_code");
		lua_pushnil(L);
		lua_rawset(L, x);

		lua_pushliteral(L, "area_code");
		lua_pushnil(L);
		lua_rawset(L, x);
	}

	return 2;
}

int _GetNickList(lua_State *L)
{
	int result = 1;

	if (lua_gettop(L) == 1) {
		const char *nicklist = GetNickList();

		if (!nicklist || nicklist[0] == '\0')
			result = 0;

		lua_pushboolean(L, result);
		lua_pushstring(L, nicklist);

		if (nicklist)
			free((void*)nicklist);

		return 2;
	} else {
		luaL_error(L, "Error calling VH:GetNickList; expected 0 argument but got %d", lua_gettop(L) - 1);
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}
}

int _GetOPList(lua_State *L)
{
	int result = 1;

	if (lua_gettop(L) == 1) {
		cServerDC *server = GetCurrentVerlihub();

		if (server) {
			string oplist;
			server->mOpList.GetNickList(oplist, false);

			if (oplist.empty())
				result = 0;

			lua_pushboolean(L, result);
			lua_pushstring(L, oplist.c_str());
			return 2;
		} else {
			luaerror(L, ERR_SERV);
			return 2;
		}
	} else {
		luaL_error(L, "Error calling VH:GetOPList; expected 0 argument but got %d", lua_gettop(L) - 1);
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}
}

int _GetBotList(lua_State *L)
{
	int result = 1;

	if (lua_gettop(L) == 1) {
		cServerDC *server = GetCurrentVerlihub();

		if (server) {
			string botlist;
			server->mRobotList.GetNickList(botlist, false);

			if (botlist.empty())
				result = 0;

			lua_pushboolean(L, result);
			lua_pushstring(L, botlist.c_str());
			return 2;
		} else {
			luaerror(L, ERR_SERV);
			return 2;
		}
	} else {
		luaL_error(L, "Error calling VH:GetBotList; expected 0 argument but got %d", lua_gettop(L) - 1);
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}
}

int _GetUserClass(lua_State *L)
{
	string nick;
	int uclass;

	if(lua_gettop(L) == 2) {
		if(!lua_isstring(L, 2)) {
			luaerror(L, ERR_PARAM);
			return 2;
		}
		nick = lua_tostring(L, 2);
		uclass = GetUserClass(nick.c_str());
		lua_pushboolean(L, 1);
		lua_pushnumber(L, uclass);
		return 2;
	} else {
		luaL_error(L, "Error calling VH:GetNickList; expected 1 argument but got %d", lua_gettop(L) - 1);
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}
}

int _GetUserHost(lua_State *L)
{
	string nick;
	const char *host;

	if(lua_gettop(L) == 2) {
		if(!lua_isstring(L, 2)) {
			luaerror(L, ERR_PARAM);
			return 2;
		}
		nick = lua_tostring(L, 2);
		host = GetUserHost(nick.c_str());
		lua_pushboolean(L, 1);
		lua_pushstring(L, host);
		return 2;
	} else {
		luaL_error(L, "Error calling VH:GetUserHost; expected 1 argument but got %d", lua_gettop(L) - 1);
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}
}

int _GetUserIP(lua_State *L)
{
	string nick;
	const char *ip;

	if(lua_gettop(L) == 2) {
		if(!lua_isstring(L, 2)) {
			luaerror(L, ERR_PARAM);
			return 2;
		}
		nick = lua_tostring(L, 2);
		ip = GetUserIP(nick.c_str());
		lua_pushboolean(L, 1);
		lua_pushstring(L, ip);
		return 2;
	} else {
		luaL_error(L, "Error calling VH:GetUserIP; expected 1 argument but got %d", lua_gettop(L) - 1);
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}
}

int _IsUserOnline(lua_State *L)
{
	if(lua_gettop(L) == 2) {
		cServerDC *server = GetCurrentVerlihub();
		if(server == NULL) {
			luaerror(L, ERR_SERV);
			return 2;
		}
		if(!lua_isstring(L, 2)) {
			luaerror(L, ERR_PARAM);
			return 2;
		}
		string nick = lua_tostring(L, 2);
		cUser *usr = server->mUserList.GetUserByNick(nick);
		lua_pushboolean(L, (usr == NULL ? 0 : 1));
		return 1;
	} else {
		luaL_error(L, "Error calling VH:IsUserOnline; expected 1 argument but got %d", lua_gettop(L) - 1);
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}
}

int _GetUserVersion(lua_State *L)
{
	if (lua_gettop(L) < 2) {
		luaL_error(L, "Error calling VH:GetUserVersion, expected 1 argument but got %d.", lua_gettop(L) - 1);
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}

	cServerDC *serv = GetCurrentVerlihub();

	if (serv == NULL) {
		luaerror(L, ERR_SERV);
		return 2;
	}

	if (!lua_isstring(L, 2)) {
		luaerror(L, ERR_PARAM);
		return 2;
	}

	string nick = lua_tostring(L, 2);
	cUser *usr = serv->mUserList.GetUserByNick(nick);

	if ((usr == NULL) || (usr->mxConn == NULL)) {
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}

	lua_pushboolean(L, 1);
	lua_pushstring(L, usr->mxConn->mVersion.c_str());
	return 2;
}

int _GetUserSupports(lua_State *L)
{
	if (lua_gettop(L) < 2) {
		luaL_error(L, "Error calling VH:GetUserSupports, expected 1 argument but got %d.", lua_gettop(L) - 1);
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}

	cServerDC *serv = GetCurrentVerlihub();

	if (serv == NULL) {
		luaerror(L, ERR_SERV);
		return 2;
	}

	if (!lua_isstring(L, 2)) {
		luaerror(L, ERR_PARAM);
		return 2;
	}

	string nick = lua_tostring(L, 2);
	cUser *usr = serv->mUserList.GetUserByNick(nick);

	if ((usr == NULL) || (usr->mxConn == NULL)) {
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}

	lua_pushboolean(L, 1);
	lua_pushstring(L, usr->mxConn->mSupportsText.c_str());
	return 2;
}

int _GetUserHubURL(lua_State *L)
{
	if (lua_gettop(L) < 2) {
		luaL_error(L, "Error calling VH:GetUserHubURL, expected 1 argument but got %d.", lua_gettop(L) - 1);
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}

	cServerDC *serv = GetCurrentVerlihub();

	if (!serv) {
		luaerror(L, ERR_SERV);
		return 2;
	}

	if (!lua_isstring(L, 2)) {
		luaerror(L, ERR_PARAM);
		return 2;
	}

	string nick = lua_tostring(L, 2);
	cUser *user = serv->mUserList.GetUserByNick(nick);

	if (!user || !user->mxConn) {
		luaerror(L, "User not found");
		return 2;
	}

	lua_pushboolean(L, 1);
	lua_pushstring(L, user->mxConn->mHubURL.c_str());
	return 2;
}

int _GetUserExtJSON(lua_State *L)
{
	if (lua_gettop(L) < 2) {
		luaL_error(L, "Error calling VH:GetUserExtJSON, expected 1 argument but got %d.", lua_gettop(L) - 1);
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}

	cServerDC *serv = GetCurrentVerlihub();

	if (!serv) {
		luaerror(L, ERR_SERV);
		return 2;
	}

	if (!lua_isstring(L, 2)) {
		luaerror(L, ERR_PARAM);
		return 2;
	}

	string nick = lua_tostring(L, 2);
	cUser *user = serv->mUserList.GetUserByNick(nick);

	if (!user) {
		luaerror(L, "User not found");
		return 2;
	}

	lua_pushboolean(L, 1);
	lua_pushstring(L, user->mExtJSON.c_str());
	return 2;
}

int _InUserSupports(lua_State *L)
{
	if (lua_gettop(L) < 3) {
		luaL_error(L, "Error calling VH:InUserSupports, expected 2 arguments but got %d.", lua_gettop(L) - 1);
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}

	cServerDC *serv = GetCurrentVerlihub();

	if (!serv) {
		luaerror(L, ERR_SERV);
		return 2;
	}

	if (!lua_isstring(L, 2) || !lua_isstring(L, 3)) {
		luaerror(L, ERR_PARAM);
		return 2;
	}

	string nick = lua_tostring(L, 2);
	string flag = lua_tostring(L, 3);
	cUser *user = serv->mUserList.GetUserByNick(nick);

	if (!user || !user->mxConn) {
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}

	if (
		((flag.size() == 6) && (StrCompare(flag, 0, 6, "OpPlus") == 0) && (user->mxConn->mFeatures & eSF_OPPLUS)) ||
		((flag.size() == 7) && (StrCompare(flag, 0, 7, "NoHello") == 0) && (user->mxConn->mFeatures & eSF_NOHELLO)) ||
		((flag.size() == 9) && (StrCompare(flag, 0, 9, "NoGetINFO") == 0) && (user->mxConn->mFeatures & eSF_NOGETINFO)) ||
		((flag.size() == 4) && (StrCompare(flag, 0, 4, "DHT0") == 0) && (user->mxConn->mFeatures & eSF_DHT0)) ||
		((flag.size() == 9) && (StrCompare(flag, 0, 9, "QuickList") == 0) && (user->mxConn->mFeatures & eSF_QUICKLIST)) ||
		((flag.size() == 7) && (StrCompare(flag, 0, 7, "BotINFO") == 0) && (user->mxConn->mFeatures & eSF_BOTINFO)) ||
		((((flag.size() == 6) && (StrCompare(flag, 0, 6, "ZPipe0") == 0)) || ((flag.size() == 5) && (StrCompare(flag, 0, 5, "ZPipe") == 0))) && (user->mxConn->mFeatures & eSF_ZLIB)) ||
		((flag.size() == 8) && (StrCompare(flag, 0, 8, "ChatOnly") == 0) && (user->mxConn->mFeatures & eSF_CHATONLY)) ||
		((flag.size() == 4) && (StrCompare(flag, 0, 4, "MCTo") == 0) && (user->mxConn->mFeatures & eSF_MCTO)) ||
		((flag.size() == 11) && (StrCompare(flag, 0, 11, "UserCommand") == 0) && (user->mxConn->mFeatures & eSF_USERCOMMAND)) ||
		((flag.size() == 7) && (StrCompare(flag, 0, 7, "BotList") == 0) && (user->mxConn->mFeatures & eSF_BOTLIST)) ||
		((flag.size() == 8) && (StrCompare(flag, 0, 8, "HubTopic") == 0) && (user->mxConn->mFeatures & eSF_HUBTOPIC)) ||
		((flag.size() == 7) && (StrCompare(flag, 0, 7, "UserIP2") == 0) && (user->mxConn->mFeatures & eSF_USERIP2)) ||
		((flag.size() == 9) && (StrCompare(flag, 0, 9, "TTHSearch") == 0) && (user->mxConn->mFeatures & eSF_TTHSEARCH)) ||
		((flag.size() == 4) && (StrCompare(flag, 0, 4, "Feed") == 0) && (user->mxConn->mFeatures & eSF_FEED)) ||
		((flag.size() == 4) && (StrCompare(flag, 0, 4, "TTHS") == 0) && (user->mxConn->mFeatures & eSF_TTHS)) ||
		((flag.size() == 2) && (StrCompare(flag, 0, 2, "IN") == 0) && (user->mxConn->mFeatures & eSF_IN)) ||
		((flag.size() == 6) && (StrCompare(flag, 0, 6, "BanMsg") == 0) && (user->mxConn->mFeatures & eSF_BANMSG)) ||
		((flag.size() == 3) && (StrCompare(flag, 0, 3, "TLS") == 0) && (user->mxConn->mFeatures & eSF_TLS)) ||
		((flag.size() == 8) && (StrCompare(flag, 0, 8, "FailOver") == 0) && (user->mxConn->mFeatures & eSF_FAILOVER)) ||
		((flag.size() == 10) && (StrCompare(flag, 0, 10, "NickChange") == 0) && (user->mxConn->mFeatures & eSF_NICKCHANGE)) ||
		((flag.size() == 10) && (StrCompare(flag, 0, 10, "ClientNick") == 0) && (user->mxConn->mFeatures & eSF_CLIENTNICK)) ||
		((flag.size() == 5) && (StrCompare(flag, 0, 5, "ZLine") == 0) && (user->mxConn->mFeatures & eSF_ZLINE)) ||
		((flag.size() == 9) && (StrCompare(flag, 0, 9, "GetZBlock") == 0) && (user->mxConn->mFeatures & eSF_GETZBLOCK)) ||
		((flag.size() == 4) && (StrCompare(flag, 0, 4, "ACTM") == 0) && (user->mxConn->mFeatures & eSF_ACTM)) ||
		((flag.size() == 8) && (StrCompare(flag, 0, 8, "SaltPass") == 0) && (user->mxConn->mFeatures & eSF_SALTPASS)) ||
		((flag.size() == 8) && (StrCompare(flag, 0, 8, "NickRule") == 0) && (user->mxConn->mFeatures & eSF_NICKRULE)) ||
		((flag.size() == 10) && (StrCompare(flag, 0, 10, "SearchRule") == 0) && (user->mxConn->mFeatures & eSF_SEARRULE)) ||
		((flag.size() == 6) && (StrCompare(flag, 0, 6, "HubURL") == 0) && (user->mxConn->mFeatures & eSF_HUBURL)) ||
		((flag.size() == 8) && (StrCompare(flag, 0, 8, "ExtJSON2") == 0) && (user->mxConn->mFeatures & eSF_EXTJSON2))
	) {
		lua_pushboolean(L, 1);
		lua_pushboolean(L, 1);
		return 2;
	}

	lua_pushboolean(L, 1);
	lua_pushboolean(L, 0);
	return 2;
}

int _Ban(lua_State *L)
{
	string nick, op, reason;
	unsigned howlong;
	int bantype;

	if(lua_gettop(L) == 6) // todo: add operator and user notes
	{
		if(!lua_isstring(L, 2)) {
			luaerror(L, ERR_PARAM);
			return 2;
		}
		nick = lua_tostring(L, 2);
		if(!lua_isstring(L, 3)) {
			luaerror(L, ERR_PARAM);
			return 2;
		}
		op = lua_tostring(L, 3);
		if(!lua_isstring(L, 4)) {
			luaerror(L, ERR_PARAM);
			return 2;
		}
		reason = lua_tostring(L, 4);
		if(!lua_isnumber(L, 5)) {
			luaerror(L, ERR_PARAM);
			return 2;
		}
		howlong = (unsigned) lua_tonumber(L, 5);
		if(!lua_isnumber(L, 6)) {
			luaerror(L, ERR_PARAM);
			return 2;
		}
		bantype = (int) lua_tonumber(L, 6);
		if(!Ban(nick.c_str(), op, reason, howlong, bantype)) {
			//lua_pushboolean(L, 0);
			luaerror(L, "User not found");
			return 2;
		}
	} else {
		luaL_error(L, "Error calling VH:Ban; expected 5 argument but got %d", lua_gettop(L) - 1);
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}
	lua_pushboolean(L, 1);
	return 1;
}

int _KickUser(lua_State *L)
{
	int args = lua_gettop(L) - 1;

	if (args < 3) {
		luaL_error(L, "Error calling VH:KickUser, expected atleast 3 arguments but got %d.", args);
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}

	if (!lua_isstring(L, 2) || !lua_isstring(L, 3) ||!lua_isstring(L, 4) || ((args >= 4) && !lua_isstring(L, 5)) || ((args >= 5) && !lua_isstring(L, 6))) {
		luaerror(L, ERR_PARAM);
		return 2;
	}

	string oper = lua_tostring(L, 2), nick = lua_tostring(L, 3), why = lua_tostring(L, 4), note_op, note_usr;

	if (nick.empty() || oper.empty()) {
		luaerror(L, ERR_PARAM);
		return 2;
	}

	if (args >= 4)
		note_op = lua_tostring(L, 5);

	if (args >= 5)
		note_usr = lua_tostring(L, 6);

	if (!KickUser(oper.c_str(), nick.c_str(), why.c_str(), (note_op.size() ? note_op.c_str() : NULL), (note_usr.size() ? note_usr.c_str() : NULL))) {
		luaerror(L, ERR_CALL);
		return 2;
	}

	lua_pushboolean(L, 1);
	lua_pushnil(L);
	return 2;
}

int _KickRedirUser(lua_State *L)
{
	int args = lua_gettop(L) - 1;

	if (args < 4) {
		luaL_error(L, "Error calling VH:KickRedirUser, expected atleast 4 arguments but got %d.", lua_gettop(L) - 1);
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}

	cServerDC *serv = GetCurrentVerlihub();

	if (!serv) {
		luaerror(L, ERR_SERV);
		return 2;
	}

	if (!lua_isstring(L, 2) || !lua_isstring(L, 3) || !lua_isstring(L, 4) || !lua_isstring(L, 5) || ((args >= 5) && !lua_isstring(L, 6)) || ((args >= 6) && !lua_isstring(L, 7))) {
		luaerror(L, ERR_PARAM);
		return 2;
	}

	string niop = lua_tostring(L, 2), nius = lua_tostring(L, 3), why = lua_tostring(L, 4), addr = lua_tostring(L, 5), note_op, note_usr;

	if (niop.empty() || nius.empty() || addr.empty()) {
		luaerror(L, ERR_PARAM);
		return 2;
	}

	if (args >= 5)
		note_op = lua_tostring(L, 6);

	if (args >= 6)
		note_usr = lua_tostring(L, 7);

	cUser *oper = serv->mUserList.GetUserByNick(niop);

	if (!oper) {
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}

	cUser *user = serv->mUserList.GetUserByNick(nius);

	if (!user || !user->mxConn) {
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}

	user->mxConn->mCloseRedirect = addr; // set redirect
	serv->DCKickNick(NULL, oper, nius, why, (eKI_CLOSE | eKI_WHY | eKI_PM | eKI_BAN), note_op, note_usr); // kick user
	lua_pushboolean(L, 1);
	lua_pushnil(L);
	return 2;
}

int _DelNickTempBan(lua_State *L)
{
	if (lua_gettop(L) < 2) {
		luaL_error(L, "Error calling VH:DelNickTempBan, expected 1 argument but got %d.", lua_gettop(L) - 1);
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}

	if (!lua_isstring(L, 2)) {
		luaerror(L, ERR_PARAM);
		return 2;
	}

	string nick = lua_tostring(L, 2);

	if (DeleteNickTempBan(nick.c_str()))
		lua_pushboolean(L, 1);
	else
		lua_pushboolean(L, 0);

	lua_pushnil(L);
	return 2;
}

int _DelIPTempBan(lua_State *L)
{
	if (lua_gettop(L) < 2) {
		luaL_error(L, "Error calling VH:DelIPTempBan, expected 1 argument but got %d.", lua_gettop(L) - 1);
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}

	if (!lua_isstring(L, 2)) {
		luaerror(L, ERR_PARAM);
		return 2;
	}

	string ip = lua_tostring(L, 2);

	if (DeleteIPTempBan(ip.c_str()))
		lua_pushboolean(L, 1);
	else
		lua_pushboolean(L, 0);

	lua_pushnil(L);
	return 2;
}

int _SetConfig(lua_State *L)
{
	if (lua_gettop(L) < 4) {
		luaL_error(L, "Error calling VH:SetConfig, expected 3 argument but got %d.", lua_gettop(L) - 1);
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}

	if (!lua_isstring(L, 2) || !lua_isstring(L, 3) || !lua_isstring(L, 4)) {
		luaerror(L, ERR_PARAM);
		return 2;
	}

	string conf = lua_tostring(L, 2);
	string var = lua_tostring(L, 3);
	string val = lua_tostring(L, 4);

	if (!SetConfig(conf.c_str(), var.c_str(), val.c_str())) {
		luaerror(L, ERR_CALL);
		return 2;
	}

	lua_pushboolean(L, 1);
	lua_pushnil(L);
	return 2;
}

int _GetConfig(lua_State *L)
{
	if (lua_gettop(L) < 3) {
		luaL_error(L, "Error calling VH:GetConfig, expected 2 argument but got %d.", lua_gettop(L) - 1);
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}

	if (!lua_isstring(L, 2) || !lua_isstring(L, 3)) {
		luaerror(L, ERR_PARAM);
		return 2;
	}

	string conf = lua_tostring(L, 2);
	string var = lua_tostring(L, 3);
	char *val = GetConfig(conf.c_str(), var.c_str());

	if (!val) {
		luaerror(L, "Error calling GetConfig API");
		return 2;
	}

	lua_pushboolean(L, 1);
	lua_pushstring(L, val);
	free(val);
	return 2;
}

int _GetLuaBots(lua_State *L)
{
	lua_newtable(L);
	int key = 0, top = lua_gettop(L), tot = cpiLua::me->Size();
	cLuaInterpreter *li;

	for (int pos = 0; pos < tot; pos++) {
		li = cpiLua::me->mLua[pos];

		if (li && li->mScriptName.size()) {
			for (cLuaInterpreter::tvBot::iterator it = li->botList.begin(); it != li->botList.end(); ++it) {
				lua_pushnumber(L, ++key);
				lua_newtable(L);
				int dep = lua_gettop(L);
				lua_pushliteral(L, "sScriptname");
				lua_pushstring(L, li->mScriptName.c_str());
				lua_rawset(L, dep);
				lua_pushliteral(L, "sNick");
				lua_pushstring (L, it->first.c_str());
				lua_rawset(L, dep);
				lua_pushliteral(L, "sMyINFO");
				lua_pushstring(L, it->second.uMyINFO.c_str());
				lua_rawset(L, dep);
				lua_pushliteral(L, "iShare");
				lua_pushnumber(L, it->second.uShare);
				lua_rawset(L, dep);
				lua_pushliteral(L, "iClass");
				lua_pushnumber(L, it->second.uClass);
				lua_rawset(L, dep);
				lua_rawset(L, top);
			}
		}
	}

	return 1;
}

int _RegBot(lua_State *L)
{
	if (lua_gettop(L) < 7) {
		luaL_error(L, "Error calling VH:RegBot, expected 6 argument but got %d.", lua_gettop(L) - 1);
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}

	if (!lua_isstring(L, 2) || (!lua_isnumber(L, 3) && !lua_isstring(L, 3)) || !lua_isstring(L, 4) || !lua_isstring(L, 5) || !lua_isstring(L, 6) || (!lua_isnumber(L, 7) && !lua_isstring(L, 7))) {
		luaerror(L, ERR_PARAM);
		return 2;
	}

	cServerDC *serv = GetCurrentVerlihub();

	if (!serv) {
		luaerror(L, ERR_SERV);
		return 2;
	}

	cpiLua *plug = (cpiLua*)serv->mPluginManager.GetPlugin(LUA_PI_IDENTIFIER);

	if (!plug) {
		luaerror(L, ERR_PLUG);
		return 2;
	}

	cLuaInterpreter *li = FindLua(L);

	if (!li) {
		luaerror(L, ERR_LUA);
		return 2;
	}

	const string nick = lua_tostring(L, 2);
	int bad = CheckBotNick(nick); // check bot nick

	switch (bad) {
		case 2:
			luaerror(L, "Bot nick can not be empty");
			break;
		case 3:
			luaerror(L, "Bot nick contains bad characters");
			break;
		case 4:
			luaerror(L, "Bot nick is reserved");
			break;
	}

	if (bad > 1)
		return 2;

	if (serv->mRobotList.ContainsNick(nick)) {
		luaerror(L, "Bot already exists");
		return 2;
	}

	int iclass;

	if (lua_isstring(L, 3)) {
		string temp = lua_tostring(L, 3);

		if (temp.empty())
			iclass = 0;
		else
			iclass = StrToInt(temp);
	} else
		iclass = (int)lua_tonumber(L, 3);

	if ((iclass < eUC_PINGER) || (iclass > eUC_MASTER) || ((iclass > eUC_ADMIN) && (iclass < eUC_MASTER))) {
		luaerror(L, ERR_CLASS);
		return 2;
	}

	cPluginRobot *robot = plug->NewRobot(nick, iclass);

	if (!robot) {
		luaerror(L, "Error registering bot");
		return 2;
	}

	string desc = lua_tostring(L, 4);
	string speed = lua_tostring(L, 5);
	string email = lua_tostring(L, 6);
	string share;
	int ishare;

	if (lua_isstring(L, 7)) {
		share = lua_tostring(L, 7);

		if (share.empty()) {
			share = "0";
			ishare = 0;
		} else
			ishare = StrToInt(share);
	} else {
		ishare = (int)lua_tonumber(L, 7);
		share = IntToStr(ishare);
	}

	serv->mP.Create_MyINFO(robot->mMyINFO, nick, desc, speed, email, share, false); // send myinfo, dont reserve for pipe, we are not sending this
	string data;
	data.reserve(robot->mMyINFO.size() + 1); // first use, reserve for pipe
	data = robot->mMyINFO;
	serv->mUserList.SendToAll(data, serv->mC.delayed_myinfo, true);

	if (robot->mClass >= serv->mC.oplist_class) { // send short oplist, reserve for pipe
		serv->mP.Create_OpList(data, nick, true);
		serv->mUserList.SendToAll(data, serv->mC.delayed_myinfo, true);
	}

	serv->mP.Create_BotList(data, nick, true); // send short botlist, reserve for pipe
	serv->mUserList.SendToAllWithFeature(data, eSF_BOTLIST, serv->mC.delayed_myinfo, true);

	li->addBot(nick.c_str(), robot->mMyINFO.c_str(), ishare, iclass); // add to lua bots
	lua_pushboolean(L, 1);
	lua_pushnil(L);
	return 2;
}

int _EditBot(lua_State *L)
{
	if (lua_gettop(L) < 7) {
		luaL_error(L, "Error calling VH:EditBot, expected 6 argument but got %d.", lua_gettop(L) - 1);
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}

	if (!lua_isstring(L, 2) || (!lua_isnumber(L, 3) && !lua_isstring(L, 3)) || !lua_isstring(L, 4) || !lua_isstring(L, 5) || !lua_isstring(L, 6) || (!lua_isnumber(L, 7) && !lua_isstring(L, 7))) {
		luaerror(L, ERR_PARAM);
		return 2;
	}

	cServerDC *serv = GetCurrentVerlihub();

	if (!serv) {
		luaerror(L, ERR_SERV);
		return 2;
	}

	cLuaInterpreter *li = FindLua(L);

	if (!li) {
		luaerror(L, ERR_LUA);
		return 2;
	}

	const string nick = lua_tostring(L, 2);
	int bad = CheckBotNick(nick); // check bot nick

	switch (bad) {
		case 2:
			luaerror(L, "Bot nick can not be empty");
			break;
		case 3:
			luaerror(L, "Bot nick contains bad characters");
			break;
		case 4:
			luaerror(L, "Bot nick is reserved");
			break;
	}

	if (bad > 1)
		return 2;

	if (!serv->mRobotList.ContainsNick(nick)) {
		luaerror(L, "Bot not found");
		return 2;
	}

	cUserRobot *robot = (cUserRobot*)serv->mRobotList.GetUserBaseByNick(nick);

	if (!robot) {
		luaerror(L, "Error getting bot");
		return 2;
	}

	int iclass;

	if (lua_isstring(L, 3)) {
		string temp = lua_tostring(L, 3);

		if (temp.empty())
			iclass = 0;
		else
			iclass = StrToInt(temp);
	} else
		iclass = (int)lua_tonumber(L, 3);

	if ((iclass < eUC_PINGER) || (iclass > eUC_MASTER) || ((iclass > eUC_ADMIN) && (iclass < eUC_MASTER))) {
		luaerror(L, ERR_CLASS);
		return 2;
	}

	string desc = lua_tostring(L, 4);
	string speed = lua_tostring(L, 5);
	string email = lua_tostring(L, 6);
	string share;
	int ishare;

	if (lua_isstring(L, 7)) {
		share = lua_tostring(L, 7);

		if (share.empty()) {
			share = "0";
			ishare = 0;
		} else
			ishare = StrToInt(share);
	} else {
		ishare = (int)lua_tonumber(L, 7);
		share = IntToStr(ishare);
	}

	string data;

	if (robot->mClass != iclass) { // different class
		if ((robot->mClass < serv->mC.opchat_class) && (iclass >= serv->mC.opchat_class)) { // add to opchat list
			if (!serv->mOpchatList.ContainsHash(robot->mNickHash))
				serv->mOpchatList.AddWithHash(robot, robot->mNickHash);

		} else if ((robot->mClass >= serv->mC.opchat_class) && (iclass < serv->mC.opchat_class)) { // remove from opchat list
			if (serv->mOpchatList.ContainsHash(robot->mNickHash))
				serv->mOpchatList.RemoveByHash(robot->mNickHash);
		}

		if ((robot->mClass < serv->mC.oplist_class) && (iclass >= serv->mC.oplist_class)) { // changing from user to op
			if (!serv->mOpList.ContainsHash(robot->mNickHash)) { // add to oplist
				serv->mOpList.AddWithHash(robot, robot->mNickHash);

				serv->mP.Create_OpList(data, nick, true); // send short oplist, reserve for pipe
				serv->mUserList.SendToAll(data, serv->mC.delayed_myinfo, true);
			}

		} else if ((robot->mClass >= serv->mC.oplist_class) && (iclass < serv->mC.oplist_class)) { // changing from op to user
			if (serv->mOpList.ContainsHash(robot->mNickHash)) { // remove from oplist
				serv->mOpList.RemoveByHash(robot->mNickHash);

				serv->mP.Create_Quit(data, nick, true); // send quit, reserve for pipe
				serv->mUserList.SendToAll(data, serv->mC.delayed_myinfo, true);

				serv->mP.Create_BotList(data, nick, true); // send short botlist after quit, reserve for pipe
				serv->mUserList.SendToAllWithFeature(data, eSF_BOTLIST, serv->mC.delayed_myinfo, true);
			}
		}

		robot->mClass = (tUserCl)iclass; // set new class
	}

	serv->mP.Create_MyINFO(robot->mMyINFO, nick, desc, speed, email, share, false); // send new myinfo after quit, dont reserve for pipe, we are not sending this

	if (data.capacity() < (robot->mMyINFO.size() + 1)) // reserve for pipe
		data.reserve(robot->mMyINFO.size() + 1);

	data = robot->mMyINFO;
	serv->mUserList.SendToAll(data, serv->mC.delayed_myinfo, true);

	li->editBot(nick.c_str(), robot->mMyINFO.c_str(), ishare, iclass); // edit in lua bots
	lua_pushboolean(L, 1);
	lua_pushnil(L);
	return 2;
}

int _UnRegBot(lua_State *L)
{
	if (lua_gettop(L) < 2) {
		luaL_error(L, "Error calling VH:UnRegBot, expected 1 argument but got %d.", lua_gettop(L) - 1);
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}

	if (!lua_isstring(L, 2)) {
		luaerror(L, ERR_PARAM);
		return 2;
	}

	cServerDC *serv = GetCurrentVerlihub();

	if (!serv) {
		luaerror(L, ERR_SERV);
		return 2;
	}

	cpiLua *plug = (cpiLua*)serv->mPluginManager.GetPlugin(LUA_PI_IDENTIFIER);

	if (!plug) {
		luaerror(L, ERR_PLUG);
		return 2;
	}

	cLuaInterpreter *li = FindLua(L);

	if (!li) {
		luaerror(L, ERR_LUA);
		return 2;
	}

	const string nick = lua_tostring(L, 2);
	int bad = CheckBotNick(nick); // check bot nick

	switch (bad) {
		case 2:
			luaerror(L, "Bot nick can not be empty");
			break;
		case 3:
			luaerror(L, "Bot nick contains bad characters");
			break;
		case 4:
			luaerror(L, "Bot nick is reserved");
			break;
	}

	if (bad > 1)
		return 2;

	if (!serv->mRobotList.ContainsNick(nick)) {
		luaerror(L, "Bot not found");
		return 2;
	}

	cUserRobot *robot = (cUserRobot*)serv->mRobotList.GetUserBaseByNick(nick);

	if (!robot) {
		luaerror(L, "Error getting bot");
		return 2;
	}

	li->delBot(nick.c_str()); // delete from lua bots
	plug->DelRobot(robot); // delete bot
	lua_pushboolean(L, 1);
	lua_pushnil(L);
	return 2;
}

int _IsBot(lua_State *L)
{
	if (lua_gettop(L) < 2) {
		luaL_error(L, "Error calling VH:IsBot, expected 1 argument but got %d.", lua_gettop(L) - 1);
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}

	if (!lua_isstring(L, 2)) {
		luaerror(L, ERR_PARAM);
		return 2;
	}

	cServerDC *serv = GetCurrentVerlihub();

	if (!serv) {
		luaerror(L, ERR_SERV);
		return 2;
	}

	const string nick = lua_tostring(L, 2);
	int bad = CheckBotNick(nick); // check bot nick

	switch (bad) {
		case 2:
			luaerror(L, "Bot nick can not be empty");
			break;
		case 3:
			luaerror(L, "Bot nick contains bad characters");
			break;
		case 4:
			luaerror(L, "Bot nick is reserved");
			break;
	}

	if (bad > 1)
		return 2;

	if (!serv->mRobotList.ContainsNick(nick)) {
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}

	cUserRobot *robot = (cUserRobot*)serv->mRobotList.GetUserBaseByNick(nick);

	if (!robot) {
		luaerror(L, "Error getting bot");
		return 2;
	}

	lua_pushboolean(L, 1);
	lua_pushnil(L);
	return 2;
}

int _SQLQuery(lua_State *L)
{
	if (lua_gettop(L) < 2) {
		luaL_error(L, "Error calling VH:SQLQuery, expected 1 argument but got %d.", lua_gettop(L) - 1);
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}

	cServerDC *serv = GetCurrentVerlihub();

	if (!serv) {
		luaerror(L, ERR_SERV);
		return 2;
	}

	cpiLua *plug = (cpiLua*)serv->mPluginManager.GetPlugin(LUA_PI_IDENTIFIER);

	if (!plug) {
		luaerror(L, ERR_LUA);
		return 2;
	}

	if (!plug->mQuery) {
		luaerror(L, ERR_QUERY);
		return 2;
	}

	if (!lua_isstring(L, 2)) {
	    luaerror(L, ERR_PARAM);
	    return 2;
	}

	plug->mQuery->Clear();
	plug->mQuery->OStream() << lua_tostring(L, 2);
	plug->mQuery->Query();
	int res = plug->mQuery->StoreResult();
	lua_pushboolean(L, 1);

	if (res)
		lua_pushnumber(L, res);
	else
		lua_pushnumber(L, 0);

	return 2;
}

int _SQLFetch(lua_State *L)
{
	if (lua_gettop(L) < 2) {
		luaL_error(L, "Error calling VH:SQLFetch, expected 1 argument but got %d.", lua_gettop(L) - 1);
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}

	cServerDC *serv = GetCurrentVerlihub();

	if (!serv) {
		luaerror(L, ERR_SERV);
		return 2;
	}

	cpiLua *plug = (cpiLua*)serv->mPluginManager.GetPlugin(LUA_PI_IDENTIFIER);

	if (!plug) {
		luaerror(L, ERR_LUA);
		return 2;
	}

	if (!plug->mQuery) {
		luaerror(L, ERR_QUERY);
		return 2;
	}

	if (!lua_isnumber(L, 2)) {
		luaerror(L, ERR_PARAM);
		return 2;
	}

	int pos = (int)lua_tonumber(L, 2);

	if (!plug->mQuery->GetResult()) {
		luaerror(L, "No result");
		return 2;
	}

	plug->mQuery->DataSeek(pos);
	MYSQL_ROW row;

	if (!(row = plug->mQuery->Row())) {
		luaerror(L, "Error fetching row");
		return 2;
	}

	lua_pushboolean(L, 1);
	pos = 0;

	while (pos < plug->mQuery->Cols()) {
		lua_pushstring(L, row[pos]);
		pos++;
	}

	return pos + 1;
}

int _SQLFree(lua_State *L)
{
	cServerDC *serv = GetCurrentVerlihub();

	if (!serv) {
		luaerror(L, ERR_SERV);
		return 2;
	}

	cpiLua *plug = (cpiLua*)serv->mPluginManager.GetPlugin(LUA_PI_IDENTIFIER);

	if (!plug) {
		luaerror(L, ERR_LUA);
		return 2;
	}

	if (!plug->mQuery) {
		luaerror(L, ERR_QUERY);
		return 2;
	}

	plug->mQuery->Clear();
	lua_pushboolean(L, 1);
	lua_pushnil(L);
	return 2;
}

int _GetVHCfgDir(lua_State *L)
{
	if(lua_gettop(L) == 1) {
		lua_pushboolean(L, 1);
		lua_pushstring(L, GetVHCfgDir());
		return 2;
	} else {
		luaL_error(L, "Error calling VH:GetVHCfgDir; expected 0 argument but got %d", lua_gettop(L) -1);
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}
}

int _GetUpTime(lua_State *L)
{
	cServerDC *server = GetCurrentVerlihub();

	if (server == NULL) {
		luaerror(L, ERR_SERV);
		return 2;
	}

	/*
		res,int = VH:GetUpTime() -- return seconds, no arguments, backward compatibility
		res,int = VH:GetUpTime(1) -- return seconds, argument = 1, new style
		res,int = VH:GetUpTime(2) -- return milliseconds, argument = 2, new style
	*/

	int sf = 1;

	if (lua_gettop(L) > 2) {
		luaL_error(L, "Error calling VH:GetUpTime, expected 0 or 1 argument but got %d", lua_gettop(L) - 1); // is this even needed?
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}

	if (lua_gettop(L) == 2) {
		if (!lua_isnumber(L, 2)) {
			luaerror(L, ERR_PARAM);
			return 2;
		}

		int r = (int)lua_tonumber(L, 2);
		if (r == 2) sf = 2;
	}

	cTime upTime;
	upTime = server->mTime;
	upTime -= server->mStartTime;
	lua_pushboolean(L, 1);

	if (sf == 1)
		lua_pushnumber(L, upTime.Sec());
	else
		lua_pushnumber(L, upTime.MiliSec());

	return 2;
}

int _GetServFreq(lua_State *L)
{
	cServerDC *serv = GetCurrentVerlihub();

	if (!serv) {
		luaerror(L, ERR_SERV);
		return 2;
	}

	ostringstream freq;
	freq << serv->mFrequency.GetMean(serv->mTime);
	lua_pushboolean(L, 1);
	lua_pushstring(L, freq.str().c_str());
	return 2;
}

int _GetHubIp(lua_State *L)
{
	cServerDC *server = GetCurrentVerlihub();
	if(server == NULL) {
		luaerror(L, ERR_SERV);
		return 2;
	}
	lua_pushboolean(L, 1);
	lua_pushstring(L, server->mAddr.c_str());
	return 2;
}

int _GetHubSecAlias(lua_State *L)
{
	cServerDC *server = GetCurrentVerlihub();
	if(server == NULL) {
		luaerror(L, ERR_SERV);
		return 2;
	}
	lua_pushboolean(L, 1);
	lua_pushstring(L, server->mC.hub_security.c_str());
	return 2;
}

int _GetUsersCount(lua_State *L)
{
	lua_pushboolean(L, 1);
	lua_pushnumber(L, GetUsersCount());
	return 2;
}

int _GetTotalShareSize(lua_State *L)
{
	lua_pushboolean(L, 1);
	lua_pushnumber(L, GetTotalShareSize());
	return 2;
}

int _ReportUser(lua_State *L)
{
	if (lua_gettop(L) != 3) {
		luaL_error(L, "Error calling VH:ReportUser, expected 2 arguments but got %d.", lua_gettop(L) - 1);
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}

	cServerDC *serv = GetCurrentVerlihub();

	if (serv == NULL) {
		luaerror(L, ERR_SERV);
		return 2;
	}

	if (!lua_isstring(L, 2) || !lua_isstring(L, 3)) {
		luaerror(L, ERR_PARAM);
		return 2;
	}

	string nick = lua_tostring(L, 2);
	string msg = lua_tostring(L, 3);
	cUser *usr = serv->mUserList.GetUserByNick(nick);

	if ((usr == NULL) || (usr->mxConn == NULL)) {
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}

	serv->ReportUserToOpchat(usr->mxConn, msg, false);
	lua_pushboolean(L, 1);
	return 1;
}

int _ParseCommand(lua_State *L)
{
	if (lua_gettop(L) != 4) {
		luaL_error(L, "Error calling VH:ParseCommand, expected 3 arguments but got %d.", lua_gettop(L) - 1);
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}

	if (!lua_isstring(L, 2) || !lua_isstring(L, 3) || !lua_isnumber(L, 4)) {
		luaerror(L, ERR_PARAM);
		return 2;
	}

	string nick = lua_tostring(L, 2);
	string cmd = lua_tostring(L, 3);
	int pm = (int)lua_tonumber(L, 4);

	if (!ParseCommand(nick.c_str(), cmd.c_str(), pm)) {
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}

	lua_pushboolean(L, 1);
	return 1;
}

int _SetTempRights(lua_State *L)
{
	luaL_error(L, "VH:SetTempRights not implemented yet");
	return 2;
}

int _GetTempRights(lua_State *L)
{
	luaL_error(L, "VH:GetTempRights not implemented yet");
	return 2;
}

cLuaInterpreter *FindLua(lua_State *L)
{
	cLuaInterpreter *temp;
	int vSize = cpiLua::me->Size();

	for (int i = 0; i < vSize; i++) {
		temp = cpiLua::me->mLua[i];

		if (temp && (temp->mL == L))
			return temp;
	}

	return NULL;
}

int _AddChatUser(lua_State *L)
{
	if (lua_gettop(L) < 2) {
		luaL_error(L, "Error calling VH:AddChatUser, expected 1 argument but got %d.", lua_gettop(L) - 1);
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}

	cServerDC *serv = GetCurrentVerlihub();

	if (!serv) {
		luaerror(L, ERR_SERV);
		return 2;
	}

	if (!lua_isstring(L, 2)) {
		luaerror(L, ERR_PARAM);
		return 2;
	}

	string nick = lua_tostring(L, 2);
	cUser *user = serv->mUserList.GetUserByNick(nick);

	if (!user || !user->mxConn) {
		luaerror(L, "User not found");
		return 2;
	}

	if (!serv->mChatUsers.ContainsHash(user->mNickHash)) {
		serv->mChatUsers.AddWithHash(user, user->mNickHash);
		lua_pushboolean(L, 1);
		lua_pushnil(L);
	} else {
		luaerror(L, "User already in list");
	}

	return 2;
}

int _DelChatUser(lua_State *L)
{
	if (lua_gettop(L) < 2) {
		luaL_error(L, "Error calling VH:DelChatUser, expected 1 argument but got %d.", lua_gettop(L) - 1);
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}

	cServerDC *serv = GetCurrentVerlihub();

	if (!serv) {
		luaerror(L, ERR_SERV);
		return 2;
	}

	if (!lua_isstring(L, 2)) {
		luaerror(L, ERR_PARAM);
		return 2;
	}

	string nick = lua_tostring(L, 2);
	cUser *user = serv->mUserList.GetUserByNick(nick);

	if (!user || !user->mxConn) {
		luaerror(L, "User not found");
		return 2;
	}

	if (serv->mChatUsers.ContainsHash(user->mNickHash)) {
		serv->mChatUsers.RemoveByHash(user->mNickHash);
		lua_pushboolean(L, 1);
		lua_pushnil(L);
	} else {
		luaerror(L, "User not in list");
	}

	return 2;
}

int _IsChatUser(lua_State *L)
{
	if (lua_gettop(L) < 2) {
		luaL_error(L, "Error calling VH:IsChatUser, expected 1 argument but got %d.", lua_gettop(L) - 1);
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}

	cServerDC *serv = GetCurrentVerlihub();

	if (!serv) {
		luaerror(L, ERR_SERV);
		return 2;
	}

	if (!lua_isstring(L, 2)) {
		luaerror(L, ERR_PARAM);
		return 2;
	}

	string nick = lua_tostring(L, 2);
	cUser *user = serv->mUserList.GetUserByNick(nick);

	if (!user || !user->mxConn) {
		luaerror(L, "User not found");
		return 2;
	}

	if (serv->mChatUsers.ContainsHash(user->mNickHash)) {
		lua_pushboolean(L, 1);
	} else {
		lua_pushboolean(L, 0);
	}

	lua_pushnil(L);
	return 2;
}

int _AddRegUser(lua_State *L)
{
	int args = lua_gettop(L) - 1;

	if (args < 3) {
		luaL_error(L, "Error calling VH:AddRegUser, expected atleast 3 arguments but got %d.", args);
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}

	if (!lua_isstring(L, 2) || !lua_isstring(L, 3) || !lua_isnumber(L, 4) || ((args >= 4) && !lua_isstring(L, 5))) {
		luaerror(L, ERR_PARAM);
		return 2;
	}

	const string nick = lua_tostring(L, 2);

	if (nick.empty()) {
		luaerror(L, ERR_PARAM);
		return 2;
	}

	const string pass = lua_tostring(L, 3); // can be empty
	int clas = int(lua_tonumber(L, 4));

	if ((clas < eUC_PINGER) || (clas == eUC_NORMUSER) || ((clas > eUC_ADMIN) && (clas < eUC_MASTER)) || (clas > eUC_MASTER)) { // validate class number, todo: can user implement his own classes?
		luaerror(L, ERR_CLASS);
		return 2;
	}

	string op;

	if (args >= 4)
		op = lua_tostring(L, 5);

	if (AddRegUser(nick.c_str(), clas, pass.c_str(), op.c_str()))
		lua_pushboolean(L, 1);
	else
		lua_pushboolean(L, 0);

	lua_pushnil(L);
	return 2;
}

int _DelRegUser(lua_State *L)
{
	if (lua_gettop(L) < 2) {
		luaL_error(L, "Error calling VH:DelRegUser, expected 1 argument but got %d.", lua_gettop(L) - 1);
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}

	if (!lua_isstring(L, 2)) {
		luaerror(L, ERR_PARAM);
		return 2;
	}

	const string nick = lua_tostring(L, 2);

	if (nick.empty()) {
		luaerror(L, ERR_PARAM);
		return 2;
	}

	if (DelRegUser(nick.c_str()))
		lua_pushboolean(L, 1);
	else
		lua_pushboolean(L, 0);

	lua_pushnil(L);
	return 2;
}

int _SetRegClass(lua_State *L)
{
	int args = lua_gettop(L) - 1;

	if (args < 2) {
		luaL_error(L, "Error calling VH:SetRegClass, expected 2 argument but got %d.", args);
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}

	if (!lua_isstring(L, 2) || !lua_isnumber(L, 3)) {
		luaerror(L, ERR_PARAM);
		return 2;
	}

	const string nick = lua_tostring(L, 2);

	if (nick.empty()) {
		luaerror(L, ERR_PARAM);
		return 2;
	}

	int clas = int(lua_tonumber(L, 3));

	if ((clas < eUC_NORMUSER) || ((clas > eUC_ADMIN) && (clas < eUC_MASTER)) || (clas > eUC_MASTER)) { // validate class number, todo: can user implement his own classes?
		luaerror(L, ERR_CLASS);
		return 2;
	}

	if (SetRegClass(nick.c_str(), clas))
		lua_pushboolean(L, 1);
	else
		lua_pushboolean(L, 0);

	lua_pushnil(L);
	return 2;
}

int _GetTopic(lua_State *L)
{
	cServerDC *serv = GetCurrentVerlihub();

	if (!serv) {
		luaerror(L, ERR_SERV);
		return 2;
	}

	lua_pushstring(L, serv->mC.hub_topic.c_str());
	return 1;
}

int _SetTopic(lua_State *L)
{
	if (lua_gettop(L) < 2) {
		luaL_error(L, "Error calling VH:SetTopic, expected 1 argument but got %d.", lua_gettop(L) - 1);
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}

	cServerDC *serv = GetCurrentVerlihub();

	if (!serv) {
		luaerror(L, ERR_SERV);
		return 2;
	}

	cLuaInterpreter *li = FindLua(L);

	if (!li) {
		luaerror(L, ERR_LUA);
		return 2;
	}

	if (!lua_isstring(L, 2)) {
		luaerror(L, ERR_PARAM);
		return 2;
	}

	string topic = lua_tostring(L, 2);
	string omsg;
	serv->mP.Create_HubName(omsg, serv->mC.hub_name, topic, false); // dont reserve for pipe, buffer is copied before adding pipe
	serv->SendToAll(omsg, eUC_NORMUSER, eUC_MASTER);
	SetConfig(li->mConfigName.c_str(), "hub_topic", topic.c_str());
	lua_pushboolean(L, 1);
	lua_pushnil(L);
	return 2;
}

int _ScriptCommand(lua_State *L)
{
	int args = lua_gettop(L) - 1;

	if (args < 2) {
		luaL_error(L, "Error calling VH:ScriptCommand, expected atleast 2 arguments but got %d.", args);
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}

	if (!lua_isstring(L, 2) || !lua_isstring(L, 3) || ((args >= 3) && !lua_isnumber(L, 4))) {
		luaerror(L, ERR_PARAM);
		return 2;
	}

	cLuaInterpreter *li = FindLua(L);

	if (!li) {
		luaerror(L, ERR_LUA);
		return 2;
	}

	string cmd = lua_tostring(L, 2), data = lua_tostring(L, 3), plug("lua");
	bool inst = false;

	if (args >= 3)
		inst = (int(lua_tonumber(L, 4)) > 0);

	if (!ScriptCommand(&cmd, &data, &plug, &li->mScriptName, inst)) {
		luaerror(L, ERR_CALL);
		return 2;
	}

	lua_pushboolean(L, 1);
	lua_pushnil(L);
	return 2;
}

int _ScriptQuery(lua_State *L)
{
	int arg_num = lua_gettop(L) - 1;
	if (arg_num < 2) {
		luaL_error(L, "Error calling VH:ScriptQuery, expected 2 to 4 arguments but got %d.", arg_num);
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}

	if (!lua_isstring(L, 2) || !lua_isstring(L, 3)) {
		luaerror(L, ERR_PARAM);
		return 2;
	}

	if ((arg_num > 2 && !lua_isstring(L, 4)) || (arg_num > 3 && !lua_isnumber(L, 5))) {
		luaerror(L, ERR_PARAM);
		return 2;
	}

	cLuaInterpreter *li = FindLua(L);

	if (!li) {
		luaerror(L, ERR_LUA);
		return 2;
	}

	string cmd = lua_tostring(L, 2);
	string data = lua_tostring(L, 3);
	string recipient = (arg_num > 2) ? lua_tostring(L, 4) : "";
	int use_long_output = (arg_num > 3) ? lua_tonumber(L, 5) : 0;
	ScriptResponses responses;

	if (!ScriptQuery(&cmd, &data, &recipient, &li->mScriptName, &responses)) {
		luaerror(L, ERR_CALL);
		return 2;
	}

	lua_pushboolean(L, 1);
	lua_newtable(L);
	int z = lua_gettop(L);

	for (size_t i = 0; i < responses.size(); i++) {
		if (use_long_output) {
			lua_pushnumber(L, i);
			lua_newtable(L);
			lua_pushnumber(L, 0);
			lua_pushstring(L, responses[i].data.c_str());
			lua_rawset(L, -3);
			lua_pushnumber(L, 1);
			lua_pushstring(L, responses[i].sender.c_str());
			lua_rawset(L, -3);
			lua_rawset(L, z);
		} else {
			lua_pushnumber(L, i);
			lua_pushstring(L, responses[i].data.c_str());
			lua_rawset(L, -3);  // store the pair on the table
		}
	}

	return 2;
}

void luaerror(lua_State *L, const char *errstr)
{
	lua_pushboolean(L, 0);
	lua_pushstring(L, errstr);
}

string IntToStr(int val)
{
	stringstream ss;
	ss << val;
	return ss.str();
}

int StrToInt(string const &str)
{
	stringstream ss(str);
	int val;
	ss >> val;
	return val;
}

}; // namespace nVerliHub
