/*
	Copyright (C) 2003-2005 Daniel Muller, dan at verliba dot cz
	Copyright (C) 2006-2025 Verlihub Team, info at verlihub dot net

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

#include "src/script_api.h"
#include "src/cconnchoose.h"
using namespace nVerliHub::nEnums;
#include "src/cserverdc.h"
#include "cpiperl.h"
#include "callbacks.h"
using namespace nVerliHub;
using namespace nVerliHub::nPerlPlugin;

static cServerDC * GetCurrentVerlihub() {
	return (cServerDC *)cServerDC::sCurrentServer;
}

static cpiPerl * GetPI() {
	cServerDC *server = GetCurrentVerlihub();
	return (cpiPerl *)server->mPluginManager.GetPlugin(PERLSCRIPT_PI_IDENTIFIER);
}

int nVerliHub::nPerlPlugin::nCallback::SQLQuery(const char *query) {
	cpiPerl *pi = GetPI();

	pi->mQuery->Clear();
	pi->mQuery->OStream() << query;
	pi->mQuery->Query();
	
	return pi->mQuery->StoreResult();
}

MYSQL_ROW nVerliHub::nPerlPlugin::nCallback::SQLFetch(int r, int &cols) {
	cpiPerl *pi = GetPI();
	if(!pi->mQuery->GetResult()) {
		// FIXME pass error to perl?
		return NULL;
	}

	pi->mQuery->DataSeek(r);

	MYSQL_ROW row;

	if(!(row = pi->mQuery->Row())) {
		// FIXME pass error to perl?
		return NULL;
	}

	cols = pi->mQuery->Cols();

	return row;
}

int nVerliHub::nPerlPlugin::nCallback::SQLFree() {
	cpiPerl *pi = GetPI();
	pi->mQuery->Clear();
	return 1;
}

int nVerliHub::nPerlPlugin::nCallback::IsUserOnline(const char *nick) {
	cServerDC *server = GetCurrentVerlihub();
	cUser *usr = server->mUserList.GetUserByNick(nick);
	return usr == NULL ? 0 : 1;
}

int nVerliHub::nPerlPlugin::nCallback::GetUpTime() {
	cServerDC *server = GetCurrentVerlihub();
	cTime upTime = server->mTime;
	upTime -= server->mStartTime;
	return upTime.Sec();
}

const char *nVerliHub::nPerlPlugin::nCallback::GetHubIp() {
	cServerDC *server = GetCurrentVerlihub();
	return server->mAddr.c_str();
}

const char *nVerliHub::nPerlPlugin::nCallback::GetHubSecAlias() {
	cServerDC *server = GetCurrentVerlihub();
	return server->mC.hub_security.c_str();
}

const char *nVerliHub::nPerlPlugin::nCallback::GetOPList() {
	cServerDC *server = GetCurrentVerlihub();
	string list;
	server->mOpList.GetNickList(list);
	return strdup(list.c_str());
}

const char *nVerliHub::nPerlPlugin::nCallback::GetBotList() {
	cServerDC *server = GetCurrentVerlihub();
	string list;
	server->mRobotList.GetNickList(list);
	return strdup(list.c_str());
}


bool nVerliHub::nPerlPlugin::nCallback::RegBot(const char *nick, int clas, const char *desc, const char *conn, const char *mail, const char *shar)
{
	cServerDC *serv = GetCurrentVerlihub();

	if (!serv)
		return false;

	cpiPerl *perl = GetPI();

	if (!perl)
		return false;

	string info;
	serv->mP.Create_MyINFO(info, nick, desc, conn, mail, shar);

	if (!perl->NewRobot(nick, clas, info)) { // note: this will show user to all
		// error: "Error adding bot; it may already exist"
	    return false;
	}

	//perl->mPerl.addBot(nick, shar, (char*)info.c_str(), clas);
	return true;
}

bool nVerliHub::nPerlPlugin::nCallback::EditBot(const char *nick, int clas, const char *desc, const char *conn, const char *mail, const char *shar)
{
	cServerDC *serv = GetCurrentVerlihub();

	if (!serv)
		return false;

	cUserRobot *robot = (cUserRobot*)serv->mRobotList.GetUserBaseByNick(nick);

	if (!robot) {
		// error: "???"
		return false;
	}

	serv->mP.Create_MyINFO(robot->mFakeMyINFO, nick, desc, conn, mail, shar);
	//pi->mPerl.editBot(nick, shar, (char*)robot->mFakeMyINFO.c_str(), clas);
	string temp(robot->mFakeMyINFO);
	serv->mUserList.SendToAll(temp, serv->mC.delayed_myinfo, true); // show new myinfo to all

	if (clas >= 3) { // todo: oplist_class, userip
		serv->mP.Create_OpList(temp, nick); // send short oplist
		serv->mUserList.SendToAll(temp, serv->mC.delayed_myinfo, true);
	}

	return true;
}

bool nVerliHub::nPerlPlugin::nCallback::UnRegBot(const char *nick)
{
	cServerDC *serv = GetCurrentVerlihub();

	if (!serv)
		return false;

	cpiPerl *perl = GetPI();

	if (!perl)
		return false;

	cUserRobot *robot = (cUserRobot*)serv->mRobotList.GetUserBaseByNick(nick);

	if (robot) {
		//perl->mPerl.delBot(nick);
		perl->DelRobot(robot); // delete bot, this will also send quit to all
	} else {
		// error: "Bot doesn't exist"
		return false;
	}

	return true;
}

int nVerliHub::nPerlPlugin::nCallback::IsBot(const char *nick)
{
	cServerDC *serv = GetCurrentVerlihub();

	if (!serv)
		return 0;

	return (serv->mRobotList.ContainsNick(nick) ? 1 : 0);
}

const char *nVerliHub::nPerlPlugin::nCallback::GetTopic() {
	cServerDC *server = GetCurrentVerlihub();
	return server->mC.hub_topic.c_str();
}

bool nVerliHub::nPerlPlugin::nCallback::SetTopic(const char *_topic) {
	cServerDC *server = GetCurrentVerlihub();
	std::string topic = _topic;
	std::string message;
	SetConfig("config", "hub_topic", _topic);
	server->mP.Create_HubName(message, server->mC.hub_name, topic);
	server->SendToAll(message, eUC_NORMUSER, eUC_MASTER);
	return true;
}

bool nVerliHub::nPerlPlugin::nCallback::ScriptCommand(const char *_cmd, const char *_data) {
	//cServerDC *server = GetCurrentVerlihub();
	std::string cmd = _cmd;
	std::string data = _data;
	std::string plugin("perl");
	::ScriptCommand(&cmd, &data, &plugin, &(GetPI()->mPerl.mScriptStack.back()));
	return true;
}

bool nVerliHub::nPerlPlugin::nCallback::InUserSupports(const char *nick, const char *_flag) {
	cServerDC *serv = GetCurrentVerlihub();

	if (!serv)
		return false;

	std::string flag = _flag;
	int iflag = atoi(_flag); // todo: IsNumber
	cUser *user = serv->mUserList.GetUserByNick(nick);

	if (!user || !user->mxConn)
		return false;

	if (
		((flag == "OpPlus") && (user->mxConn->mFeatures & eSF_OPPLUS)) ||
		((flag == "NoHello") && (user->mxConn->mFeatures & eSF_NOHELLO)) ||
		((flag == "NoGetINFO") && (user->mxConn->mFeatures & eSF_NOGETINFO)) ||
		((flag == "DHT0") && (user->mxConn->mFeatures & eSF_DHT0)) ||
		((flag == "QuickList") && (user->mxConn->mFeatures & eSF_QUICKLIST)) ||
		((flag == "BotINFO") && (user->mxConn->mFeatures & eSF_BOTINFO)) ||
		(((flag == "ZPipe0") || (flag == "ZPipe")) && (user->mxConn->mFeatures & eSF_ZLIB)) ||
		((flag == "ChatOnly") && (user->mxConn->mFeatures & eSF_CHATONLY)) ||
		((flag == "MCTo") && (user->mxConn->mFeatures & eSF_MCTO)) ||
		((flag == "UserCommand") && (user->mxConn->mFeatures & eSF_USERCOMMAND)) ||
		((flag == "BotList") && (user->mxConn->mFeatures & eSF_BOTLIST)) ||
		((flag == "HubTopic") && (user->mxConn->mFeatures & eSF_HUBTOPIC)) ||
		((flag == "UserIP2") && (user->mxConn->mFeatures & eSF_USERIP2)) ||
		((flag == "TTHSearch") && (user->mxConn->mFeatures & eSF_TTHSEARCH)) ||
		((flag == "Feed") && (user->mxConn->mFeatures & eSF_FEED)) ||
		((flag == "TTHS") && (user->mxConn->mFeatures & eSF_TTHS)) ||
		((flag == "IN") && (user->mxConn->mFeatures & eSF_IN)) ||
		((flag == "BanMsg") && (user->mxConn->mFeatures & eSF_BANMSG)) ||
		((flag == "TLS") && (user->mxConn->mFeatures & eSF_TLS)) ||
		(user->mxConn->mFeatures & iflag)
	)
		return true;

	return false;
}

bool nVerliHub::nPerlPlugin::nCallback::ReportUser(const char *nick, const char *msg) {
	cServerDC *serv = GetCurrentVerlihub();
	cUser *usr = serv->mUserList.GetUserByNick(nick);
	if ((usr == NULL) || (usr->mxConn == NULL))
		return false;
	serv->ReportUserToOpchat(usr->mxConn, msg, false);
	return true;
}

