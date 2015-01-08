/**************************************************************************
*   Copyright (C) 2003 by Dan Muller                                      *
*   dan at verliba.cz                                                     *
*                                                                         *
*   Copyright (C) 2011 by Shurik                                          *
*   shurik at sbin.ru                                                     *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
*   This program is distributed in the hope that it will be useful,       *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
*   GNU General Public License for more details.                          *
*                                                                         *
*   You should have received a copy of the GNU General Public License     *
*   along with this program; if not, write to the                         *
*   Free Software Foundation, Inc.,                                       *
*   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
***************************************************************************/
#include <config.h>
#include "src/cserverdc.h"
#include "src/cbanlist.h"
#include "src/stringutils.h"
#include "cpiperl.h"
#include <dlfcn.h>

using namespace nVerliHub::nUtils;

static const char * toString(int number)
{
	ostringstream os;
	os << number;
	return os.str().c_str();
}

nVerliHub::nPerlPlugin::cpiPerl::cpiPerl():
mConsole(this), mQuery(NULL)
{
	mName = PERLSCRIPT_PI_IDENTIFIER;
	mVersion= PERLSCRIPT_VERSION;
}

nVerliHub::nPerlPlugin::cpiPerl::~cpiPerl()
{
	char *args[] = { (char *)"UnLoad", NULL };
	mPerl.CallArgv(PERL_CALL, args);

	if(mQuery) {
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
		if((filename.size() > 3) && (StrCompare(filename,filename.size()-3,3,".pl")==0)) {
			pathname = mScriptDir + filename;
			char *argv[] = { (char*)"", (char*)pathname.c_str(), NULL };
			mPerl.Parse(2, argv);
			// FIXME: check weither all ok?
			if(Log(1))
				LogStream() << "Success loading and parsing Perl script: " << filename << endl;
		}
	}

	closedir(dir);
	return true;
}

bool nVerliHub::nPerlPlugin::cpiPerl::OnNewConn(cConnDC * conn)
{
	char *args[] =  {	(char *)"VH_OnNewConn",
				(char *)conn->AddrIP().c_str(), NULL };
	bool ret = mPerl.CallArgv(PERL_CALL, args);
	return ret;
}

bool nVerliHub::nPerlPlugin::cpiPerl::OnCloseConn(cConnDC * conn)
{
	char *args[] =  {	(char *)"VH_OnCloseConn",
				(char *)conn->AddrIP().c_str(), NULL };
	bool ret = mPerl.CallArgv(PERL_CALL, args);
	return ret;
}

bool nVerliHub::nPerlPlugin::cpiPerl::OnCloseConnEx(cConnDC * conn)
{
	bool ret;
	if (conn->mpUser != NULL) {
		char *args[] =  {	(char *)"VH_OnCloseConnEx",
					(char *)conn->AddrIP().c_str(),
					(char *)conn->mpUser->mNick.c_str(), NULL };
		ret = mPerl.CallArgv(PERL_CALL, args);
	} else {
		char *args[] =  {	(char *)"VH_OnCloseConnEx",
					(char *)conn->AddrIP().c_str(), NULL };
		ret = mPerl.CallArgv(PERL_CALL, args);
	}
	return ret;
}

bool nVerliHub::nPerlPlugin::cpiPerl::OnParsedMsgAny(cConnDC *conn , cMessageDC *msg)
{
	if (conn->mpUser != NULL) {
		char *args[] =  {	(char *)"VH_OnParsedMsgAny",
					(char *)conn->mpUser->mNick.c_str(),
					(char *)msg->mStr.c_str(),
					NULL };
		bool ret = mPerl.CallArgv(PERL_CALL, args);
		return ret;
	}
	return true;
}

