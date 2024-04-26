/*
	Copyright (C) 2003-2005 Daniel Muller, dan at verliba dot cz
	Copyright (C) 2006-2024 Verlihub Team, info at verlihub dot net

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
#include "src/cdcproto.h"
#include "src/cserverdc.h"
#include "src/i18n.h"
#include "src/script_api.h"
#include <dirent.h>
#include <string>
#include <sstream>
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
	const char *args[] = { NULL };
	CallAll("UnLoad", args);

	if (mQuery != NULL) {
		mQuery->Clear();
		delete mQuery;
		mQuery = NULL;
	}

	this->Empty();
}

void cpiLua::SetLogLevel(int level)
{
	ostringstream val;
	val << level;
	SetConfig("pi_lua", "log_level", val.str().c_str());
	this->log_level = level;
}

void cpiLua::SetErrClass(int eclass)
{
	ostringstream val;
	val << eclass;
	SetConfig("pi_lua", "err_class", val.str().c_str());
	this->err_class = eclass;
}

void cpiLua::ReportLuaError(const string &err)
{
	if (ErrLog(0))
		LogStream() << err << endl;
}

void cpiLua::OnLoad(cServerDC *serv)
{
	cVHPlugin::OnLoad(serv);
	mQuery = new cQuery(serv->mMySQL);
	this->server = serv;
	mScriptDir = serv->mConfigBaseDir + "/scripts/";

	ostringstream def;
	def << this->log_level;
	char *level = GetConfig("pi_lua", "log_level", def.str().c_str()); // get log level

	if (level) {
		if (IsNumber(level))
			this->log_level = atoi(level);

		free(level);
	}

	def.str("");
	def << this->err_class;
	char *eclass = GetConfig("pi_lua", "err_class", def.str().c_str()); // get error class

	if (eclass) {
		if (IsNumber(eclass))
			this->err_class = atoi(eclass);

		free(eclass);
	}

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
	RegisterCallBack("VH_OnParsedMsgSupports");
	RegisterCallBack("VH_OnParsedMsgMyHubURL");
	RegisterCallBack("VH_OnParsedMsgExtJSON");
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
	RegisterCallBack("VH_OnUserInList");
	RegisterCallBack("VH_OnUserLogin");
	RegisterCallBack("VH_OnUserLogout");
	RegisterCallBack("VH_OnCloneCountLow");
	RegisterCallBack("VH_OnTimer");
	RegisterCallBack("VH_OnNewReg");
	RegisterCallBack("VH_OnDelReg");
	RegisterCallBack("VH_OnBadPass");
	RegisterCallBack("VH_OnNewBan");
	RegisterCallBack("VH_OnUnBan");
	RegisterCallBack("VH_OnSetConfig");
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
		LogStream() << "Opening Lua script directory: " << mScriptDir << endl;

	DIR *dir = opendir(mScriptDir.c_str());

	if (!dir) {
		if (Log(1))
			LogStream() << "Error opening Lua script directory: " << mScriptDir << endl;

		return false;
	}

	string pathname, filename, config("config");

	if (server)
		config = server->mDBConf.config_name;

	struct dirent *dent = NULL;
	vector<string> filenames;

	while (NULL != (dent = readdir(dir))) {
		filename = dent->d_name;

		if ((filename.size() > 4) && (StrCompare(filename, filename.size() - 4, 4, ".lua") == 0))
			filenames.push_back(filename);
	}

	closedir(dir);
	sort(filenames.begin(), filenames.end());
	cLuaInterpreter *ip;

	for (size_t i = 0; i < filenames.size(); i++) {
		filename = filenames[i];
		pathname = mScriptDir + filename;

		try {
			ip = new cLuaInterpreter(config, pathname);
		} catch (const char *ex) {
			if (Log(1))
				LogStream() << "Failed creating cLuaInterpreter for script: " << filename << " [ " << ex << " ]" << endl;

			continue;
		}

		if (ip->Init()) {
			AddData(ip);
			ip->Load();

			if (Log(1))
				LogStream() << "Success loading and parsing Lua script: " << filename << endl;
		} else {
			if (Log(1))
				LogStream() << "Failed loading or parsing Lua script: " << filename << endl;

			delete ip;
			ip = NULL;
		}
	}

	return true;
}

bool cpiLua::CallAll(const char *fncname, const char *args[], cConnDC *conn)
{
	bool ret = true;

	if (Size()) {
		tvLuaInterpreter::iterator it;

		for (it = mLua.begin(); it != mLua.end(); ++it) {
			if (*it) {
				if (!(*it)->CallFunction(fncname, args, conn))
					ret = false;
			}
		}
	}

	return ret;
}

bool cpiLua::OnNewConn(cConnDC *conn)
{
	bool res = true;

	if (conn) {
		const char *args[] = {
			conn->AddrIP().c_str(),
			toString(conn->AddrPort()),
			conn->GetServAddr().c_str(),
			toString(conn->GetServPort()),
			NULL
		};

		res = CallAll("VH_OnNewConn", args, conn);
		delete [] args[1];
		delete [] args[3];
	}

	return res;
}

bool cpiLua::OnCloseConn(cConnDC *conn)
{
	if (conn != NULL) {
		if (conn->mpUser != NULL) {
			const char *args[] = {
				conn->AddrIP().c_str(),
				conn->mpUser->mNick.c_str(),
				NULL
			};

			return CallAll("VH_OnCloseConn", args, conn);
		} else {
			const char *args[] = {
				conn->AddrIP().c_str(),
				NULL
			};

			return CallAll("VH_OnCloseConn", args, conn);
		}
	}

	return true;
}

bool cpiLua::OnCloseConnEx(cConnDC *conn)
{
	bool res = true;

	if (conn) {
		if (conn->mpUser) {
			const char *args[] = {
				conn->AddrIP().c_str(),
				toString(conn->mCloseReason),
				conn->mpUser->mNick.c_str(),
				NULL
			};

			res = CallAll("VH_OnCloseConnEx", args, conn);
			delete [] args[1];
		} else {
			const char *args[] = {
				conn->AddrIP().c_str(),
				toString(conn->mCloseReason),
				NULL
			};

			res = CallAll("VH_OnCloseConnEx", args, conn);
			delete [] args[1];
		}
	}

	return res;
}

bool cpiLua::OnParsedMsgChat(cConnDC *conn, cMessageDC *msg)
{
	if ((conn != NULL) && (conn->mpUser != NULL) && (msg != NULL)) {
		const char *args[] = {
			conn->mpUser->mNick.c_str(),
			msg->ChunkString(eCH_CH_MSG).c_str(),
			NULL
		}; // eCH_CH_ALL, eCH_CH_NICK, eCH_CH_MSG

		return CallAll("VH_OnParsedMsgChat", args, conn);
	}

	return true;
}

bool cpiLua::OnParsedMsgPM(cConnDC *conn, cMessageDC *msg)
{
	if ((conn != NULL) && (conn->mpUser != NULL) && (msg != NULL)) {
		const char *args[] = {
			conn->mpUser->mNick.c_str(),
			msg->ChunkString(eCH_PM_MSG).c_str(),
			msg->ChunkString(eCH_PM_TO).c_str(),
			NULL
		}; // eCH_PM_ALL, eCH_PM_TO, eCH_PM_FROM, eCH_PM_CHMSG, eCH_PM_NICK, eCH_PM_MSG

		return CallAll("VH_OnParsedMsgPM", args, conn);
	}

	return true;
}

bool cpiLua::OnParsedMsgMCTo(cConnDC *conn, cMessageDC *msg)
{
	if ((conn != NULL) && (conn->mpUser != NULL) && (msg != NULL)) {
		const char *args[] = {
			conn->mpUser->mNick.c_str(),
			msg->ChunkString(eCH_MCTO_MSG).c_str(),
			msg->ChunkString(eCH_MCTO_TO).c_str(),
			NULL
		}; // eCH_MCTO_ALL, eCH_MCTO_TO, eCH_MCTO_CHMSG, eCH_MCTO_NICK, eCH_MCTO_MSG

		return CallAll("VH_OnParsedMsgMCTo", args, conn);
	}

	return true;
}

bool cpiLua::OnParsedMsgSupports(cConnDC *conn, cMessageDC *msg, string *back)
{
	if (conn && msg && back) {
		const char *args[] = {
			conn->AddrIP().c_str(),
			msg->mStr.c_str(),
			back->c_str(),
			NULL
		};

		return CallAll("VH_OnParsedMsgSupports", args, conn);
	}

	return true;
}

bool cpiLua::OnParsedMsgMyHubURL(cConnDC *conn, cMessageDC *msg)
{
	if (conn && conn->mpUser && msg) {
		const char *args[] = {
			conn->mpUser->mNick.c_str(),
			msg->mStr.c_str(),
			NULL
		};

		return CallAll("VH_OnParsedMsgMyHubURL", args, conn);
	}

	return true;
}

bool cpiLua::OnParsedMsgExtJSON(cConnDC *conn, cMessageDC *msg)
{
	if (conn && conn->mpUser && msg) {
		const char *args[] = {
			conn->mpUser->mNick.c_str(),
			msg->mStr.c_str(),
			NULL
		};

		return CallAll("VH_OnParsedMsgExtJSON", args, conn);
	}

	return true;
}

bool cpiLua::OnParsedMsgBotINFO(cConnDC *conn, cMessageDC *msg)
{
	if ((conn != NULL) && (conn->mpUser != NULL) && (msg != NULL)) {
		const char *args[] = {
			conn->mpUser->mNick.c_str(),
			msg->mStr.c_str(),
			NULL
		};

		return CallAll("VH_OnParsedMsgBotINFO", args, conn);
	}

	return true;
}

bool cpiLua::OnParsedMsgVersion(cConnDC *conn, cMessageDC *msg)
{
	if ((conn != NULL) && (msg != NULL)) {
		const char *args[] = {
			conn->AddrIP().c_str(),
			msg->mStr.c_str(),
			NULL
		};

		return CallAll("VH_OnParsedMsgVersion", args, conn);
	}

	return true;
}

bool cpiLua::OnParsedMsgMyPass(cConnDC *conn, cMessageDC *msg)
{
	if ((conn != NULL) && (conn->mpUser != NULL) && (msg != NULL)) {
		const char *args[] = {
			conn->mpUser->mNick.c_str(),
			//msg->ChunkString(eCH_1_ALL).c_str(), // dont show pass itself
			NULL
		}; // eCH_1_ALL, eCH_1_PARAM

		return CallAll("VH_OnParsedMsgMyPass", args, conn);
	}

	return true;
}

bool cpiLua::OnParsedMsgRevConnectToMe(cConnDC *conn, cMessageDC *msg)
{
	if ((conn != NULL) && (conn->mpUser != NULL) && (msg != NULL)) {
		const char *args[] = {
			conn->mpUser->mNick.c_str(),
			msg->ChunkString(eCH_RC_OTHER).c_str(),
			NULL
		};

		return CallAll("VH_OnParsedMsgRevConnectToMe", args, conn);
	}

	return true;
}

bool cpiLua::OnParsedMsgConnectToMe(cConnDC *conn, cMessageDC *msg)
{
	if ((conn != NULL) && (conn->mpUser != NULL) && (msg != NULL)) {
		const char *args[] = {
			conn->mpUser->mNick.c_str(),
			msg->ChunkString(eCH_CM_NICK).c_str(),
			msg->ChunkString(eCH_CM_IP).c_str(),
			msg->ChunkString(eCH_CM_PORT).c_str(),
			NULL
		}; // eCH_CM_NICK, eCH_CM_ACTIVE, eCH_CM_IP, eCH_CM_PORT

		return CallAll("VH_OnParsedMsgConnectToMe", args, conn);
	}

	return true;
}

bool cpiLua::OnParsedMsgSearch(cConnDC *conn, cMessageDC *msg)
{
	if (conn && conn->mpUser && msg) {
		string data;

		switch (msg->mType) {
			case eDC_SEARCH_PAS:
				data = msg->ChunkString(eCH_PS_ALL);
				break;

			case eDC_SEARCH:
				data = msg->ChunkString(eCH_AS_ALL);
				break;

			case eDC_TTHS:
				server->mP.Create_Search(data, msg->ChunkString(eCH_SA_ADDR), msg->ChunkString(eCH_SA_TTH), false);
				break;

			case eDC_TTHS_PAS:
				server->mP.Create_Search(data, msg->ChunkString(eCH_SP_NICK), msg->ChunkString(eCH_SP_TTH), true);
				break;

			default:
				return true;
		}

		const char *args[] = {
			conn->mpUser->mNick.c_str(),
			data.c_str(),
			NULL
		};

		return CallAll("VH_OnParsedMsgSearch", args, conn);
	}

	return true;
}

bool cpiLua::OnParsedMsgSR(cConnDC *conn, cMessageDC *msg)
{
	if ((conn != NULL) && (conn->mpUser != NULL) && (msg != NULL)) {
		const char *args[] = {
			conn->mpUser->mNick.c_str(),
			msg->ChunkString(eCH_SR_ALL).c_str(),
			NULL
		}; // eCH_SR_ALL, eCH_SR_FROM, eCH_SR_PATH, eCH_SR_SIZE, eCH_SR_SLOTS, eCH_SR_SL_FR, eCH_SR_SL_TO, eCH_SR_HUBINFO, eCH_SR_TO

		return CallAll("VH_OnParsedMsgSR", args, conn);
	}

	return true;
}

bool cpiLua::OnParsedMsgMyINFO(cConnDC *conn, cMessageDC *msg)
{
	if ((conn != NULL) && (conn->mpUser != NULL) && (msg != NULL)) {
		const char *args[] = {
			conn->mpUser->mNick.c_str(),
			msg->ChunkString(eCH_MI_ALL).c_str(),
			NULL
		}; // eCH_MI_ALL, eCH_MI_DEST, eCH_MI_NICK, eCH_MI_INFO, eCH_MI_DESC, eCH_MI_SPEED, eCH_MI_MAIL, eCH_MI_SIZE

		return CallAll("VH_OnParsedMsgMyINFO", args, conn);
	}

	return true;
}

bool cpiLua::OnFirstMyINFO(cConnDC *conn, cMessageDC *msg)
{
	if ((conn != NULL) && (conn->mpUser != NULL) && (msg != NULL)) {
		const char *args[] = {
			msg->ChunkString(eCH_MI_NICK).c_str(),
			msg->ChunkString(eCH_MI_DESC).c_str(),
			msg->ChunkString(eCH_MI_SPEED).c_str(),
			msg->ChunkString(eCH_MI_MAIL).c_str(),
			msg->ChunkString(eCH_MI_SIZE).c_str(),
			NULL
		}; // eCH_MI_ALL, eCH_MI_DEST, eCH_MI_NICK, eCH_MI_INFO, eCH_MI_DESC, eCH_MI_SPEED, eCH_MI_MAIL, eCH_MI_SIZE

		return CallAll("VH_OnFirstMyINFO", args, conn);
	}

	return true;
}

bool cpiLua::OnParsedMsgValidateNick(cConnDC *conn, cMessageDC *msg)
{
	if ((conn != NULL) && (conn->mpUser != NULL) && (msg != NULL)) {
		const char *args[] = {
			msg->ChunkString(eCH_1_ALL).c_str(),
			conn->AddrIP().c_str(),
			NULL
		}; // eCH_1_ALL, eCH_1_PARAM

		return CallAll("VH_OnParsedMsgValidateNick", args, conn);
	}

	return true;
}

bool cpiLua::OnParsedMsgAny(cConnDC *conn, cMessageDC *msg)
{
	if ((conn != NULL) && (conn->mpUser != NULL) && (msg != NULL)) {
		const char *args[] = {
			conn->mpUser->mNick.c_str(),
			msg->mStr.c_str(),
			NULL
		};

		return CallAll("VH_OnParsedMsgAny", args, conn);
	}

	return true;
}

bool cpiLua::OnParsedMsgAnyEx(cConnDC *conn, cMessageDC *msg)
{
	bool res = true;

	if (conn && msg && !conn->mpUser) {
		const char *args[] = {
			conn->AddrIP().c_str(),
			msg->mStr.c_str(),
			toString(conn->AddrPort()),
			toString(conn->GetServPort()),
			NULL
		};

		res = CallAll("VH_OnParsedMsgAnyEx", args, conn);
		delete [] args[2];
		delete [] args[3];
	}

	return res;
}

bool cpiLua::OnUnknownMsg(cConnDC *conn, cMessageDC *msg)
{
	bool res = true;

	if (conn && msg && msg->mStr.size()) {
		if (conn->mpUser && conn->mpUser->mInList) { // only after login
			const char *args[] = {
				conn->mpUser->mNick.c_str(),
				msg->mStr.c_str(),
				"1",
				conn->AddrIP().c_str(),
				NULL
			};

			res = CallAll("VH_OnUnknownMsg", args, conn);
		} else {
			const char *args[] = {
				conn->AddrIP().c_str(),
				msg->mStr.c_str(),
				"0",
				NULL
			};

			res = CallAll("VH_OnUnknownMsg", args, conn);
		}
	}

	return res;
}

/*
bool cpiLua::OnUnparsedMsg(cConnDC *conn, cMessageDC *msg)
{
	bool res = true;

	if (conn && msg && msg->mStr.size()) {
		if (conn->mpUser) {
			const char *args[] = {
				conn->mpUser->mNick.c_str(),
				msg->mStr.c_str(),
				"1",
				NULL
			};

			res = CallAll("VH_OnUnparsedMsg", args, conn);
		} else {
			const char *args[] = {
				conn->AddrIP().c_str(),
				msg->mStr.c_str(),
				"0",
				NULL
			};

			res = CallAll("VH_OnUnparsedMsg", args, conn);
		}
	}

	return res;
}
*/

