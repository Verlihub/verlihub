/*
	Copyright (C) 2007-2012 Frog, frg at otaku-anime dot net
	Copyright (C) 2006-2012 Verlihub Team, devs at verlihub-project dot org
	Copyright (C) 2013-2014 RoLex, webmaster at feardc dot net

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
//#include <string.h>
#include <algorithm>

namespace nVerliHub {
	using namespace nUtils;
	using namespace nTables;
	using namespace nEnums;
	using namespace nSocket;
	using namespace nMySQL;
	namespace nPythonPlugin {

// initializing static members
void*         cpiPython::lib_handle = NULL;
w_TBegin      cpiPython::lib_begin = NULL;
w_TEnd        cpiPython::lib_end = NULL;
w_TReserveID  cpiPython::lib_reserveid = NULL;
w_TLoad       cpiPython::lib_load = NULL;
w_TUnload     cpiPython::lib_unload = NULL;
w_THasHook    cpiPython::lib_hashook = NULL;
w_TCallHook   cpiPython::lib_callhook = NULL;
w_THookName   cpiPython::lib_hookname = NULL;
w_Tpack       cpiPython::lib_pack = NULL;
w_Tunpack     cpiPython::lib_unpack = NULL;
w_TLogLevel   cpiPython::lib_loglevel = NULL;
w_Tpackprint  cpiPython::lib_packprint = NULL;
string        cpiPython::botname = "";
string        cpiPython::opchatname = "";
int           cpiPython::log_level = 1;
cServerDC    *cpiPython::server = NULL;
cpiPython    *cpiPython::me = NULL;

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
	SetConf("pi_python", "log_level", o.str().c_str());
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
	//if (wrapper_path) lib_handle = dlopen( (string(wrapper_path) + "/libvh_python_wrapper.so").c_str(), RTLD_LAZY | RTLD_GLOBAL);
	if (!lib_handle)  lib_handle = dlopen("@CMAKE_INSTALL_PREFIX@/lib/libvh_python_wrapper.so", RTLD_LAZY | RTLD_GLOBAL);
	// RTLD_GLOBAL exports all symbols from libvh_python_wrapper.so
	// without RTLD_GLOBAL the lib will fail to import other python modules
	// because they won't see any symbols from the linked libpython2.5
	if (!lib_handle)
	{
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


	if (!lib_begin || !lib_end || !lib_reserveid || !lib_load || !lib_unload || !lib_hashook || !lib_callhook || !lib_hookname || !lib_pack || !lib_unpack || !lib_loglevel || !lib_packprint)
	{ log("PY: cpiPython::OnLoad   Error locating vh_python_wrapper function symbols: %s\n", dlerror()); return; }

	w_Tcallback* callbacklist = (w_Tcallback*) calloc(W_MAX_CALLBACKS, sizeof(void*));
	callbacklist[W_SendDataToUser] = &_SendDataToUser;
	callbacklist[W_SendDataToAll] = &_SendDataToAll;
	callbacklist[W_SendPMToAll] = &_SendPMToAll;
	callbacklist[W_mc] = &_mc;
	callbacklist[W_usermc] = &_usermc;
	callbacklist[W_classmc] = &_classmc;
	callbacklist[W_pm] = &_pm;
	callbacklist[W_CloseConnection] = &_CloseConnection;
	callbacklist[W_GetMyINFO] = &_GetMyINFO;
	callbacklist[W_SetMyINFO] = &_SetMyINFO;
	callbacklist[W_GetUserClass] = &_GetUserClass;
	callbacklist[W_GetNickList] = &_GetNickList;
	callbacklist[W_GetOpList] = &_GetOpList;
	callbacklist[W_GetUserHost] = &_GetUserHost;
	callbacklist[W_GetUserIP] = &_GetUserIP;
	callbacklist[W_GetUserCC] = &_GetUserCC;
	callbacklist[W_GetIPCC] = &_GetIPCC;
	callbacklist[W_GetIPCN] = &_GetIPCN;
	callbacklist[W_Ban] = &_Ban;
	callbacklist[W_KickUser] = &_KickUser;
	callbacklist[W_ParseCommand] = &_ParseCommand;
	callbacklist[W_SetConfig] = &_SetConfig;
	callbacklist[W_GetConfig] = &_GetConfig;
	callbacklist[W_AddRobot] = &_AddRobot;
	callbacklist[W_DelRobot] = &_DelRobot;
	callbacklist[W_SQL] = &_SQL;
	callbacklist[W_GetUsersCount] = &_GetUsersCount;
	callbacklist[W_GetTotalShareSize] = &_GetTotalShareSize;
	callbacklist[W_UserRestrictions] = &_UserRestrictions;
	callbacklist[W_Topic] = &_Topic;


	const char *level = GetConf("pi_python", "log_level");
	if (level && strlen(level) > 0) log_level = char2int(level[0]);


	if ( !lib_begin(callbacklist) )
	{ log("PY: cpiPython::OnLoad   Initiating vh_python_wrapper failed!\n"); return; }
	online = true;
	lib_loglevel(log_level);

	AutoLoad();
}

bool cpiPython::RegisterAll()
{
	RegisterCallBack("VH_OnNewConn");
	RegisterCallBack("VH_OnCloseConn");
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
	RegisterCallBack("VH_OnParsedMsgSupport");
	RegisterCallBack("VH_OnParsedMsgBotINFO");
	RegisterCallBack("VH_OnParsedMsgVersion");
	RegisterCallBack("VH_OnParsedMsgMyPass");
	RegisterCallBack("VH_OnUnknownMsg");
	RegisterCallBack("VH_OnOperatorCommand");
	RegisterCallBack("VH_OnOperatorKicks");
	RegisterCallBack("VH_OnOperatorDrops");
	RegisterCallBack("VH_OnValidateTag");
	RegisterCallBack("VH_OnUserCommand");
	RegisterCallBack("VH_OnUserLogin");
	RegisterCallBack("VH_OnUserLogout");
	RegisterCallBack("VH_OnTimer");
	RegisterCallBack("VH_OnNewReg");
	RegisterCallBack("VH_OnNewBan");
	return true;
}

bool cpiPython::AutoLoad()
{
	if (log_level < 1) { if(Log(0)) LogStream() << "Open dir: " << mScriptDir << endl; }
	log1( "PY: Autoload   Loading scripts from dir: %s\n", mScriptDir.c_str() );
	string pathname, filename;
	DIR *dir = opendir(mScriptDir.c_str());
	if(!dir)
	{
		if (log_level < 1) { if(Log(1)) LogStream() << "Open dir error" << endl; }
		log1( "PY: Autoload   Failed to open directory\n" );
		return false;
	}
	struct dirent *dent = NULL;

	while(NULL != (dent=readdir(dir)))
	{
		filename = dent->d_name;
		if((filename.size() > 4) && (StrCompare(filename,filename.size()-3,3,".py")==0))
		{
			pathname = mScriptDir + filename;
			cPythonInterpreter *ip = new cPythonInterpreter (pathname);
			if(!ip) continue;

			mPython.push_back(ip);
			if(ip->Init())
			{
				if (log_level < 1) { if(Log(1)) LogStream() << "Success loading Python script: " << filename << endl; }
				log1( "PY: Autoload   Success loading script: [ %d ] %s\n", ip->id, filename.c_str() );
			}
			else
			{
				if (log_level < 1) { if(Log(1)) LogStream() << "Failed loading Python script: " << filename << endl; }
				log1( "PY: Autoload   Failed loading script: [ %d ] %s\n", ip->id, filename.c_str() );
				mPython.pop_back();
				delete ip;
			}
		}
	}

	closedir(dir);
	return true;
}

const char *cpiPython::GetName (const char *path)
{
	if (!path || !strlen(path)) return NULL;
	for (int i = strlen(path) - 1; i >= 0; i--)
		if (path[i] == '/' || path[i] == '\\') return &path[i+1];
	return path;
}

// MyINFO example:
// $MyINFO $ALL <nick> <interest>$ $<speed\x01>$<e-mail>$<sharesize>$
// $MyINFO $ALL nick <++ V:0.668,M:P,H:39/0/0,S:1>$ $DSL\x01$$74894830123$
// at this point the tag has already been validated by the hub so we do only the simplest tests here
int cpiPython::SplitMyINFO(const char *msg, const char **nick, const char **desc, const char **tag, const char **speed, const char **mail, const char **size)
{
	const char *begin = "$MyINFO $ALL ";
	int dollars[5] = { -1, -1, -1, -1, -1 };
	int spacepos=0, validtag=0, tagstart=0, tagend=0;  // spacepos = position of space after nick
	int len = strlen(msg);
	int start = strlen(begin);
	if (len < 21 ) return 0;
	if (strncmp(msg, begin, start)) return 0;
	for (int pos = start, dollar = 0; pos < len && dollar < 5; pos++)
	{
		switch (msg[pos])
		{
			case '$': dollars[dollar] = pos; dollar++; break;
			case ' ': if (!spacepos and !dollar) spacepos = pos; break;
			case '<': if (!dollar) tagstart = pos; break;
			case '>': if (!dollar) tagend = pos; break;
		}
	}
	if (dollars[4] != len-1 || !spacepos) return 0;
	if (tagstart && tagend && msg[tagend+1] == '$') validtag = 1;

	string s = msg;
	string _nick  = s.substr( start, spacepos - start );
	string _desc  = (validtag) ? s.substr( spacepos + 1, tagstart - spacepos - 1 ) : s.substr( spacepos + 1, dollars[0] - spacepos - 1 );
	string _tag   = (validtag) ? s.substr( tagstart, dollars[0] - tagstart ) : "";
	string _speed = s.substr( dollars[1] + 1, dollars[2] - dollars[1] - 1 );
	string _mail  = s.substr( dollars[2] + 1, dollars[3] - dollars[2] - 1 );
	string _size  = s.substr( dollars[3] + 1, dollars[4] - dollars[3] - 1 );

	(*nick)  =  strdup(  _nick.c_str());
	(*desc)  =  strdup(  _desc.c_str());
	(*tag)   =  strdup(   _tag.c_str());
	(*speed) =  strdup( _speed.c_str());
	(*mail)  =  strdup(  _mail.c_str());
	(*size)  =  strdup(  _size.c_str());

	log5("PY: SplitMyINFO: [%s] \n    dollars:%d,%d,%d,%d,%d / tag start:%d,end:%d / space:%d\n    nick:%s/desc:%s/tag:%s/speed:%s/mail:%s/size:%s\n",
			s.c_str(), dollars[0], dollars[1], dollars[2], dollars[3], dollars[4], tagstart, tagend, spacepos,
			*nick, *desc, *tag, *speed, *mail, *size);

	return 1;
}

w_Targs* cpiPython::SQL (int id, w_Targs* args) // (char *query)
{
	char *query;
	string q;
	long limit;
	if (!lib_begin || !lib_pack || !lib_unpack || !lib_packprint || !mQuery) return NULL;
	if (!lib_unpack(args, "sl", &query, &limit)) return NULL;
	if (!query) return NULL;
	if (limit < 1) limit = 100;
	log4("PY: SQL   query: %s\n", query);
	q = string() + query;
	mQuery->OStream() << q;
	if (mQuery->Query() < 0) { mQuery->Clear(); return lib_pack("lllp", (long)0, (long)0, (long)0, (void*)NULL); }  // error
	int rows = mQuery->StoreResult();
	if (limit < rows) rows = limit;
	if (rows < 1) { mQuery->Clear(); return lib_pack("lllp", (long)1, (long)0, (long)0, (void*)NULL); }
	int cols = mQuery->Cols();
	char * nil = (char*)"NULL";
	char ** res = (char **) calloc (cols*rows, sizeof(char*));
	if (!res) { log1("PY: SQL   malloc failed\n"); mQuery->Clear(); return lib_pack("lllp", (long)0, (long)0, (long)0, (void*)NULL); }
	for (int r = 0 ; r < rows; r++)
	{
		mQuery->DataSeek(r);
		MYSQL_ROW row;
		row = mQuery->Row();
		if (!row)
		{ log1("PY: SQL   failed to fetch row: %d\n", r); mQuery->Clear(); free(res); return lib_pack("lllp", (long)0, (long)0, (long)0, (void*)NULL); }

		for (int i = 0; i < cols; i++)
			res[(r*cols)+i] = strdup( (row[i]) ? row[i] : nil );
	}
	mQuery->Clear();
	return lib_pack("lllp", (long)1, (long)rows, (long)cols, (void*)res);
}



const char *cpiPython::GetConf(const char *conf, const char *var)
{
	if (!conf || !var) { log2("PY: GetConf   wrong parameters\n"); return NULL; }
	// first let's check hub's internal config:
	if(!strcmp(conf, "config"))
	{
		static string res, file(server->mDBConf.config_name);
		cConfigItemBase *ci = NULL;
		if(file == server->mDBConf.config_name)
		{
			ci = server->mC[var];
			if (ci)
			{
				ci->ConvertTo(res);
				log3("PY: GetConf   got result from mDBConf: %s\n", res.c_str());
				return strdup(res.c_str());
			}
		}
		return NULL;
		/*char *s = (char*) calloc(1000, sizeof(char));
		int size = GetConfig((char*)conf, (char*)var, (char*)s, 999);
		if (size < 0) { log("PY: GetConf: error in script_api's GetConfig"); return NULL; }
		if (size > 999)
		{
			free(s);
			s = (char*) calloc(size+1, sizeof(char));
			GetConfig((char*)conf, (char*)var, s, 999);
			return s;
		}*/

	}
	// let's try searching the database directly:
	if (!lib_begin || !lib_pack || !lib_unpack || !lib_packprint) return NULL;
	log3("PY: GetConf   file != 'config'... calling SQL\n");
	string query = string() + "select val from SetupList where file='" + conf + "' and var='" + var + "'";
	w_Targs *a = lib_pack( "sl", query.c_str(), (long)1 );
	log3("PY: GetConf   calling SQL with params: %s\n", lib_packprint(a));
	w_Targs *ret = SQL (-2, a);
	freee(a);
	if (!ret) return NULL;
	long res, rows, cols;
	char **list;
	log3("PY: GetConf   SQL returned %s\n", lib_packprint(ret));
	if (!lib_unpack( ret, "lllp", &res, &rows, &cols, (void**) &list ))
	{
		log3("PY: GetConf   call to SQL function failed\n");
		freee(ret); return NULL;
	}
	freee(ret);
	if (!res || !rows || !cols || !list || !list[0]) return NULL;
	log3("PY: GetConf   returning value: %s\n", list[0]);
	const char * result = list[0]; free(list); return result;
}