bool nVerliHub::nPerlPlugin::cpiPerl::OnParsedMsgAnyEx(cConnDC *conn , cMessageDC *msg)
{
	if (conn->mpUser == NULL) {
		char *args[] =  {	(char *)"VH_OnParsedMsgAnyEx",
					(char *)conn->AddrIP().c_str(),
					(char *)msg->mStr.c_str(),
					NULL };
		bool ret = mPerl.CallArgv(PERL_CALL, args);
		return ret;
	}
	return true;
}

bool nVerliHub::nPerlPlugin::cpiPerl::OnUnknownMsg(cConnDC *conn , cMessageDC *msg)
{
	char *args[] =  {	(char *)"VH_OnUnknownMsg",
				(char *)(conn->mpUser?conn->mpUser->mNick.c_str():conn->AddrIP().c_str()),
				(char *)msg->mStr.c_str(),
				NULL };
	bool ret = mPerl.CallArgv(PERL_CALL, args);
	return ret;
}

bool nVerliHub::nPerlPlugin::cpiPerl::OnParsedMsgSupport(cConnDC *conn , cMessageDC *msg)
{
	char *args[] =  {	(char *)"VH_OnParsedMsgSupport",
				(char *)conn->AddrIP().c_str(),
				(char *)msg->mStr.c_str(),
				NULL };
	bool ret = mPerl.CallArgv(PERL_CALL, args);
	return ret;
}

bool nVerliHub::nPerlPlugin::cpiPerl::OnParsedMsgBotINFO(cConnDC *conn , cMessageDC *msg)
{
	char *args[] =  {	(char *)"VH_OnParsedMsgBotINFO",
				(char *)conn->mpUser->mNick.c_str(),
				(char *)msg->mStr.c_str(),
				NULL };
	bool ret = mPerl.CallArgv(PERL_CALL, args);
	return ret;
}

bool nVerliHub::nPerlPlugin::cpiPerl::OnParsedMsgVersion(cConnDC *conn , cMessageDC *msg)
{
	char *args[] =  {	(char *)"VH_OnParsedMsgVersion",
				(char *)conn->AddrIP().c_str(),
				(char *)msg->mStr.c_str(),
				NULL };
	bool ret = mPerl.CallArgv(PERL_CALL, args);
	return ret;
}

bool nVerliHub::nPerlPlugin::cpiPerl::OnParsedMsgValidateNick(cConnDC *conn , cMessageDC *msg)
{
	char *args[] =  {	(char *)"VH_OnParsedMsgValidateNick",
				(char *)conn->AddrIP().c_str(),
				(char *)msg->ChunkString(eCH_1_ALL).c_str(),
				NULL }; // eCH_1_ALL, eCH_1_PARAM
	bool ret = mPerl.CallArgv(PERL_CALL, args);
	return ret;
}

bool nVerliHub::nPerlPlugin::cpiPerl::OnParsedMsgMyPass(cConnDC *conn , cMessageDC *msg)
{
	char *args[] =  {	(char *)"VH_OnParsedMsgMyPass",
				(char *)conn->AddrIP().c_str(),
				(char *)msg->ChunkString(eCH_1_ALL).c_str(),
				NULL }; // eCH_1_ALL, eCH_1_PARAM
	bool ret = mPerl.CallArgv(PERL_CALL, args);
	return ret;
}

bool nVerliHub::nPerlPlugin::cpiPerl::OnParsedMsgMyINFO(cConnDC *conn , cMessageDC *msg)
{
	char *args[] =  {	(char *)"VH_OnParsedMsgMyINFO",
				(char *)conn->AddrIP().c_str(),
				(char *)msg->ChunkString(eCH_MI_ALL).c_str(),
				NULL }; // eCH_MI_ALL, eCH_MI_DEST, eCH_MI_NICK, eCH_MI_INFO, eCH_MI_DESC, eCH_MI_SPEED, eCH_MI_MAIL, eCH_MI_SIZE;
	bool ret = mPerl.CallArgv(PERL_CALL, args);
	return ret;
}