bool cpiLua::OnOperatorKicks(cUser *op, cUser *user, string *why)
{
	if (op && user && why) {
		const char *args[] = {
			op->mNick.c_str(),
			user->mNick.c_str(),
			why->c_str(),
			NULL
		};

		return CallAll("VH_OnOperatorKicks", args, op->mxConn);
	}

	return true;
}

bool cpiLua::OnOperatorDrops(cUser *op, cUser *user, string *why)
{
	if (op && user && why) {
		const char *args[] = {
			op->mNick.c_str(),
			user->mNick.c_str(),
			why->c_str(),
			NULL
		};

		return CallAll("VH_OnOperatorDrops", args, op->mxConn);
	}

	return true;
}

bool cpiLua::OnOperatorCommand(cConnDC *conn, string *command)
{
	if ((conn != NULL) && (conn->mpUser != NULL) && (command != NULL)) {
		if (mConsole.DoCommand(*command, conn)) return false;

		const char *args[] = {
			conn->mpUser->mNick.c_str(),
			command->c_str(),
			NULL
		};

		return CallAll("VH_OnOperatorCommand", args, conn);
	}

	return true;
}

bool cpiLua::OnUserCommand(cConnDC *conn, string *command)
{
	if ((conn != NULL) && (conn->mpUser != NULL) && (command != NULL)) {
		const char *args[] = {
			conn->mpUser->mNick.c_str(),
			command->c_str(),
			NULL
		};

		return CallAll("VH_OnUserCommand", args, conn);
	}

	return true;
}

