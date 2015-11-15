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

#include "cpilua.h"
#include "src/stringutils.h"
#include "src/cbanlist.h"
#include "src/cserverdc.h"
#include "src/i18n.h"
#include "src/script_api.h"
#include <dirent.h>
#include <string>
#include <cctype>

namespace nVerliHub {
	using namespace nUtils;
	using namespace nSocket;
	using namespace nPlugin;
	using namespace nMySQL;
	namespace nLuaPlugin {

cServerDC *cpiLua::server = NULL;
cpiLua *cpiLua::me = NULL;
int cpiLua::log_level = 1;
int cpiLua::err_class = 3;

cpiLua::cpiLua():
	mConsole(this),
	mQuery(NULL)
{
	mName = LUA_PI_IDENTIFIER;
	mVersion = LUA_PI_VERSION;
	me = this;
}

cpiLua::~cpiLua()
{
	ostringstream val;
	val << this->log_level;
	this->SetConf("pi_lua", "log_level", val.str().c_str());
	val.str("");
	val << this->err_class;
	this->SetConf("pi_lua", "err_class", val.str().c_str());

	if (mQuery != NULL) {
		mQuery->Clear();
		delete mQuery;
	}

	this->Empty();
}

bool cpiLua::IsNumber(const char *num)
{
	if (!num || !strlen(num))
		return false;

	for (unsigned int i = 0; i < strlen(num); i++) {
		if (!isdigit(num[i]))
			return false;
	}

	return true;
}

void cpiLua::SetLogLevel(int level)
{
	ostringstream val;
	val << level;
	this->SetConf("pi_lua", "log_level", val.str().c_str());
	this->log_level = level;
}

void cpiLua::SetErrClass(int eclass)
{
	ostringstream val;
	val << eclass;
	this->SetConf("pi_lua", "err_class", val.str().c_str());
	this->err_class = eclass;
}

const char* cpiLua::GetConf(const char *conf, const char *var, const char *def)
{
	if (!server || !conf || !var)
		return def;

	string file(conf), val(def ? def : "");
	static string res;
	bool found = true, delci = false;
	cConfigItemBase *ci = NULL;

	if (file == server->mDBConf.config_name) {
		ci = server->mC[var];
	} else {
		delci = true;
		ci = new cConfigItemBaseString(val, var);
		found = server->mSetupList.LoadItem(file.c_str(), ci);
	}

	if (ci) {
		if (found)
			ci->ConvertTo(res);

		if (delci)
			delete ci;

		ci = NULL;

		if (found)
			return res.c_str();
	}

	return def;
}

bool cpiLua::SetConf(const char *conf, const char *var, const char *val)
{
	if (!server || !conf || !var)
		return false;

	string file(conf), def(val ? val : "");
	bool delci = false;
	cConfigItemBase *ci = NULL;

	if (file == server->mDBConf.config_name) {
		ci = server->mC[var];
	} else {
		delci = true;
		ci = new cConfigItemBaseString(def, var);
	}

	if (ci) {
		ci->ConvertFrom(def);
		server->mSetupList.SaveItem(file.c_str(), ci);

		if (delci)
			delete ci;

		ci = NULL;
		return true;
	}

	return false;
}

void cpiLua::OnLoad(cServerDC *serv)
{
	cVHPlugin::OnLoad(serv);
	mQuery = new cQuery(serv->mMySQL);
	this->server = serv;
	mScriptDir = serv->mConfigBaseDir + "/scripts/";

	ostringstream def;
	def << this->log_level;
	const char *level = this->GetConf("pi_lua", "log_level", def.str().c_str()); // get log level

	if (IsNumber(level))
		this->log_level = atoi(level);

	def.str("");
	def << this->err_class;
	const char *eclass = this->GetConf("pi_lua", "err_class", def.str().c_str()); // get error class

	if (IsNumber(eclass))
		this->err_class = atoi(eclass);

	AutoLoad();
}

bool cpiLua::RegisterAll()
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
	RegisterCallBack("VH_OnParsedMsgSupport");
	RegisterCallBack("VH_OnParsedMsgBotINFO");
	RegisterCallBack("VH_OnParsedMsgVersion");
	RegisterCallBack("VH_OnParsedMsgMyPass");
	RegisterCallBack("VH_OnUnknownMsg");
	//RegisterCallBack("VH_OnUnparsedMsg");
	RegisterCallBack("VH_OnOperatorCommand");
	RegisterCallBack("VH_OnUserCommand");
	RegisterCallBack("VH_OnHubCommand");
	RegisterCallBack("VH_OnOperatorKicks");
	RegisterCallBack("VH_OnOperatorDrops");
	RegisterCallBack("VH_OnValidateTag");
	RegisterCallBack("VH_OnUserLogin");
	RegisterCallBack("VH_OnUserLogout");
	RegisterCallBack("VH_OnTimer");
	RegisterCallBack("VH_OnNewReg");
	RegisterCallBack("VH_OnDelReg");
	RegisterCallBack("VH_OnNewBan");
	RegisterCallBack("VH_OnUnBan");
	RegisterCallBack("VH_OnUpdateClass");
	RegisterCallBack("VH_OnScriptCommand");
	RegisterCallBack("VH_OnCtmToHub");
	RegisterCallBack("VH_OnOpChatMessage");
	RegisterCallBack("VH_OnPublicBotMessage");
	RegisterCallBack("VH_OnUnLoad");
	return true;
}

bool cpiLua::AutoLoad()
{
	if (Log(0))
		LogStream() << "Opening directory: " << mScriptDir << endl;

	DIR *dir = opendir(mScriptDir.c_str());

	if (!dir) {
		if (Log(1))
			LogStream() << "Error opening directory" << endl;

		return false;
	}

	string pathname, filename;
	struct dirent *dent = NULL;

	while (NULL != (dent = readdir(dir))) {
		filename = dent->d_name;

		if ((filename.size() > 4) && (StrCompare(filename, filename.size() - 4, 4, ".lua") == 0)) {
			pathname = mScriptDir + filename;
			cLuaInterpreter *ip = new cLuaInterpreter(pathname);

			if (ip) {
				if (ip->Init()) {
					AddData(ip);
					ip->Load();

					if (Log(1))
						LogStream() << "Success loading and parsing Lua script: " << filename << endl;
				} else {
					if (Log(1))
						LogStream() << "Failed loading or parsing Lua script: " << filename << endl;

					delete ip;
				}
			}
		}
	}

	closedir(dir);
	return true;
}

bool cpiLua::CallAll(const char *fncname, char *args[], cConnDC *conn)
{
	bool ret = true;

	if (Size()) {
		tvLuaInterpreter::iterator it;

		for (it = mLua.begin(); it != mLua.end(); ++it) {
			if (!(*it)->CallFunction(fncname, args, conn))
				ret = false;

			/*
				todo
					we dont break here
					it means that all scripts get function call
					ret is set to what last script returns, even if previous returned false
					look over this
			*/
		}
	}

	return ret;
}

bool cpiLua::OnNewConn(cConnDC *conn)
{
	if (conn) {
		char *args[] = {
			(char*)conn->AddrIP().c_str(),
			(char*)toString(conn->AddrPort()),
			(char*)conn->GetServAddr().c_str(),
			(char*)toString(conn->GetServPort()),
			NULL
		};

		return CallAll("VH_OnNewConn", args, conn);
	}

	return true;
}

bool cpiLua::OnCloseConn(cConnDC *conn)
{
	if (conn != NULL) {
		if (conn->mpUser != NULL) {
			char * args[] = {
				(char *)conn->AddrIP().c_str(),
				(char *)conn->mpUser->mNick.c_str(),
				NULL
			};

			return CallAll("VH_OnCloseConn", args, conn);
		} else {
			char * args[] = {
				(char *)conn->AddrIP().c_str(),
				NULL
			};

			return CallAll("VH_OnCloseConn", args, conn);
		}
	}

	return true;
}

bool cpiLua::OnCloseConnEx(cConnDC *conn) // @todo: dont include this callback in your final scripts, it might change before final release
{
	if (conn != NULL) {
		if (conn->mpUser != NULL) {
			char * args[] = {
				(char *)conn->AddrIP().c_str(),
				(char *)toString(conn->mCloseReason),
				(char *)conn->mpUser->mNick.c_str(),
				NULL
			};

			return CallAll("VH_OnCloseConnEx", args, conn);
		} else {
			char * args[] = {
				(char *)conn->AddrIP().c_str(),
				(char *)toString(conn->mCloseReason),
				NULL
			};

			return CallAll("VH_OnCloseConnEx", args, conn);
		}
	}

	return true;
}

bool cpiLua::OnParsedMsgChat(cConnDC *conn, cMessageDC *msg)
{
	if ((conn != NULL) && (conn->mpUser != NULL) && (msg != NULL)) {
		char * args[] = {
			(char *)conn->mpUser->mNick.c_str(),
			(char *)msg->ChunkString(eCH_CH_MSG).c_str(),
			NULL
		}; // eCH_CH_ALL, eCH_CH_NICK, eCH_CH_MSG

		return CallAll("VH_OnParsedMsgChat", args, conn);
	}

	return true;
}

bool cpiLua::OnParsedMsgPM(cConnDC *conn, cMessageDC *msg)
{
	if ((conn != NULL) && (conn->mpUser != NULL) && (msg != NULL)) {
		char * args[] = {
			(char *)conn->mpUser->mNick.c_str(),
			(char *)msg->ChunkString(eCH_PM_MSG).c_str(),
			(char *)msg->ChunkString(eCH_PM_TO).c_str(),
			NULL
		}; // eCH_PM_ALL, eCH_PM_TO, eCH_PM_FROM, eCH_PM_CHMSG, eCH_PM_NICK, eCH_PM_MSG

		return CallAll("VH_OnParsedMsgPM", args, conn);
	}

	return true;
}

bool cpiLua::OnParsedMsgMCTo(cConnDC *conn, cMessageDC *msg)
{
	if ((conn != NULL) && (conn->mpUser != NULL) && (msg != NULL)) {
		char * args[] = {
			(char *)conn->mpUser->mNick.c_str(),
			(char *)msg->ChunkString(eCH_MCTO_MSG).c_str(),
			(char *)msg->ChunkString(eCH_MCTO_TO).c_str(),
			NULL
		}; // eCH_MCTO_ALL, eCH_MCTO_TO, eCH_MCTO_FROM, eCH_MCTO_CHMSG, eCH_MCTO_NICK, eCH_MCTO_MSG

		return CallAll("VH_OnParsedMsgMCTo", args, conn);
	}

	return true;
}

bool cpiLua::OnParsedMsgSupport(cConnDC *conn, cMessageDC *msg)
{
	if ((conn != NULL) && (msg != NULL)) {
	    char * args[] = {
			(char *)conn->AddrIP().c_str(),
			(char *)msg->mStr.c_str(),
			NULL
		};

		return CallAll("VH_OnParsedMsgSupport", args, conn);
	}

	return true;
}

bool cpiLua::OnParsedMsgBotINFO(cConnDC *conn, cMessageDC *msg)
{
	if ((conn != NULL) && (conn->mpUser != NULL) && (msg != NULL)) {
	    char * args[] = {
			(char *)conn->mpUser->mNick.c_str(),
			(char *)msg->mStr.c_str(),
			NULL
		};

		return CallAll("VH_OnParsedMsgBotINFO", args, conn);
	}

	return true;
}

bool cpiLua::OnParsedMsgVersion(cConnDC *conn, cMessageDC *msg)
{
	if ((conn != NULL) && (msg != NULL)) {
	    char * args[] = {
			(char *)conn->AddrIP().c_str(),
			(char *)msg->mStr.c_str(),
			NULL
		};

		return CallAll("VH_OnParsedMsgVersion", args, conn);
	}

	return true;
}

bool cpiLua::OnParsedMsgMyPass(cConnDC *conn, cMessageDC *msg)
{
	if ((conn != NULL) && (conn->mpUser != NULL) && (msg != NULL)) {
		char * args[] = {
			(char *)conn->mpUser->mNick.c_str(),
			//(char *)msg->ChunkString(eCH_1_ALL).c_str(), // dont show pass itself
			NULL
		}; // eCH_1_ALL, eCH_1_PARAM

		return CallAll("VH_OnParsedMsgMyPass", args, conn);
	}

	return true;
}

bool cpiLua::OnParsedMsgRevConnectToMe(cConnDC *conn, cMessageDC *msg)
{
	if ((conn != NULL) && (conn->mpUser != NULL) && (msg != NULL)) {
		char * args[] = {
			(char *)conn->mpUser->mNick.c_str(),
			(char *)msg->ChunkString(eCH_RC_OTHER).c_str(),
			NULL
		};

		return CallAll("VH_OnParsedMsgRevConnectToMe", args, conn);
	}

	return true;
}

bool cpiLua::OnParsedMsgConnectToMe(cConnDC *conn, cMessageDC *msg)
{
	if ((conn != NULL) && (conn->mpUser != NULL) && (msg != NULL)) {
		char * args[] = {
			(char *)conn->mpUser->mNick.c_str(),
			(char *)msg->ChunkString(eCH_CM_NICK).c_str(),
			(char *)msg->ChunkString(eCH_CM_IP).c_str(),
			(char *)msg->ChunkString(eCH_CM_PORT).c_str(),
			NULL
		}; // eCH_CM_NICK, eCH_CM_ACTIVE, eCH_CM_IP, eCH_CM_PORT

		return CallAll("VH_OnParsedMsgConnectToMe", args, conn);
	}

	return true;
}

bool cpiLua::OnParsedMsgSearch(cConnDC *conn, cMessageDC *msg)
{
	if ((conn != NULL) && (conn->mpUser != NULL) && (msg != NULL)) {
		char * args[] = {
			(char *)conn->mpUser->mNick.c_str(),
			(char *)msg->ChunkString(eCH_AS_ALL).c_str(),
			NULL
		}; // active: eCH_AS_ALL, eCH_AS_ADDR, eCH_AS_IP, eCH_AS_PORT, eCH_AS_QUERY; passive: eCH_PS_ALL, eCH_PS_NICK, eCH_PS_QUERY

		return CallAll("VH_OnParsedMsgSearch", args, conn);
	}

	return true;
}

bool cpiLua::OnParsedMsgSR(cConnDC *conn, cMessageDC *msg)
{
	if ((conn != NULL) && (conn->mpUser != NULL) && (msg != NULL)) {
		char * args[] = {
			(char *)conn->mpUser->mNick.c_str(),
			(char *)msg->ChunkString(eCH_SR_ALL).c_str(),
			NULL
		}; // eCH_SR_ALL, eCH_SR_FROM, eCH_SR_PATH, eCH_SR_SIZE, eCH_SR_SLOTS, eCH_SR_SL_FR, eCH_SR_SL_TO, eCH_SR_HUBINFO, eCH_SR_TO

		return CallAll("VH_OnParsedMsgSR", args, conn);
	}

	return true;
}

bool cpiLua::OnParsedMsgMyINFO(cConnDC *conn, cMessageDC *msg)
{
	if ((conn != NULL) && (conn->mpUser != NULL) && (msg != NULL)) {
		char * args[] = {
			(char *)conn->mpUser->mNick.c_str(),
			(char *)msg->ChunkString(eCH_MI_ALL).c_str(),
			NULL
		}; // eCH_MI_ALL, eCH_MI_DEST, eCH_MI_NICK, eCH_MI_INFO, eCH_MI_DESC, eCH_MI_SPEED, eCH_MI_MAIL, eCH_MI_SIZE

		return CallAll("VH_OnParsedMsgMyINFO", args, conn);
	}

	return true;
}

bool cpiLua::OnFirstMyINFO(cConnDC *conn, cMessageDC *msg)
{
	if ((conn != NULL) && (conn->mpUser != NULL) && (msg != NULL)) {
		char * args[] = {
			(char *)msg->ChunkString(eCH_MI_NICK).c_str(),
			(char *)msg->ChunkString(eCH_MI_DESC).c_str(),
			(char *)msg->ChunkString(eCH_MI_SPEED).c_str(),
			(char *)msg->ChunkString(eCH_MI_MAIL).c_str(),
			(char *)msg->ChunkString(eCH_MI_SIZE).c_str(),
			NULL
		}; // eCH_MI_ALL, eCH_MI_DEST, eCH_MI_NICK, eCH_MI_INFO, eCH_MI_DESC, eCH_MI_SPEED, eCH_MI_MAIL, eCH_MI_SIZE

		return CallAll("VH_OnFirstMyINFO", args, conn);
	}

	return true;
}

bool cpiLua::OnParsedMsgValidateNick(cConnDC *conn, cMessageDC *msg)
{
	if ((conn != NULL) && (conn->mpUser != NULL) && (msg != NULL)) {
		char * args[] = {
			(char *)msg->ChunkString(eCH_1_ALL).c_str(),
			(char *)conn->AddrIP().c_str(),
			NULL
		}; // eCH_1_ALL, eCH_1_PARAM

		return CallAll("VH_OnParsedMsgValidateNick", args, conn);
	}

	return true;
}

bool cpiLua::OnParsedMsgAny(cConnDC *conn, cMessageDC *msg)
{
	if ((conn != NULL) && (conn->mpUser != NULL) && (msg != NULL)) {
		char * args[] = {
		    (char *)conn->mpUser->mNick.c_str(),
			(char *)msg->mStr.c_str(),
			NULL
		};

		return CallAll("VH_OnParsedMsgAny", args, conn);
	}

	return true;
}

bool cpiLua::OnParsedMsgAnyEx(cConnDC *conn, cMessageDC *msg)
{
	if ((conn != NULL) && (conn->mpUser == NULL) && (msg != NULL)) {
		char * args[] = {
			(char *)conn->AddrIP().c_str(),
			(char *)msg->mStr.c_str(),
			(char *)toString(conn->AddrPort()),
			(char *)toString(conn->GetServPort()),
			NULL
		};

		return CallAll("VH_OnParsedMsgAnyEx", args, conn);
	}

	return true;
}

bool cpiLua::OnUnknownMsg(cConnDC *conn, cMessageDC *msg)
{
	if ((conn != NULL) && (msg != NULL) && (msg->mStr.size() > 0)) {
		if (conn->mpUser != NULL) {
			char * args[] = {
				(char *)conn->mpUser->mNick.c_str(),
				(char *)msg->mStr.c_str(),
				(char *)toString(1),
				(char *)conn->AddrIP().c_str(),
				NULL
			};

			return CallAll("VH_OnUnknownMsg", args, conn);
		} else {
			char * args[] = {
				(char *)conn->AddrIP().c_str(),
				(char *)msg->mStr.c_str(),
				(char *)toString(0),
				NULL
			};

			return CallAll("VH_OnUnknownMsg", args, conn);
		}
	}

	return true;
}

/*
bool cpiLua::OnUnparsedMsg(cConnDC *conn, cMessageDC *msg)
{
	if ((conn != NULL) && (msg != NULL) && (msg->mStr.size() > 0)) {
		if (conn->mpUser != NULL) {
			char * args[] = {
				(char *)conn->mpUser->mNick.c_str(),
				(char *)msg->mStr.c_str(),
				(char *)toString(1),
				NULL
			};

			return CallAll("VH_OnUnparsedMsg", args, conn);
		} else {
			char * args[] = {
				(char *)conn->AddrIP().c_str(),
				(char *)msg->mStr.c_str(),
				(char *)toString(0),
				NULL
			};

			return CallAll("VH_OnUnparsedMsg", args, conn);
		}
	}

	return true;
}
*/

bool cpiLua::OnOperatorKicks(cUser *OP, cUser *user, string *reason)
{
	if ((OP != NULL) && (user != NULL) && (reason != NULL)) {
		char * args[] = {
			(char *)OP->mNick.c_str(),
			(char *)user->mNick.c_str(),
			(char *)reason->c_str(),
			NULL
		};

		return CallAll("VH_OnOperatorKicks", args, OP->mxConn);
	}

	return true;
}

bool cpiLua::OnOperatorDrops(cUser *OP, cUser *user)
{
	if ((OP != NULL) && (user != NULL)) {
		char * args[] = {
			(char *)OP->mNick.c_str(),
			(char *)user->mNick.c_str(),
			NULL
		};

		return CallAll("VH_OnOperatorDrops", args, OP->mxConn);
	}

	return true;
}

bool cpiLua::OnOperatorCommand(cConnDC *conn, string *command)
{
	if ((conn != NULL) && (conn->mpUser != NULL) && (command != NULL)) {
		if (mConsole.DoCommand(*command, conn)) return false;

		char * args[] = {
			(char *)conn->mpUser->mNick.c_str(),
			(char *)command->c_str(),
			NULL
		};

		return CallAll("VH_OnOperatorCommand", args, conn);
	}

	return true;
}

bool cpiLua::OnUserCommand(cConnDC *conn, string *command)
{
	if ((conn != NULL) && (conn->mpUser != NULL) && (command != NULL)) {
		char * args[] = {
			(char *)conn->mpUser->mNick.c_str(),
			(char *)command->c_str(),
			NULL
		};

		return CallAll("VH_OnUserCommand", args, conn);
	}

	return true;
}

bool cpiLua::OnHubCommand(cConnDC *conn, string *command, int op, int pm)
{
	if ((conn != NULL) && (conn->mpUser != NULL) && (command != NULL)) {
		char * args[] = {
			(char *)conn->mpUser->mNick.c_str(),
			(char *)command->c_str(),
			(char *)toString(op),
			(char *)toString(pm),
			NULL
		};

		return CallAll("VH_OnHubCommand", args, conn);
	}

	return true;
}

bool cpiLua::OnValidateTag(cConnDC *conn, cDCTag *tag)
{
	if ((conn != NULL) && (conn->mpUser != NULL) && (tag != NULL)) {
		char * args[] = {
			(char *)conn->mpUser->mNick.c_str(),
			(char *)tag->mTag.c_str(),
			NULL
		};

		return CallAll("VH_OnValidateTag", args, conn);
	}

	return true;
}

bool cpiLua::OnUserLogin(cUser *user)
{
	if (user != NULL) {
		if (user->mxConn != NULL) {
			char * args[] = {
				(char *)user->mNick.c_str(),
				(char *)user->mxConn->AddrIP().c_str(),
				NULL
			};

			return CallAll("VH_OnUserLogin", args, user->mxConn);
		} else {
			char * args[] = {
				(char *)user->mNick.c_str(),
				NULL
			};

			return CallAll("VH_OnUserLogin", args);
		}
	}

	return true;
}

bool cpiLua::OnUserLogout(cUser *user)
{
	if (user != NULL) {
		if (user->mxConn != NULL) {
			char * args[] = {
				(char *)user->mNick.c_str(),
				(char *)user->mxConn->AddrIP().c_str(),
				NULL
			};

			return CallAll("VH_OnUserLogout", args, user->mxConn);
		} else {
			char * args[] = {
				(char *)user->mNick.c_str(),
				NULL
			};

			return CallAll("VH_OnUserLogout", args);
		}
	}

	return true;
}

bool cpiLua::OnTimer(long msec)
{
	char * args[] = {
		(char *)longToString(msec),
		NULL
	};

	return CallAll("VH_OnTimer", args);
}

bool cpiLua::OnNewReg(cUser *user, string mNick, int mClass)
{
	if (user != NULL) {
		char * args[] = {
			(char *)mNick.c_str(),
			(char *)toString(mClass),
			(char *)user->mNick.c_str(),
			NULL
		};

		return CallAll("VH_OnNewReg", args, user->mxConn);
	}

	return true;
}

bool cpiLua::OnDelReg(cUser *user, string mNick, int mClass)
{
	if (user != NULL) {
		char * args[] = {
			(char *)mNick.c_str(),
			(char *)toString(mClass),
			(char *)user->mNick.c_str(),
			NULL
		};

		return CallAll("VH_OnDelReg", args, user->mxConn);
	}

	return true;
}

bool cpiLua::OnUpdateClass(cUser *user, string mNick, int oldClass, int newClass)
{
	if (user != NULL) {
		char * args[] = {
			(char *)mNick.c_str(),
			(char *)toString(oldClass),
			(char *)toString(newClass),
			(char *)user->mNick.c_str(),
			NULL
		};

		return CallAll("VH_OnUpdateClass", args, user->mxConn);
	}

	return true;
}

bool cpiLua::OnNewBan(cUser *user, cBan *ban)
{
	if ((user != NULL) && (ban != NULL)) {
		string ranmin, ranmax;

		if (ban->mRangeMin > 0)
			cBanList::Num2Ip(ban->mRangeMin, ranmin);

		if (ban->mRangeMax > 0)
			cBanList::Num2Ip(ban->mRangeMax, ranmax);

		char * args[] = {
			(char *)ban->mIP.c_str(),
			(char *)ban->mNick.c_str(),
			(char *)ban->mHost.c_str(),
			(char *)(ban->mShare > 0 ? longToString(ban->mShare) : ""),
			(char *)ranmin.c_str(),
			(char *)ranmax.c_str(),
			(char *)toString(ban->mType),
			(char *)longToString(ban->mDateEnd),
			(char *)ban->mReason.c_str(),
			(char *)ban->mNickOp.c_str(),
			NULL
		};

		return CallAll("VH_OnNewBan", args, user->mxConn);
	}

	return true;
}

bool cpiLua::OnUnBan(cUser *user, string nick, string op, string reason)
{
	if (user != NULL) {
		char * args[] = {
			(char *)nick.c_str(),
			(char *)op.c_str(),
			(char *)reason.c_str(),
			NULL
		};

		return CallAll("VH_OnUnBan", args, user->mxConn);
	}

	return true;
}

bool cpiLua::OnScriptCommand(string *cmd, string *data, string *plug, string *script)
{
	if (cmd && data && plug && script) {
		char *args[] = {
			(char*)cmd->c_str(),
			(char*)data->c_str(),
			(char*)plug->c_str(),
			(char*)script->c_str(),
			NULL
		};

		CallAll("VH_OnScriptCommand", args);
	}

	return true;
}

bool cpiLua::OnCtmToHub(cConnDC *conn, string *ref)
{
	if (conn && ref) {
		char *args[] = {
			(char*)conn->mMyNick.c_str(),
			(char*)conn->AddrIP().c_str(),
			(char*)toString(conn->AddrPort()),
			(char*)toString(conn->GetServPort()),
			(char*)ref->c_str(),
			NULL
		};

		return CallAll("VH_OnCtmToHub", args, conn);
	}

	return true;
}

bool cpiLua::OnOpChatMessage(string *nick, string *data)
{
	if (nick && data) {
		char *args[] = {
			(char*)nick->c_str(),
			(char*)data->c_str(),
			NULL
		};

		CallAll("VH_OnOpChatMessage", args);
	}

	return true;
}

bool cpiLua::OnPublicBotMessage(string *nick, string *data, int min_class, int max_class)
{
	if (nick && data) {
		char *args[] = {
			(char*)nick->c_str(),
			(char*)data->c_str(),
			(char*)toString(min_class),
			(char*)toString(max_class),
			NULL
		};

		CallAll("VH_OnPublicBotMessage", args);
	}

	return true;
}

bool cpiLua::OnUnLoad(long code)
{
	char *args[] = {
		(char*)toString(code),
		NULL
	};

	CallAll("VH_OnUnLoad", args);
	return true;
}

char * cpiLua::toString(int num)
{
	return autosprintf("%d", num);
}

char * cpiLua::longToString(long num)
{
	return autosprintf("%lu", num);
}

	}; // namepsace nLuaPlugin
}; // namespace nVerliHub

REGISTER_PLUGIN(nVerliHub::nLuaPlugin::cpiLua);