bool nVerliHub::nPerlPlugin::cpiPerl::OnFirstMyINFO(cConnDC *conn , cMessageDC *msg)
{
	if ((conn != NULL) && (conn->mpUser != NULL) && (msg != NULL)) {
		char *args[] =  {	(char *) "VH_OnFirstMyINFO",
					(char *) msg->ChunkString(eCH_MI_NICK).c_str(),
					(char *) msg->ChunkString(eCH_MI_DESC).c_str(),
					(char *) msg->ChunkString(eCH_MI_SPEED).c_str(),
					(char *) msg->ChunkString(eCH_MI_MAIL).c_str(),
					(char *) msg->ChunkString(eCH_MI_SIZE).c_str(),
					NULL };
		bool ret = mPerl.CallArgv(PERL_CALL, args);
		return ret;
	}
	return true;
}

bool nVerliHub::nPerlPlugin::cpiPerl::OnParsedMsgSearch(cConnDC *conn , cMessageDC *msg)
{
	char *args[] =  {	(char *)"VH_OnParsedMsgSearch",
				(char *)conn->AddrIP().c_str(),
				(char *)msg->mStr.c_str(),
				NULL }; // active: eCH_AS_ALL, eCH_AS_ADDR, eCH_AS_IP, eCH_AS_PORT, eCH_AS_QUERY; passive: eCH_PS_ALL, eCH_PS_NICK, eCH_PS_QUERY;
	bool ret = mPerl.CallArgv(PERL_CALL, args);
	return ret;
}

bool nVerliHub::nPerlPlugin::cpiPerl::OnParsedMsgSR(cConnDC *conn , cMessageDC *msg)
{
	char *args[] =  {	(char *)"VH_OnParsedMsgSR",
				(char *)conn->AddrIP().c_str(),
				(char *)msg->ChunkString(eCH_SR_ALL).c_str(),
				NULL }; // eCH_SR_ALL, eCH_SR_FROM, eCH_SR_PATH, eCH_SR_SIZE, eCH_SR_SLOTS, eCH_SR_SL_FR, eCH_SR_SL_TO, eCH_SR_HUBINFO, eCH_SR_TO;
	bool ret = mPerl.CallArgv(PERL_CALL, args);
	return ret;
}

bool nVerliHub::nPerlPlugin::cpiPerl::OnParsedMsgChat(cConnDC *conn , cMessageDC *msg)
{
	char *args[]= {		(char *)"VH_OnParsedMsgChat",
				(char *)conn->mpUser->mNick.c_str(), 
				(char *) msg->ChunkString(eCH_CH_MSG).c_str(), 
				NULL}; // eCH_CH_ALL, eCH_CH_NICK, eCH_CH_MSG;
	bool ret = mPerl.CallArgv(PERL_CALL, args);
	return ret;
}

bool nVerliHub::nPerlPlugin::cpiPerl::OnParsedMsgPM(cConnDC *conn , cMessageDC *msg)
{
	char *args[]= {		(char *)"VH_OnParsedMsgPM",
				(char *)conn->mpUser->mNick.c_str(), 
				(char *) msg->ChunkString(eCH_PM_MSG).c_str(), 
				(char *) msg->ChunkString(eCH_PM_TO).c_str(),
				NULL}; // eCH_PM_ALL, eCH_PM_TO, eCH_PM_FROM, eCH_PM_CHMSG, eCH_PM_NICK, eCH_PM_MSG;
	bool ret = mPerl.CallArgv(PERL_CALL, args);
	return ret;
}

bool nVerliHub::nPerlPlugin::cpiPerl::OnParsedMsgMCTo(cConnDC *conn , cMessageDC *msg)
{
	char *args[]= {		(char *) "VH_OnParsedMsgMCTo",
				(char *) conn->mpUser->mNick.c_str(), 
				(char *) msg->ChunkString(eCH_PM_MSG).c_str(), 
				(char *) msg->ChunkString(eCH_PM_TO).c_str(),
				NULL}; // eCH_PM_ALL, eCH_PM_TO, eCH_PM_FROM, eCH_PM_CHMSG, eCH_PM_NICK, eCH_PM_MSG;
	bool ret = mPerl.CallArgv(PERL_CALL, args);
	return ret;
}

