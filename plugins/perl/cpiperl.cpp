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

#include <config.h>
#include "src/cserverdc.h"
#include "src/cbanlist.h"
#include "src/stringutils.h"
#include "cpiperl.h"
#include <dlfcn.h>

using namespace nVerliHub::nUtils;

const string toString(int number)
{
	ostringstream os;
	os << number;
	return os.str();
}

nVerliHub::nPerlPlugin::cpiPerl::cpiPerl():
	mQuery(NULL),
	mConsole(this)
{
	mName = PERLSCRIPT_PI_IDENTIFIER;
	mVersion= PERLSCRIPT_VERSION;
}

nVerliHub::nPerlPlugin::cpiPerl::~cpiPerl()
{
	const char *args[] = { "UnLoad", NULL };
	mPerl.CallArgv(PERL_CALL, args);

	if (mQuery) {
		mQuery->Clear();
		delete mQuery;
	}
}

bool nVerliHub::nPerlPlugin::cpiPerl::RegisterAll()
{
	RegisterCallBack("VH_OnNewConn");
	RegisterCallBack("VH_OnCloseConn");
	RegisterCallBack("VH_OnCloseConnEx");
	RegisterCallBack("VH_OnParsedMsgChat");
	RegisterCallBack("VH_OnParsedMsgPM");
	RegisterCallBack("VH_OnParsedMsgMCTo");
	RegisterCallBack("VH_OnParsedMsgSearch");
	RegisterCallBack("VH_OnParsedMsgSR");
	RegisterCallBack("VH_OnParsedMsgConnectToMe");
	RegisterCallBack("VH_OnParsedMsgRevConnectToMe");
	RegisterCallBack("VH_OnParsedMsgMyINFO");
	RegisterCallBack("VH_OnFirstMyINFO");
	RegisterCallBack("VH_OnParsedMsgValidateNick");
	RegisterCallBack("VH_OnParsedMsgAny");
	RegisterCallBack("VH_OnParsedMsgAnyEx");
	RegisterCallBack("VH_OnParsedMsgSupports");
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
	RegisterCallBack("VH_OnDelReg");
	RegisterCallBack("VH_OnNewBan");
	RegisterCallBack("VH_OnUnBan");
	RegisterCallBack("VH_OnScriptCommand");
	RegisterCallBack("VH_OnUpdateClass");
	RegisterCallBack("VH_OnHubName");
	RegisterCallBack("VH_OnHubCommand");
	return true;
}

void nVerliHub::nPerlPlugin::cpiPerl::OnLoad(cServerDC* server)
{
	cVHPlugin::OnLoad(server);

	mQuery = new nMySQL::cQuery(server->mMySQL);

	mScriptDir = mServer->mConfigBaseDir + "/scripts/";

	AutoLoad();
}

bool nVerliHub::nPerlPlugin::cpiPerl::AutoLoad()
{
	if(Log(0))
		LogStream() << "Open dir: " << mScriptDir << endl;
	string pathname, filename;
	DIR *dir = opendir(mScriptDir.c_str());
	if(!dir) {
		if(Log(1))
			LogStream() << "Error opening directory" << endl;
		return false;
	}
	struct dirent *dent = NULL;

	while(NULL != (dent=readdir(dir))) {
		filename = dent->d_name;
		if((filename.size() > 3) && (StrCompare(filename, filename.size() - 3, 3, ".pl") == 0)) {
			pathname = mScriptDir + filename;
			const char *argv[] = { "", pathname.c_str(), NULL };

			if (mPerl.Parse(2, argv)) {
				if (ErrLog(1))
					LogStream() << "Failed parsing Perl script: " << filename << endl;
			} else {
				if (Log(1))
					LogStream() << "Success loading and parsing Perl script: " << filename << endl;
			}
		}
	}

	closedir(dir);
	return true;
}

bool nVerliHub::nPerlPlugin::cpiPerl::OnNewConn(cConnDC * conn)
{
	const char *args[] = { "VH_OnNewConn",
				conn->AddrIP().c_str(), NULL };
	bool ret = mPerl.CallArgv(PERL_CALL, args);
	return ret;
}

