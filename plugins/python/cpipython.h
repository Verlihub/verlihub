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

#ifndef CPIPYTHON_H
#define CPIPYTHON_H

#include "cpythoninterpreter.h"
#include "cconsole.h"
#include "src/cconndc.h"
#include "src/cvhplugin.h"
#include "src/cserverdc.h"
#include "src/cuser.h"
#include <iostream>
#include <vector>
#include <dlfcn.h>

#ifndef _WIN32
#define __int64 long long
#endif

#define PYTHON_PI_IDENTIFIER "Python"

#define w_ret0    cpiPython::lib_pack("l", (long)0)
#define w_ret1    cpiPython::lib_pack("l", (long)1)
#define w_retnone cpiPython::lib_pack("")

// logging levels:
// 0 - plugin critical errors, interpreter errors, bad call arguments
// 1 - callback / hook logging - only their status
// 2 - all function parameters and return values are printed
// 3 - debugging info is printed
#define log(...)                                  { printf( __VA_ARGS__ ); fflush(stdout); }
#define log1(...) { if (cpiPython::log_level > 0) { printf( __VA_ARGS__ ); fflush(stdout); }; }
#define log2(...) { if (cpiPython::log_level > 1) { printf( __VA_ARGS__ ); fflush(stdout); }; }
#define log3(...) { if (cpiPython::log_level > 2) { printf( __VA_ARGS__ ); fflush(stdout); }; }
#define log4(...) { if (cpiPython::log_level > 3) { printf( __VA_ARGS__ ); fflush(stdout); }; }
#define log5(...) { if (cpiPython::log_level > 4) { printf( __VA_ARGS__ ); fflush(stdout); }; }

#define dprintf(...) { printf("%s:%u: "__FILE__, __LINE__); printf( __VA_ARGS__ ); fflush(stdout); }

using std::vector;
namespace nVerliHub {
namespace nPythonPlugin {

class cpiPython : public nPlugin::cVHPlugin
{
public:
	cpiPython();
	virtual ~cpiPython();
	bool RegisterAll();
	virtual void OnLoad(nSocket::cServerDC *);
	virtual bool OnNewConn(nSocket::cConnDC *);
	virtual bool OnCloseConn(nSocket::cConnDC *);
	virtual bool OnCloseConnEx(nSocket::cConnDC *);
	virtual bool OnParsedMsgChat(nSocket::cConnDC *, nProtocol::cMessageDC *);
	virtual bool OnParsedMsgPM(nSocket::cConnDC *, nProtocol::cMessageDC *);
	virtual bool OnParsedMsgMCTo(nSocket::cConnDC *, nProtocol::cMessageDC *);
	virtual bool OnParsedMsgSearch(nSocket::cConnDC *, nProtocol::cMessageDC *);
	virtual bool OnParsedMsgSR(nSocket::cConnDC *, nProtocol::cMessageDC *);
	virtual bool OnParsedMsgMyINFO(nSocket::cConnDC *, nProtocol::cMessageDC *);
	virtual bool OnFirstMyINFO(nSocket::cConnDC *, nProtocol::cMessageDC *);
	virtual bool OnParsedMsgValidateNick(nSocket::cConnDC *, nProtocol::cMessageDC *);
	virtual bool OnParsedMsgAny(nSocket::cConnDC *, nProtocol::cMessageDC *);
	virtual bool OnParsedMsgAnyEx(nSocket::cConnDC *, nProtocol::cMessageDC *);
	virtual bool OnOpChatMessage(std::string *, std::string *);
	virtual bool OnPublicBotMessage(std::string *, std::string *, int, int);
	virtual bool OnUnLoad(long);
	virtual bool OnCtmToHub(nSocket::cConnDC *, std::string *);
	virtual bool OnParsedMsgSupports(nSocket::cConnDC *, nProtocol::cMessageDC *, string *);
	virtual bool OnParsedMsgMyHubURL(nSocket::cConnDC *, nProtocol::cMessageDC *);
	virtual bool OnParsedMsgExtJSON(nSocket::cConnDC *, nProtocol::cMessageDC *);
	virtual bool OnParsedMsgBotINFO(nSocket::cConnDC *, nProtocol::cMessageDC *);
	virtual bool OnParsedMsgVersion(nSocket::cConnDC *, nProtocol::cMessageDC *);
	virtual bool OnParsedMsgMyPass(nSocket::cConnDC *, nProtocol::cMessageDC *);
	virtual bool OnParsedMsgConnectToMe(nSocket::cConnDC *, nProtocol::cMessageDC *);
	virtual bool OnParsedMsgRevConnectToMe(nSocket::cConnDC *, nProtocol::cMessageDC *);
	virtual bool OnUnknownMsg(nSocket::cConnDC *, nProtocol::cMessageDC *);
	virtual bool OnOperatorCommand(nSocket::cConnDC *, std::string *);
	virtual bool OnOperatorKicks(cUser *, cUser *, std::string *);
	virtual bool OnOperatorDrops(cUser *, cUser *, std::string *);
	virtual bool OnValidateTag(nSocket::cConnDC *, cDCTag *);
	virtual bool OnUserCommand(nSocket::cConnDC *, std::string *);
	virtual bool OnHubCommand(nSocket::cConnDC *conn, std::string *command, int is_op_cmd, int in_pm);
	virtual bool OnScriptCommand(std::string *cmd, std::string *data, std::string *plug, std::string *script);
	virtual bool OnScriptQuery(std::string *cmd, std::string *data, std::string *recipient, std::string *sender, ScriptResponses *resp);
	virtual bool OnUserLogin(cUser *);
	virtual bool OnUserLogout(cUser *);
	virtual bool OnTimer(__int64);
	virtual bool OnNewReg(cUser* User, std::string mNick, int mClass);
	virtual bool OnNewBan(cUser *, nTables::cBan *);
	virtual bool OnSetConfig(cUser *user, std::string *conf, std::string *var, std::string *val_new, std::string *val_old, int val_type);

