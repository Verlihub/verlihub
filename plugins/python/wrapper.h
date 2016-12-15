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

#ifndef WRAPPER_H
#define WRAPPER_H

extern "C" {
#include <Python.h>
#include <stdio.h>
#include <stdlib.h>
}

#include <string>
#include <sstream>
#include <vector>

// user rights
#define w_UR_CHAT 0x0001
#define w_UR_PM 0x0002
#define w_UR_SEARCH 0x0004
#define w_UR_CTM 0x0008

#define freee(x) if (x) free(x)

// a string the user would not write
#define W_NOSTRING = "SG083tcs0ODEgbns623OFfew"

// function positions in the hook table
enum {
	W_OnNewConn,
	W_OnCloseConn,
	W_OnCloseConnEx,
	W_OnParsedMsgChat,
	W_OnParsedMsgPM,
	W_OnParsedMsgMCTo,
	W_OnParsedMsgSearch,
	W_OnParsedMsgSR,
	W_OnParsedMsgMyINFO,
	W_OnFirstMyINFO,
	W_OnParsedMsgValidateNick,
	W_OnParsedMsgAny,
	W_OnParsedMsgAnyEx,
	W_OnOpChatMessage,
	W_OnPublicBotMessage,
	W_OnUnLoad,
	W_OnCtmToHub,
	W_OnParsedMsgSupports,
	W_OnParsedMsgMyHubURL,
	W_OnParsedMsgExtJSON,
	W_OnParsedMsgBotINFO,
	W_OnParsedMsgVersion,
	W_OnParsedMsgMyPass,
	W_OnParsedMsgConnectToMe,
	W_OnParsedMsgRevConnectToMe,
	W_OnUnknownMsg,
	W_OnOperatorCommand,
	W_OnOperatorKicks,
	W_OnOperatorDrops,
	W_OnOperatorDropsWithReason,
	W_OnValidateTag,
	W_OnUserCommand,
	W_OnHubCommand,
	W_OnScriptCommand,
	W_OnScriptQuery,
	W_OnUserLogin,
	W_OnUserLogout,
	W_OnTimer,
	W_OnNewReg,
	W_OnNewBan,
	W_OnSetConfig,
};

// MAX_HOOKS must be more than the number of elements in above enum
const int W_MAX_HOOKS = 100;

// function positions in the callback table
enum {
	W_SendToOpChat,
	W_SendToActive,
	W_SendToPassive,
	W_SendToActiveClass,
	W_SendToPassiveClass,
	W_SendDataToUser,
	W_SendDataToAll,
	W_SendPMToAll,
	W_CloseConnection,
	W_GetMyINFO,
	W_SetMyINFO,
	W_GetUserClass,
	W_GetUserHost,
	W_GetUserIP,
	W_GetUserHubURL,
	W_GetUserExtJSON,
	W_GetUserCC,
#ifdef HAVE_LIBGEOIP
	W_GetIPCC,
	W_GetIPCN,
	W_GetGeoIP,
#endif
	W_GetNickList,
	W_GetOpList,
	W_GetBotList,
	W_AddRegUser,
	W_DelRegUser,
	W_Ban,
	W_KickUser,
	W_DelNickTempBan,
	W_DelIPTempBan,
	W_ParseCommand,
	W_ScriptCommand,
	W_ScriptQuery,
	W_SetConfig,
	W_GetConfig,
	W_IsRobotNickBad,
	W_AddRobot,
	W_DelRobot,
	W_SQL,
	W_SQLQuery,
	W_SQLFetch,
	W_SQLFree,
	W_GetServFreq,
	W_GetUsersCount,
	W_GetTotalShareSize,
	W_UserRestrictions,
	W_Topic,
	W_mc,
	W_usermc,
	W_classmc,
	W_pm,
	W_name_and_version,
	W_StopHub,
};

// MAX_CALLBACKS must be more than the number of elements in above enum
const int W_MAX_CALLBACKS = 100;

// possible return values, as follows: nothing, long, double, const char*, void*, const char**
enum { w_ret_none, w_ret_int, w_ret_float, w_ret_char, w_ret_void, w_ret_tab };

// the maximum number of values that can be returned inside w_Treturn
#define W_MAX_RETVALS 10

// values returned by IsRobotNickBad:
enum {
	eBOT_OK,
	eBOT_EXISTS,
	eBOT_WITHOUT_NICK,
	eBOT_BAD_CHARS,
	eBOT_RESERVED_NICK,
	eBOT_API_ERROR,
};

typedef struct {
	char type;
	union {
		long l;
		char *s;
		double d;
		void *p;
	};
} w_Telement;

typedef struct {
	const char *format;
	w_Telement  args[];
} w_Targs;

typedef w_Targs *(*w_Tcallback)(int, w_Targs *);

typedef struct {
	int            id;
	PyThreadState *state;
	char          *path;
	char          *name;
	w_Tcallback   *callbacks;
	char          *hooks;
	char          *botname;
	char          *opchatname;
	bool           use_old_ontimer;
	char          *config_name;
} w_TScript;

// the following functions are all you need to use in the plugin that loads this wrapper

extern "C" {
int w_Begin(w_Tcallback *callbacks);
int w_End();
int w_ReserveID();
int w_Load(w_Targs *args);
int w_Unload(int id);
int w_HasHook(int id, int hook);

// w_CallHook's non-empty/non-zero return means further processing by other plugins or the hub
w_Targs *w_CallHook(int id, int num, w_Targs *params);
PyObject *w_GetHook(int hook);
const char *w_HookName(int hook);
const char *w_CallName(int callback);

w_Targs *w_pack(const char *format, ...);
int w_unpack(w_Targs *a, const char *format, ...);
void w_LogLevel(int level);
const char *w_packprint(w_Targs *a);
}

// typedefs for pointers to functions to be used in plugin via dlsym()
typedef int         (*w_TBegin)(w_Tcallback *);
typedef int         (*w_TEnd)(void);
typedef int         (*w_TReserveID)(void);
typedef int         (*w_TLoad)(w_Targs *);
typedef int         (*w_TUnload)(int);
typedef int         (*w_THasHook)(int, int);
typedef w_Targs    *(*w_TCallHook)(int, int, w_Targs *);
typedef const char *(*w_THookName)(int);
typedef const char *(*w_TCallName)(int callback);
typedef w_Targs    *(*w_Tpack)(const char *, ...);
typedef int         (*w_Tunpack)(w_Targs *, const char *, ...);
typedef void        (*w_TLogLevel)(int);
typedef const char *(*w_Tpackprint)(w_Targs *);

#endif