bool nVerliHub::nPerlPlugin::cpiPerl::OnCloseConn(cConnDC * conn)
{
	const char *args[] = { "VH_OnCloseConn",
				conn->AddrIP().c_str(), NULL };
	bool ret = mPerl.CallArgv(PERL_CALL, args);
	return ret;
}

bool nVerliHub::nPerlPlugin::cpiPerl::OnCloseConnEx(cConnDC * conn)
{
	bool ret;
	if (conn->mpUser != NULL) {
		const char *args[] = { "VH_OnCloseConnEx",
					conn->AddrIP().c_str(),
					conn->mpUser->mNick.c_str(), NULL };
		ret = mPerl.CallArgv(PERL_CALL, args);
	} else {
		const char *args[] = { "VH_OnCloseConnEx",
					conn->AddrIP().c_str(), NULL };
		ret = mPerl.CallArgv(PERL_CALL, args);
	}
	return ret;
}

bool nVerliHub::nPerlPlugin::cpiPerl::OnParsedMsgAny(cConnDC *conn , cMessageDC *msg)
{
	if (conn->mpUser != NULL) {
		const char *args[] = { "VH_OnParsedMsgAny",
					conn->mpUser->mNick.c_str(),
					msg->mStr.c_str(),
					NULL };
		bool ret = mPerl.CallArgv(PERL_CALL, args);
		return ret;
	}
	return true;
}

bool nVerliHub::nPerlPlugin::cpiPerl::OnParsedMsgAnyEx(cConnDC *conn , cMessageDC *msg)
{
	if (conn->mpUser == NULL) {
		const char *args[] = { "VH_OnParsedMsgAnyEx",
					conn->AddrIP().c_str(),
					msg->mStr.c_str(),
					NULL };
		bool ret = mPerl.CallArgv(PERL_CALL, args);
		return ret;
	}
	return true;
}

bool nVerliHub::nPerlPlugin::cpiPerl::OnUnknownMsg(cConnDC *conn , cMessageDC *msg)
{
	const char *args[] = { "VH_OnUnknownMsg",
				(conn->mpUser?conn->mpUser->mNick.c_str():conn->AddrIP().c_str()),
				msg->mStr.c_str(),
				NULL };
	bool ret = mPerl.CallArgv(PERL_CALL, args);
	return ret;
}

bool nVerliHub::nPerlPlugin::cpiPerl::OnParsedMsgSupports(cConnDC *conn, cMessageDC *msg, string *back)
{
	bool ret = true;

	if (conn && msg && back) {
		const char *args[] = { "VH_OnParsedMsgSupports",
			conn->AddrIP().c_str(),
			msg->mStr.c_str(),
			back->c_str(),
			NULL
		};

		ret = mPerl.CallArgv(PERL_CALL, args);
	}

	return ret;
}

bool nVerliHub::nPerlPlugin::cpiPerl::OnParsedMsgBotINFO(cConnDC *conn , cMessageDC *msg)
{
	const char *args[] = { "VH_OnParsedMsgBotINFO",
				conn->mpUser->mNick.c_str(),
				msg->mStr.c_str(),
				NULL };
	bool ret = mPerl.CallArgv(PERL_CALL, args);
	return ret;
}

bool nVerliHub::nPerlPlugin::cpiPerl::OnParsedMsgVersion(cConnDC *conn , cMessageDC *msg)
{
	const char *args[] = { "VH_OnParsedMsgVersion",
				conn->AddrIP().c_str(),
				msg->mStr.c_str(),
				NULL };
	bool ret = mPerl.CallArgv(PERL_CALL, args);
	return ret;
}

bool nVerliHub::nPerlPlugin::cpiPerl::OnParsedMsgValidateNick(cConnDC *conn , cMessageDC *msg)
{
	const char *args[] = { "VH_OnParsedMsgValidateNick",
				conn->AddrIP().c_str(),
				msg->ChunkString(eCH_1_ALL).c_str(),
				NULL }; // eCH_1_ALL, eCH_1_PARAM
	bool ret = mPerl.CallArgv(PERL_CALL, args);
	return ret;
}

bool nVerliHub::nPerlPlugin::cpiPerl::OnParsedMsgMyPass(cConnDC *conn , cMessageDC *msg)
{
	const char *args[] = { "VH_OnParsedMsgMyPass",
				conn->AddrIP().c_str(),
				msg->ChunkString(eCH_1_ALL).c_str(),
				NULL }; // eCH_1_ALL, eCH_1_PARAM
	bool ret = mPerl.CallArgv(PERL_CALL, args);
	return ret;
}