	// Common code for OnParsedMsgMyINFO and OnFirstMyINFO:
	bool OnParsedMsgMyINFO__(nSocket::cConnDC *, nProtocol::cMessageDC *, int, const char *);

	bool AutoLoad();
	static const char *GetName(const char *path);
	int SplitMyINFO(const char *msg, char **nick, char **desc, char **tag, 
		char **speed, char **mail, char **size);
	w_Targs *SQL(int id, w_Targs *args);
	void LogLevel(int);
	bool IsNumber(const char *s);
	int char2int(char c);
	cPythonInterpreter *GetInterpreter(int id);
	bool CallAll(int func, w_Targs *args);
	unsigned int Size() { return mPython.size(); }

	void Empty()
	{
		tvPythonInterpreter::iterator it;
		for (it = mPython.begin(); it != mPython.end(); ++it) {
			if (*it != NULL) delete *it;
			*it = NULL;
		}
		mPython.clear();
	}

	void AddData(cPythonInterpreter *ip, size_t position = (size_t)-1)
	{
		if (position >= mPython.size())
			mPython.push_back(ip);
		else
			mPython.insert(mPython.begin() + position, ip);
	}

	bool RemoveByName(const string &name)
	{
		tvPythonInterpreter::iterator it;
		cPythonInterpreter *li;
		for (it = mPython.begin(); it != mPython.end(); ++it) {
			li = *it;
			if (li == NULL || li->mScriptName.compare(name))
				continue;
			delete li;
			mPython.erase(it);
			return true;
		}
		log("PY: ERROR: Failed to remove interpreter for %s\n", name.c_str());
		return false;
	}

	cPythonInterpreter *operator[](unsigned int i)
	{
		if (i > Size()) return NULL;
		return mPython[i];
	}

	cConsole mConsole;
	nMySQL::cQuery *mQuery;
	typedef vector<cPythonInterpreter *> tvPythonInterpreter;
	tvPythonInterpreter mPython;
	string mScriptDir;
	bool online;

	static void        *lib_handle;
	static w_TBegin     lib_begin;
	static w_TEnd       lib_end;
	static w_TReserveID lib_reserveid;
	static w_TLoad      lib_load;
	static w_TUnload    lib_unload;
	static w_THasHook   lib_hashook;
	static w_TCallHook  lib_callhook;
	static w_THookName  lib_hookname;
	static w_Tpack      lib_pack;
	static w_Tunpack    lib_unpack;
	static w_TLogLevel  lib_loglevel;
	static w_Tpackprint lib_packprint;

