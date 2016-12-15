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

#define ERR_PARAM "Wrong parameters"
#define ERR_CALL "Call error"
#define ERR_SERV "Error getting server"
#define ERR_LUA "Error getting Lua"
#define ERR_PLUG "Error getting plugin"
#define ERR_CLASS "Invalid class number"

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

cServerDC * GetCurrentVerlihub()
{
	return (cServerDC *)cServerDC::sCurrentServer;
}

int _SendToUser(lua_State *L)
{
	string data, nick;

	if(lua_gettop(L) == 3) {
		if(!lua_isstring(L, 2))
		{
			luaerror(L, ERR_PARAM);
			return 2;
		}
		data = lua_tostring(L, 2);
		if(!lua_isstring(L, 3))
		{
			luaerror(L, ERR_PARAM);
			return 2;
		}
		nick = lua_tostring(L, 3);
		if(!SendDataToUser(data.c_str(), nick.c_str()))
		{
			luaerror(L, ERR_CALL);
			return 2;
		}
	} else {
		luaL_error(L, "Error calling VH:SendToUser; expected 2 arguments but got %d", lua_gettop(L) - 1);
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}
	lua_pushboolean(L, 1);
	return 1;
}

int _SendToClass(lua_State *L)
{
	string data;
	int min_class, max_class;

	if(lua_gettop(L) == 4) {
		if(!lua_isstring(L, 2)) {
			luaerror(L, ERR_PARAM);
			return 2;
		}
		data = lua_tostring(L, 2);
		if(!lua_isnumber(L, 3)) {
			luaerror(L, ERR_PARAM);
			return 2;
		}
		min_class = (int)lua_tonumber(L, 3);
		if(!lua_isnumber(L, 4)) {
			luaerror(L, ERR_PARAM);
			return 2;
		}
		max_class = (int)lua_tonumber(L, 4);
		if(!SendToClass(data.c_str(), min_class, max_class)) {
			luaerror(L, ERR_CALL);
			return 2;
		}
	} else {
		luaL_error(L, "Error calling VH:SendToClass; expected 3 arguments but got %d", lua_gettop(L) - 1);
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}
	lua_pushboolean(L, 1);
	return 1;
}

int _SendToAll(lua_State *L)
{
	string data;

	if(lua_gettop(L) == 2) {
		if(!lua_isstring(L, 2)) {
			luaerror(L, ERR_PARAM);
			return 2;
		}
		data = lua_tostring(L, 2);
		if(!SendToAll(data.c_str())) {
			luaerror(L, ERR_CALL);
			return 2;
		}
	} else {
		luaL_error(L, "Error calling VH:SendToAll; expected 1 argument but got %d", lua_gettop(L) - 1);
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}
	lua_pushboolean(L, 1);
	lua_pushnil(L);
	return 2;
}

int _SendToActive(lua_State *L)
{
	if (lua_gettop(L) == 2) {
		if (!lua_isstring(L, 2)) {
			luaerror(L, ERR_PARAM);
			return 2;
		}

		string data = lua_tostring(L, 2);

		if (!SendToActive(data.c_str())) {
			luaerror(L, ERR_CALL);
			return 2;
		}
	} else {
		luaL_error(L, "Error calling VH:SendToActive, expected 1 argument but got %d.", lua_gettop(L) - 1);
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}

	lua_pushboolean(L, 1);
	return 1;
}

int _SendToActiveClass(lua_State *L)
{
	if (lua_gettop(L) == 4) {
		if (!lua_isstring(L, 2)) {
			luaerror(L, ERR_PARAM);
			return 2;
		}

		string data = lua_tostring(L, 2);

		if (!lua_isnumber(L, 3)) {
			luaerror(L, ERR_PARAM);
			return 2;
		}

		int min_class = (int)lua_tonumber(L, 3);

		if (!lua_isnumber(L, 4)) {
			luaerror(L, ERR_PARAM);
			return 2;
		}

		int max_class = (int)lua_tonumber(L, 4);

		if (!SendToActiveClass(data.c_str(), min_class, max_class)) {
			luaerror(L, ERR_CALL);
			return 2;
		}
	} else {
		luaL_error(L, "Error calling VH:SendToActiveClass, expected 3 arguments but got %d.", lua_gettop(L) - 1);
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}

	lua_pushboolean(L, 1);
	return 1;
}