bool cpiLua::OnHubCommand(cConnDC *conn, string *command, int op, int pm)
{
	bool res = true;

	if (conn && conn->mpUser && command) {
		const char *args[] = {
			conn->mpUser->mNick.c_str(),
			command->c_str(),
			toString(op),
			toString(pm),
			NULL
		};

		res = CallAll("VH_OnHubCommand", args, conn);
		delete [] args[2];
		delete [] args[3];
	}

	return res;
}

bool cpiLua::OnValidateTag(cConnDC *conn, cDCTag *tag)
{
	if ((conn != NULL) && (conn->mpUser != NULL) && (tag != NULL)) {
		const char *args[] = {
			conn->mpUser->mNick.c_str(),
			tag->mTag.c_str(),
			NULL
		};

		return CallAll("VH_OnValidateTag", args, conn);
	}

	return true;
}

bool cpiLua::OnUserInList(cUser *user)
{
	if (user) {
		if (user->mxConn) {
			const char *args[] = {
				user->mNick.c_str(),
				user->mxConn->AddrIP().c_str(),
				NULL
			};

			return CallAll("VH_OnUserInList", args, user->mxConn);
		} else {
			const char *args[] = {
				user->mNick.c_str(),
				NULL
			};

			return CallAll("VH_OnUserInList", args);
		}
	}

	return true;
}