bool cpiPython::SetConf(const char *conf, const char *var, const char *val)
{
	if (!conf || !var || !val) { log2("PY: SetConf: wrong parameters\n"); return false; }
	// first let's check hub's internal config:
	if(!strcmp(conf, "config"))
	{
		string file(server->mDBConf.config_name);
		cConfigItemBase *ci = NULL;
		if(file == server->mDBConf.config_name)
		{
			ci = server->mC[var];
			if (ci)
			{
				ci->ConvertFrom(val);
				log3("PY: SetConf   set the value directly in mDBConf to: %s\n", val);
				return true;
			}
		}
		return false;
		/*if (SetConfig((char*)conf, (char*)var, (char*)val)) return true;
		return false;*/
	}
	// let's try searching the database directly:
	if (!lib_begin || !lib_pack || !lib_unpack || !lib_packprint) return false;
	log3("PY: SetConf   file != 'config', file == '%s'\n", conf);
	string query = string() + "delete from SetupList where file='" + conf + "' and var='" + var + "'";
	w_Targs *a = lib_pack( "sl", query.c_str(), (long)1 );
	log3("PY: SetConf   calling SQL with params: %s\n", lib_packprint(a));
	w_Targs *ret = SQL (-2, a);
	if (a) free(a);
	long res, rows, cols;
	char **list;
	log3("PY: SetConf   SQL returned %s\n", lib_packprint(ret));
	if (!lib_unpack( ret, "lllp", &res, &rows, &cols, (void**) &list ))
	{ log3("PY: SetConf   call to SQL function failed\n"); freee(ret); return false; }
	freee(ret->args[3].p);
	freee(ret);
	if (!res) { log2("requested config variable ( %s in %s ) does not exist\n", var, conf); };

	query = string("") + "insert into SetupList (file, var, val) values ('" + conf + "', '" + var + "', '" + val + "')";
	a = lib_pack( "sl", query.c_str(), (long)1 );
	log3("PY: SetConf   calling SQL with params: %s\n", lib_packprint(a));
	ret = SQL (-2, a);
	freee(a);
	log3("PY: SetConf   SQL returned %s\n", lib_packprint(ret));
	if (!lib_unpack( ret, "lllp", &res, &rows, &cols, (void**) &list ))
	{ log3("PY: SetConf   call to SQL function failed\n"); freee(ret); return false; }
	freee(ret->args[3].p);
	freee(ret);
	if (!res) return false;
	return true;
}