int _SendToPassive(lua_State *L)
{
	if (lua_gettop(L) == 2) {
		if (!lua_isstring(L, 2)) {
			luaerror(L, ERR_PARAM);
			return 2;
		}

		string data = lua_tostring(L, 2);

		if (!SendToPassive(data.c_str())) {
			luaerror(L, ERR_CALL);
			return 2;
		}
	} else {
		luaL_error(L, "Error calling VH:SendToPassive, expected 1 argument but got %d.", lua_gettop(L) - 1);
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}

	lua_pushboolean(L, 1);
	return 1;
}

int _SendToPassiveClass(lua_State *L)
{
	if (lua_gettop(L) == 4) {
		if (!lua_isstring(L, 2)) {
			luaerror(L, ERR_PARAM);
			return 2;
		}

		string data = lua_tostring(L, 2);

		if (!lua_isnumber(L, 3)) {
			luaerror(L, ERR_PARAM);
			return 2;
		}

		int min_class = (int)lua_tonumber(L, 3);

		if (!lua_isnumber(L, 4)) {
			luaerror(L, ERR_PARAM);
			return 2;
		}

		int max_class = (int)lua_tonumber(L, 4);

		if (!SendToPassiveClass(data.c_str(), min_class, max_class)) {
			luaerror(L, ERR_CALL);
			return 2;
		}
	} else {
		luaL_error(L, "Error calling VH:SendToPassiveClass, expected 3 arguments but got %d.", lua_gettop(L) - 1);
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}

	lua_pushboolean(L, 1);
	return 1;
}

int _SendPMToAll(lua_State *L)
{
	string data, from;
	int min_class = 0, max_class = 10;

	if(lua_gettop(L) >= 2) {
		if(!lua_isstring(L, 2)) {
			luaerror(L, ERR_PARAM);
			return 2;
		}
		data = lua_tostring(L, 2);
		if(!lua_isstring(L, 3)) {
			luaerror(L, ERR_PARAM);
			return 2;
		}
		from = lua_tostring(L, 3);

		if(lua_isnumber(L, 4)) {
			min_class = (int) lua_tonumber(L, 4);
		}

		if(lua_isnumber(L, 5)) {
			max_class = (int) lua_tonumber(L, 5);
		}

		/*
		string start, end;
		cServerDC *server = GetCurrentVerlihub();

		if(server == NULL) {
			luaerror(L, ERR_SERV);
			return 2;
		}

		server->mP.Create_PMForBroadcast(start, end, from, from, data);
		server->SendToAllWithNick(start, end, min_class, max_class);
		*/

		SendPMToAll(data.c_str(), from.c_str(), min_class, max_class);
	} else {
		luaL_error(L, "Error calling VH:SendPMToAll; expected at least 4 arguments but got %d", lua_gettop(L) - 1);
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}
	lua_pushboolean(L, 1);
	return 1;
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

	if (!lua_isstring(L, 2) || !lua_isstring(L, 3)) {
		luaerror(L, ERR_PARAM);
		return 2;
	}

	if ((args >= 4) && (!lua_isnumber(L, 4) || !lua_isnumber(L, 5))) {
		luaerror(L, ERR_PARAM);
		return 2;
	}

	string nick = lua_tostring(L, 2);
	string text = lua_tostring(L, 3);
	int min_class = 0, max_class = 10;

	if (args >= 4) {
		min_class = (int)lua_tonumber(L, 4);
		max_class = (int)lua_tonumber(L, 5);
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
		luaerror(L, ERR_CALL);
		return 2;
	}

	string nick("");

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
	cUser *usr = serv->mUserList.GetUserByNick(nick);

	if (!usr || !usr->mxConn) {
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}

	lua_pushboolean(L, 1);
	lua_pushstring(L, usr->mxConn->mCC.c_str());
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
	cUser *usr = serv->mUserList.GetUserByNick(nick);

	if (!usr || !usr->mxConn) {
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}

	lua_pushboolean(L, 1);
	lua_pushstring(L, usr->mxConn->mCN.c_str());
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
	cUser *usr = serv->mUserList.GetUserByNick(nick);

	if (!usr || !usr->mxConn) {
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}

	lua_pushboolean(L, 1);
	lua_pushstring(L, usr->mxConn->mCity.c_str());
	return 2;
}