bool cpiLua::OnUserLogin(cUser *user)
{
	if (user) {
		if (user->mxConn) {
			const char *args[] = {
				user->mNick.c_str(),
				user->mxConn->AddrIP().c_str(),
				NULL
			};

			return CallAll("VH_OnUserLogin", args, user->mxConn);
		} else {
			const char *args[] = {
				user->mNick.c_str(),
				NULL
			};

			return CallAll("VH_OnUserLogin", args);
		}
	}

	return true;
}

bool cpiLua::OnUserLogout(cUser *user)
{
	if (user) {
		if (user->mxConn) {
			const char *args[] = {
				user->mNick.c_str(),
				user->mxConn->AddrIP().c_str(),
				NULL
			};

			return CallAll("VH_OnUserLogout", args, user->mxConn);
		} else {
			const char *args[] = {
				user->mNick.c_str(),
				NULL
			};

			return CallAll("VH_OnUserLogout", args);
		}
	}

	return true;
}

bool cpiLua::OnCloneCountLow(cUser *user, string nick, int count)
{
	bool res = true;

	if (!user)
		return res;

	const char *args[] = {
		user->mNick.c_str(),
		nick.c_str(),
		toString(count),
		NULL
	};

	if (user->mxConn)
		res = CallAll("VH_OnCloneCountLow", args, user->mxConn);
	else
		res = CallAll("VH_OnCloneCountLow", args);

	delete [] args[2];
	return res;
}