void cpiPython::LogLevel( int level )
{
	int old = log_level;
	log_level = level;
	ostringstream o;
	o << log_level;
	SetConf("pi_python", "log_level", o.str().c_str());
	log("PY: log_level changed: %d --> %d\n", old, (int)log_level);
	if (lib_loglevel) lib_loglevel ( log_level );
}

bool cpiPython::IsNumber( const char* s )
{
	if (!s || !strlen(s)) return false;
	for (int i=0; i<strlen(s); i++)
		switch (s[i])
		{
			case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9': break;
			default: return false;
		}
	return true;
}

int cpiPython::char2int( char c )
{
	switch (c)
	{
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
	for(it = mPython.begin(); it != mPython.end(); ++it)
		if ((*it)->id == id) return (*it);
	return NULL;
}

bool cpiPython::CallAll(int func, w_Targs* args)  // the default handler: returns true unless the calback returns 0
{
	if (!online) return true;
	w_Targs* result;
	bool ret = true;
	long l;
	if (func != W_OnTimer) log2("PY: CallAll %s: parameters %s\n", lib_hookname(func), lib_packprint(args))  // no ';' since this would break 'else'
	else log4("PY: CallAll %s\n", lib_hookname(func));
	if(Size())
	{
		tvPythonInterpreter::iterator it;
		for(it = mPython.begin(); it != mPython.end(); ++it)
		{
			result = (*it)->CallFunction(func, args);
			if(!result)
			{
				// callback doesn't exist or a failure
				if (func != W_OnTimer) log4("PY: CallAll %s: returned NULL\n", lib_hookname(func));
				continue;
			}
			if(lib_unpack(result, "l", &l))  // default return value is 1L meaning: further processing,
			{
				if (func != W_OnTimer) log3("PY: CallAll %s: returned l:%ld\n", lib_hookname(func), l);
				if (!l) ret = false;  // 0L means no more processing outside this plugin
			}
			else   // something unknown was returned... we will let the hub call other plugins
			{
				log1("PY: CallAll %s: unexpected return value %s\n", lib_hookname(func), lib_packprint(result));
			}
			free(result);
		}
	}
	free(args); // WARNING: args is freed, do not try to access it after calling CallAll!
	return ret;
}


bool cpiPython::OnNewConn(cConnDC *conn)
{
	if(conn != NULL)
	{
		w_Targs* args = lib_pack( "s", conn->AddrIP().c_str());
		return CallAll(W_OnNewConn, args);
	}
	return true;
}

bool cpiPython::OnCloseConn(cConnDC *conn)
{
	if(conn != NULL)
	{
		w_Targs* args = lib_pack( "s", conn->AddrIP().c_str());
		return CallAll(W_OnCloseConn, args);
	}
	return true;
}

bool cpiPython::OnParsedMsgChat(cConnDC *conn, cMessageDC *msg)
{
	if (!online) return true;
	if((conn != NULL) && (conn->mpUser != NULL) && (msg != NULL))
	{
		int func = W_OnParsedMsgChat;
		w_Targs* args = lib_pack( "ss", conn->mpUser->mNick.c_str(), msg->ChunkString(eCH_CH_MSG).c_str());
		log2("PY: Call %s: parameters %s\n", lib_hookname(func), lib_packprint(args));
		bool ret = true;
		w_Targs *result;
		long l;
		char *nick = NULL;
		char *message = NULL;

		if(Size())
		{
			tvPythonInterpreter::iterator it;
			for(it = mPython.begin(); it != mPython.end(); ++it)
			{
				result = (*it)->CallFunction(func, args);
				if(!result)
				{
					log3("PY: Call %s: returned NULL\n", lib_hookname(func));
					continue;
				}
				if(lib_unpack(result, "l", &l))  // default return value is 1L meaning: further processing,
				{
					log3("PY: Call %s: returned l:%ld\n", lib_hookname(func), l);
					if (!l) ret = false;  // 0L means no more processing outside this plugin
				}
				else if (lib_unpack(result, "ss", &nick, &message))  // script wants to change nick or contents of the message
				{
					// normally you would use SendDataToAll and return 0 from your script
					// but this kind of message modification allows you to process it not by just one but as many scripts as you want
					log2("PY: modifying message - Call %s: returned %s\n", lib_hookname(func), lib_packprint(result));
					if (nick)
					{
						string &nick0 = msg->ChunkString(eCH_CH_NICK);
						nick0 = nick;
						msg->ApplyChunk(eCH_CH_NICK);
					}
					if (message)
					{
						string &message0 = msg->ChunkString(eCH_CH_MSG);
						message0 = message;
						msg->ApplyChunk(eCH_CH_MSG);
					}
					ret = true;  // we've changed the message so we want the hub to process it and send it
				}
				else   // something unknown was returned... we will let the hub call other plugins
					log1("PY: Call %s: unexpected return value: %s\n", lib_hookname(func), lib_packprint(result));
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
	if((conn != NULL) && (conn->mpUser != NULL) && (msg != NULL))
	{
		w_Targs* args = lib_pack( "sss", conn->mpUser->mNick.c_str(), msg->ChunkString(eCH_PM_MSG).c_str(), msg->ChunkString(eCH_PM_TO).c_str());
		return CallAll(W_OnParsedMsgPM, args);
	}
	return true;
}

bool cpiPython::OnParsedMsgMCTo(cConnDC *conn, cMessageDC *msg)
{
	if ((conn != NULL) && (conn->mpUser != NULL) && (msg != NULL)) {
		w_Targs* args = lib_pack("sss", conn->mpUser->mNick.c_str(), msg->ChunkString(eCH_MCTO_MSG).c_str(), msg->ChunkString(eCH_MCTO_TO).c_str());
		return CallAll(W_OnParsedMsgMCTo, args);
	}

	return true;
}

bool cpiPython::OnParsedMsgSupport(cConnDC *conn, cMessageDC *msg)
{
	if((conn != NULL) && (conn->mpUser != NULL) && (msg != NULL))
	{
		w_Targs* args = lib_pack( "ss", conn->mpUser->mNick.c_str(), msg->mStr.c_str());
		return CallAll(W_OnParsedMsgSupport, args);
	}
	return true;
}

bool cpiPython::OnParsedMsgBotINFO(cConnDC *conn, cMessageDC *msg)
{
	if ((conn != NULL) && (conn->mpUser != NULL) && (msg != NULL)) {
		w_Targs* args = lib_pack("ss", conn->mpUser->mNick.c_str(), msg->mStr.c_str());
		return CallAll(W_OnParsedMsgBotINFO, args);
	}

	return true;
}

bool cpiPython::OnParsedMsgVersion(cConnDC *conn, cMessageDC *msg)
{
	if ((conn != NULL) && (msg != NULL)) {
		w_Targs* args = lib_pack("ss", conn->AddrIP().c_str(), msg->mStr.c_str());
		return CallAll(W_OnParsedMsgVersion, args);
	}

	return true;
}

bool cpiPython::OnParsedMsgMyPass(cConnDC *conn, cMessageDC *msg)
{
	if((conn != NULL) && (conn->mpUser != NULL) && (msg != NULL))
	{
		w_Targs* args = lib_pack( "ss", conn->mpUser->mNick.c_str(), msg->ChunkString(eCH_1_ALL).c_str());
		return CallAll(W_OnParsedMsgMyPass, args);
	}
	return true;
}

bool cpiPython::OnParsedMsgRevConnectToMe(cConnDC *conn, cMessageDC *msg)
{
	if((conn != NULL) && (conn->mpUser != NULL) && (msg != NULL))
	{
		w_Targs* args = lib_pack( "ss", conn->mpUser->mNick.c_str(), msg->ChunkString(eCH_RC_OTHER).c_str());
		return CallAll(W_OnParsedMsgRevConnectToMe, args);
	}
	return true;
}

bool cpiPython::OnParsedMsgConnectToMe(cConnDC *conn, cMessageDC *msg)
{
	if((conn != NULL) && (conn->mpUser != NULL) && (msg != NULL))
	{
		w_Targs* args = lib_pack( "ssss", conn->mpUser->mNick.c_str(), msg->ChunkString(eCH_CM_NICK).c_str(), msg->ChunkString(eCH_CM_IP).c_str(), msg->ChunkString(eCH_CM_PORT).c_str());
		return CallAll(W_OnParsedMsgConnectToMe, args);
	}
	return true;
}

bool cpiPython::OnParsedMsgSearch(cConnDC *conn, cMessageDC *msg)
{
	if((conn != NULL) && (conn->mpUser != NULL) && (msg != NULL))
	{
		w_Targs* args = lib_pack( "ss", conn->mpUser->mNick.c_str(), msg->ChunkString(eCH_AS_ALL).c_str());
		return CallAll(W_OnParsedMsgSearch, args);
	}
	return true;
}

bool cpiPython::OnParsedMsgSR(cConnDC *conn, cMessageDC *msg)
{
	if((conn != NULL) && (conn->mpUser != NULL) && (msg != NULL))
	{
		w_Targs* args = lib_pack( "ss", conn->mpUser->mNick.c_str(), msg->ChunkString(eCH_SR_ALL).c_str());
		return CallAll(W_OnParsedMsgSR, args);
	}
	return true;
}

bool cpiPython::OnParsedMsgMyINFO(cConnDC *conn, cMessageDC *msg)
{
	if (!online) return true;
	if((conn != NULL) && (conn->mpUser != NULL) && (msg != NULL))
	{
		int func = W_OnParsedMsgMyINFO;
		const char *original = msg->mStr.c_str();
		const char *origdesc = NULL, *origtag = NULL, *origspeed = NULL, *origmail = NULL, *origsize = NULL;
		const char *n, *desc, *tag, *speed, *mail, *size; // will be assigned by SplitMyINFO (even with NULL values)
		const char *nick = conn->mpUser->mNick.c_str();
		if (!SplitMyINFO(original, &n, &origdesc, &origtag, &origspeed, &origmail, &origsize))
		{ log1("PY: Call OnParsedMsgMyINFO: malformed myinfo message: %s\n", original); return true; }
		w_Targs* args = lib_pack( "ssssss", n, origdesc, origtag, origspeed, origmail, origsize);
		log2("PY: Call %s: parameters %s\n", lib_hookname(func), lib_packprint(args));
		bool ret = true;
		w_Targs *result;
		long l;


		if(Size())
		{
			tvPythonInterpreter::iterator it;
			for(it = mPython.begin(); it != mPython.end(); ++it)
			{
				result = (*it)->CallFunction(func, args);
				if(!result)
				{
					log3("PY: Call %s: returned NULL\n", lib_hookname(func));
					continue;
				}
				if(lib_unpack(result, "l", &l))  // default return value is 1L meaning: further processing,
				{
					log3("PY: Call %s: returned l:%ld\n", lib_hookname(func), l);
					if (!l) ret = false;  // 0L means no more processing outside this plugin
				}
				else if (lib_unpack(result, "sssss", &desc, &tag, &speed, &mail, &size))  // script wants to change the contents of myinfo
				{
					log2("PY: modifying message - Call %s: returned %s\n", lib_hookname(func), lib_packprint(result));
					if (desc || tag || speed || mail || size)
					{
						// message chunks need updating to new MyINFO
						// $MyINFO $ALL <nick> <interest>$ $<speed\x01>$<e-mail>$<sharesize>$
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
						//msg->mType = eDC_MYINFO;
						msg->Parse();
						if (msg->SplitChunks())
							log1("cpiPython::OnParsedMsgMyINFO: failed to split new MyINFO into chunks\n");
						conn->mpUser->mEmail = msg->ChunkString(eCH_MI_MAIL);
					}

					ret = true;  // we've changed myinfo so we want the hub to store it now
				}
				else   // something unknown was returned... we will let the hub call other plugins
					log1("PY: Call %s: unexpected return value: %s\n", lib_hookname(func), lib_packprint(result));
				free(result);
			}
		}
		freee(args);
		freee(n); freee(origdesc); freee(origtag); freee(origspeed); freee(origmail); freee(origsize);
		return ret;
	}
	return true; // true means further processing
}

bool cpiPython::OnFirstMyINFO(cConnDC *conn, cMessageDC *msg)
{
	if (!online) return true;
	if((conn != NULL) && (conn->mpUser != NULL) && (msg != NULL))
	{
		int func = W_OnFirstMyINFO;
		const char *original = msg->mStr.c_str();
		const char *origdesc = NULL, *origtag = NULL, *origspeed = NULL, *origmail = NULL, *origsize = NULL;
		const char *n, *desc, *tag, *speed, *mail, *size; // will be assigned by SplitMyINFO (even with NULL values)
		const char *nick = conn->mpUser->mNick.c_str();
		if (!SplitMyINFO(original, &n, &origdesc, &origtag, &origspeed, &origmail, &origsize))
		{ log1("PY: Call OnFirstMyINFO: malformed myinfo message: %s\n", original); return true; }
		w_Targs* args = lib_pack( "ssssss", n, origdesc, origtag, origspeed, origmail, origsize);
		log2("PY: Call %s: parameters %s\n", lib_hookname(func), lib_packprint(args));
		bool ret = true;
		w_Targs *result;
		long l;


		if(Size())
		{
			tvPythonInterpreter::iterator it;
			for(it = mPython.begin(); it != mPython.end(); ++it)
			{
				result = (*it)->CallFunction(func, args);
				if(!result)
				{
					log3("PY: Call %s: returned NULL\n", lib_hookname(func));
					continue;
				}
				if(lib_unpack(result, "l", &l))  // default return value is 1L meaning: further processing,
				{
					log3("PY: Call %s: returned l:%ld\n", lib_hookname(func), l);
					if (!l) ret = false;  // 0L means no more processing outside this plugin
				}
				else if (lib_unpack(result, "sssss", &desc, &tag, &speed, &mail, &size))  // script wants to change the contents of myinfo
				{
					log2("PY: modifying message - Call %s: returned %s\n", lib_hookname(func), lib_packprint(result));
					if (desc || tag || speed || mail || size)
					{
						// message chunks need updating to new MyINFO
						// $MyINFO $ALL <nick> <interest>$ $<speed\x01>$<e-mail>$<sharesize>$
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
						//msg->mType = eDC_MYINFO;
						msg->Parse();
						if (msg->SplitChunks())
							log1("cpiPython::OnFirstMyINFO: failed to split new MyINFO into chunks\n");
						conn->mpUser->mEmail = msg->ChunkString(eCH_MI_MAIL);
					}

					ret = true;  // we've changed myinfo so we want the hub to store it now
				}
				else   // something unknown was returned... we will let the hub call other plugins
					log1("PY: Call %s: unexpected return value: %s\n", lib_hookname(func), lib_packprint(result));
				free(result);
			}
		}
		freee(args);
		freee(n); freee(origdesc); freee(origtag); freee(origspeed); freee(origmail); freee(origsize);
		return ret;
	}
	return true; // true means further processing
}

bool cpiPython::OnParsedMsgValidateNick(cConnDC *conn, cMessageDC *msg)
{
	if((conn != NULL) && (conn->mpUser != NULL) && (msg != NULL))
	{
		w_Targs* args = lib_pack( "s", msg->ChunkString(eCH_1_ALL).c_str());
		return CallAll(W_OnParsedMsgValidateNick, args);
	}
	return true;
}

bool cpiPython::OnParsedMsgAny(cConnDC *conn, cMessageDC *msg)
{
	if((conn != NULL) && (conn->mpUser != NULL) && (msg != NULL))
	{
		w_Targs* args = lib_pack( "ss", conn->mpUser->mNick.c_str(), msg->mStr.c_str());
		return CallAll(W_OnParsedMsgAny, args);
	}
	return true;
}

bool cpiPython::OnParsedMsgAnyEx(cConnDC *conn, cMessageDC *msg)
{
	if ((conn != NULL) && (conn->mpUser == NULL) && (msg != NULL)) {
		w_Targs* args = lib_pack("ss", conn->AddrIP().c_str(), msg->mStr.c_str());
		return CallAll(W_OnParsedMsgAnyEx, args);
	}

	return true;
}

bool cpiPython::OnUnknownMsg(cConnDC *conn, cMessageDC *msg)
{
	if((conn != NULL) && (conn->mpUser != NULL) && (msg != NULL))
	{
		w_Targs* args = lib_pack( "ss", conn->mpUser->mNick.c_str(), msg->mStr.c_str());
		return CallAll(W_OnUnknownMsg, args);
	}
	return true;
}

bool cpiPython::OnOperatorCommand(cConnDC *conn, string *command)
{
	if((conn != NULL) && (conn->mpUser != NULL) && (command != NULL))
	{
		if(mConsole.DoCommand(*command, conn)) return false;

		w_Targs* args = lib_pack( "ss", conn->mpUser->mNick.c_str(), command->c_str());
		return CallAll(W_OnOperatorCommand, args);
	}
	return true;
}


bool cpiPython::OnOperatorKicks(cUser *OP, cUser *user, string *reason)
{
	if((OP != NULL) && (user !=NULL) && (reason != NULL))
	{
		w_Targs* args = lib_pack( "sss", OP->mNick.c_str(), user->mNick.c_str(), reason->c_str());
		return CallAll(W_OnOperatorKicks, args);
	}
	return true;
}

bool cpiPython::OnOperatorDrops(cUser *OP, cUser *user)
{
	if((OP != NULL) && (user != NULL))
	{
		w_Targs* args = lib_pack( "ss", OP->mNick.c_str(), user->mNick.c_str());
		return CallAll(W_OnOperatorDrops, args);
	}
	return true;
}

bool cpiPython::OnUserCommand(cConnDC *conn, string *command)
{
	if((conn != NULL) && (conn->mpUser != NULL) && (command != NULL))
	{
		w_Targs* args = lib_pack( "ss", conn->mpUser->mNick.c_str(), command->c_str());
		return CallAll(W_OnUserCommand, args);
	}
	return true;
}

bool cpiPython::OnValidateTag(cConnDC *conn, cDCTag *tag)
{
	if((conn != NULL) && (conn->mpUser != NULL) && (tag != NULL))
	{
		w_Targs* args = lib_pack( "ss", conn->mpUser->mNick.c_str(), tag->mTag.c_str());
		return CallAll(W_OnValidateTag, args);
	}
	return true;
}

bool cpiPython::OnUserLogin(cUser *user)
{
	if(user != NULL)
	{
		w_Targs* args = lib_pack( "s", user->mNick.c_str());
		return CallAll(W_OnUserLogin, args);
	}
	return true;
}

bool cpiPython::OnUserLogout(cUser *user)
{
	if(user != NULL)
	{
		w_Targs* args = lib_pack( "s", user->mNick.c_str());
		return CallAll(W_OnUserLogout, args);
	}
	return true;
}

bool cpiPython::OnTimer(long msec)
{

	//return true;  // disabled for now
	w_Targs* args = lib_pack( "" );
	return CallAll(W_OnTimer, args);
}

bool cpiPython::OnNewReg(cRegUserInfo *reginfo)
{
	if(reginfo != NULL)
	{
		w_Targs* args = lib_pack( "sss", reginfo->mRegOp.c_str(), reginfo->mNick.c_str(), reginfo->mClass);
		return CallAll(W_OnNewReg, args);
	}
	return true;
}

bool cpiPython::OnNewBan(cBan *ban)
{
//long mDateStart, long mDateEnd, int mType, unsigned long mRangeMin, unsigned long mRangeMax, string mMail, __int64 mShare, string mHost, string mNick, string mIP
	if(ban != NULL)
	{
		w_Targs* args = lib_pack( "ssss", ban->mNickOp.c_str(), ban->mIP.c_str(), ban->mNick.c_str(), ban->mReason.c_str());
		return CallAll(W_OnNewBan, args);
	}
	return true;
}

	}; // namespace nPythonPlugin
	using namespace nPythonPlugin;
// CALLBACKS:


w_Targs* _usermc (int id, w_Targs* args) // (char *data, char *nick)
{
	char *data, *nick;
	if(!cpiPython::lib_unpack(args, "ss", &data, &nick))
		return NULL;
	if(!data || !nick)
		return NULL;
	string msg = string() + "<" + cpiPython::botname + "> " + data + "|";
	cUser *u = cpiPython::me->server->mUserList.GetUserByNick(nick);
	if(u && u->mxConn) {
		u->mxConn->Send( msg, true );
		return w_ret1;
	}
	return NULL;
}

w_Targs* _mc (int id, w_Targs* args) // (char *data)
{
	char *data;
	if (!cpiPython::lib_unpack(args, "s", &data)) return NULL;
	if (!data)
		return NULL;
	string msg = string() + "<" + cpiPython::botname + "> " + data + "|";
	cpiPython::me->server->SendToAll(msg, 0, 10);
	return w_ret1;
}

w_Targs* _classmc (int id, w_Targs* args) // (char *data)
{
	char *data;
	long minclass, maxclass;
	if(!cpiPython::lib_unpack(args, "sll", &data, &minclass, &maxclass))
		return NULL;
	if(!data)
		return NULL;
	string msg = string() + "<" + cpiPython::botname + "> " + data + "|";
	// we would use the call below but it is buggy - it does not care about provided class range
	//cpiPython::me->server->SendToAll(msg, minclass, maxclass);
	// therefore we have to do this the hard way:

	// nlist looks like this: "$NickList nick1$$nick2$$lastnick$$"
	string nlist = cpiPython::me->server->mUserList.GetNickList();
	string nick;
	cUser *u;
	log4("Py: classmc   got nicklist: %s\n", nlist.c_str());
	if (nlist.length() < 13) return NULL;
	int start=10, pos=0;
	const char *separator = "$$";
	while(start < nlist.length())
	{
		pos = nlist.find( separator, start );
		if (pos == string::npos) break;
		nick = nlist.substr( start, pos - start );
		log4("Py: classmc   got nick: %s\n", nick.c_str());
		start = pos + 2;
		u = cpiPython::me->server->mUserList.GetUserByNick(nick.c_str());
		if (u && u->mxConn)
		{
			if (u->mClass < minclass || u->mClass > maxclass) continue;
			u->mxConn->Send( msg, true );
			log4("PY: classmc   sending message to %s\n", nick.c_str());
		}
	}
	return w_ret1;
}

w_Targs* _pm (int id, w_Targs* args) // (char *data, char *nick)
{
	char *data, *nick;
	if (!cpiPython::lib_unpack(args, "ss", &data, &nick)) return NULL;
	if (!data || !nick) return NULL;
	string msg = string() + "$To: " + nick + " From: " + cpiPython::botname + " $<" + cpiPython::botname + "> " + data + "|";
	cUser *u = cpiPython::me->server->mUserList.GetUserByNick(nick);
	if (u && u->mxConn) { u->mxConn->Send( msg, true ); return w_ret1; }
	return NULL;
}

w_Targs* _SendDataToUser (int id, w_Targs* args) // (char *data, char *nick)
{
	char *data, *nick;
	if (!cpiPython::lib_unpack(args, "ss", &data, &nick)) return NULL;
	if (!data || !nick) return NULL;
	string d = data;
	cUser *u = cpiPython::me->server->mUserList.GetUserByNick(nick);
	if (u && u->mxConn) { u->mxConn->Send( d, true ); return w_ret1; }
	return NULL;
}

w_Targs* _SendDataToAll (int id, w_Targs* args) // (char *data, long min_class, long max_class)
{
	char *data;
	long minclass, maxclass;
	if (!cpiPython::cpiPython::lib_unpack(args, "sll", &data, &minclass, &maxclass)) return NULL;
	if (!data) return NULL;
	string msg = data;
	// we would use the call below but it is buggy - it does not care about provided class range
	//cpiPython::me->server->SendToAll(msg, minclass, maxclass);
	// therefore we have to do this the hard way:

	// nlist looks like this: "$NickList nick1$$nick2$$lastnick$$"
	string nlist = cpiPython::me->server->mUserList.GetNickList();
	string nick;
	cUser *u;
	log4("Py: SendDataToAll   got nicklist: %s\n", nlist.c_str());
	if (nlist.length() < 13) return NULL;
	int start=10, pos=0;
	const char *separator = "$$";
	while(start < nlist.length())
	{
		pos = nlist.find( separator, start );
		if (pos == string::npos) break;
		nick = nlist.substr( start, pos - start );
		log4("Py: SendDataToAll   got nick: %s\n", nick.c_str());
		start = pos + 2;
		u = cpiPython::me->server->mUserList.GetUserByNick(nick.c_str());
		if (u && u->mxConn)
		{
			if (u->mClass < minclass || u->mClass > maxclass) continue;
			u->mxConn->Send( msg, true );
			log4("PY: SendDataToAll   sending message to %s\n", nick.c_str());
		}
	}
	return w_ret1;
}

w_Targs* _SendPMToAll (int id, w_Targs* args) // (char *data, char *from, long min_class, long max_class)
{
	char *data, *from;
	long min_class, max_class;
	if (!cpiPython::lib_unpack(args, "ssll", &data, &from, &min_class, &max_class)) return NULL;
	if (!data || !from) return NULL;
	string start, end;
	cpiPython::me->server->mP.Create_PMForBroadcast(start, end, from, from, data);
	cpiPython::me->server->SendToAllWithNick(start,end, min_class, max_class);
	cpiPython::me->server->LastBCNick = from;
	return w_ret1;
}

w_Targs* _CloseConnection (int id, w_Targs* args) // (char *nick)
{
	char *nick;
	if (!cpiPython::lib_unpack(args, "s", &nick)) return NULL;
	if (!nick) return NULL;
	cUser *u = cpiPython::me->server->mUserList.GetUserByNick(nick);
	if (u && u->mxConn) { u->mxConn->CloseNow(); return w_ret1; }
	return NULL;
}

w_Targs* _GetMyINFO (int id, w_Targs* args) // (char *nick)
{
	char *nick;
	if (!cpiPython::lib_unpack(args, "s", &nick)) return NULL;
	if (!nick) return NULL;
	cUser *u = cpiPython::me->server->mUserList.GetUserByNick(nick);
	if (!u) return NULL;
	const char *n, *desc, *tag, *speed, *mail, *size;
	if (!cpiPython::me->SplitMyINFO( u->mMyINFO.c_str(), &n, &desc, &tag, &speed, &mail, &size ))
	{ log1("PY: Call GetMyINFO   malformed myinfo message: %s\n", u->mMyINFO.c_str()); return NULL; }
	w_Targs* res = cpiPython::lib_pack( "ssssss", n, desc, tag, speed, mail, size);
	return res;
}

w_Targs* _SetMyINFO (int id, w_Targs* args) // (char *nick)
{
	char *nick, *desc, *tag, *speed, *email, *size;
	if (!cpiPython::lib_unpack(args, "ssssss", &nick, &desc, &tag, &speed, &email, &size)) { log1( "PY SetMyINFO   wrong parameters\n"); return NULL; }
	if (!nick) { log1( "PY SetMyINFO   parameter error: nick is NULL\n"); return NULL; }
	cUser *u = cpiPython::me->server->mUserList.GetUserByNick(nick);
	if(!u) { log1( "PY SetMyINFO   user %s not found\n", nick ); return NULL; }

	string nfo = u->mMyINFO;
	if (nfo.length() < 20) { log1( "PY SetMyINFO   couldn't read user's current MyINFO\n"); return NULL; }

	const char *n, *origdesc = NULL, *origtag = NULL, *origspeed = NULL, *origmail = NULL, *origsize = NULL;
	if (!cpiPython::me->SplitMyINFO(nfo.c_str(), &n, &origdesc, &origtag, &origspeed, &origmail, &origsize))
	{ log1("PY: Call SetMyINFO   malformed myinfo message: %s\n", nfo.c_str()); return NULL; }

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

	freee(n); freee(origdesc); freee(origtag); freee(origspeed); freee(origmail); freee(origsize);

	u->mMyINFO = newinfo;
	u->mMyINFO_basic = newinfo;
	cpiPython::me->server->mUserList.SendToAll(newinfo, true, true);
	return w_ret1;
}

w_Targs* _GetUserClass (int id, w_Targs* args) // (char *nick)
{
	char *nick;
	long uclass = -1;
	if (!cpiPython::lib_unpack(args, "s", &nick)) return NULL;
	if (!nick) return NULL;
	cUser *u = cpiPython::me->server->mUserList.GetUserByNick(nick);
	if (u) uclass = u->mClass;
	return cpiPython::lib_pack( "l", uclass);
}

w_Targs* _GetNickList (int id, w_Targs* args) // ()
{
	return cpiPython::lib_pack( "s", strdup(cpiPython::me->server->mUserList.GetNickList().c_str()));
}

w_Targs* _GetOpList (int id, w_Targs* args) // ()
{
	return cpiPython::lib_pack( "s", strdup(cpiPython::me->server->mOpList.GetNickList().c_str()));
}

w_Targs* _GetUserHost (int id, w_Targs* args) // (char *nick)
{
	char *nick;
	if (!cpiPython::lib_unpack(args, "s", &nick)) return NULL;
	if (!nick) return NULL;
	const char *host = "";
	cUser *u = cpiPython::me->server->mUserList.GetUserByNick(nick);
	if (u && u->mxConn)
	{
		if (!cpiPython::me->server->mUseDNS) u->mxConn->DNSLookup();
		host = u->mxConn->AddrHost().c_str();
	}
	return cpiPython::lib_pack( "s", strdup(host));
}

w_Targs* _GetUserIP (int id, w_Targs* args) // (char *nick)
{
	char *nick;
	if (!cpiPython::lib_unpack(args, "s", &nick)) return NULL;
	if (!nick) return NULL;
	const char *ip = "";
	cUser *u = cpiPython::me->server->mUserList.GetUserByNick(nick);
	if (u && u->mxConn) ip = u->mxConn->AddrIP().c_str();
	return cpiPython::lib_pack( "s", strdup(ip));
}

w_Targs* _GetUserCC (int id, w_Targs* args) // (char* nick)
{
	char *nick;
	if (!cpiPython::lib_unpack(args, "s", &nick)) return NULL;
	if (!nick) return NULL;
	const char *cc = "";
	cUser *u = cpiPython::me->server->mUserList.GetUserByNick(nick);
	if (u && u->mxConn) cc = u->mxConn->mCC.c_str();
	return cpiPython::lib_pack( "s", strdup(cc));
}

w_Targs* _GetIPCC (int id, w_Targs* args) // (char* ip)
{
	char *ip;
	if (!cpiPython::lib_unpack(args, "s", &ip)) return NULL;
	if (!ip) return NULL;
	string ccstr;
	cpiPython::me->server->sGeoIP.GetCC(ip, ccstr);
	const char *cc = "";
	cc = ccstr.c_str();
	return cpiPython::lib_pack("s", strdup(cc));
}

w_Targs* _GetIPCN (int id, w_Targs* args) // (char* ip)
{
	char *ip;
	if (!cpiPython::lib_unpack(args, "s", &ip)) return NULL;
	if (!ip) return NULL;
	string cnstr;
	cpiPython::me->server->sGeoIP.GetCN(ip, cnstr);
	const char *cn = "";
	cn = cnstr.c_str();
	return cpiPython::lib_pack("s", strdup(cn));
}

w_Targs* _Ban (int id, w_Targs* args) // (char *nick, long howlong, long bantype)
{
	return NULL; // not implemented yet
}

w_Targs* _KickUser (int id, w_Targs* args) // (char *op, char *nick, char *data)
{
	char *op, *nick, *data;
	if (!cpiPython::lib_unpack(args, "sss", &op, &nick, &data)) return NULL;
	if (!nick || !op || !data) return NULL;
	cUser *u = cpiPython::me->server->mUserList.GetUserByNick(op);
	if (!u) return NULL;
	ostringstream os;
	cpiPython::me->server->DCKickNick(&os, u, nick, data, eKCK_Drop | eKCK_Reason | eKCK_PM | eKCK_TBAN);
	return w_ret1;
}

w_Targs* _ParseCommand (int id, w_Targs* args) // (char *data)
{
	return NULL; // not implemented yet
}

w_Targs* _SetConfig (int id, w_Targs* args) // (char *conf, char *var, char *val)
{
	char *conf, *var, *val;
	if (!cpiPython::lib_unpack(args, "sss", &conf, &var, &val)) return NULL;
	if (!conf || !var || !val) return NULL;
	cpiPython *pi = cpiPython::me;
	if (!pi) { log1("PY: GetInterpreter: cannot find any interpreter with given id: %d\n", id); return NULL; }
	if ( pi->SetConf(conf, var, val) ) w_ret1; return NULL;
}

w_Targs* _GetConfig (int id, w_Targs* args) // (char *conf, char *var)
{
	char *conf, *var;
	if (!cpiPython::lib_unpack(args, "ss", &conf, &var)) return NULL;
	if (!conf || !var) return NULL;
	return cpiPython::lib_pack( "s", cpiPython::me->GetConf(conf, var));
}

w_Targs* _AddRobot (int id, w_Targs* args) // (char *nick, long uclass, char *desc, char *speed, char *email, char *share)
{
	char *nick, *desc, *speed, *email, *share;
	long uclass;
	if (!cpiPython::lib_unpack(args, "slssss", &nick, &uclass, &desc, &speed, &email, &share)) return NULL;
	if (!nick || !desc || !speed || !email || !share) return NULL;
	cPluginRobot *robot = cpiPython::me->NewRobot(nick, uclass);
	if(robot != NULL)
	{
		cpiPython::me->server->mP.Create_MyINFO(robot->mMyINFO, robot->mNick, desc, speed, email, share); // create myinfo
		robot->mMyINFO_basic = robot->mMyINFO;

		static string msg;
		msg.erase();
		cpiPython::me->server->mP.Create_Hello(msg, robot->mNick); // send hello
		cpiPython::me->server->mHelloUsers.SendToAll(msg, cpiPython::me->server->mC.delayed_myinfo, true);
		cpiPython::me->server->mUserList.SendToAll(robot->mMyINFO, cpiPython::me->server->mC.delayed_myinfo, true); // send myinfo

		if (robot->mClass >= eUC_OPERATOR) { // send short oplist
			msg.erase();
			cpiPython::me->server->mP.Create_OpList(msg, robot->mNick);
			cpiPython::me->server->mUserList.SendToAll(msg, cpiPython::me->server->mC.delayed_myinfo, true);
		}

		msg.erase();
		cpiPython::me->server->mP.Create_BotList(msg, robot->mNick); // send short botlist
		cpiPython::me->server->mUserList.SendToAllWithFeature(msg, eSF_BOTLIST, cpiPython::me->server->mC.delayed_myinfo, true);

		return w_ret1;
	}
	return NULL;
}

w_Targs* _DelRobot (int id, w_Targs* args) // (char* nick)
{
	char *nick;
	if (!cpiPython::lib_unpack(args, "s", &nick)) return NULL;
	if (!nick || strlen(nick)==0) return NULL;
	cPluginRobot *robot = (cPluginRobot *)cpiPython::me->server->mUserList.GetUserByNick(nick);
	if(robot)
		if (cpiPython::me->DelRobot(robot)) return w_ret1;
	return NULL;
}

w_Targs* _SQL (int id, w_Targs* args) // (char *query)
{
	return cpiPython::me->SQL( id, args );
}

w_Targs* _GetUsersCount (int id, w_Targs* args) // ()
{
	return cpiPython::lib_pack("l", (long)cpiPython::me->server->mUserCountTot);
}

w_Targs* _GetTotalShareSize (int id, w_Targs* args) // ()
{
	__int64 share = cpiPython::me->server->GetTotalShareSize();
	ostringstream o;
	o << share;
	return cpiPython::lib_pack("s", strdup(o.str().c_str()));
}

w_Targs* _UserRestrictions (int id, w_Targs* args) // (char *nick, char *nochattime, char *nopmtime, char *nosearchtime, char *noctmtime)
{
	long res = 0;
	char *nick, *nochattime, *nopmtime, *nosearchtime, *noctmtime;
	if (!cpiPython::lib_unpack(args, "sssss", &nick, &nochattime, &nopmtime, &nosearchtime, &noctmtime)) return NULL;
	if (!nick) return NULL;
	string chat = (nochattime) ? nochattime : "";
	string pm = (nopmtime) ? nopmtime : "";
	string search = (nosearchtime) ? nosearchtime : "";
	string ctm = (noctmtime) ? noctmtime : "";

	if (!nick || strlen(nick)==0) return NULL;
	cUser *u = (cUser *)cpiPython::me->server->mUserList.GetUserByNick(nick);
	if(!u) return NULL;

	long period;
	long now = (long) nUtils::cTime().Sec();
	long week = 3600 * 24 * 7;

	if (chat.length())
	{
		if (chat == "0") u->mGag = 1;
		else if (chat == "1") u->mGag = now + week;
		else
		{
			period = cpiPython::me->server->Str2Period (chat, cerr);
			if (!period) res = 1;
			else u->mGag = now + period;
		}
	}
	if (pm.length())
	{
		if (pm == "0") u->mNoPM = 1;
		else if (pm == "1") u->mNoPM = now + week;
		else
		{
			period = cpiPython::me->server->Str2Period (pm, cerr);
			if (!period) res = 1;
			else u->mNoPM = now + period;
		}
	}
	if (search.length())
	{
		if (search == "0") u->mNoSearch = 1;
		else if (search == "1") u->mNoSearch = now + week;
		else
		{
			period = cpiPython::me->server->Str2Period (search, cerr);
			if (!period) res = 1;
			else u->mNoSearch = now + period;
		}
	}
	if (ctm.length())
	{
		if (ctm == "0") u->mNoCTM = 1;
		else if (ctm == "1") u->mNoCTM = now + week;
		else
		{
			period = cpiPython::me->server->Str2Period (ctm, cerr);
			if (!period) res = 1;
			else u->mNoCTM = now + period;
		}
	}
	if (res) return NULL;

	if (!u->mGag || u->mGag > now) 			res |= w_UR_CHAT;
	if (!u->mNoPM || u->mNoPM > now) 		res |= w_UR_PM;
	if (!u->mNoSearch || u->mNoSearch > now) 	res |= w_UR_SEARCH;
	if (!u->mNoCTM || u->mNoCTM > now) 		res |= w_UR_CTM;
	return cpiPython::lib_pack( "l", res);
}

w_Targs* _Topic (int id, w_Targs* args) // (char* topic)
{
	char *topic;
	if (!cpiPython::lib_unpack(args, "s", &topic)) return NULL;
	if (topic && strlen(topic) < 1024)
	{
		cpiPython::me->server->mC.hub_topic = topic;
		string msg, sTopic;
		sTopic = topic;
		cpiPython::me->server->mP.Create_HubName( msg, cpiPython::me->server->mC.hub_name, sTopic);

		cpiPython::me->server->mUserList.SendToAll(msg, eUC_NORMUSER, eUC_MASTER);
	}
	return cpiPython::lib_pack("s", strdup(cpiPython::me->server->mC.hub_topic.c_str()));
}
}; // namespace nVerliHub
REGISTER_PLUGIN(nVerliHub::nPythonPlugin::cpiPython);
