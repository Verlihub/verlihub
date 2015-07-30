/*
	Copyright (C) 2003-2005 Daniel Muller, dan at verliba dot cz
	Copyright (C) 2006-2015 Verlihub Team, info at verlihub dot net

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
		data = (char *)lua_tostring(L, 2);
		if(!lua_isstring(L, 3))
		{
			luaerror(L, ERR_PARAM);
			return 2;
		}
		nick = (char *)lua_tostring(L, 3);
		if(!SendDataToUser((char *)data.c_str(), (char *)nick.c_str()))
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
		data = (char *)lua_tostring(L, 2);
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
		if(!SendToClass((char*)data.c_str(), min_class, max_class)) {
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
		data = (char *)lua_tostring(L, 2);
		if(!SendToAll((char*)data.c_str())) {
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
	return 1;
}

int _SendToActive(lua_State *L)
{
	if (lua_gettop(L) == 2) {
		if (!lua_isstring(L, 2)) {
			luaerror(L, ERR_PARAM);
			return 2;
		}

		string data = (char*)lua_tostring(L, 2);

		if (!SendToActive((char*)data.c_str())) {
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

		string data = (char *)lua_tostring(L, 2);

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

		if (!SendToActiveClass((char*)data.c_str(), min_class, max_class)) {
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

		string data = (char*)lua_tostring(L, 2);

		if (!SendToPassive((char*)data.c_str())) {
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

		string data = (char *)lua_tostring(L, 2);

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

		if (!SendToPassiveClass((char*)data.c_str(), min_class, max_class)) {
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
		data = (char *) lua_tostring(L, 2);
		if(!lua_isstring(L, 3)) {
			luaerror(L, ERR_PARAM);
			return 2;
		}
		from = (char *) lua_tostring(L, 3);

		if(lua_isnumber(L, 4)) {
			min_class = (int) lua_tonumber(L, 4);
		}

		if(lua_isnumber(L, 5)) {
			max_class = (int) lua_tonumber(L, 5);
		}

		/*string start, end;
		cServerDC *server = GetCurrentVerlihub();
		if(server == NULL) {
			luaerror(L, "Error getting server");
			return 2;
		}
		server->mP.Create_PMForBroadcast(start, end, from, from, data);
		server->SendToAllWithNick(start,end, min_class, max_class);*/
		SendPMToAll((char *) data.c_str(),(char *)  from.c_str(), min_class, max_class);
	} else {
		luaL_error(L, "Error calling VH:SendPMToAll; expected at least 4 arguments but got %d", lua_gettop(L) - 1);
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}
	lua_pushboolean(L, 1);
	return 1;
}

int _SendToOpChat(lua_State *L)
{
	if (lua_gettop(L) < 2) {
		luaL_error(L, "Error calling VH:SendToOpChat, expected 1 argument but got %d.", lua_gettop(L) - 1);
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}

	if (!lua_isstring(L, 2)) {
		luaerror(L, ERR_PARAM);
		return 2;
	}

	string data = (char*)lua_tostring(L, 2);

	if (data.empty()) {
		luaerror(L, ERR_CALL);
		return 2;
	}

	if (!SendToOpChat((char*)data.c_str())) {
		luaerror(L, ERR_CALL);
		return 2;
	}

	lua_pushboolean(L, 1);
	lua_pushnil(L);
	return 2;
}

int _Disconnect(lua_State *L)
{
	string nick;

	if(lua_gettop(L) == 2) {
		if(!lua_isstring(L, 2)) {
			luaerror(L, ERR_PARAM);
			return 2;
		}
		nick = (char *) lua_tostring(L, 2);
		if(!CloseConnection((char*)nick.c_str())) {
			luaerror(L, ERR_CALL);
			return 2;
		}
	} else {
		luaL_error(L, "Error calling VH:Disconnect; expected 1 argument but got %d", lua_gettop(L) - 1);
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}
	lua_pushboolean(L, 1);
	return 1;
}