bool cpiLua::OnTimer(__int64 msec)
{
	std::stringstream ss;
	ss << msec;
	std::string s = ss.str();

	const char *args[] = {
		s.c_str(),
		NULL
	};

	bool res = CallAll("VH_OnTimer", args);
	return res;
}

bool cpiLua::OnNewReg(cUser *user, string mNick, int mClass)
{
	bool res = true;

	if (user) {
		const char *args[] = {
			mNick.c_str(),
			toString(mClass),
			user->mNick.c_str(),
			NULL
		};

		res = CallAll("VH_OnNewReg", args, user->mxConn);
		delete [] args[1];
	}

	return res;
}

bool cpiLua::OnDelReg(cUser *user, string mNick, int mClass)
{
	bool res = true;

	if (user) {
		const char *args[] = {
			mNick.c_str(),
			toString(mClass),
			user->mNick.c_str(),
			NULL
		};

		res = CallAll("VH_OnDelReg", args, user->mxConn);
		delete [] args[1];
	}

	return res;
}

bool cpiLua::OnUpdateClass(cUser *user, string mNick, int oldClass, int newClass)
{
	bool res = true;

	if (user) {
		const char *args[] = {
			mNick.c_str(),
			toString(oldClass),
			toString(newClass),
			user->mNick.c_str(),
			NULL
		};

		res = CallAll("VH_OnUpdateClass", args, user->mxConn);
		delete [] args[1];
		delete [] args[2];
	}

	return res;
}