bool nVerliHub::nPerlPlugin::cpiPerl::OnValidateTag(cConnDC *conn , cDCTag *tag)
{
	char *args[]= {		(char *)"VH_OnValidateTag",
				(char *)conn->mpUser->mNick.c_str(),
				(char *) tag->mTag.c_str(),
				NULL};
	bool ret = mPerl.CallArgv(PERL_CALL, args);
	return ret;
}

bool nVerliHub::nPerlPlugin::cpiPerl::OnOperatorCommand(cConnDC *conn , std::string *str)
{
	if((conn != NULL) && (conn->mpUser != NULL) && (str != NULL))
		if(mConsole.DoCommand(*str, conn))
			return false;

	char *args[]= {		(char *)"VH_OnOperatorCommand",
				(char *)conn->mpUser->mNick.c_str(),
				(char *) str->c_str(),
				NULL};
	bool ret = mPerl.CallArgv(PERL_CALL, args);
	return ret;
}

bool nVerliHub::nPerlPlugin::cpiPerl::OnOperatorKicks(cUser *op , cUser *user, std::string *reason)
{
	char *args[]= {		(char *)"VH_OnOperatorKicks",
				(char *)op->mNick.c_str(),
				(char *)user->mNick.c_str(),
				(char *)reason->c_str(),
				NULL };
	bool ret = mPerl.CallArgv(PERL_CALL, args);
	return ret;
}

bool nVerliHub::nPerlPlugin::cpiPerl::OnOperatorDrops(cUser *op , cUser *user)
{
	char *args[]= {		(char *)"VH_OnOperatorDrops",
				(char *)op->mNick.c_str(),
				(char *)user->mNick.c_str(),
				NULL };
	bool ret = mPerl.CallArgv(PERL_CALL, args);
	return ret;
}

bool nVerliHub::nPerlPlugin::cpiPerl::OnUserCommand(cConnDC *conn , std::string *str)
{
	char *args[]= {		(char *)"VH_OnUserCommand",
				(char *)conn->mpUser->mNick.c_str(),
				(char *) str->c_str(),
				NULL};
	bool ret = mPerl.CallArgv(PERL_CALL, args);
	return ret;
}

bool nVerliHub::nPerlPlugin::cpiPerl::OnHubCommand(cConnDC *conn , std::string *command, int op, int pm)
{
	char *args[]= {		(char *)"VH_OnHubCommand",
				(char *)conn->mpUser->mNick.c_str(),
				(char *)command->c_str(),
				(char *)toString(op),
				(char *)toString(pm),
				NULL};
	bool ret = mPerl.CallArgv(PERL_CALL, args);
	return ret;
}

bool nVerliHub::nPerlPlugin::cpiPerl::OnUserLogin(cUser *user)
{
	char *args[]= {		(char *)"VH_OnUserLogin",
				(char *)user->mNick.c_str(),
				NULL };
	bool ret = mPerl.CallArgv(PERL_CALL, args);
	return ret;
}

bool nVerliHub::nPerlPlugin::cpiPerl::OnUserLogout(cUser *user)
{
	char *args[]= {		(char *)"VH_OnUserLogout",
				(char *)user->mNick.c_str(),
				NULL };
	bool ret = mPerl.CallArgv(PERL_CALL, args);
	return ret;
}

bool nVerliHub::nPerlPlugin::cpiPerl::OnTimer(long msec)
{
	std::stringstream s;
	s << msec;
	char *args[]= {		(char *)"VH_OnTimer",
				(char *)s.str().c_str(),
				NULL };
	bool ret = mPerl.CallArgv(PERL_CALL, args);
	return ret;
}