bool nVerliHub::nPerlPlugin::cpiPerl::OnParsedMsgMyINFO(cConnDC *conn , cMessageDC *msg)
{
	const char *args[] = { "VH_OnParsedMsgMyINFO",
				conn->AddrIP().c_str(),
				msg->ChunkString(eCH_MI_ALL).c_str(),
				NULL }; // eCH_MI_ALL, eCH_MI_DEST, eCH_MI_NICK, eCH_MI_INFO, eCH_MI_DESC, eCH_MI_SPEED, eCH_MI_MAIL, eCH_MI_SIZE;
	bool ret = mPerl.CallArgv(PERL_CALL, args);
	return ret;
}

bool nVerliHub::nPerlPlugin::cpiPerl::OnFirstMyINFO(cConnDC *conn , cMessageDC *msg)
{
	if ((conn != NULL) && (conn->mpUser != NULL) && (msg != NULL)) {
		const char *args[] = { "VH_OnFirstMyINFO",
					msg->ChunkString(eCH_MI_NICK).c_str(),
					msg->ChunkString(eCH_MI_DESC).c_str(),
					msg->ChunkString(eCH_MI_SPEED).c_str(),
					msg->ChunkString(eCH_MI_MAIL).c_str(),
					msg->ChunkString(eCH_MI_SIZE).c_str(),
					NULL };
		bool ret = mPerl.CallArgv(PERL_CALL, args);
		return ret;
	}
	return true;
}

bool nVerliHub::nPerlPlugin::cpiPerl::OnParsedMsgSearch(cConnDC *conn , cMessageDC *msg)
{
	const char *args[] = { "VH_OnParsedMsgSearch",
				conn->AddrIP().c_str(),
				msg->mStr.c_str(),
				NULL }; // active: eCH_AS_ALL, eCH_AS_ADDR, eCH_AS_IP, eCH_AS_PORT, eCH_AS_QUERY; passive: eCH_PS_ALL, eCH_PS_NICK, eCH_PS_QUERY;
	bool ret = mPerl.CallArgv(PERL_CALL, args);
	return ret;
}

bool nVerliHub::nPerlPlugin::cpiPerl::OnParsedMsgSR(cConnDC *conn , cMessageDC *msg)
{
	const char *args[] = { "VH_OnParsedMsgSR",
				conn->AddrIP().c_str(),
				msg->ChunkString(eCH_SR_ALL).c_str(),
				NULL }; // eCH_SR_ALL, eCH_SR_FROM, eCH_SR_PATH, eCH_SR_SIZE, eCH_SR_SLOTS, eCH_SR_SL_FR, eCH_SR_SL_TO, eCH_SR_HUBINFO, eCH_SR_TO;
	bool ret = mPerl.CallArgv(PERL_CALL, args);
	return ret;
}

bool nVerliHub::nPerlPlugin::cpiPerl::OnParsedMsgChat(cConnDC *conn , cMessageDC *msg)
{
	const char *args[] = { "VH_OnParsedMsgChat",
				conn->mpUser->mNick.c_str(),
				msg->ChunkString(eCH_CH_MSG).c_str(),
				NULL}; // eCH_CH_ALL, eCH_CH_NICK, eCH_CH_MSG;
	bool ret = mPerl.CallArgv(PERL_CALL, args);
	return ret;
}

bool nVerliHub::nPerlPlugin::cpiPerl::OnParsedMsgPM(cConnDC *conn , cMessageDC *msg)
{
	const char *args[] = { "VH_OnParsedMsgPM",
				conn->mpUser->mNick.c_str(),
				msg->ChunkString(eCH_PM_MSG).c_str(),
				msg->ChunkString(eCH_PM_TO).c_str(),
				NULL}; // eCH_PM_ALL, eCH_PM_TO, eCH_PM_FROM, eCH_PM_CHMSG, eCH_PM_NICK, eCH_PM_MSG;
	bool ret = mPerl.CallArgv(PERL_CALL, args);
	return ret;
}