bool cpiLua::OnBadPass(cUser *user)
{
	if (user && user->mxConn) {
		const char *args[] = {
			user->mNick.c_str(),
			user->mxConn->AddrIP().c_str(),
			NULL
		};

		return CallAll("VH_OnBadPass", args, user->mxConn);
	}

	return true;
}

bool cpiLua::OnNewBan(cUser *user, cBan *ban)
{
	bool res = true;

	if (user && ban) {
		string ranmin, ranmax;

		if (ban->mRangeMin > 0)
			cBanList::Num2Ip(ban->mRangeMin, ranmin);

		if (ban->mRangeMax > 0)
			cBanList::Num2Ip(ban->mRangeMax, ranmax);

		const char *args[] = {
			ban->mIP.c_str(),
			ban->mNick.c_str(),
			ban->mHost.c_str(),
			((ban->mShare > 0) ? longToString(ban->mShare) : ""),
			ranmin.c_str(),
			ranmax.c_str(),
			toString(ban->mType),
			longToString(ban->mDateEnd),
			ban->mReason.c_str(),
			ban->mNickOp.c_str(),
			NULL
		};

		res = CallAll("VH_OnNewBan", args, user->mxConn);

		if (ban->mShare > 0)
			delete [] args[3];

		delete [] args[6];
		delete [] args[7];
	}

	return res;
}