bool nVerliHub::nPerlPlugin::cpiPerl::OnNewReg(cRegUserInfo *reginfo)
{
	char *args[]= {		(char *)"VH_OnNewReg",
				NULL };
	bool ret = mPerl.CallArgv(PERL_CALL, args);
	return ret;
}

bool nVerliHub::nPerlPlugin::cpiPerl::OnNewBan(cBan *ban)
{
	char *args[]= {		(char *)"VH_OnNewBan",
				(char *)ban->mIP.c_str(),
				(char *)ban->mNick.c_str(),
				(char *)ban->mReason.c_str(),
				NULL};
	bool ret = mPerl.CallArgv(PERL_CALL, args);
	return ret;
}

bool nVerliHub::nPerlPlugin::cpiPerl::OnUnBan(std::string nick, std::string op, std::string reason)
{
	char *args[]= {		(char *)"VH_OnUnBan",
				(char *) nick.c_str(),
				(char *) op.c_str(),
				(char *) reason.c_str(),
				NULL};
	bool ret = mPerl.CallArgv(PERL_CALL, args);
	return ret;
}

bool nVerliHub::nPerlPlugin::cpiPerl::OnParsedMsgConnectToMe(cConnDC *conn, cMessageDC *msg)
{
	bool ret = true;
	if((conn != NULL) && (conn->mpUser != NULL) && (msg != NULL)) {
		char * args[] = {	(char *)"VH_OnParsedMsgConnectToMe",
					(char *)conn->mpUser->mNick.c_str(),
					(char *)msg->ChunkString(eCH_CM_NICK).c_str(),
					(char *)msg->ChunkString(eCH_CM_IP).c_str(),
					(char *)msg->ChunkString(eCH_CM_PORT).c_str(),
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
		char * args[] = {	(char *)"VH_OnParsedMsgRevConnectToMe",
					(char *)conn->mpUser->mNick.c_str(),
					(char *)msg->ChunkString(eCH_RC_OTHER).c_str(),
					NULL
		}; // eCH_CM_NICK, eCH_CM_ACTIVE, eCH_CM_IP, eCH_CM_PORT
		ret = mPerl.CallArgv(PERL_CALL, args);
	}
	return ret;
}

bool nVerliHub::nPerlPlugin::cpiPerl::OnDelReg(std::string mNick, int mClass)
{
	char *args[]= {		(char *)"VH_OnDelReg",
				(char *) mNick.c_str(),
				(char *) toString(mClass),
				NULL};
	bool ret = mPerl.CallArgv(PERL_CALL, args);
	return ret;
}

bool nVerliHub::nPerlPlugin::cpiPerl::OnUpdateClass(std::string mNick, int oldClass, int newClass)
{
	char *args[]= {		(char *)"VH_OnUpdateClass",
				(char *) mNick.c_str(),
				(char *) toString(oldClass),
				(char *) toString(newClass),
				NULL};
	bool ret = mPerl.CallArgv(PERL_CALL, args);
	return ret;
}

bool nVerliHub::nPerlPlugin::cpiPerl::OnScriptCommand(std::string cmd, std::string data, std::string plug, std::string script)
{
	char *args[]= {		(char *) "VH_OnScriptCommand",
				(char *) cmd.c_str(),
				(char *) data.c_str(),
				(char *) plug.c_str(),
				(char *) script.c_str(),
				NULL};
	bool ret = mPerl.CallArgv(PERL_CALL, args);
	return ret;
}

bool nVerliHub::nPerlPlugin::cpiPerl::OnHubName(std::string nick, std::string hubname)
{
	char *args[]= {		(char *)"VH_OnHubName",
				(char *) nick.c_str(),
				(char *) hubname.c_str(),
				NULL};
	bool ret = mPerl.CallArgv(PERL_CALL, args);
	return ret;
}

REGISTER_PLUGIN(nVerliHub::nPerlPlugin::cpiPerl);