bool nVerliHub::nPerlPlugin::cpiPerl::OnParsedMsgMCTo(cConnDC *conn , cMessageDC *msg)
{
	const char *args[] = { "VH_OnParsedMsgMCTo",
				conn->mpUser->mNick.c_str(),
				msg->ChunkString(eCH_PM_MSG).c_str(),
				msg->ChunkString(eCH_PM_TO).c_str(),
				NULL}; // eCH_PM_ALL, eCH_PM_TO, eCH_PM_FROM, eCH_PM_CHMSG, eCH_PM_NICK, eCH_PM_MSG;
	bool ret = mPerl.CallArgv(PERL_CALL, args);
	return ret;
}

bool nVerliHub::nPerlPlugin::cpiPerl::OnValidateTag(cConnDC *conn , cDCTag *tag)
{
	const char *args[] = { "VH_OnValidateTag",
				conn->mpUser->mNick.c_str(),
				tag->mTag.c_str(),
				NULL};
	bool ret = mPerl.CallArgv(PERL_CALL, args);
	return ret;
}

bool nVerliHub::nPerlPlugin::cpiPerl::OnOperatorCommand(cConnDC *conn , std::string *str)
{
	if ((conn != NULL) && (conn->mpUser != NULL) && (str != NULL)) {
		if (mConsole.DoCommand(*str, conn))
			return false;

		const char *args[] = { "VH_OnOperatorCommand",
					conn->mpUser->mNick.c_str(),
					str->c_str(),
					NULL};
		bool ret = mPerl.CallArgv(PERL_CALL, args);
		return ret;
	}
	return true;
}

bool nVerliHub::nPerlPlugin::cpiPerl::OnOperatorKicks(cUser *op , cUser *user, std::string *reason)
{
	const char *args[] = { "VH_OnOperatorKicks",
				op->mNick.c_str(),
				user->mNick.c_str(),
				reason->c_str(),
				NULL };
	bool ret = mPerl.CallArgv(PERL_CALL, args);
	return ret;
}

bool nVerliHub::nPerlPlugin::cpiPerl::OnOperatorDrops(cUser *op , cUser *user, std::string *why)
{
	const char *args[] = { "VH_OnOperatorDrops",
				op->mNick.c_str(),
				user->mNick.c_str(),
				why->c_str(),
				NULL };
	bool ret = mPerl.CallArgv(PERL_CALL, args);
	return ret;
}

bool nVerliHub::nPerlPlugin::cpiPerl::OnUserCommand(cConnDC *conn , std::string *str)
{
	const char *args[] = { "VH_OnUserCommand",
				conn->mpUser->mNick.c_str(),
				str->c_str(),
				NULL};
	bool ret = mPerl.CallArgv(PERL_CALL, args);
	return ret;
}

bool nVerliHub::nPerlPlugin::cpiPerl::OnHubCommand(cConnDC *conn , std::string *command, int op, int pm)
{
	const char *args[] = { "VH_OnHubCommand",
				conn->mpUser->mNick.c_str(),
				command->c_str(),
				toString(op).c_str(),
				toString(pm).c_str(),
				NULL};
	bool ret = mPerl.CallArgv(PERL_CALL, args);
	return ret;
}

bool nVerliHub::nPerlPlugin::cpiPerl::OnUserLogin(cUser *user)
{
	const char *args[] = { "VH_OnUserLogin",
				user->mNick.c_str(),
				NULL };
	bool ret = mPerl.CallArgv(PERL_CALL, args);
	return ret;
}

bool nVerliHub::nPerlPlugin::cpiPerl::OnUserLogout(cUser *user)
{
	const char *args[] = { "VH_OnUserLogout",
				user->mNick.c_str(),
				NULL };
	bool ret = mPerl.CallArgv(PERL_CALL, args);
	return ret;
}

bool nVerliHub::nPerlPlugin::cpiPerl::OnTimer(__int64 msec)
{
	std::stringstream ss;
	ss << msec;
	std::string s = ss.str();
	const char *args[] = { "VH_OnTimer",
				s.c_str(),
				NULL };
	bool ret = mPerl.CallArgv(PERL_CALL, args);
	return ret;
}

