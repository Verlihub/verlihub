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

#include "cpipython.h"
#include "src/stringutils.h"
#include "src/cbanlist.h"
#include "src/cserverdc.h"
#include "src/tchashlistmap.h"
#include "src/ctime.h"
#include <dirent.h>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>

namespace nVerliHub {
using namespace nUtils;
using namespace nTables;
using namespace nEnums;
using namespace nSocket;
using namespace nMySQL;
namespace nPythonPlugin {

// initializing static members
void         *cpiPython::lib_handle    = NULL;
w_TBegin      cpiPython::lib_begin     = NULL;
w_TEnd        cpiPython::lib_end       = NULL;
w_TReserveID  cpiPython::lib_reserveid = NULL;
w_TLoad       cpiPython::lib_load      = NULL;
w_TUnload     cpiPython::lib_unload    = NULL;
w_THasHook    cpiPython::lib_hashook   = NULL;
w_TCallHook   cpiPython::lib_callhook  = NULL;
w_THookName   cpiPython::lib_hookname  = NULL;
w_Tpack       cpiPython::lib_pack      = NULL;
w_Tunpack     cpiPython::lib_unpack    = NULL;
w_TLogLevel   cpiPython::lib_loglevel  = NULL;
w_Tpackprint  cpiPython::lib_packprint = NULL;
string        cpiPython::botname       = "";
string        cpiPython::opchatname    = "";
int           cpiPython::log_level     = 1;
cServerDC    *cpiPython::server        = NULL;
cpiPython    *cpiPython::me            = NULL;



cpiPython::cpiPython() : mConsole(this), mQuery(NULL)
{
	mName = PYTHON_PI_IDENTIFIER;
	mVersion = PYTHON_PI_VERSION;
	me = this;
	online = false;
}

cpiPython::~cpiPython()
{
	ostringstream o;
	o << log_level;
	SetConfig("pi_python", "log_level", o.str().c_str());
	this->Empty();
	if (lib_end) (*lib_end)();
	if (lib_handle) dlclose(lib_handle);
	log1("PY: cpiPython::destructor   Plugin ready to be unloaded\n");
	delete mQuery;
	return;
}

void cpiPython::OnLoad(cServerDC *server)
{
	log4("PY: cpiPython::OnLoad\n");
	cVHPlugin::OnLoad(server);
	mQuery = new cQuery(server->mMySQL);
	mScriptDir = mServer->mConfigBaseDir + "/scripts/";
	this->server = server;
	botname = server->mC.hub_security;
	opchatname = server->mC.opchat_name;

	log4("PY: cpiPython::OnLoad   dlopen...\n");
	if (!lib_handle)
		lib_handle = dlopen("@CMAKE_INSTALL_PREFIX@/@PLUGINDIR@/libvh_python_wrapper.so",
			RTLD_LAZY | RTLD_GLOBAL);
	// RTLD_GLOBAL exports all symbols from libvh_python_wrapper.so
	// without RTLD_GLOBAL the lib will fail to import other python modules
	// because they won't see any symbols from the linked libpython2.5
	if (!lib_handle) {
		log("PY: cpiPython::OnLoad   Error during dlopen(): %s\n", dlerror());
		return;
	}

	*(void **)(&lib_begin)     = dlsym(lib_handle, "w_Begin");
	*(void **)(&lib_end)       = dlsym(lib_handle, "w_End");
	*(void **)(&lib_reserveid) = dlsym(lib_handle, "w_ReserveID");
	*(void **)(&lib_load)      = dlsym(lib_handle, "w_Load");
	*(void **)(&lib_unload)    = dlsym(lib_handle, "w_Unload");
	*(void **)(&lib_hashook)   = dlsym(lib_handle, "w_HasHook");
	*(void **)(&lib_callhook)  = dlsym(lib_handle, "w_CallHook");
	*(void **)(&lib_hookname)  = dlsym(lib_handle, "w_HookName");
	*(void **)(&lib_pack)      = dlsym(lib_handle, "w_pack");
	*(void **)(&lib_unpack)    = dlsym(lib_handle, "w_unpack");
	*(void **)(&lib_loglevel)  = dlsym(lib_handle, "w_LogLevel");
	*(void **)(&lib_packprint) = dlsym(lib_handle, "w_packprint");

	if (!lib_begin || !lib_end || !lib_reserveid || !lib_load || !lib_unload || !lib_hashook
	|| !lib_callhook || !lib_hookname || !lib_pack || !lib_unpack || !lib_loglevel || !lib_packprint) {
		log("PY: cpiPython::OnLoad   Error locating vh_python_wrapper function symbols: %s\n", dlerror());
		return;
	}

	w_Tcallback *callbacklist = (w_Tcallback *)calloc(W_MAX_CALLBACKS, sizeof(void *));

	callbacklist[W_SendToOpChat]       = &_SendToOpChat;
	callbacklist[W_SendToActive]       = &_SendToActive;
	callbacklist[W_SendToPassive]      = &_SendToPassive;
	callbacklist[W_SendToActiveClass]  = &_SendToActiveClass;
	callbacklist[W_SendToPassiveClass] = &_SendToPassiveClass;
	callbacklist[W_SendDataToUser]     = &_SendDataToUser;
	callbacklist[W_SendDataToAll]      = &_SendDataToAll;
	callbacklist[W_SendPMToAll]        = &_SendPMToAll;
	callbacklist[W_mc]                 = &_mc;
	callbacklist[W_usermc]             = &_usermc;
	callbacklist[W_classmc]            = &_classmc;
	callbacklist[W_pm]                 = &_pm;
	callbacklist[W_CloseConnection]    = &_CloseConnection;
	callbacklist[W_GetMyINFO]          = &_GetMyINFO;
	callbacklist[W_SetMyINFO]          = &_SetMyINFO;
	callbacklist[W_GetUserClass]       = &_GetUserClass;
	callbacklist[W_GetNickList]        = &_GetNickList;
	callbacklist[W_GetOpList]          = &_GetOpList;
	callbacklist[W_GetBotList]         = &_GetBotList;
	callbacklist[W_GetUserHost]        = &_GetUserHost;
	callbacklist[W_GetUserIP]          = &_GetUserIP;
	callbacklist[W_GetUserHubURL]      = &_GetUserHubURL;
	callbacklist[W_GetUserExtJSON]     = &_GetUserExtJSON;
	callbacklist[W_GetUserCC]          = &_GetUserCC;
#ifdef HAVE_LIBGEOIP
	callbacklist[W_GetIPCC]            = &_GetIPCC;
	callbacklist[W_GetIPCN]            = &_GetIPCN;
	callbacklist[W_GetGeoIP]           = &_GetGeoIP;
#endif
	callbacklist[W_AddRegUser]         = &_AddRegUser;
	callbacklist[W_DelRegUser]         = &_DelRegUser;
	callbacklist[W_Ban]                = &_Ban;
	callbacklist[W_KickUser]           = &_KickUser;
	callbacklist[W_DelNickTempBan]     = &_DelNickTempBan;
	callbacklist[W_DelIPTempBan]       = &_DelIPTempBan;
	callbacklist[W_ParseCommand]       = &_ParseCommand;
	callbacklist[W_ScriptCommand]      = &_ScriptCommand;
	callbacklist[W_ScriptQuery]        = &_ScriptQuery;
	callbacklist[W_SetConfig]          = &_SetConfig;
	callbacklist[W_GetConfig]          = &_GetConfig;
	callbacklist[W_IsRobotNickBad]     = &_IsRobotNickBad;
	callbacklist[W_AddRobot]           = &_AddRobot;
	callbacklist[W_DelRobot]           = &_DelRobot;
	callbacklist[W_SQL]                = &_SQL;
	callbacklist[W_GetServFreq]        = &_GetServFreq;
	callbacklist[W_GetUsersCount]      = &_GetUsersCount;
	callbacklist[W_GetTotalShareSize]  = &_GetTotalShareSize;
	callbacklist[W_UserRestrictions]   = &_UserRestrictions;
	callbacklist[W_Topic]              = &_Topic;
	callbacklist[W_name_and_version]   = &_name_and_version;
	callbacklist[W_StopHub]            = &_StopHub;

	ostringstream o;
	o << log_level;
	char *level = GetConfig("pi_python", "log_level", o.str().c_str());
	if (level && strlen(level) > 0) log_level = char2int(level[0]);
	freee(level);

	if (!lib_begin(callbacklist)) {
		log("PY: cpiPython::OnLoad   Initiating vh_python_wrapper failed!\n");
		return;
	}
	online = true;
	lib_loglevel(log_level);

	AutoLoad();
}

bool cpiPython::RegisterAll()
{
	RegisterCallBack("VH_OnNewConn");
	RegisterCallBack("VH_OnCloseConn");
	RegisterCallBack("VH_OnCloseConnEx");
	RegisterCallBack("VH_OnParsedMsgChat");
	RegisterCallBack("VH_OnParsedMsgPM");
	RegisterCallBack("VH_OnParsedMsgMCTo");
	RegisterCallBack("VH_OnParsedMsgSearch");
	RegisterCallBack("VH_OnParsedMsgConnectToMe");
	RegisterCallBack("VH_OnParsedMsgRevConnectToMe");
	RegisterCallBack("VH_OnParsedMsgSR");
	RegisterCallBack("VH_OnParsedMsgMyINFO");
	RegisterCallBack("VH_OnFirstMyINFO");
	RegisterCallBack("VH_OnParsedMsgValidateNick");
	RegisterCallBack("VH_OnParsedMsgAny");
	RegisterCallBack("VH_OnParsedMsgAnyEx");
	RegisterCallBack("VH_OnOpChatMessage");
	RegisterCallBack("VH_OnPublicBotMessage");
	RegisterCallBack("VH_OnUnLoad");
	RegisterCallBack("VH_OnCtmToHub");
	RegisterCallBack("VH_OnParsedMsgSupports");
	RegisterCallBack("VH_OnParsedMsgMyHubURL");
	RegisterCallBack("VH_OnParsedMsgExtJSON");
	RegisterCallBack("VH_OnParsedMsgBotINFO");
	RegisterCallBack("VH_OnParsedMsgVersion");
	RegisterCallBack("VH_OnParsedMsgMyPass");
	RegisterCallBack("VH_OnUnknownMsg");
	RegisterCallBack("VH_OnOperatorCommand");
	RegisterCallBack("VH_OnOperatorKicks");
	RegisterCallBack("VH_OnOperatorDrops");
	RegisterCallBack("VH_OnValidateTag");
	RegisterCallBack("VH_OnUserCommand");
	RegisterCallBack("VH_OnHubCommand");
	RegisterCallBack("VH_OnScriptCommand");
	RegisterCallBack("VH_OnScriptQuery");
	RegisterCallBack("VH_OnUserLogin");
	RegisterCallBack("VH_OnUserLogout");
	RegisterCallBack("VH_OnTimer");
	RegisterCallBack("VH_OnNewReg");
	RegisterCallBack("VH_OnNewBan");
	RegisterCallBack("VH_OnSetConfig");
	return true;
}

bool cpiPython::AutoLoad()
{
	if ((log_level < 1) && Log(0))
		LogStream() << "Opening directory: " << mScriptDir << endl;
	log1("PY: Autoload, loading scripts from directory: %s\n", mScriptDir.c_str());

	DIR *dir = opendir(mScriptDir.c_str());
	if (!dir) {
		if ((log_level < 1) && Log(1))
			LogStream() << "Error opening directory" << endl;
		log1("PY: Autoload, filed to open directory\n");
		return false;
	}
	string filename, pathname;
	struct dirent *dent = NULL;
	vector<string> filenames;

	while (NULL != (dent = readdir(dir))) {
		filename = dent->d_name;
		if ((filename.size() > 3) && (StrCompare(filename, filename.size() - 3, 3, ".py") == 0))
			filenames.push_back(filename);
	}
	closedir(dir);
	sort(filenames.begin(), filenames.end());

	for (size_t i = 0; i < filenames.size(); i++) {
		const string &filename = filenames[i];
		pathname = mScriptDir + filename;
		cPythonInterpreter *ip = new cPythonInterpreter(pathname);
		if (!ip) continue;
		AddData(ip);
		if (ip->Init()) {
			if ((log_level < 1) && Log(1))
				LogStream() << "Success loading and parsing Python script: " << filename << endl;
			log1("PY: Autoload, success loading script: %s [%d]\n", filename.c_str(), ip->id);
		} else {
			if ((log_level < 1) && Log(1))
				LogStream() << "Failed loading or parsing Python script: " << filename << endl;
			log1("PY: Autoload, failed loading script: %s\n", filename.c_str());
			RemoveByName(pathname);
		}
	}
	return true;
}

const char *cpiPython::GetName(const char *path)
{
	if (!path || !strlen(path)) return NULL;
	for (int i = strlen(path) - 1; i >= 0; i--)
		if (path[i] == '/' || path[i] == '\\')
			return &path[i + 1];
	return path;
}

// MyINFO example:
// $MyINFO $ALL <nick> <interest>$ $<speed\x01>$<e-mail>$<sharesize>$
// $MyINFO $ALL nick <++ V:0.668,M:P,H:39/0/0,S:1>$ $DSL\x01$$74894830123$
// at this point the tag has already been validated by the hub so we do only the simplest tests here
int cpiPython::SplitMyINFO(const char *msg, char **nick, char **desc, 
	char **tag, char **speed, char **mail, char **size)
{
	const char *begin = "$MyINFO $ALL ";
	int dollars[5] = { -1, -1, -1, -1, -1 };
	int spacepos = 0;  // position of space after nick
	int validtag = 0, tagstart = 0, tagend = 0;
	int len = strlen(msg);
	int start = strlen(begin);
	
	if (len < 21) return 0;
	if (strncmp(msg, begin, start)) return 0;
	
	for (int pos = start, dollar = 0; pos < len && dollar < 5; pos++) {
		switch (msg[pos]) {
			case '$': dollars[dollar] = pos; dollar++; break;
			case ' ': if (!spacepos and !dollar) spacepos = pos; break;
			case '<': if (!dollar) tagstart = pos; break;
			case '>': if (!dollar) tagend = pos; break;
		}
	}
	if (dollars[4] != len - 1 || !spacepos) return 0;
	if (tagstart && tagend && msg[tagend + 1] == '$') validtag = 1;

	string s = msg;
	string _nick  = s.substr(start, spacepos - start);
	string _desc  = (validtag) ? s.substr(spacepos + 1, tagstart - spacepos - 1)
	                           : s.substr(spacepos + 1, dollars[0] - spacepos - 1);
	string _tag   = (validtag) ? s.substr(tagstart, dollars[0] - tagstart) : "";
	string _speed = s.substr(dollars[1] + 1, dollars[2] - dollars[1] - 1);
	string _mail  = s.substr(dollars[2] + 1, dollars[3] - dollars[2] - 1);
	string _size  = s.substr(dollars[3] + 1, dollars[4] - dollars[3] - 1);

	(*nick)  = strdup(_nick.c_str());
	(*desc)  = strdup(_desc.c_str());
	(*tag)   = strdup(_tag.c_str());
	(*speed) = strdup(_speed.c_str());
	(*mail)  = strdup(_mail.c_str());
	(*size)  = strdup(_size.c_str());

	log5("PY: SplitMyINFO: [%s] \n"
		"    dollars:%d,%d,%d,%d,%d / tag start:%d,end:%d / space:%d\n"
		"    nick:%s/desc:%s/tag:%s/speed:%s/mail:%s/size:%s\n",
		s.c_str(), dollars[0], dollars[1], dollars[2], dollars[3], dollars[4], 
		tagstart, tagend, spacepos, *nick, *desc, *tag, *speed, *mail, *size);

	return 1;
}

w_Targs *cpiPython::SQL(int id, w_Targs *args)  // (char *query)
{
	const char *query;
	string q;
	long limit;
	if (!lib_begin || !lib_pack || !lib_unpack || !lib_packprint || !mQuery) return NULL;
	if (!lib_unpack(args, "sl", &query, &limit)) return NULL;
	if (!query) return NULL;
	if (limit < 1) limit = 100;
	log4("PY: SQL   query: %s\n", query);
	q = string() + query;
	mQuery->OStream() << q;
	if (mQuery->Query() < 0) {
		mQuery->Clear();
		return lib_pack("lllp", (long)0, (long)0, (long)0, (void *)NULL);
	}  // error
	int rows = mQuery->StoreResult();
	if (limit < rows) rows = limit;
	if (rows < 1) {
		mQuery->Clear();
		return lib_pack("lllp", (long)1, (long)0, (long)0, (void *)NULL);
	}
	int cols = mQuery->Cols();
	char **res = (char **)calloc(cols * rows, sizeof(char *));
	if (!res) {
		log1("PY: SQL   malloc failed\n");
		mQuery->Clear();
		return lib_pack("lllp", (long)0, (long)0, (long)0, (void *)NULL);
	}
	for (int r = 0; r < rows; r++) {
		mQuery->DataSeek(r);
		MYSQL_ROW row;
		row = mQuery->Row();
		if (!row) {
			log1("PY: SQL   failed to fetch row: %d\n", r);
			mQuery->Clear();
			free(res);
			return lib_pack("lllp", (long)0, (long)0, (long)0, (void *)NULL);
		}
		for (int i = 0; i < cols; i++)
			res[(r * cols) + i] = strdup((row[i]) ? row[i] : "NULL");
	}
	mQuery->Clear();
	return lib_pack("lllp", (long)1, (long)rows, (long)cols, (void *)res);
}

void cpiPython::LogLevel(int level)
{
	int old = log_level;
	log_level = level;
	ostringstream o;
	o << log_level;
	SetConfig("pi_python", "log_level", o.str().c_str());
	log("PY: log_level changed: %d --> %d\n", old, (int)log_level);
	if (lib_loglevel) lib_loglevel(log_level);
}

bool cpiPython::IsNumber(const char *s)
{
	if (!s || !strlen(s)) return false;

	for (unsigned int i = 0; i < strlen(s); i++) {
		if (!isdigit(s[i])) return false;
	}
	return true;
}

int cpiPython::char2int(char c)
{
	switch (c) {
		case '0': return 0;
		case '1': return 1;
		case '2': return 2;
		case '3': return 3;
		case '4': return 4;
		case '5': return 5;
		case '6': return 6;
		case '7': return 7;
		case '8': return 8;
		case '9': return 9;
		default: return -1;
	}
}

cPythonInterpreter *cpiPython::GetInterpreter(int id)
{
	tvPythonInterpreter::iterator it;
	for (it = mPython.begin(); it != mPython.end(); ++it)
		if ((*it)->id == id) return (*it);
	return NULL;
}

// the default handler: returns true unless the calback returns 0
bool cpiPython::CallAll(int func, w_Targs *args)
{
	if (!online) return true;
	w_Targs *result;
	bool ret = true;
	long num;
	if (func != W_OnTimer) {
		log2("PY: CallAll %s: parameters %s\n", lib_hookname(func), lib_packprint(args))
	} else {
		log4("PY: CallAll %s\n", lib_hookname(func));
	}
	if (Size()) {
		tvPythonInterpreter::iterator it;
		for (it = mPython.begin(); it != mPython.end(); ++it) {
			result = (*it)->CallFunction(func, args);
			if (!result) {
				// callback doesn't exist or a failure
				if (func != W_OnTimer) log4("PY: CallAll %s: returned NULL\n", lib_hookname(func));
				continue;
			}
			if (lib_unpack(result, "l", &num)) {
				// default return value is 1L, which means: further processing
				if (func != W_OnTimer) log3("PY: CallAll %s: returned int:%ld\n", lib_hookname(func), num);
				if (!num) ret = false;  // 0L means no more processing outside this plugin
			} else {
				// something unknown was returned... we will let the hub call other plugins
				log1("PY: CallAll %s: unexpected return value %s\n", lib_hookname(func), lib_packprint(result));
			}
			free(result);
		}
	}
	free(args);  // WARNING: args is freed, do not try to access it after calling CallAll!
	return ret;
}

bool cpiPython::OnNewConn(cConnDC *conn)
{
	if (conn != NULL) {
		w_Targs *args = lib_pack("s", conn->AddrIP().c_str());
		return CallAll(W_OnNewConn, args);
	}
	return true;
}

bool cpiPython::OnCloseConn(cConnDC *conn)
{
	if (conn != NULL) {
		w_Targs *args = lib_pack("s", conn->AddrIP().c_str());
		return CallAll(W_OnCloseConn, args);
	}
	return true;
}

bool cpiPython::OnCloseConnEx(cConnDC *conn)
{
	if (conn != NULL) {
		const char *ip = conn->AddrIP().c_str();
		long reason = conn->mCloseReason;
		const char *nick = (conn->mpUser ? conn->mpUser->mNick.c_str() : "");
		w_Targs *args = lib_pack("sls", ip, reason, nick);
		return CallAll(W_OnCloseConnEx, args);
	}
	return true;
}

bool cpiPython::OnParsedMsgChat(cConnDC *conn, cMessageDC *msg)
{
	if (!online) return true;
	if ((conn != NULL) && (conn->mpUser != NULL) && (msg != NULL)) {
		int func = W_OnParsedMsgChat;
		w_Targs *args = lib_pack("ss", conn->mpUser->mNick.c_str(), msg->ChunkString(eCH_CH_MSG).c_str());
		log2("PY: Call %s: parameters %s\n", lib_hookname(func), lib_packprint(args));
		bool ret = true;
		w_Targs *result;
		long num;
		const char *nick = NULL;
		const char *message = NULL;

		if (Size()) {
			tvPythonInterpreter::iterator it;
			for (it = mPython.begin(); it != mPython.end(); ++it) {
				result = (*it)->CallFunction(func, args);
				if (!result) {
					log3("PY: Call %s: returned NULL\n", lib_hookname(func));
					continue;
				}
				if (lib_unpack(result, "l", &num)) {
					// default return value is 1L, which means: further processing
					log3("PY: Call %s: returned int:%ld\n", lib_hookname(func), num);
					if (!num) ret = false;  // 0L means no more processing outside this plugin
				} else if (lib_unpack(result, "ss", &nick, &message)) {
					// Script wants to change nick or contents of the message.
					// Normally you would use SendDataToAll and return 0 from your script,
					// but this kind of message modification allows you to process it 
					// not by just one but as many scripts as you want.
					log2("PY: modifying message - Call %s: returned %s\n",
						lib_hookname(func), lib_packprint(result));
					if (nick) {
						string &nick0 = msg->ChunkString(eCH_CH_NICK);
						nick0 = nick;
						msg->ApplyChunk(eCH_CH_NICK);
					}
					if (message) {
						string &message0 = msg->ChunkString(eCH_CH_MSG);
						message0 = message;
						msg->ApplyChunk(eCH_CH_MSG);
					}
					ret = true;  // we've changed the message so we want the hub to process it and send it
				} else {
					// something unknown was returned... we will let the hub call other plugins
					log1("PY: Call %s: unexpected return value: %s\n",
						lib_hookname(func), lib_packprint(result));
				}
				free(result);
			}
		}
		free(args);
		return ret;
	}
	return true;
}

bool cpiPython::OnParsedMsgPM(cConnDC *conn, cMessageDC *msg)
{
	if ((conn != NULL) && (conn->mpUser != NULL) && (msg != NULL)) {
		w_Targs *args = lib_pack("sss", conn->mpUser->mNick.c_str(),
			msg->ChunkString(eCH_PM_MSG).c_str(), msg->ChunkString(eCH_PM_TO).c_str());
		return CallAll(W_OnParsedMsgPM, args);
	}
	return true;
}

bool cpiPython::OnParsedMsgMCTo(cConnDC *conn, cMessageDC *msg)
{
	if ((conn != NULL) && (conn->mpUser != NULL) && (msg != NULL)) {
		w_Targs *args = lib_pack("sss", conn->mpUser->mNick.c_str(),
			msg->ChunkString(eCH_MCTO_MSG).c_str(), msg->ChunkString(eCH_MCTO_TO).c_str());
		return CallAll(W_OnParsedMsgMCTo, args);
	}
	return true;
}

bool cpiPython::OnParsedMsgSupports(cConnDC *conn, cMessageDC *msg, string *back)
{
	if (conn && msg && back) {
		w_Targs *args = lib_pack("sss", conn->AddrIP().c_str(), msg->mStr.c_str(), back->c_str());
		return CallAll(W_OnParsedMsgSupports, args);
	}
	return true;
}

bool cpiPython::OnParsedMsgMyHubURL(cConnDC *conn, cMessageDC *msg)
{
	if (conn && conn->mpUser && msg) {
		w_Targs *args = lib_pack("ss", conn->mpUser->mNick.c_str(), msg->mStr.c_str());
		return CallAll(W_OnParsedMsgMyHubURL, args);
	}
	return true;
}

bool cpiPython::OnParsedMsgExtJSON(cConnDC *conn, cMessageDC *msg)
{
	if (conn && conn->mpUser && msg) {
		w_Targs *args = lib_pack("ss", conn->mpUser->mNick.c_str(), msg->mStr.c_str());
		return CallAll(W_OnParsedMsgExtJSON, args);
	}
	return true;
}

bool cpiPython::OnParsedMsgBotINFO(cConnDC *conn, cMessageDC *msg)
{
	if ((conn != NULL) && (conn->mpUser != NULL) && (msg != NULL)) {
		w_Targs *args = lib_pack("ss", conn->mpUser->mNick.c_str(), msg->mStr.c_str());
		return CallAll(W_OnParsedMsgBotINFO, args);
	}
	return true;
}

bool cpiPython::OnParsedMsgVersion(cConnDC *conn, cMessageDC *msg)
{
	if ((conn != NULL) && (msg != NULL)) {
		w_Targs *args = lib_pack("ss", conn->AddrIP().c_str(), msg->mStr.c_str());
		return CallAll(W_OnParsedMsgVersion, args);
	}
	return true;
}

bool cpiPython::OnParsedMsgMyPass(cConnDC *conn, cMessageDC *msg)
{
	if ((conn != NULL) && (conn->mpUser != NULL) && (msg != NULL)) {
		w_Targs *args = lib_pack("ss", conn->mpUser->mNick.c_str(), msg->ChunkString(eCH_1_ALL).c_str());
		return CallAll(W_OnParsedMsgMyPass, args);
	}
	return true;
}

bool cpiPython::OnParsedMsgRevConnectToMe(cConnDC *conn, cMessageDC *msg)
{
	if ((conn != NULL) && (conn->mpUser != NULL) && (msg != NULL)) {
		w_Targs *args = lib_pack("ss", conn->mpUser->mNick.c_str(), msg->ChunkString(eCH_RC_OTHER).c_str());
		return CallAll(W_OnParsedMsgRevConnectToMe, args);
	}
	return true;
}

bool cpiPython::OnParsedMsgConnectToMe(cConnDC *conn, cMessageDC *msg)
{
	if ((conn != NULL) && (conn->mpUser != NULL) && (msg != NULL)) {
		w_Targs *args = lib_pack("ssss", conn->mpUser->mNick.c_str(), msg->ChunkString(eCH_CM_NICK).c_str(),
			msg->ChunkString(eCH_CM_IP).c_str(), msg->ChunkString(eCH_CM_PORT).c_str());
		return CallAll(W_OnParsedMsgConnectToMe, args);
	}
	return true;
}

bool cpiPython::OnParsedMsgSearch(cConnDC *conn, cMessageDC *msg)
{
	if ((conn != NULL) && (conn->mpUser != NULL) && (msg != NULL)) {
		w_Targs *args = lib_pack("ss", conn->mpUser->mNick.c_str(), msg->ChunkString(eCH_AS_ALL).c_str());
		return CallAll(W_OnParsedMsgSearch, args);
	}
	return true;
}

bool cpiPython::OnParsedMsgSR(cConnDC *conn, cMessageDC *msg)
{
	if ((conn != NULL) && (conn->mpUser != NULL) && (msg != NULL)) {
		w_Targs *args = lib_pack("ss", conn->mpUser->mNick.c_str(), msg->ChunkString(eCH_SR_ALL).c_str());
		return CallAll(W_OnParsedMsgSR, args);
	}
	return true;
}

// Common code for OnParsedMsgMyINFO and OnFirstMyINFO
bool cpiPython::OnParsedMsgMyINFO__(cConnDC *conn, cMessageDC *msg, int func, const char *funcname)
{
	if (!funcname) funcname = "???";
	if (!online) return true;
	if ((conn != NULL) && (conn->mpUser != NULL) && (msg != NULL)) {
		const char *original = msg->mStr.c_str();
		char *n, *origdesc, *origtag, *origspeed, *origmail, *origsize;
		const char *desc, *tag, *speed, *mail, *size;
		const char *nick = conn->mpUser->mNick.c_str();
		if (!SplitMyINFO(original, &n, &origdesc, &origtag, &origspeed, &origmail, &origsize)) {
			log1("PY: Call %s: malformed myinfo message: %s\n", funcname, original);
			return true;
		}
		w_Targs *args = lib_pack("ssssss", n, origdesc, origtag, origspeed, origmail, origsize);
		log2("PY: Call %s: parameters %s\n", lib_hookname(func), lib_packprint(args));
		bool ret = true;
		w_Targs *result;
		long num;

		if (Size()) {
			tvPythonInterpreter::iterator it;
			for (it = mPython.begin(); it != mPython.end(); ++it) {
				result = (*it)->CallFunction(func, args);
				if (!result) {
					log3("PY: Call %s: returned NULL\n", lib_hookname(func));
					continue;
				}
				if (lib_unpack(result, "l", &num)) {
					// default return value is 1L, which means: further processing
					log3("PY: Call %s: returned int:%ld\n", lib_hookname(func), num);
					if (!num) ret = false;  // 0L means no more processing outside this plugin
				} else if (lib_unpack(result, "sssss", &desc, &tag, &speed, &mail, &size)) {
					// script wants to change the contents of myinfo
					log2("PY: modifying message - Call %s: returned %s\n",
						lib_hookname(func), lib_packprint(result));
					if (desc || tag || speed || mail || size) {
						// message chunks need updating to new MyINFO, which has the format:
						// $MyINFO $ALL <nick> <interests>$ $<speed\x01>$<e-mail>$<sharesize>$
						// $MyINFO $ALL nick <++ V:0.668,M:P,H:39/0/0,S:1>$ $DSL$$74894830123$
						string newinfo = "$MyINFO $ALL ";
						newinfo += nick;
						newinfo += " ";
						newinfo += (desc) ? desc : origdesc;
						newinfo += (tag) ? tag : origtag;
						newinfo += "$ $";
						newinfo += (speed) ? speed : origspeed;
						newinfo += "$";
						newinfo += (mail) ? mail : origmail;
						newinfo += "$";
						newinfo += (size) ? size : origsize;
						newinfo += "$";
						log3("myinfo: [ %s ] will become: [ %s ]\n", original, newinfo.c_str());

						msg->ReInit();
						msg->mStr = newinfo;
						// msg->mType = eDC_MYINFO;
						msg->Parse();
						if (msg->SplitChunks())
							log1("cpiPython::%s: failed to split new MyINFO into chunks\n", funcname);
						conn->mpUser->mEmail = msg->ChunkString(eCH_MI_MAIL);
					}
					ret = true;  // we've changed myinfo so we want the hub to store it now
				} else {
					// something unknown was returned... we will let the hub call other plugins
					log1("PY: Call %s: unexpected return value: %s\n",
						lib_hookname(func), lib_packprint(result));
				}
				free(result);
			}
		}
		freee(args);
		freee(n);
		freee(origdesc);
		freee(origtag);
		freee(origspeed);
		freee(origmail);
		freee(origsize);
		return ret;
	}
	return true;  // true means further processing
}

bool cpiPython::OnParsedMsgMyINFO(cConnDC *conn, cMessageDC *msg)
{
	return OnParsedMsgMyINFO__(conn, msg, W_OnParsedMsgMyINFO, "OnParsedMsgMyINFO");
}

bool cpiPython::OnFirstMyINFO(cConnDC *conn, cMessageDC *msg)
{
	return OnParsedMsgMyINFO__(conn, msg, W_OnFirstMyINFO, "OnFirstMyINFO");
}

bool cpiPython::OnParsedMsgValidateNick(cConnDC *conn, cMessageDC *msg)
{
	if ((conn != NULL) && (conn->mpUser != NULL) && (msg != NULL)) {
		w_Targs *args = lib_pack("s", msg->ChunkString(eCH_1_ALL).c_str());
		return CallAll(W_OnParsedMsgValidateNick, args);
	}
	return true;
}

bool cpiPython::OnParsedMsgAny(cConnDC *conn, cMessageDC *msg)
{
	if ((conn != NULL) && (conn->mpUser != NULL) && (msg != NULL)) {
		w_Targs *args = lib_pack("ss", conn->mpUser->mNick.c_str(), msg->mStr.c_str());
		return CallAll(W_OnParsedMsgAny, args);
	}
	return true;
}

bool cpiPython::OnParsedMsgAnyEx(cConnDC *conn, cMessageDC *msg)
{
	if ((conn != NULL) && (conn->mpUser == NULL) && (msg != NULL)) {
		w_Targs *args = lib_pack("ss", conn->AddrIP().c_str(), msg->mStr.c_str());
		return CallAll(W_OnParsedMsgAnyEx, args);
	}
	return true;
}

bool cpiPython::OnOpChatMessage(string *nick, string *data)
{
	if (nick && data) {
		w_Targs *args = lib_pack("ss", nick->c_str(), data->c_str());
		return CallAll(W_OnOpChatMessage, args);
	}
	return true;
}

bool cpiPython::OnPublicBotMessage(string *nick, string *data, int min_class, int max_class)
{
	if (nick && data) {
		w_Targs *args = lib_pack("ssll", nick->c_str(), data->c_str(), (long)min_class, (long)max_class);
		return CallAll(W_OnPublicBotMessage, args);
	}
	return true;
}

bool cpiPython::OnUnLoad(long code)
{
	w_Targs *args = lib_pack("l", (long)code);
	return CallAll(W_OnUnLoad, args);
}

bool cpiPython::OnCtmToHub(cConnDC *conn, string *ref)
{
	if (conn && ref) {
		w_Targs *args = lib_pack("sslls", conn->mMyNick.c_str(), conn->AddrIP().c_str(),
			conn->AddrPort(), conn->GetServPort(), ref->c_str());
		return CallAll(W_OnCtmToHub, args);
	}
	return true;
}

bool cpiPython::OnUnknownMsg(cConnDC *conn, cMessageDC *msg)
{
	if ((conn != NULL) && (conn->mpUser != NULL) && (msg != NULL)) {
		w_Targs *args = lib_pack("ss", conn->mpUser->mNick.c_str(), msg->mStr.c_str());
		return CallAll(W_OnUnknownMsg, args);
	}
	return true;
}

bool cpiPython::OnOperatorCommand(cConnDC *conn, string *command)
{
	if ((conn != NULL) && (conn->mpUser != NULL) && (command != NULL)) {
		if (mConsole.DoCommand(*command, conn)) return false;
		w_Targs *args = lib_pack("ss", conn->mpUser->mNick.c_str(), command->c_str());
		return CallAll(W_OnOperatorCommand, args);
	}
	return true;
}

bool cpiPython::OnOperatorKicks(cUser *op, cUser *user, string *why)
{
	if (op && user && why) {
		w_Targs *args = lib_pack("sss", op->mNick.c_str(), user->mNick.c_str(), why->c_str());
		return CallAll(W_OnOperatorKicks, args);
	}
	return true;
}

bool cpiPython::OnOperatorDrops(cUser *op, cUser *user, string *why)
{
	if (op && user && why) {
		bool res1, res2;
		w_Targs *args = lib_pack("ss", op->mNick.c_str(), user->mNick.c_str());
		// Calling the legacy version first
		res1 = CallAll(W_OnOperatorDrops, args);
		args = lib_pack("sss", op->mNick.c_str(), user->mNick.c_str(), why->c_str());
		res2 = CallAll(W_OnOperatorDropsWithReason, args);
		return res1 && res2;
	}
	return true;
}

bool cpiPython::OnUserCommand(cConnDC *conn, string *command)
{
	if ((conn != NULL) && (conn->mpUser != NULL) && (command != NULL)) {
		w_Targs *args = lib_pack("ss", conn->mpUser->mNick.c_str(), command->c_str());
		return CallAll(W_OnUserCommand, args);
	}
	return true;
}

bool cpiPython::OnHubCommand(cConnDC *conn, string *command, int is_op_cmd, int in_pm)
{
	// we chop the first char off command and put it in the prefix variable.
	if (conn && conn->mpUser && command && command->size() > 0) {
		long uclass = conn->mpUser->mClass;
		const char *nick = conn->mpUser->mNick.c_str();
		string prefix(*command, 0, 1);
		w_Targs *args = lib_pack("sslls", nick, command->c_str() + 1, uclass, (long)in_pm, prefix.c_str());
		return CallAll(W_OnHubCommand, args);
	}
	return true;
}

bool cpiPython::OnScriptCommand(string *cmd, string *data, string *plug, string *script)
{
	if (cmd && data && plug && script) {
		if (plug->empty() && script->empty()) {
			if (!cmd->compare("_hub_security_change")) {
				cpiPython::botname = *data;
				log("PY: botname was updated to: %s\n", cpiPython::botname.c_str());
			} else if (!cmd->compare("_opchat_name_change")) {
				cpiPython::opchatname = *data;
				log("PY: opchatname was updated to: %s\n", cpiPython::opchatname.c_str());
			}
		}
		w_Targs *args = lib_pack("ssss", cmd->c_str(), data->c_str(), plug->c_str(), script->c_str());
		return CallAll(W_OnScriptCommand, args);
	}
	return true;
}

bool cpiPython::OnScriptQuery(string *cmd, string *data, string *recipient, string *sender, ScriptResponses *resp)
{
	if (!online || !cmd || !data || !recipient || !sender || !resp) return true;
	int func = W_OnScriptQuery;
	w_Targs *args = lib_pack("ssss", cmd->c_str(), data->c_str(), recipient->c_str(), sender->c_str());
	log2("PY: Call %s: parameters %s\n", lib_hookname(func), lib_packprint(args));
	w_Targs *result;

	if (Size()) {
		tvPythonInterpreter::iterator it;
		for (it = mPython.begin(); it != mPython.end(); ++it) {
			const char *response;
			bool should_call = (*it)->receive_all_script_queries;
			if (!should_call) {
				if (!recipient->size() || !recipient->compare("python") || !recipient->compare((*it)->mScriptName))
					should_call = true;
			}
			if (!should_call) continue;
			if (!cmd->compare("_get_script_file")) {
				resp->push_back(ScriptResponse((*it)->mScriptName, (*it)->mScriptName));
				continue;
			}
			if (!cmd->compare("_get_script_name")) {
				resp->push_back(ScriptResponse((*it)->name, (*it)->mScriptName));
				continue;
			}
			if (!cmd->compare("_get_script_version")) {
				resp->push_back(ScriptResponse((*it)->version, (*it)->mScriptName));
				continue;
			}
			result = (*it)->CallFunction(func, args);
			if (!result) continue;
			if (lib_unpack(result, "s", &response)) {
				if (response) {
					ScriptResponse sr;
					sr.data = string(response);
					sr.sender = (*it)->mScriptName;
					resp->push_back(sr);
				}
			}
			free(result);
		}
	}
	free(args);
	return true;
}

bool cpiPython::OnValidateTag(cConnDC *conn, cDCTag *tag)
{
	if ((conn != NULL) && (conn->mpUser != NULL) && (tag != NULL)) {
		w_Targs *args = lib_pack("ss", conn->mpUser->mNick.c_str(), tag->mTag.c_str());
		return CallAll(W_OnValidateTag, args);
	}
	return true;
}

bool cpiPython::OnUserLogin(cUser *user)
{
	if (user != NULL) {
		w_Targs *args = lib_pack("s", user->mNick.c_str());
		return CallAll(W_OnUserLogin, args);
	}
	return true;
}

bool cpiPython::OnUserLogout(cUser *user)
{
	if (user != NULL) {
		w_Targs *args = lib_pack("s", user->mNick.c_str());
		return CallAll(W_OnUserLogout, args);
	}
	return true;
}

bool cpiPython::OnTimer(__int64 msec)
{
	w_Targs *args = lib_pack("d", 1.0 * msec / 1000.0);
	return CallAll(W_OnTimer, args);
}

bool cpiPython::OnNewReg(cUser *op, string nick, int cls)  // todo: is not called
{
	const char *opnick = (op ? op->mNick : cpiPython::botname).c_str();
	w_Targs *args = lib_pack("ssl", opnick, nick.c_str(), (long)cls);
	return CallAll(W_OnNewReg, args);
}

bool cpiPython::OnNewBan(cUser* user, cBan *ban) // todo: is not called
{
	if (ban != NULL) {
		w_Targs *args = lib_pack("ssss", ban->mNickOp.c_str(), ban->mIP.c_str(),
			ban->mNick.c_str(), ban->mReason.c_str());
		return CallAll(W_OnNewBan, args);
	}
	return true;
}

bool cpiPython::OnSetConfig(cUser *user, string *conf, string *var, string *val_new, string *val_old, int val_type)
{
	if (user && conf && var && val_new && val_old) {
		w_Targs *args = lib_pack("sssssl", user->mNick.c_str(), conf->c_str(), var->c_str(), 
			val_new->c_str(), val_old->c_str(), val_type);
		return CallAll(W_OnSetConfig, args);
	}
	return true;
}

};  // namespace nPythonPlugin

using namespace nPythonPlugin;

// CALLBACKS:

w_Targs *_usermc(int id, w_Targs *args)
{
	const char *msg, *nick, *mynick;
	if (!cpiPython::lib_unpack(args, "sss", &msg, &nick, &mynick)) return NULL;
	if (!msg) return NULL;
	if (!mynick) mynick = cpiPython::botname.c_str();

	string data = string() + "<" + mynick + "> " + msg + PipeIfMissing(msg);
	cUser *u = cpiPython::me->server->mUserList.GetUserByNick(nick);

	if (u && u->mxConn) {
		u->mxConn->Send(data, false);
		return w_ret1;
	}
	return NULL;
}

w_Targs *_mc(int id, w_Targs *args)
{
	const char *msg, *mynick;
	if (!cpiPython::lib_unpack(args, "ss", &msg, &mynick)) return NULL;
	if (!msg) return NULL;
	if (!mynick) mynick = cpiPython::botname.c_str();
	if (!SendToChat(mynick, msg, 0, 10)) return NULL;
	return w_ret1;
}

w_Targs *_classmc(int id, w_Targs *args)
{
	const char *msg, *mynick;
	long minclass, maxclass;
	if (!cpiPython::lib_unpack(args, "slls", &msg, &minclass, &maxclass, &mynick)) return NULL;
	if (!msg) return NULL;
	if (!mynick) mynick = cpiPython::botname.c_str();
	if (!SendToChat(mynick, msg, minclass, maxclass)) return NULL;
	return w_ret1;
}

w_Targs *_pm(int id, w_Targs *args)
{
	const char *msg, *nick, *from, *mynick;
	if (!cpiPython::lib_unpack(args, "ssss", &msg, &nick, &from, &mynick)) return NULL;
	if (!msg || !nick) return NULL;
	if (!from) from = cpiPython::botname.c_str();
	if (!mynick) mynick = from;

	string data = string() + "$To: " + nick + " From: " + from + " $<" + mynick + "> " + msg + PipeIfMissing(msg);
	cUser *u = cpiPython::me->server->mUserList.GetUserByNick(nick);
	if (u && u->mxConn) {
		u->mxConn->Send(data, false);
		return w_ret1;
	}
	return NULL;
}

w_Targs *_SendToOpChat(int id, w_Targs *args)
{
	const char *data, *mynick;
	if (!cpiPython::lib_unpack(args, "ss", &data, &mynick) || !data) return NULL;
	if (!SendToOpChat(data, mynick)) return NULL;
	return w_ret1;
}

w_Targs *_SendToActive(int id, w_Targs *args)
{
	const char *data;
	if (!cpiPython::lib_unpack(args, "s", &data) || !data) return NULL;
	string msg = string() + data + PipeIfMissing(data);
	cpiPython::me->server->mActiveUsers.SendToAll(msg, false, false);
	return w_ret1;
}

w_Targs *_SendToPassive(int id, w_Targs *args)
{
	const char *data;
	if (!cpiPython::lib_unpack(args, "s", &data) || !data) return NULL;
	string msg = string() + data + PipeIfMissing(data);
	cpiPython::me->server->mPassiveUsers.SendToAll(msg, false, false);
	return w_ret1;
}

w_Targs *_SendToActiveClass(int id, w_Targs *args)
{
	const char *data;
	long minclass, maxclass;
	if (!cpiPython::lib_unpack(args, "sll", &data, &minclass, &maxclass) || !data) return NULL;
	string msg = string() + data + PipeIfMissing(data);
	cpiPython::me->server->mActiveUsers.SendToAllWithClass(msg, minclass, maxclass, false, false);
	return w_ret1;
}

w_Targs *_SendToPassiveClass(int id, w_Targs *args)
{
	const char *data;
	long minclass, maxclass;
	if (!cpiPython::lib_unpack(args, "sll", &data, &minclass, &maxclass) || !data) return NULL;
	string msg = string() + data + PipeIfMissing(data);
	cpiPython::me->server->mPassiveUsers.SendToAllWithClass(msg, minclass, maxclass, false, false);
	return w_ret1;
}

w_Targs *_SendDataToUser(int id, w_Targs *args)
{
	const char *data, *nick;
	if (!cpiPython::lib_unpack(args, "ss", &data, &nick)) return NULL;
	if (!data || !nick) return NULL;
	string msg = string() + data + PipeIfMissing(data);
	cUser *u = cpiPython::me->server->mUserList.GetUserByNick(nick);
	if (u && u->mxConn) {
		u->mxConn->Send(msg, false);
		return w_ret1;
	}
	return NULL;
}

w_Targs *_SendDataToAll(int id, w_Targs *args)
{
	const char *data;
	long minclass, maxclass;
	if (!cpiPython::lib_unpack(args, "sll", &data, &minclass, &maxclass)) return NULL;
	if (!data) return NULL;
	string msg = string() + data + PipeIfMissing(data);
	// We didn't simply call cpiPython::me->server->SendToAll(msg, minclass, maxclass),
	// because at the time of writing it was buggy: it didn't care about provided class range.

	// nlist looks like this: "$NickList nick1$$nick2$$lastnick$$"
	string nlist = cpiPython::me->server->mUserList.GetNickList();
	string nick;
	cUser *u;
	log4("Py: SendDataToAll   got nicklist: %s\n", nlist.c_str());
	if (nlist.length() < 13) return NULL;
	size_t pos = 0, start = 10;
	const char *separator = "$$";
	while (start < nlist.length()) {
		pos = nlist.find(separator, start);
		if (pos == string::npos) break;
		nick = nlist.substr(start, pos - start);
		log4("Py: SendDataToAll   got nick: %s\n", nick.c_str());
		start = pos + 2;
		u = cpiPython::me->server->mUserList.GetUserByNick(nick.c_str());
		if (u && u->mxConn) {
			if (u->mClass < minclass || u->mClass > maxclass) continue;
			u->mxConn->Send(msg, false);
			log4("PY: SendDataToAll   sending message to %s\n", nick.c_str());
		}
	}
	return w_ret1;
}

w_Targs *_SendPMToAll(int id, w_Targs *args)
{
	const char *data, *from;
	long min_class, max_class;
	if (!cpiPython::lib_unpack(args, "ssll", &data, &from, &min_class, &max_class)) return NULL;
	if (!data || !from) return NULL;
	string start, end;
	cpiPython::me->server->mP.Create_PMForBroadcast(start, end, from, from, data);
	cpiPython::me->server->SendToAllWithNick(start, end, min_class, max_class);
	cpiPython::me->server->LastBCNick = from;
	return w_ret1;
}

w_Targs *_CloseConnection(int id, w_Targs *args)
{
	const char *nick;
	long nice, reason;
	if (!cpiPython::lib_unpack(args, "sll", &nick, &nice, &reason)) return NULL;
	if (!nick) return NULL;
	cUser *u = cpiPython::me->server->mUserList.GetUserByNick(nick);
	if (u && u->mxConn) {
		if (nice == 1) nice = 1000;
		u->mxConn->CloseNice(nice, reason);
		return w_ret1;
	}
	return NULL;
}

w_Targs *_GetMyINFO(int id, w_Targs *args)
{
	const char *nick;
	if (!cpiPython::lib_unpack(args, "s", &nick)) return NULL;
	if (!nick) return NULL;
	cUser *u = cpiPython::me->server->mUserList.GetUserByNick(nick);
	if (!u) return NULL;
	char *n, *desc, *tag, *speed, *mail, *size;
	if (!cpiPython::me->SplitMyINFO(u->mMyINFO.c_str(), &n, &desc, &tag, &speed, &mail, &size)) {
		log1("PY: Call GetMyINFO   malformed myinfo message: %s\n", u->mMyINFO.c_str());
		return NULL;
	}
	w_Targs *res = cpiPython::lib_pack("ssssss", n, desc, tag, speed, mail, size);
	return res;
}

w_Targs *_SetMyINFO(int id, w_Targs *args)
{
	const char *nick, *desc, *tag, *speed, *email, *size;
	if (!cpiPython::lib_unpack(args, "ssssss", &nick, &desc, &tag, &speed, &email, &size)) {
		log1("PY SetMyINFO   wrong parameters\n");
		return NULL;
	}
	if (!nick) {
		log1("PY SetMyINFO   parameter error: nick is NULL\n");
		return NULL;
	}
	cUser *u = cpiPython::me->server->mUserList.GetUserByNick(nick);
	if (!u) {
		log1("PY SetMyINFO   user %s not found\n", nick);
		return NULL;
	}
	string nfo = u->mMyINFO;
	if (nfo.length() < 20) {
		log1("PY SetMyINFO   couldn't read user's current MyINFO\n");
		return NULL;
	}
	char *n, *origdesc, *origtag, *origspeed, *origmail, *origsize;
	if (!cpiPython::me->SplitMyINFO(nfo.c_str(), &n, &origdesc, &origtag, &origspeed, &origmail, &origsize)) {
		log1("PY: Call SetMyINFO   malformed myinfo message: %s\n", nfo.c_str());
		return NULL;
	}

	string newinfo = "$MyINFO $ALL ";
	newinfo += n;
	newinfo += " ";
	newinfo += (desc) ? desc : origdesc;
	newinfo += (tag) ? tag : origtag;
	newinfo += "$ $";
	newinfo += (speed) ? speed : origspeed;
	newinfo += "$";
	newinfo += (email) ? email : origmail;
	newinfo += "$";
	newinfo += (size) ? size : origsize;
	newinfo += "$";
	log3("PY SetMyINFO   myinfo: %s  --->  %s\n", nfo.c_str(), newinfo.c_str());

	freee(n);
	freee(origdesc);
	freee(origtag);
	freee(origspeed);
	freee(origmail);
	freee(origsize);

	u->mMyINFO = newinfo;
	u->mMyINFO_basic = newinfo;
	cpiPython::me->server->mUserList.SendToAll(newinfo, true, true);
	return w_ret1;
}

w_Targs *_GetUserClass(int id, w_Targs *args)
{
	const char *nick;
	long uclass = -2;
	if (!cpiPython::lib_unpack(args, "s", &nick)) return NULL;
	if (!nick) return NULL;
	cUser *u = cpiPython::me->server->mUserList.GetUserByNick(nick);
	if (u) uclass = u->mClass;
	return cpiPython::lib_pack("l", uclass);
}

w_Targs *_GetNickList(int id, w_Targs *args)
{
	return cpiPython::lib_pack("s", strdup(cpiPython::me->server->mUserList.GetNickList().c_str()));
}

w_Targs *_GetOpList(int id, w_Targs *args)
{
	return cpiPython::lib_pack("s", strdup(cpiPython::me->server->mOpList.GetNickList().c_str()));
}

w_Targs *_GetBotList(int id, w_Targs *args)
{
	return cpiPython::lib_pack("s", strdup(cpiPython::me->server->mRobotList.GetNickList().c_str()));
}

w_Targs *_GetUserHost(int id, w_Targs *args)
{
	const char *nick;
	if (!cpiPython::lib_unpack(args, "s", &nick)) return NULL;
	if (!nick) return NULL;
	const char *host = "";
	cUser *u = cpiPython::me->server->mUserList.GetUserByNick(nick);
	if (u && u->mxConn) {
		if (!cpiPython::me->server->mUseDNS) u->mxConn->DNSLookup();
		host = u->mxConn->AddrHost().c_str();
	}
	return cpiPython::lib_pack("s", strdup(host));
}

w_Targs *_GetUserIP(int id, w_Targs *args)
{
	const char *nick;
	if (!cpiPython::lib_unpack(args, "s", &nick)) return NULL;
	if (!nick) return NULL;
	const char *ip = "";
	cUser *u = cpiPython::me->server->mUserList.GetUserByNick(nick);
	if (u && u->mxConn) ip = u->mxConn->AddrIP().c_str();
	return cpiPython::lib_pack("s", strdup(ip));
}

w_Targs *_GetUserHubURL(int id, w_Targs *args)
{
	const char *nick;
	if (!cpiPython::lib_unpack(args, "s", &nick)) return NULL;
	if (!nick) return NULL;
	const char *url = "";
	cUser *user = cpiPython::me->server->mUserList.GetUserByNick(nick);
	if (user && user->mxConn) url = user->mxConn->mHubURL.c_str();
	return cpiPython::lib_pack("s", strdup(url));
}

w_Targs *_GetUserExtJSON(int id, w_Targs *args)
{
	const char *nick;
	if (!cpiPython::lib_unpack(args, "s", &nick)) return NULL;
	if (!nick) return NULL;
	const char *pars = "";
	cUser *user = cpiPython::me->server->mUserList.GetUserByNick(nick);
	if (user) pars = user->mExtJSON.c_str();
	return cpiPython::lib_pack("s", strdup(pars));
}

w_Targs *_GetUserCC(int id, w_Targs *args)
{
	const char *nick;
	if (!cpiPython::lib_unpack(args, "s", &nick)) return NULL;
	if (!nick) return NULL;
	const char *cc = "";
	cUser *u = cpiPython::me->server->mUserList.GetUserByNick(nick);
	if (u && u->mxConn) cc = u->mxConn->mCC.c_str();
	return cpiPython::lib_pack("s", strdup(cc));
}

#ifdef HAVE_LIBGEOIP

w_Targs *_GetIPCC(int id, w_Targs *args)
{
	const char *ip;
	if (!cpiPython::lib_unpack(args, "s", &ip)) return NULL;
	if (!ip) return NULL;
	string ccstr;
	cpiPython::me->server->sGeoIP.GetCC(ip, ccstr);
	const char *cc = ccstr.c_str();
	return cpiPython::lib_pack("s", strdup(cc));
}

w_Targs *_GetIPCN(int id, w_Targs *args)
{
	const char *ip;
	if (!cpiPython::lib_unpack(args, "s", &ip)) return NULL;
	if (!ip) return NULL;
	string cnstr;
	cpiPython::me->server->sGeoIP.GetCN(ip, cnstr);
	const char *cn = cnstr.c_str();
	return cpiPython::lib_pack("s", strdup(cn));
}

w_Targs *_GetGeoIP(int id, w_Targs *args)
{
	const char *ip, *db;
	if (!cpiPython::lib_unpack(args, "ss", &ip, &db)) return NULL;
	if (!ip) return NULL;
	if (!db) db = "";
	string s_ip(ip);
	string s_db(db);
	string geo_host, geo_ran_lo, geo_ran_hi, geo_cc, geo_ccc, geo_cn, geo_reg_code, geo_reg_name;
	string geo_tz, geo_cont, geo_city, geo_post, cont;
	float geo_lat, geo_lon;
	int geo_met, geo_area;

	if (!cpiPython::me->server->sGeoIP.GetGeoIP(geo_host, geo_ran_lo, geo_ran_hi, geo_cc,
			geo_ccc, geo_cn, geo_reg_code, geo_reg_name, geo_tz, geo_cont, geo_city, geo_post,
			geo_lat, geo_lon, geo_met, geo_area, s_ip, s_db))
		return NULL;

	if (geo_cont == "AF") cont = "Africa";
	else if (geo_cont == "AS") cont = "Asia";
	else if (geo_cont == "EU") cont = "Europe";
	else if (geo_cont == "NA") cont = "North America";
	else if (geo_cont == "SA") cont = "South America";
	else if (geo_cont == "OC") cont = "Oceania";
	else if (geo_cont == "AN") cont = "Antarctica";

	vector<string> *data = new vector<string>();
	data->push_back("host");
	data->push_back(geo_host);
	data->push_back("range_low");
	data->push_back(geo_ran_lo);
	data->push_back("range_high");
	data->push_back(geo_ran_hi);
	data->push_back("country_code");
	data->push_back(geo_cc);
	data->push_back("country_code_xxx");
	data->push_back(geo_ccc);
	data->push_back("country");
	data->push_back(geo_cn);
	data->push_back("region_code");
	data->push_back(geo_reg_code);
	data->push_back("region");
	data->push_back(geo_reg_name);
	data->push_back("time_zone");
	data->push_back(geo_tz);
	data->push_back("continent_code");
	data->push_back(geo_cont);
	data->push_back("continent");
	data->push_back(cont);
	data->push_back("city");
	data->push_back(geo_city);
	data->push_back("postal_code");
	data->push_back(geo_post);

	return cpiPython::lib_pack("sdsdslslp", "latitude", geo_lat, "longitude", geo_lon,
		"metro_code", geo_met, "area_code", geo_area, (void*)data);
}

#endif

w_Targs *_AddRegUser(int id, w_Targs *args)
{
	const char *nick, *password, *op;
	long uclass;
	if (!cpiPython::lib_unpack(args, "slss", &nick, &uclass, &password, &op)) return NULL;
	if (uclass < -1 || uclass == 0 || uclass > 5) return NULL;
	if (!nick || !strlen(nick)) return NULL;
	if (!password) password = "";
	if (!op) op = "";
	if (AddRegUser(nick, (int)uclass, password, op)) return w_ret1;
	return NULL;
}

w_Targs *_DelRegUser(int id, w_Targs *args)
{
	const char *nick;
	if (!cpiPython::lib_unpack(args, "s", &nick)) return NULL;
	if (!nick || !strlen(nick)) return NULL;
	if (DelRegUser(nick)) return w_ret1;
	return NULL;
}

w_Targs *_Ban(int id, w_Targs *args)
{
	const char *op, *nick, *reason;
	long seconds, ban_type;
	if (!cpiPython::lib_unpack(args, "sssll", &op, &nick, &reason, &seconds, &ban_type)) return NULL;
	if (!op || !nick) return NULL;
	if (!reason) reason = "";
	if (Ban(nick, op, reason, (unsigned)seconds, (unsigned)ban_type)) return w_ret1;
	return NULL;
}

w_Targs *_KickUser(int id, w_Targs *args)
{
	const char *op, *nick, *reason, *address;
	if (!cpiPython::lib_unpack(args, "ssss", &op, &nick, &reason, &address)) { return NULL; }
	if (!nick || !op || !reason) return NULL;
	cUser *user = cpiPython::me->server->mUserList.GetUserByNick(op);
	if (!user) return NULL;
	if (address && strlen(address)) user->mxConn->mCloseRedirect = address;
	cpiPython::me->server->DCKickNick(NULL, user, nick, reason, (eKI_CLOSE | eKI_WHY | eKI_PM | eKI_BAN));
	return w_ret1;
}

w_Targs* _DelNickTempBan(int id, w_Targs *args)
{
	const char *nick;

	if (!cpiPython::lib_unpack(args, "s", &nick))
		return NULL;

	if (!nick)
		return NULL;

	if (DeleteNickTempBan(nick))
		return w_ret1;

	return NULL;
}

w_Targs* _DelIPTempBan(int id, w_Targs *args)
{
	const char *ip;

	if (!cpiPython::lib_unpack(args, "s", &ip))
		return NULL;

	if (!ip)
		return NULL;

	if (DeleteIPTempBan(ip))
		return w_ret1;

	return NULL;
}

w_Targs* _ParseCommand(int id, w_Targs *args)
{
	const char *nick, *cmd;
	int pm;
	if (!cpiPython::lib_unpack(args, "ssl", &nick, &cmd, &pm)) return NULL;
	if (!nick || !cmd) return NULL;
	if (!ParseCommand(nick, cmd, pm)) return NULL;
	return w_ret1;
}

w_Targs* _ScriptCommand(int id, w_Targs *args)
{
	const char *cmd, *data;
	if (!cpiPython::lib_unpack(args, "ss", &cmd, &data)) return NULL;
	if (!cmd || !data) return NULL;
	string plug("python"), s_cmd(cmd), s_data(data);
	if (!ScriptCommand(&s_cmd, &s_data, &plug, &cpiPython::me->GetInterpreter(id)->mScriptName))
		return NULL;
	return w_ret1;
}

w_Targs* _ScriptQuery(int id, w_Targs *args)
{
	const char *cmd, *data, *recipient = NULL;
	long use_long_output;
	ScriptResponses responses;
	if (!cpiPython::lib_unpack(args, "sssl", &cmd, &data, &recipient, &use_long_output)) return NULL;
	if (!cmd || !data) return NULL;
	if (!recipient) recipient = "";
	string s_cmd(cmd), s_data(data), s_recipient(recipient);
	string s_sender(cpiPython::me->GetInterpreter(id)->mScriptName);
	
	if (!ScriptQuery(&s_cmd, &s_data, &s_recipient, &s_sender, &responses))
		return NULL;
	if (responses.size() == 0)
		return cpiPython::lib_pack("lllp", (long)1, (long)0, (long)0, (void *)NULL);
	int rows = responses.size();
	int cols = (use_long_output ? 2 : 1);
	const char **res = (const char **)calloc(cols * rows, sizeof(char *));
	if (!res) {
		log1("PY: ScriptQuery   malloc failed\n");
		return NULL;
	}
	for (int r = 0; r < rows; r++) {
		if (cols == 1) {
			res[r] = strdup(responses[r].data.c_str());
		} else {
			res[(r * cols)] = strdup(responses[r].data.c_str());
			res[(r * cols) + 1] = strdup(responses[r].sender.c_str());
		}
	}
	return cpiPython::lib_pack("lllp", (long)1, (long)rows, (long)cols, (void *)res);
}

w_Targs *_SetConfig(int id, w_Targs *args)
{
	const char *conf, *var, *val;
	if (!cpiPython::lib_unpack(args, "sss", &conf, &var, &val)) return NULL;
	if (!conf || !var || !val) return NULL;
	if (SetConfig(conf, var, val)) w_ret1;
	return NULL;
}

w_Targs *_GetConfig(int id, w_Targs *args)
{
	const char *conf, *var, *def_val;
	if (!cpiPython::lib_unpack(args, "sss", &conf, &var, &def_val)) return NULL;
	if (!conf || !var) return NULL;
	const char *val = GetConfig(conf, var, def_val);
	if (!val) return NULL;
	return cpiPython::lib_pack("s", val);
}

long is_robot_nick_bad(const char *nick)
{
	// Returns: 0 = OK, 1 = already exists, 2 = empty, 3 = bad character, 4 = reserved nick.
	if (!nick || !strlen(nick)) return eBOT_WITHOUT_NICK;
	string badchars(string(BAD_NICK_CHARS_NMDC) + string(BAD_NICK_CHARS_OWN)), s_nick(nick);
	if (s_nick.find_first_of(badchars) != s_nick.npos) return eBOT_BAD_CHARS;
	cServerDC *server = cpiPython::me->server;
	if ((s_nick == server->mC.hub_security) || (s_nick == server->mC.opchat_name))
		return eBOT_RESERVED_NICK;
	if (server->mRobotList.ContainsNick(s_nick)) return eBOT_EXISTS;
	return eBOT_OK;
}

w_Targs *_IsRobotNickBad(int id, w_Targs *args)
{
	const char *nick;
	long bad = (cpiPython::lib_unpack(args, "s", &nick) ? eBOT_OK : eBOT_API_ERROR);
	if (!bad) bad = is_robot_nick_bad(nick);
	return cpiPython::lib_pack("l", bad);
}

w_Targs *_AddRobot(int id, w_Targs *args)
{
	const char *nick, *desc, *speed, *email, *share;
	long uclass;
	if (!cpiPython::lib_unpack(args, "slssss", &nick, &uclass, &desc, &speed, &email, &share)) return NULL;
	if (!nick || !desc || !speed || !email || !share) return NULL;
	if (is_robot_nick_bad(nick)) return NULL;
	if (uclass < -1 || (uclass > 5 && uclass != 10)) uclass = 0;

	cPluginRobot *robot = cpiPython::me->NewRobot(nick, uclass);
	if (robot != NULL) {
		cServerDC *server = cpiPython::me->server;
		server->mP.Create_MyINFO(robot->mMyINFO, robot->mNick, desc, speed, email, share);
		robot->mMyINFO_basic = robot->mMyINFO;

		string msg;
		server->mP.Create_Hello(msg, robot->mNick);
		server->mHelloUsers.SendToAll(msg, server->mC.delayed_myinfo, true);
		server->mUserList.SendToAll(robot->mMyINFO, server->mC.delayed_myinfo, true);
		server->mInProgresUsers.SendToAll(robot->mMyINFO, server->mC.delayed_myinfo, true);

		if (robot->mClass >= eUC_OPERATOR) {
			server->mP.Create_OpList(msg, robot->mNick);
			server->mUserList.SendToAll(msg, server->mC.delayed_myinfo, true);
			server->mInProgresUsers.SendToAll(msg, server->mC.delayed_myinfo, true);
		}
		server->mP.Create_BotList(msg, robot->mNick);
		server->mUserList.SendToAllWithFeature(msg, eSF_BOTLIST, server->mC.delayed_myinfo, true);
		return w_ret1;
	}
	return NULL;
}

w_Targs *_DelRobot(int id, w_Targs *args)
{
	const char *nick;
	if (!cpiPython::lib_unpack(args, "s", &nick)) return NULL;
	if (!nick || strlen(nick) == 0) return NULL;
	cPluginRobot *robot = (cPluginRobot *)cpiPython::me->server->mUserList.GetUserByNick(nick);
	if (robot)
		if (cpiPython::me->DelRobot(robot)) return w_ret1;
	return NULL;
}

w_Targs *_SQL(int id, w_Targs *args)
{
	return cpiPython::me->SQL(id, args);
}

w_Targs *_GetServFreq(int id, w_Targs *args)
{
	return cpiPython::lib_pack("d", cpiPython::me->server->mFrequency.GetMean(cpiPython::me->server->mTime));
}

w_Targs *_GetUsersCount(int id, w_Targs *args)
{
	return cpiPython::lib_pack("l", (long)cpiPython::me->server->mUserCountTot);
}

w_Targs *_GetTotalShareSize(int id, w_Targs *args)
{
	__int64 share = cpiPython::me->server->mTotalShare;
	ostringstream o;
	o << share;
	return cpiPython::lib_pack("s", strdup(o.str().c_str()));
}

w_Targs *_UserRestrictions(int id, w_Targs *args)
{
	long res = 0;
	const char *nick, *nochattime, *nopmtime, *nosearchtime, *noctmtime;
	if (!cpiPython::lib_unpack(args, "sssss", &nick, &nochattime, &nopmtime, &nosearchtime, &noctmtime))
		return NULL;
	if (!nick || strlen(nick) == 0) return NULL;
	string chat = (nochattime) ? nochattime : "";
	string pm = (nopmtime) ? nopmtime : "";
	string search = (nosearchtime) ? nosearchtime : "";
	string ctm = (noctmtime) ? noctmtime : "";
	cUser *u = (cUser *)cpiPython::me->server->mUserList.GetUserByNick(nick);
	if (!u) return NULL;

	long period;
	long now = (long)nUtils::cTime().Sec();
	long week = 3600 * 24 * 7;

	if (chat.length()) {
		if (chat == "0") u->mGag = 1;
		else if (chat == "1") u->mGag = now + week;
		else {
			period = cpiPython::me->server->Str2Period(chat, cerr);
			if (!period) res = 1;
			else u->mGag = now + period;
		}
	}
	if (pm.length()) {
		if (pm == "0") u->mNoPM = 1;
		else if (pm == "1") u->mNoPM = now + week;
		else {
			period = cpiPython::me->server->Str2Period(pm, cerr);
			if (!period) res = 1;
			else u->mNoPM = now + period;
		}
	}
	if (search.length()) {
		if (search == "0") u->mNoSearch = 1;
		else if (search == "1") u->mNoSearch = now + week;
		else {
			period = cpiPython::me->server->Str2Period(search, cerr);
			if (!period) res = 1;
			else u->mNoSearch = now + period;
		}
	}
	if (ctm.length()) {
		if (ctm == "0") u->mNoCTM = 1;
		else if (ctm == "1") u->mNoCTM = now + week;
		else {
			period = cpiPython::me->server->Str2Period(ctm, cerr);
			if (!period) res = 1;
			else u->mNoCTM = now + period;
		}
	}
	if (res) return NULL;
	if (!u->mGag || u->mGag > now) res |= w_UR_CHAT;
	if (!u->mNoPM || u->mNoPM > now) res |= w_UR_PM;
	if (!u->mNoSearch || u->mNoSearch > now) res |= w_UR_SEARCH;
	if (!u->mNoCTM || u->mNoCTM > now) res |= w_UR_CTM;
	return cpiPython::lib_pack("l", res);
}

w_Targs *_Topic(int id, w_Targs *args)
{
	const char *topic;
	if (!cpiPython::lib_unpack(args, "s", &topic)) return NULL;
	if (topic && strlen(topic) < 1024) {
		cpiPython::me->server->mC.hub_topic = topic;
		string msg, sTopic;
		sTopic = topic;
		cpiPython::me->server->mP.Create_HubName(msg, cpiPython::me->server->mC.hub_name, sTopic);
		cpiPython::me->server->mUserList.SendToAll(msg, eUC_NORMUSER, eUC_MASTER);
	}
	return cpiPython::lib_pack("s", strdup(cpiPython::me->server->mC.hub_topic.c_str()));
}

w_Targs *_name_and_version(int id, w_Targs *args)
{
	const char *name, *version;
	cPythonInterpreter *py = cpiPython::me->GetInterpreter(id);
	if (!py) return NULL;
	if (!cpiPython::lib_unpack(args, "ss", &name, &version)) return NULL;
	if (!name || !strlen(name)) name = py->name.c_str();
	else py->name = name;
	if (!version || !strlen(version)) version = py->version.c_str();
	else py->version = version;
	return cpiPython::lib_pack("ss", strdup(name), strdup(version));
}

w_Targs *_StopHub(int id, w_Targs *args)
{
	long code, delay;
	if (!cpiPython::lib_unpack(args, "ll", &code, &delay)) return NULL;
	if (StopHub(code, delay)) return w_ret1;
	return NULL;
}

};  // namespace nVerliHub

REGISTER_PLUGIN(nVerliHub::nPythonPlugin::cpiPython);