int _DisconnectNice(lua_State *L)
{
	if (lua_gettop(L) == 2) {
		if (!lua_isstring(L, 2)) {
			luaerror(L, ERR_PARAM);
			return 2;
		}

		string nick = (char *)lua_tostring(L, 2);

		if (!CloseConnectionNice((char*)nick.c_str())) {
			luaerror(L, ERR_CALL);
			return 2;
		}
	} else {
		luaL_error(L, "Error calling VH:DisconnectNice, expected 1 argument but got %d.", lua_gettop(L) - 1);
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}

	lua_pushboolean(L, 1);
	return 1;
}

int _StopHub(lua_State *L)
{
	if (lua_gettop(L) == 2) {
		if (!lua_isnumber(L, 2)) {
			luaerror(L, ERR_PARAM);
			return 2;
		}

		int code = (int)lua_tonumber(L, 2);

		if (!StopHub(code)) {
			luaerror(L, ERR_CALL);
			return 2;
		}
	} else {
		luaL_error(L, "Error calling VH:StopHub, expected 1 argument but got %d.", lua_gettop(L) - 1);
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}

	lua_pushboolean(L, 1);
	return 1;
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
		nick = (char *) lua_tostring(L, 2);
		myinfo = GetMyINFO( (char*) nick.c_str());
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

	string nick = (char*)lua_tostring(L, 2);
	cUser *usr = serv->mUserList.GetUserByNick(nick);

	if (!usr || !usr->mxConn) {
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}

	lua_pushboolean(L, 1);
	lua_pushstring(L, (char*)usr->mxConn->mCC.c_str());
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

	string nick = (char*)lua_tostring(L, 2);
	cUser *usr = serv->mUserList.GetUserByNick(nick);

	if (!usr || !usr->mxConn) {
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}

	lua_pushboolean(L, 1);
	lua_pushstring(L, (char*)usr->mxConn->mCN.c_str());
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

	string nick = (char*)lua_tostring(L, 2);
	cUser *usr = serv->mUserList.GetUserByNick(nick);

	if (!usr || !usr->mxConn) {
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}

	lua_pushboolean(L, 1);
	lua_pushstring(L, (char*)usr->mxConn->mCity.c_str());
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

	string ip = (char*)lua_tostring(L, 2);
	char *cc = GetIPCC((char*)ip.c_str());

	if (cc) {
		lua_pushboolean(L, 1);
		lua_pushstring(L, (char*)cc);
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

	string ip = (char*)lua_tostring(L, 2);
	char *cn = GetIPCN((char*)ip.c_str());

	if (cn) {
		lua_pushboolean(L, 1);
		lua_pushstring(L, (char*)cn);
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
			db = (char*)lua_tostring(L, 3);
	}

	string ip = (char*)lua_tostring(L, 2);
	string city;

	if (serv->sGeoIP.GetCity(city, ip, db)) {
		lua_pushboolean(L, 1);
		lua_pushstring(L, (char*)city.c_str());
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

	string nick = (char*)lua_tostring(L, 2);
	string db;

	if (lua_gettop(L) > 2) {
		if (!lua_isstring(L, 3)) {
			luaerror(L, ERR_PARAM);
			return 2;
		} else
			db = (char*)lua_tostring(L, 3);
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
		if (geo_host.empty()) { lua_pushnil(L); } else { lua_pushstring(L, (char*)geo_host.c_str()); }
		lua_rawset(L, x);

		lua_pushliteral(L, "range_low");
		if (geo_ran_lo.empty()) { lua_pushnil(L); } else { lua_pushstring(L, (char*)geo_ran_lo.c_str()); }
		lua_rawset(L, x);

		lua_pushliteral(L, "range_high");
		if (geo_ran_hi.empty()) { lua_pushnil(L); } else { lua_pushstring(L, (char*)geo_ran_hi.c_str()); }
		lua_rawset(L, x);

		lua_pushliteral(L, "country_code");
		if (geo_cc.empty()) { lua_pushnil(L); } else { lua_pushstring(L, (char*)geo_cc.c_str()); }
		lua_rawset(L, x);

		lua_pushliteral(L, "country_code_xxx");
		if (geo_ccc.empty()) { lua_pushnil(L); } else { lua_pushstring(L, (char*)geo_ccc.c_str()); }
		lua_rawset(L, x);

		lua_pushliteral(L, "country");
		if (geo_cn.empty()) { lua_pushnil(L); } else { lua_pushstring(L, (char*)geo_cn.c_str()); }
		lua_rawset(L, x);

		lua_pushliteral(L, "region_code");
		if (geo_reg_code.empty()) { lua_pushnil(L); } else { lua_pushstring(L, (char*)geo_reg_code.c_str()); }
		lua_rawset(L, x);

		lua_pushliteral(L, "region");
		if (geo_reg_name.empty()) { lua_pushnil(L); } else { lua_pushstring(L, (char*)geo_reg_name.c_str()); }
		lua_rawset(L, x);

		lua_pushliteral(L, "time_zone");
		if (geo_tz.empty()) { lua_pushnil(L); } else { lua_pushstring(L, (char*)geo_tz.c_str()); }
		lua_rawset(L, x);

		lua_pushliteral(L, "continent_code");
		if (geo_cont.empty()) { lua_pushnil(L); } else { lua_pushstring(L, (char*)geo_cont.c_str()); }
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
		if (cont.empty()) { lua_pushnil(L); } else { lua_pushstring(L, (char*)cont.c_str()); }
		lua_rawset(L, x);

		lua_pushliteral(L, "city");
		if (geo_city.empty()) { lua_pushnil(L); } else { lua_pushstring(L, (char*)geo_city.c_str()); }
		lua_rawset(L, x);

		lua_pushliteral(L, "postal_code");
		if (geo_post.empty()) { lua_pushnil(L); } else { lua_pushstring(L, (char*)geo_post.c_str()); }
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

	string host = (char*)lua_tostring(L, 2);
	string db;

	if (lua_gettop(L) > 2) {
		if (!lua_isstring(L, 3)) {
			luaerror(L, ERR_PARAM);
			return 2;
		} else
			db = (char*)lua_tostring(L, 3);
	}

	string geo_host, geo_ran_lo, geo_ran_hi, geo_cc, geo_ccc, geo_cn, geo_reg_code, geo_reg_name, geo_tz, geo_cont, geo_city, geo_post;
	float geo_lat, geo_lon;
	int geo_met, geo_area;

	if (serv->sGeoIP.GetGeoIP(geo_host, geo_ran_lo, geo_ran_hi, geo_cc, geo_ccc, geo_cn, geo_reg_code, geo_reg_name, geo_tz, geo_cont, geo_city, geo_post, geo_lat, geo_lon, geo_met, geo_area, host.c_str(), db)) {
		lua_pushboolean(L, 1);
		lua_newtable(L);
		int x = lua_gettop(L);

		lua_pushliteral(L, "host");
		if (geo_host.empty()) { lua_pushnil(L); } else { lua_pushstring(L, (char*)geo_host.c_str()); }
		lua_rawset(L, x);

		lua_pushliteral(L, "range_low");
		if (geo_ran_lo.empty()) { lua_pushnil(L); } else { lua_pushstring(L, (char*)geo_ran_lo.c_str()); }
		lua_rawset(L, x);

		lua_pushliteral(L, "range_high");
		if (geo_ran_hi.empty()) { lua_pushnil(L); } else { lua_pushstring(L, (char*)geo_ran_hi.c_str()); }
		lua_rawset(L, x);

		lua_pushliteral(L, "country_code");
		if (geo_cc.empty()) { lua_pushnil(L); } else { lua_pushstring(L, (char*)geo_cc.c_str()); }
		lua_rawset(L, x);

		lua_pushliteral(L, "country_code_xxx");
		if (geo_ccc.empty()) { lua_pushnil(L); } else { lua_pushstring(L, (char*)geo_ccc.c_str()); }
		lua_rawset(L, x);

		lua_pushliteral(L, "country");
		if (geo_cn.empty()) { lua_pushnil(L); } else { lua_pushstring(L, (char*)geo_cn.c_str()); }
		lua_rawset(L, x);

		lua_pushliteral(L, "region_code");
		if (geo_reg_code.empty()) { lua_pushnil(L); } else { lua_pushstring(L, (char*)geo_reg_code.c_str()); }
		lua_rawset(L, x);

		lua_pushliteral(L, "region");
		if (geo_reg_name.empty()) { lua_pushnil(L); } else { lua_pushstring(L, (char*)geo_reg_name.c_str()); }
		lua_rawset(L, x);

		lua_pushliteral(L, "time_zone");
		if (geo_tz.empty()) { lua_pushnil(L); } else { lua_pushstring(L, (char*)geo_tz.c_str()); }
		lua_rawset(L, x);

		lua_pushliteral(L, "continent_code");
		if (geo_cont.empty()) { lua_pushnil(L); } else { lua_pushstring(L, (char*)geo_cont.c_str()); }
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
		if (cont.empty()) { lua_pushnil(L); } else { lua_pushstring(L, (char*)cont.c_str()); }
		lua_rawset(L, x);

		lua_pushliteral(L, "city");
		if (geo_city.empty()) { lua_pushnil(L); } else { lua_pushstring(L, (char*)geo_city.c_str()); }
		lua_rawset(L, x);

		lua_pushliteral(L, "postal_code");
		if (geo_post.empty()) { lua_pushnil(L); } else { lua_pushstring(L, (char*)geo_post.c_str()); }
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
	char *nicklist;
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
	char *oplist;
	int result = 1;
	if(lua_gettop(L) == 1) {
		cServerDC *server = GetCurrentVerlihub();
		if(server) {
			oplist = (char*) server->mOpList.GetNickList().c_str();
			if(strlen(oplist) < 1) result = 0;
			lua_pushboolean(L, result);
			lua_pushstring(L, oplist);
			return 2;
		} else {
				luaerror(L, "Error getting server");
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
	char *botlist;
	int result = 1;
	if(lua_gettop(L) == 1) {
		cServerDC *server = GetCurrentVerlihub();
		if(server) {
			botlist = (char*) server->mRobotList.GetNickList().c_str();
			if(strlen(botlist) < 1) result = 0;
			lua_pushboolean(L, result);
			lua_pushstring(L, botlist);
			return 2;
		} else {
			luaerror(L, "Error getting server");
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
		nick = (char *)lua_tostring(L, 2);
		uclass = GetUserClass((char*)nick.c_str());
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
	string nick, host;

	if(lua_gettop(L) == 2) {
		if(!lua_isstring(L, 2)) {
			luaerror(L, ERR_PARAM);
			return 2;
		}
		nick = (char *)lua_tostring(L, 2);
		host = GetUserHost((char*)nick.c_str());
		lua_pushboolean(L, 1);
		lua_pushstring(L, (char *)host.c_str());
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
	string nick, ip;

	if(lua_gettop(L) == 2) {
		if(!lua_isstring(L, 2)) {
			luaerror(L, ERR_PARAM);
			return 2;
		}
		nick = (char *)lua_tostring(L, 2);
		ip = GetUserIP((char*)nick.c_str());
		lua_pushboolean(L, 1);
		lua_pushstring(L, (char *)ip.c_str());
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
			luaerror(L, "Error getting server");
			return 2;
		}
		if(!lua_isstring(L, 2)) {
			luaerror(L, ERR_PARAM);
			return 2;
		}
		string nick = (char *) lua_tostring(L, 2);
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

	string nick = (char*)lua_tostring(L, 2);
	cUser *usr = serv->mUserList.GetUserByNick(nick);

	if ((usr == NULL) || (usr->mxConn == NULL)) {
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}

	lua_pushboolean(L, 1);
	lua_pushstring(L, (char*)usr->mxConn->mVersion.c_str());
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

	string nick = (char*)lua_tostring(L, 2);
	cUser *usr = serv->mUserList.GetUserByNick(nick);

	if ((usr == NULL) || (usr->mxConn == NULL)) {
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}

	lua_pushboolean(L, 1);
	lua_pushstring(L, (char*)usr->mxConn->mSupportsText.c_str());
	return 2; // tips: return number equals number of pushed values including last null value
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

	string nick = (char *)lua_tostring(L, 2);
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
	((flag == "IPv4") && (usr->mxConn->mFeatures & eSF_IPV4)) ||
	((flag == "IP64") && (usr->mxConn->mFeatures & eSF_IP64)) ||
	((flag == "FailOver") && (usr->mxConn->mFeatures & eSF_FAILOVER)) ||
	((flag == "NickChange") && (usr->mxConn->mFeatures & eSF_NICKCHANGE)) ||
	((flag == "ClientNick") && (usr->mxConn->mFeatures & eSF_CLIENTNICK)) ||
	((flag == "FeaturedNetworks") && (usr->mxConn->mFeatures & eSF_FEATNET)) ||
	((flag == "ZLine") && (usr->mxConn->mFeatures & eSF_ZLINE)) ||
	((flag == "GetZBlock") && (usr->mxConn->mFeatures & eSF_GETZBLOCK)) ||
	((flag == "ACTM") && (usr->mxConn->mFeatures & eSF_ACTM)) ||
	((flag == "SaltPass") && (usr->mxConn->mFeatures & eSF_SALTPASS))
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
		nick = (char *) lua_tostring(L, 2);
		if(!lua_isstring(L, 3)) {
			luaerror(L, ERR_PARAM);
			return 2;
		}
		op = (char *) lua_tostring(L, 3);
		if(!lua_isstring(L, 4)) {
			luaerror(L, ERR_PARAM);
			return 2;
		}
		reason = (char *) lua_tostring(L, 4);
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
		if(!Ban((char *) nick.c_str(), op, reason, howlong, bantype)) {
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
		op = (char *)lua_tostring(L, 2);
		if(!lua_isstring(L, 3))
		{
			luaerror(L, ERR_PARAM);
			return 2;
		}
		nick = (char *)lua_tostring(L, 3);
		if(!lua_isstring(L, 4))
		{
			luaerror(L, ERR_PARAM);
			return 2;
		}
		data = (char *)lua_tostring(L, 4);
		if(!KickUser((char *)op.c_str(), (char *)nick.c_str(), (char *)data.c_str()))
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

	if (serv == NULL) {
		luaerror(L, ERR_SERV);
		return 2;
	}

	if (!lua_isstring(L, 2) || !lua_isstring(L, 3) || !lua_isstring(L, 4) || !lua_isstring(L, 5)) {
		luaerror(L, ERR_PARAM);
		return 2;
	}

	string niop = (char *)lua_tostring(L, 2);
	string nius = (char *)lua_tostring(L, 3);
	string reas = (char *)lua_tostring(L, 4);
	string addr = (char *)lua_tostring(L, 5);

	cUser *oper = serv->mUserList.GetUserByNick(niop);

	if (oper == NULL) {
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}

	cUser *user = serv->mUserList.GetUserByNick(nius);

	if ((user == NULL) || (user->mxConn == NULL)) {
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}

	user->mxConn->mCloseRedirect = addr; // set redirect
	serv->DCKickNick(NULL, oper, nius, reas, eKCK_Drop | eKCK_Reason | eKCK_PM | eKCK_TBAN); // kick user

	lua_pushboolean(L, 1);
	lua_pushnil(L);
	return 2;
}

int _SetConfig(lua_State *L)
{
	string config_name, var, val;

	if(lua_gettop(L) == 4) {
		if(!lua_isstring(L, 2)) {
			luaerror(L, ERR_PARAM);
			return 2;
		}
		config_name = (char *)lua_tostring(L, 2);
		if(!lua_isstring(L, 3)) {
			luaerror(L, ERR_PARAM);
			return 2;
		}
		var = (char *)lua_tostring(L, 3);
		if(!lua_isstring(L, 4)) {
			luaerror(L, ERR_PARAM);
			return 2;
		}
		val = (char *)lua_tostring(L, 4);
		if(!SetConfig((char *)config_name.c_str(), (char *)var.c_str(), (char *)val.c_str())) {
			//lua_pushboolean(L, 0);
			luaerror(L, ERR_CALL);
			return 2;
		}
	} else {
		luaL_error(L, "Error calling VH:SetConfig; expected 3 argument but got %d", lua_gettop(L) - 1);
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}
	lua_pushboolean(L, 1);
	return 1;
}

int _GetConfig(lua_State *L)
{
	char *val = new char[64];
	string config_name, var;
	int size;

	if(lua_gettop(L) == 3) {
		if(!lua_isstring(L, 2)) {
			luaerror(L, ERR_PARAM);
			return 2;
		}
		config_name = (char *)lua_tostring(L, 2);
		if(!lua_isstring(L, 3)) {
			luaerror(L, ERR_PARAM);
			return 2;
		}
		var = (char *)lua_tostring(L, 3);
		size = GetConfig((char *)config_name.c_str(), (char *)var.c_str(), val, 64);
		if(size < 0) {
			luaerror(L, "Error calling GetConfig API");
			return 2;
		}
		if(size >= 63) {
			delete [] val;
			val = new char[size+1];
			GetConfig((char *)config_name.c_str(), (char *)var.c_str(), val, size+1);
		}
		lua_pushboolean(L, 1);
		lua_pushstring(L, val);
		delete [] val;
		val = 0;
		return 2;
	} else {
		luaL_error(L, "Error calling VH:GetConfig; expected 2 argument but got %d", lua_gettop(L) - 1);
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}
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
					lua_pushstring(L, (char*)li->mScriptName.c_str());
					lua_rawset(L, k);
					lua_pushliteral(L, "sNick");
					lua_pushstring (L, (char*)li->botList[j]->uNick);
					lua_rawset(L, k);
					lua_pushliteral(L, "sMyINFO");
					lua_pushstring(L, (char*)li->botList[j]->uMyINFO);
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

	const string &nick = (char*)lua_tostring(L, 2);
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
		string temp = (char*)lua_tostring(L, 3);

		if (temp.empty())
			iclass = 0;
		else
			iclass = StrToInt(temp);
	} else
		iclass = (int)lua_tonumber(L, 3);

	if ((iclass < -1) || (iclass > 10) || ((iclass > 5) && (iclass < 10))) {
		luaerror(L, "Invalid bot class specified");
		return 2;
	}

	cPluginRobot *robot = plug->NewRobot(nick, iclass);

	if (!robot) {
		luaerror(L, "Error registering bot");
		return 2;
	}

	string desc = (char*)lua_tostring(L, 4);
	string speed = (char*)lua_tostring(L, 5);
	string email = (char*)lua_tostring(L, 6);
	string share;
	int ishare;

	if (lua_isstring(L, 7)) {
		share = (char*)lua_tostring(L, 7);

		if (share.empty()) {
			share = "0";
			ishare = 0;
		} else
			ishare = StrToInt(share);
	} else {
		ishare = (int)lua_tonumber(L, 7);
		share = IntToStr(ishare);
	}

	serv->mP.Create_MyINFO(robot->mMyINFO, robot->mNick, desc, speed, email, share); // create myinfo
	robot->mMyINFO_basic = robot->mMyINFO;

	static string msg;
	msg.erase();
	serv->mP.Create_Hello(msg, robot->mNick); // send hello
	serv->mHelloUsers.SendToAll(msg, serv->mC.delayed_myinfo, true);
	serv->mUserList.SendToAll(robot->mMyINFO, serv->mC.delayed_myinfo, true); // send myinfo

	if (robot->mClass >= eUC_OPERATOR) { // send short oplist
		msg.erase();
		serv->mP.Create_OpList(msg, robot->mNick);
		serv->mUserList.SendToAll(msg, serv->mC.delayed_myinfo, true);
	}

	msg.erase();
	serv->mP.Create_BotList(msg, robot->mNick); // send short botlist
	serv->mUserList.SendToAllWithFeature(msg, eSF_BOTLIST, serv->mC.delayed_myinfo, true);

	li->addBot((char*)nick.c_str(), (char*)robot->mMyINFO.c_str(), (int)ishare, (int)iclass); // add to lua bots
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

	const string &nick = (char*)lua_tostring(L, 2);
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
		string temp = (char*)lua_tostring(L, 3);

		if (temp.empty())
			iclass = 0;
		else
			iclass = StrToInt(temp);
	} else
		iclass = (int)lua_tonumber(L, 3);

	if ((iclass < -1) || (iclass > 10) || ((iclass > 5) && (iclass < 10))) {
		luaerror(L, "Invalid bot class specified");
		return 2;
	}

	string desc = (char*)lua_tostring(L, 4);
	string speed = (char*)lua_tostring(L, 5);
	string email = (char*)lua_tostring(L, 6);
	string share;
	int ishare;

	if (lua_isstring(L, 7)) {
		share = (char*)lua_tostring(L, 7);

		if (share.empty()) {
			share = "0";
			ishare = 0;
		} else
			ishare = StrToInt(share);
	} else {
		ishare = (int)lua_tonumber(L, 7);
		share = IntToStr(ishare);
	}

	robot->mMyINFO = ""; // clear old and create new myinfo
	serv->mP.Create_MyINFO(robot->mMyINFO, robot->mNick, desc, speed, email, share);
	robot->mMyINFO_basic = robot->mMyINFO;
	serv->mUserList.SendToAll(robot->mMyINFO, serv->mC.delayed_myinfo, true); // send new myinfo

	if (robot->mClass != iclass) { // different class
		if ((robot->mClass < eUC_OPERATOR) && (iclass >= eUC_OPERATOR)) { // changing to op
			if (!serv->mOpList.ContainsNick(nick)) // add to oplist
				serv->mOpList.Add(robot);

			if (!serv->mOpchatList.ContainsNick(nick)) // add to opchat list
				serv->mOpchatList.Add(robot);

			static string msg;
			msg.erase();
			serv->mP.Create_OpList(msg, robot->mNick); // send short oplist
			serv->mUserList.SendToAll(msg, serv->mC.delayed_myinfo, true);
		} else if ((robot->mClass >= eUC_OPERATOR) && (iclass < eUC_OPERATOR)) { // changing from op
			if (serv->mOpList.ContainsNick(nick)) // remove from oplist
				serv->mOpList.Remove(robot);

			if (serv->mOpchatList.ContainsNick(nick)) // remove from opchat list
				serv->mOpchatList.Remove(robot);

			// robot will remain in users oplist until they reconnect
			// solution would be to send quit and hello + myinfo with botlist again, todo
		}

		robot->mClass = (tUserCl)iclass; // set new class
	}

	li->editBot((char*)nick.c_str(), (char*)robot->mMyINFO.c_str(), (int)ishare, (int)iclass); // edit in lua bots
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

	const string &nick = (char*)lua_tostring(L, 2);
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

	li->delBot((char*)nick.c_str()); // delete from lua bots
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

	const string &nick = (char*)lua_tostring(L, 2);
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
			luaerror(L, "Error getting server");
			return 2;
		}

		cpiLua *pi = (cpiLua *)server->mPluginManager.GetPlugin(LUA_PI_IDENTIFIER);
		if(pi == NULL) {
			luaerror(L, "Error getting LUA plugin");
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
		pi->mQuery->OStream() << (char *) lua_tostring(L, 2);
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
			luaerror(L, "Error getting server");
			return 2;
		}

		cpiLua *pi = (cpiLua *)server->mPluginManager.GetPlugin(LUA_PI_IDENTIFIER);
		if(pi == NULL) {
			luaerror(L, "Error getting LUA plugin");
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
			lua_pushstring(L, (char *)row[j]);
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
			luaerror(L, "Error getting server");
			return 2;
		}

		cpiLua *pi = (cpiLua *)server->mPluginManager.GetPlugin(LUA_PI_IDENTIFIER);
		if(pi == NULL)
		{
			luaerror(L, "Error getting LUA plugin");
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
		luaerror(L, "Error getting server");
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

int _GetHubIp(lua_State *L)
{
	cServerDC *server = GetCurrentVerlihub();
	if(server == NULL) {
		luaerror(L, "Error getting server");
		return 2;
	}
	lua_pushboolean(L, 1);
	lua_pushstring(L, (char *) server->mAddr.c_str());
	return 2;
}

int _GetHubSecAlias(lua_State *L)
{
	cServerDC *server = GetCurrentVerlihub();
	if(server == NULL) {
		luaerror(L, "Error getting server");
		return 2;
	}
	lua_pushboolean(L, 1);
	lua_pushstring(L, (char *) server->mC.hub_security.c_str());
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

	string nick = (char*)lua_tostring(L, 2);
	string msg = (char*)lua_tostring(L, 3);
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

	string nick = (char*)lua_tostring(L, 2);
	string cmd = (char*)lua_tostring(L, 3);
	int pm = (int)lua_tonumber(L, 4);

	if (!ParseCommand((char*)nick.c_str(), (char*)cmd.c_str(), pm)) {
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

	string nick = (char*)lua_tostring(L, 2);
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

	string nick = (char*)lua_tostring(L, 2);
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

	string nick = (char*)lua_tostring(L, 2);
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
		luaL_error(L, "Error calling VH:AddRegUser, expected 3 or 4 arguments but got %d.", args);
		lua_pushboolean(L, 0);
		lua_pushnil(L);
		return 2;
	}

	if (!lua_isstring(L, 2) || !lua_isstring(L, 3) || !lua_isnumber(L, 4) || ((args >= 4) && !lua_isstring(L, 5))) {
		luaerror(L, ERR_PARAM);
		return 2;
	}

	const string nick = (char*)lua_tostring(L, 2);
	const string pass = (char*)lua_tostring(L, 3);
	int uclass = (int)lua_tonumber(L, 4);
	string op;

	if (args >= 4)
		op = (char*)lua_tostring(L, 5);

	if (AddRegUser((char*)nick.c_str(), uclass, (char*)pass.c_str(), (char*)op.c_str()))
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

	const string nick = (char*)lua_tostring(L, 2);

	if (DelRegUser((char*)nick.c_str()))
		lua_pushboolean(L, 1);
	else
		lua_pushboolean(L, 0);

	lua_pushnil(L);
	return 2;
}

int _GetTopic(lua_State *L)
{
	cServerDC *server = GetCurrentVerlihub();
	if(server == NULL) {
		luaerror(L, "Error getting server");
		return 2;
	}
	lua_pushstring(L, server->mC.hub_topic.c_str());
	return 1;
}

int _SetTopic(lua_State *L)
{
	cServerDC *server = GetCurrentVerlihub();
	if(server == NULL) {
		luaerror(L, "Error getting server");
		return 2;
	}
	string topic;
	if(lua_gettop(L) == 2) {
		if(!lua_isstring(L, 2)) {
			luaerror(L, ERR_PARAM);
			return 2;
		}
		topic = (char *) lua_tostring(L, 2);
	}

	string message;
	cDCProto::Create_HubName(message, server->mC.hub_name, topic);
	server->SendToAll(message, eUC_NORMUSER, eUC_MASTER);
	SetConfig((char*)"config", (char*)"hub_topic", (char*)topic.c_str());
	lua_pushboolean(L, 1);
	return 1;
}

int _ScriptCommand(lua_State *L)
{
	if (lua_gettop(L) < 3) {
		luaL_error(L, "Error calling VH:ScriptCommand, expected 2 arguments but got %d.", lua_gettop(L) - 1);
		lua_pushboolean(L, 0);
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

	string cmd = (char*)lua_tostring(L, 2);
	string data = (char*)lua_tostring(L, 3);
	string plug("lua");

	if (!ScriptCommand(&cmd, &data, &plug, &li->mScriptName)) {
		luaerror(L, ERR_CALL);
		return 2;
	}

	lua_pushboolean(L, 1);
	lua_pushnil(L);
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