	static string botname;
	static string opchatname;
	static int log_level;
	static nSocket::cServerDC *server;
	static cpiPython *me;
};

};  // namespace nPythonPlugin

inline bool EndsWithPipe(const char *data)
{
	return data && data[0] != 0 && data[strlen(data) - 1] == '|';
}

inline const char* PipeIfMissing(const char *data)
{
	return EndsWithPipe(data) ? "" : "|";
}

extern "C" w_Targs *_SendToOpChat      (int id, w_Targs *args);
extern "C" w_Targs *_SendToActive      (int id, w_Targs *args);
extern "C" w_Targs *_SendToPassive     (int id, w_Targs *args);
extern "C" w_Targs *_SendToActiveClass (int id, w_Targs *args);
extern "C" w_Targs *_SendToPassiveClass(int id, w_Targs *args);
extern "C" w_Targs *_SendDataToUser    (int id, w_Targs *args);
extern "C" w_Targs *_SendDataToAll     (int id, w_Targs *args);
extern "C" w_Targs *_SendPMToAll       (int id, w_Targs *args);
extern "C" w_Targs *_mc                (int id, w_Targs *args);
extern "C" w_Targs *_usermc            (int id, w_Targs *args);
extern "C" w_Targs *_classmc           (int id, w_Targs *args);
extern "C" w_Targs *_pm                (int id, w_Targs *args);
extern "C" w_Targs *_CloseConnection   (int id, w_Targs *args);
extern "C" w_Targs *_GetMyINFO         (int id, w_Targs *args);
extern "C" w_Targs *_SetMyINFO         (int id, w_Targs *args);
extern "C" w_Targs *_GetUserClass      (int id, w_Targs *args);
extern "C" w_Targs *_GetNickList       (int id, w_Targs *args);
extern "C" w_Targs *_GetOpList         (int id, w_Targs *args);
extern "C" w_Targs *_GetBotList        (int id, w_Targs *args);
extern "C" w_Targs *_GetUserHost       (int id, w_Targs *args);
extern "C" w_Targs *_GetUserIP         (int id, w_Targs *args);
extern "C" w_Targs *_GetUserHubURL     (int id, w_Targs *args);
extern "C" w_Targs *_GetUserExtJSON    (int id, w_Targs *args);
extern "C" w_Targs *_GetUserCC         (int id, w_Targs *args);

#ifdef HAVE_LIBGEOIP
extern "C" w_Targs *_GetIPCC           (int id, w_Targs *args);
extern "C" w_Targs *_GetIPCN           (int id, w_Targs *args);
extern "C" w_Targs *_GetGeoIP          (int id, w_Targs *args);
#endif

extern "C" w_Targs *_AddRegUser        (int id, w_Targs *args);
extern "C" w_Targs *_DelRegUser        (int id, w_Targs *args);
extern "C" w_Targs *_Ban               (int id, w_Targs *args);
extern "C" w_Targs *_KickUser          (int id, w_Targs *args);
extern "C" w_Targs *_DelNickTempBan    (int id, w_Targs *args);
extern "C" w_Targs *_DelIPTempBan      (int id, w_Targs *args);
extern "C" w_Targs *_ParseCommand      (int id, w_Targs *args);
extern "C" w_Targs *_ScriptCommand     (int id, w_Targs *args);
extern "C" w_Targs *_ScriptQuery       (int id, w_Targs *args);
extern "C" w_Targs *_SetConfig         (int id, w_Targs *args);
extern "C" w_Targs *_GetConfig         (int id, w_Targs *args);
extern "C" w_Targs *_IsRobotNickBad    (int id, w_Targs *args);
extern "C" w_Targs *_AddRobot          (int id, w_Targs *args);
extern "C" w_Targs *_DelRobot          (int id, w_Targs *args);
extern "C" w_Targs *_SQL               (int id, w_Targs *args);
extern "C" w_Targs *_GetServFreq       (int id, w_Targs *args);
extern "C" w_Targs *_GetUsersCount     (int id, w_Targs *args);
extern "C" w_Targs *_GetTotalShareSize (int id, w_Targs *args);
extern "C" w_Targs *_UserRestrictions  (int id, w_Targs *args);
extern "C" w_Targs *_Topic             (int id, w_Targs *args);
extern "C" w_Targs *_name_and_version  (int id, w_Targs *args);
extern "C" w_Targs *_StopHub           (int id, w_Targs *args);

};  // namespace nVerliHub

#endif