#ifdef HAVE_LIBGEOIP
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
		} else
			db = lua_tostring(L, 3);
	}

	string ip = lua_tostring(L, 2);
	string city;

	if (serv->sGeoIP.GetCity(city, ip, db)) {
		lua_pushboolean(L, 1);
		lua_pushstring(L, city.c_str());
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
	float geo_lat, geo_lon;
	int geo_met, geo_area;

	if (serv->sGeoIP.GetGeoIP(geo_host, geo_ran_lo, geo_ran_hi, geo_cc, geo_ccc, geo_cn, geo_reg_code, geo_reg_name, geo_tz, geo_cont, geo_city, geo_post, geo_lat, geo_lon, geo_met, geo_area, usr->mxConn->AddrIP(), db)) {
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
		lua_pushnumber(L, (float)geo_lat);
		lua_rawset(L, x);

		lua_pushliteral(L, "longitude");
		lua_pushnumber(L, (float)geo_lon);
		lua_rawset(L, x);

		lua_pushliteral(L, "metro_code");
		lua_pushnumber(L, (int)geo_met);
		lua_rawset(L, x);

		lua_pushliteral(L, "area_code");
		lua_pushnumber(L, (int)geo_area);
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
	float geo_lat, geo_lon;
	int geo_met, geo_area;

	if (serv->sGeoIP.GetGeoIP(geo_host, geo_ran_lo, geo_ran_hi, geo_cc, geo_ccc, geo_cn, geo_reg_code, geo_reg_name, geo_tz, geo_cont, geo_city, geo_post, geo_lat, geo_lon, geo_met, geo_area, host, db)) {
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
		lua_pushnumber(L, (float)geo_lat);
		lua_rawset(L, x);

		lua_pushliteral(L, "longitude");
		lua_pushnumber(L, (float)geo_lon);
		lua_rawset(L, x);

		lua_pushliteral(L, "metro_code");
		lua_pushnumber(L, (int)geo_met);
		lua_rawset(L, x);

		lua_pushliteral(L, "area_code");
		lua_pushnumber(L, (int)geo_area);
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
#endif

int _GetNickList(lua_State *L)
{
	const char *nicklist;
	int result = 1;
	if(lua_gettop(L) == 1) {
		nicklist = GetNickList();
		if(strlen(nicklist) < 1) result = 0;
		lua_pushboolean(L, result);
		lua_pushstring(L, nicklist);
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
	const char *oplist;
	int result = 1;
	if(lua_gettop(L) == 1) {
		cServerDC *server = GetCurrentVerlihub();
		if(server) {
			oplist = server->mOpList.GetNickList().c_str();
			if(strlen(oplist) < 1) result = 0;
			lua_pushboolean(L, result);
			lua_pushstring(L, oplist);
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
	const char *botlist;
	int result = 1;
	if(lua_gettop(L) == 1) {
		cServerDC *server = GetCurrentVerlihub();
		if(server) {
			botlist = server->mRobotList.GetNickList().c_str();
			if(strlen(botlist) < 1) result = 0;
			lua_pushboolean(L, result);
			lua_pushstring(L, botlist);
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

	if (serv == NULL) {
		luaerror(L, ERR_SERV);
		return 2;
	}

	if (!lua_isstring(L, 2) || !lua_isstring(L, 3)) {
		luaerror(L, ERR_PARAM);
		return 2;
	}

	string nick = lua_tostring(L, 2);
	string flag = lua_tostring(L, 3);
	cUser *usr = serv->mUserList.GetUserByNick(nick);

	if ((usr == NULL) || (usr->mxConn == NULL)) {
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}

	if (
	((flag == "OpPlus") && (usr->mxConn->mFeatures & eSF_OPPLUS)) ||
	((flag == "NoHello") && (usr->mxConn->mFeatures & eSF_NOHELLO)) ||
	((flag == "NoGetINFO") && (usr->mxConn->mFeatures & eSF_NOGETINFO)) ||
	((flag == "DHT0") && (usr->mxConn->mFeatures & eSF_DHT0)) ||
	((flag == "QuickList") && (usr->mxConn->mFeatures & eSF_QUICKLIST)) ||
	((flag == "BotINFO") && (usr->mxConn->mFeatures & eSF_BOTINFO)) ||
	(((flag == "ZPipe0") || (flag == "ZPipe")) && (usr->mxConn->mFeatures & eSF_ZLIB)) ||
	((flag == "ChatOnly") && (usr->mxConn->mFeatures & eSF_CHATONLY)) ||
	((flag == "MCTo") && (usr->mxConn->mFeatures & eSF_MCTO)) ||
	((flag == "UserCommand") && (usr->mxConn->mFeatures & eSF_USERCOMMAND)) ||
	((flag == "BotList") && (usr->mxConn->mFeatures & eSF_BOTLIST)) ||
	((flag == "HubTopic") && (usr->mxConn->mFeatures & eSF_HUBTOPIC)) ||
	((flag == "UserIP2") && (usr->mxConn->mFeatures & eSF_USERIP2)) ||
	((flag == "TTHSearch") && (usr->mxConn->mFeatures & eSF_TTHSEARCH)) ||
	((flag == "Feed") && (usr->mxConn->mFeatures & eSF_FEED)) ||
	((flag == "ClientID") && (usr->mxConn->mFeatures & eSF_CLIENTID)) ||
	((flag == "IN") && (usr->mxConn->mFeatures & eSF_IN)) ||
	((flag == "BanMsg") && (usr->mxConn->mFeatures & eSF_BANMSG)) ||
	((flag == "TLS") && (usr->mxConn->mFeatures & eSF_TLS)) ||
	((flag == "FailOver") && (usr->mxConn->mFeatures & eSF_FAILOVER)) ||
	((flag == "NickChange") && (usr->mxConn->mFeatures & eSF_NICKCHANGE)) ||
	((flag == "ClientNick") && (usr->mxConn->mFeatures & eSF_CLIENTNICK)) ||
	((flag == "FeaturedNetworks") && (usr->mxConn->mFeatures & eSF_FEATNET)) ||
	((flag == "ZLine") && (usr->mxConn->mFeatures & eSF_ZLINE)) ||
	((flag == "GetZBlock") && (usr->mxConn->mFeatures & eSF_GETZBLOCK)) ||
	((flag == "ACTM") && (usr->mxConn->mFeatures & eSF_ACTM)) ||
	((flag == "SaltPass") && (usr->mxConn->mFeatures & eSF_SALTPASS)) ||
	((flag == "NickRule") && (usr->mxConn->mFeatures & eSF_NICKRULE)) ||
	((flag == "HubURL") && (usr->mxConn->mFeatures & eSF_HUBURL)) ||
	((flag == "ExtJSON2") && (usr->mxConn->mFeatures & eSF_EXTJSON2))
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

	if(lua_gettop(L) == 6)
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
	string nick, op, data;

	if(lua_gettop(L) == 4)
	{
		if(!lua_isstring(L, 2))
		{
			luaerror(L, ERR_PARAM);
			return 2;
		}
		op = lua_tostring(L, 2);
		if(!lua_isstring(L, 3))
		{
			luaerror(L, ERR_PARAM);
			return 2;
		}
		nick = lua_tostring(L, 3);
		if(!lua_isstring(L, 4))
		{
			luaerror(L, ERR_PARAM);
			return 2;
		}
		data = lua_tostring(L, 4);
		if(!KickUser(op.c_str(), nick.c_str(), data.c_str()))
		{
			//lua_pushboolean(L, 0);
			luaerror(L, ERR_CALL);
			return 2;
		}
	}
	else
	{
		luaL_error(L, "Error calling VH:KickUser; expected 3 argument but got %d", lua_gettop(L) - 1);
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}
	lua_pushboolean(L, 1);
	return 1;
}

int _KickRedirUser(lua_State *L)
{
	if (lua_gettop(L) < 3) {
		luaL_error(L, "Error calling VH:KickRedirUser, expected 4 arguments but got %d.", lua_gettop(L) - 1);
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}

	cServerDC *serv = GetCurrentVerlihub();

	if (!serv) {
		luaerror(L, ERR_SERV);
		return 2;
	}

	if (!lua_isstring(L, 2) || !lua_isstring(L, 3) || !lua_isstring(L, 4) || !lua_isstring(L, 5)) {
		luaerror(L, ERR_PARAM);
		return 2;
	}

	string niop = lua_tostring(L, 2);
	string nius = lua_tostring(L, 3);
	string reas = lua_tostring(L, 4);
	string addr = lua_tostring(L, 5);
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
	serv->DCKickNick(NULL, oper, nius, reas, (eKI_CLOSE | eKI_WHY | eKI_PM | eKI_BAN)); // kick user
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
	cLuaInterpreter *li;
	int key = 0;
	int isize = cpiLua::me->Size();
	lua_newtable(L);
	int z = lua_gettop(L);

	for (int i = 0; i < isize; i++) {
		li = cpiLua::me->mLua[i];

		if (li) {
			for (unsigned int j = 0; j < li->botList.size(); j++) {
				if (li->botList[j] && li->mScriptName.size() && li->botList[j]->uNick && li->botList[j]->uMyINFO) {
					lua_pushnumber(L, ++key);
					lua_newtable(L);
					int k = lua_gettop(L);

					lua_pushliteral(L, "sScriptname");
					lua_pushstring(L, li->mScriptName.c_str());
					lua_rawset(L, k);
					lua_pushliteral(L, "sNick");
					lua_pushstring (L, li->botList[j]->uNick);
					lua_rawset(L, k);
					lua_pushliteral(L, "sMyINFO");
					lua_pushstring(L, li->botList[j]->uMyINFO);
					lua_rawset(L, k);
					lua_pushliteral(L, "iShare");
					lua_pushnumber(L, (int)li->botList[j]->uShare);
					lua_rawset(L, k);
					lua_pushliteral(L, "iClass");
					lua_pushnumber(L, (int)li->botList[j]->uClass);
					lua_rawset(L, k);

					lua_rawset(L, z);
				}
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

	string msg;
	serv->mP.Create_Hello(msg, robot->mNick); // send hello
	serv->mHelloUsers.SendToAll(msg, serv->mC.delayed_myinfo, true);
	serv->mP.Create_MyINFO(robot->mMyINFO, robot->mNick, desc, speed, email, share); // create myinfo
	robot->mMyINFO_basic = robot->mMyINFO;
	serv->mUserList.SendToAll(robot->mMyINFO, serv->mC.delayed_myinfo, true); // send myinfo
	serv->mInProgresUsers.SendToAll(robot->mMyINFO, serv->mC.delayed_myinfo, true);

	if (robot->mClass >= eUC_OPERATOR) { // send short oplist
		serv->mP.Create_OpList(msg, robot->mNick);
		serv->mUserList.SendToAll(msg, serv->mC.delayed_myinfo, true);
		serv->mInProgresUsers.SendToAll(msg, serv->mC.delayed_myinfo, true);
	}

	serv->mP.Create_BotList(msg, robot->mNick); // send short botlist
	serv->mUserList.SendToAllWithFeature(msg, eSF_BOTLIST, serv->mC.delayed_myinfo, true);
	serv->mInProgresUsers.SendToAllWithFeature(msg, eSF_BOTLIST, serv->mC.delayed_myinfo, true);

	li->addBot(nick.c_str(), robot->mMyINFO.c_str(), (int)ishare, (int)iclass); // add to lua bots
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

	if (robot->mClass != iclass) { // different class
		string msg;

		if ((robot->mClass < serv->mC.opchat_class) && (iclass >= serv->mC.opchat_class)) { // add to opchat list
			if (!serv->mOpchatList.ContainsNick(nick))
				serv->mOpchatList.Add(robot);
		} else if ((robot->mClass >= serv->mC.opchat_class) && (iclass < serv->mC.opchat_class)) { // remove from opchat list
			if (serv->mOpchatList.ContainsNick(nick))
				serv->mOpchatList.Remove(robot);
		}

		if ((robot->mClass < eUC_OPERATOR) && (iclass >= eUC_OPERATOR)) { // changing from user to op
			if (!serv->mOpList.ContainsNick(nick)) // add to oplist
				serv->mOpList.Add(robot);

			serv->mP.Create_OpList(msg, robot->mNick); // send short oplist to users
			serv->mUserList.SendToAll(msg, serv->mC.delayed_myinfo, true);
			serv->mInProgresUsers.SendToAll(msg, serv->mC.delayed_myinfo, true);
		} else if ((robot->mClass >= eUC_OPERATOR) && (iclass < eUC_OPERATOR)) { // changing from op to user
			if (serv->mOpList.ContainsNick(nick)) // remove from oplist
				serv->mOpList.Remove(robot);

			serv->mP.Create_Quit(msg, robot->mNick); // send quit to users
			serv->mUserList.SendToAll(msg, serv->mC.delayed_myinfo, true);
			serv->mInProgresUsers.SendToAll(msg, serv->mC.delayed_myinfo, true);
			serv->mP.Create_Hello(msg, robot->mNick); // send hello
			serv->mHelloUsers.SendToAll(msg, serv->mC.delayed_myinfo, true);
		}

		robot->mClass = (tUserCl)iclass; // set new class
	}

	robot->mMyINFO.clear(); // clear old and create new myinfo, must be sent after quit
	serv->mP.Create_MyINFO(robot->mMyINFO, robot->mNick, desc, speed, email, share);
	robot->mMyINFO_basic = robot->mMyINFO;
	serv->mUserList.SendToAll(robot->mMyINFO, serv->mC.delayed_myinfo, true); // send new myinfo
	serv->mInProgresUsers.SendToAll(robot->mMyINFO, serv->mC.delayed_myinfo, true);

	li->editBot(nick.c_str(), robot->mMyINFO.c_str(), (int)ishare, (int)iclass); // edit in lua bots
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
	if(lua_gettop(L) == 2) {
		cServerDC *server = GetCurrentVerlihub();
		if(server == NULL) {
			luaerror(L, ERR_SERV);
			return 2;
		}

		cpiLua *pi = (cpiLua *)server->mPluginManager.GetPlugin(LUA_PI_IDENTIFIER);
		if(pi == NULL) {
			luaerror(L, ERR_LUA);
			return 2;
		}

		if (!pi->mQuery) {
			luaerror(L, "mQuery is not ready");
			return 2;
		}

		if(!lua_isstring(L, 2)) {
		    luaerror(L, ERR_PARAM);
		    return 2;
		}
		pi->mQuery->Clear();
		pi->mQuery->OStream() << lua_tostring(L, 2);
		pi->mQuery->Query();
		int i = pi->mQuery->StoreResult();

		lua_pushboolean(L, 1);
		if(i > 0)
			lua_pushnumber(L, i);
		else
			lua_pushnumber(L, 0);
		return 2;
	} else {
		luaL_error(L, "Error calling VH:SQLQuery; expected 1 argument but got %d", lua_gettop(L) - 1);
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}
}

int _SQLFetch(lua_State *L)
{
	if(lua_gettop(L) == 2) {
		cServerDC *server = GetCurrentVerlihub();
		if(server == NULL) {
			luaerror(L, ERR_SERV);
			return 2;
		}

		cpiLua *pi = (cpiLua *)server->mPluginManager.GetPlugin(LUA_PI_IDENTIFIER);
		if(pi == NULL) {
			luaerror(L, ERR_LUA);
			return 2;
		}

		if (!pi->mQuery) {
			luaerror(L, "mQuery is not ready");
			return 2;
		}

		if(!lua_isnumber(L, 2)) {
			luaerror(L, ERR_PARAM);
			return 2;
		}

		int r = (int)lua_tonumber(L, 2);

		if(!pi->mQuery->GetResult()) {
			//lua_pushboolean(L, 0);
			luaerror(L, "No result");
			return 2;
		}

		pi->mQuery->DataSeek(r);

		MYSQL_ROW row;

		if(!(row = pi->mQuery->Row()))
		{
			//lua_pushboolean(L, 0);
			luaerror(L, "Error fetching row");
			return 2;
		}

		lua_pushboolean(L, 1);

		int j = 0;
		while(j < pi->mQuery->Cols()) {
			lua_pushstring(L, row[j]);
			j++;
		}
		return j + 1;
	} else {
		luaL_error(L, "Error calling VH:SQLFetch; expected 1 argument but got %d", lua_gettop(L) - 1);
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}
}

int _SQLFree(lua_State *L)
{
	if(lua_gettop(L) == 1) {
		cServerDC *server = GetCurrentVerlihub();
		if(server == NULL) {
			luaerror(L, ERR_SERV);
			return 2;
		}

		cpiLua *pi = (cpiLua *)server->mPluginManager.GetPlugin(LUA_PI_IDENTIFIER);
		if(pi == NULL)
		{
			luaerror(L, ERR_LUA);
			return 2;
		}

		if (!pi->mQuery) {
			luaerror(L, "mQuery is not ready");
			return 2;
		}

		pi->mQuery->Clear();
	} else {
		luaL_error(L, "Error calling VH:SQLFree; expected 0 argument but got %d", lua_gettop(L) - 1);
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}
	lua_pushboolean(L, 1);
	return 1;
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

	if (!user || !user->mxConn || !user->mxConn->mpUser) {
		luaerror(L, "User not found");
		return 2;
	}

	if (!serv->mChatUsers.ContainsNick(user->mxConn->mpUser->mNick)) {
		serv->mChatUsers.Add(user->mxConn->mpUser);
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

	if (!user || !user->mxConn || !user->mxConn->mpUser) {
		luaerror(L, "User not found");
		return 2;
	}

	if (serv->mChatUsers.ContainsNick(user->mxConn->mpUser->mNick)) {
		serv->mChatUsers.Remove(user->mxConn->mpUser);
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

	if (!user || !user->mxConn || !user->mxConn->mpUser) {
		luaerror(L, "User not found");
		return 2;
	}

	if (serv->mChatUsers.ContainsNick(user->mxConn->mpUser->mNick)) {
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
	const string pass = lua_tostring(L, 3);
	int uclass = (int)lua_tonumber(L, 4);

	if ((uclass < eUC_PINGER) || (uclass == eUC_NORMUSER) || ((uclass > eUC_ADMIN) && (uclass < eUC_MASTER)) || (uclass > eUC_MASTER)) { // validate class number
		luaerror(L, ERR_CLASS);
		return 2;
	}

	string op;

	if (args >= 4)
		op = lua_tostring(L, 5);

	if (AddRegUser(nick.c_str(), uclass, pass.c_str(), op.c_str()))
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

	if (DelRegUser(nick.c_str()))
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
	cDCProto::Create_HubName(omsg, serv->mC.hub_name, topic);
	serv->SendToAll(omsg, eUC_NORMUSER, eUC_MASTER);
	SetConfig(li->mConfigName.c_str(), "hub_topic", topic.c_str());
	lua_pushboolean(L, 1);
	lua_pushnil(L);
	return 2;
}

int _ScriptCommand(lua_State *L)
{
	if (lua_gettop(L) < 3) {
		luaL_error(L, "Error calling VH:ScriptCommand, expected 2 arguments but got %d.", lua_gettop(L) - 1);
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}

	if (!lua_isstring(L, 2) || !lua_isstring(L, 3)) {
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
	string plug("lua");

	if (!ScriptCommand(&cmd, &data, &plug, &li->mScriptName)) {
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