bool cpiLua::OnUnBan(cUser *user, string nick, string op, string reason)
{
	if (user != NULL) {
		const char *args[] = {
			nick.c_str(),
			op.c_str(),
			reason.c_str(),
			NULL
		};

		return CallAll("VH_OnUnBan", args, user->mxConn);
	}

	return true;
}

bool cpiLua::OnSetConfig(cUser *user, string *conf, string *var, string *val_new, string *val_old, int val_type)
{
	bool res = true;

	if (user && conf && var && val_new && val_old) {
		const char *args[] = {
			user->mNick.c_str(),
			conf->c_str(),
			var->c_str(),
			val_new->c_str(),
			val_old->c_str(),
			toString(val_type),
			NULL
		};

		res = CallAll("VH_OnSetConfig", args, user->mxConn);

		if (res && server && !strcmp(conf->c_str(), server->mDBConf.config_name.c_str()) && (!strcmp(var->c_str(), "hub_security") || !strcmp(var->c_str(), "opchat_name")) && Size()) {
			tvLuaInterpreter::iterator it;

			for (it = mLua.begin(); it != mLua.end(); ++it) {
				if (*it) {
					if (!strcmp(var->c_str(), "hub_security"))
						(*it)->VHPushString("HubSec", val_new->c_str(), true);
					else if (!strcmp(var->c_str(), "opchat_name"))
						(*it)->VHPushString("OpChat", val_new->c_str(), true);
				}
			}
		}

		delete [] args[5];
	}

	return res;
}

bool cpiLua::OnScriptCommand(string *cmd, string *data, string *plug, string *script)
{
	if (cmd && data && plug && script) {
		const char *args[] = {
			cmd->c_str(),
			data->c_str(),
			plug->c_str(),
			script->c_str(),
			NULL
		};

		CallAll("VH_OnScriptCommand", args);
	}

	return true;
}

bool cpiLua::OnCtmToHub(cConnDC *conn, string *ref)
{
	bool res = true;

	if (conn && ref) {
		const char *args[] = {
			conn->mMyNick.c_str(),
			conn->AddrIP().c_str(),
			toString(conn->AddrPort()),
			toString(conn->GetServPort()),
			ref->c_str(),
			NULL
		};

		res = CallAll("VH_OnCtmToHub", args, conn);
		delete [] args[2];
		delete [] args[3];
	}

	return res;
}

bool cpiLua::OnOpChatMessage(string *nick, string *data)
{
	if (nick && data) {
		const char *args[] = {
			nick->c_str(),
			data->c_str(),
			NULL
		};

		CallAll("VH_OnOpChatMessage", args);
	}

	return true;
}

bool cpiLua::OnPublicBotMessage(string *nick, string *data, int min_class, int max_class)
{
	bool res = true;

	if (nick && data) {
		const char *args[] = {
			nick->c_str(),
			data->c_str(),
			toString(min_class),
			toString(max_class),
			NULL
		};

		res = CallAll("VH_OnPublicBotMessage", args);
		delete [] args[2];
		delete [] args[3];
	}

	return res;
}

bool cpiLua::OnUnLoad(long code)
{
	const char *args[] = {
		longToString(code),
		NULL
	};

	bool res = CallAll("VH_OnUnLoad", args);
	delete [] args[0];
	return res;
}

char* cpiLua::toString(int num)
{
#ifdef USE_CUSTOM_AUTOSPRINTF
	return strdup(autosprintf("%d", num).c_str());
#else
	return autosprintf("%d", num);
#endif
}

char* cpiLua::longToString(long num)
{
#ifdef USE_CUSTOM_AUTOSPRINTF
	return strdup(autosprintf("%lu", num).c_str());
#else
	return autosprintf("%lu", num);
#endif
}

	}; // namepsace nLuaPlugin
}; // namespace nVerliHub

REGISTER_PLUGIN(nVerliHub::nLuaPlugin::cpiLua);