bool nVerliHub::nPerlPlugin::cpiPerl::OnNewReg(cUser *op, string nick, int uclass)
{
	const char *args[] = { "VH_OnNewReg",
				op->mNick.c_str(),
				nick.c_str(),
				toString(uclass).c_str(),
				NULL };
	bool ret = mPerl.CallArgv(PERL_CALL, args);
	return ret;
}

bool nVerliHub::nPerlPlugin::cpiPerl::OnNewBan(cUser* op, nTables::cBan *ban)
{
	const char *args[] = { "VH_OnNewBan",
				ban->mIP.c_str(),
				ban->mNick.c_str(),
				ban->mReason.c_str(),
				op->mNick.c_str(),
				NULL};
	bool ret = mPerl.CallArgv(PERL_CALL, args);
	return ret;
}

bool nVerliHub::nPerlPlugin::cpiPerl::OnUnBan(cUser *opUser, std::string nick, std::string op, std::string reason)
{
	const char *args[] = { "VH_OnUnBan",
				nick.c_str(),
				op.c_str(),
				reason.c_str(),
				NULL};
	bool ret = mPerl.CallArgv(PERL_CALL, args);
	return ret;
}

bool nVerliHub::nPerlPlugin::cpiPerl::OnParsedMsgConnectToMe(cConnDC *conn, cMessageDC *msg)
{
	bool ret = true;
	if((conn != NULL) && (conn->mpUser != NULL) && (msg != NULL)) {
		const char *args[] = { "VH_OnParsedMsgConnectToMe",
					conn->mpUser->mNick.c_str(),
					msg->ChunkString(eCH_CM_NICK).c_str(),
					msg->ChunkString(eCH_CM_IP).c_str(),
					msg->ChunkString(eCH_CM_PORT).c_str(),
					NULL
		}; // eCH_CM_NICK, eCH_CM_ACTIVE, eCH_CM_IP, eCH_CM_PORT
		ret = mPerl.CallArgv(PERL_CALL, args);
	}
	return ret;
}

bool nVerliHub::nPerlPlugin::cpiPerl::OnParsedMsgRevConnectToMe(cConnDC *conn, cMessageDC *msg)
{
	bool ret = true;
	if((conn != NULL) && (conn->mpUser != NULL) && (msg != NULL)) {
		const char *args[] = { "VH_OnParsedMsgRevConnectToMe",
					conn->mpUser->mNick.c_str(),
					msg->ChunkString(eCH_RC_OTHER).c_str(),
					NULL
		}; // eCH_CM_NICK, eCH_CM_ACTIVE, eCH_CM_IP, eCH_CM_PORT
		ret = mPerl.CallArgv(PERL_CALL, args);
	}
	return ret;
}

bool nVerliHub::nPerlPlugin::cpiPerl::OnDelReg(cUser *op, std::string mNick, int mClass)
{
	const char *args[] = { "VH_OnDelReg",
				mNick.c_str(),
				toString(mClass).c_str(),
				op->mNick.c_str(),
				NULL};
	bool ret = mPerl.CallArgv(PERL_CALL, args);
	return ret;
}

bool nVerliHub::nPerlPlugin::cpiPerl::OnUpdateClass(cUser *op, std::string mNick, int oldClass, int newClass)
{
	const char *args[] = { "VH_OnUpdateClass",
				mNick.c_str(),
				toString(oldClass).c_str(),
				toString(newClass).c_str(),
				op->mNick.c_str(),
				NULL};
	bool ret = mPerl.CallArgv(PERL_CALL, args);
	return ret;
}

bool nVerliHub::nPerlPlugin::cpiPerl::OnScriptCommand(std::string *cmd, std::string *data, std::string *plug, std::string *script)
{
	const char *args[] = { "VH_OnScriptCommand",
				cmd->c_str(),
				data->c_str(),
				plug->c_str(),
				script->c_str(),
				NULL};
	bool ret = mPerl.CallArgv(PERL_CALL, args);
	return ret;
}

bool nVerliHub::nPerlPlugin::cpiPerl::OnHubName(std::string nick, std::string hubname)
{
	const char *args[] = { "VH_OnHubName",
				nick.c_str(),
				hubname.c_str(),
				NULL};
	bool ret = mPerl.CallArgv(PERL_CALL, args);
	return ret;
}

REGISTER_PLUGIN(nVerliHub::nPerlPlugin::cpiPerl);
