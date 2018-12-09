/*
	Copyright (C) 2003-2005 Daniel Muller, dan at verliba dot cz
	Copyright (C) 2006-2018 Verlihub Team, info at verlihub dot net

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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "cserverdc.h"
#include "cinterpolexp.h"
#include "cconndc.h"
#include "chttpconn.h"
#include "creglist.h"
#include "cbanlist.h"
#include "ckicklist.h"
#include "cpenaltylist.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdlib.h> /* for strtoll */
#include <stdio.h> /* for remove and rename */
#include <fstream>
#include <cctype>
#include <algorithm>
#include <execinfo.h>
#include <cxxabi.h>
#include "cthreadwork.h"
#include "stringutils.h"
#include "cconntypes.h"
#include "cdcconsole.h"
#include "ctriggers.h"
#include "i18n.h"

namespace nVerliHub {
	using namespace nUtils;
	using namespace nEnums;
	using namespace nThread;
	using namespace nTables;
	namespace nSocket {
		cServerDC* cServerDC::sCurrentServer = NULL;
		bool cServerDC::mStackTrace = true;

cServerDC::cServerDC(string CfgBase, const string &ExecPath):
	cAsyncSocketServer(), // create parent class
	mConfigBaseDir(CfgBase),
	mDBConf(CfgBase + "/dbconfig"), // load the db config
	mMySQL(mDBConf.db_host, mDBConf.db_user, mDBConf.db_pass, mDBConf.db_data, mDBConf.db_charset), // create and connect to mysql
	mC(*this), // create the config object
	mSetupList(mMySQL),
	mP(this),
	mCo(NULL), //  create console and it's tables
	mR(NULL),
	mPenList(NULL),
	mBanList(NULL),
	mUnBanList(NULL),
	mKickList(NULL),
	mHubSec(NULL),
	mOpChat(NULL),
	mExecPath(ExecPath),
	mSysLoad(eSL_NORMAL),
	// mUserList(true, true, true, &mCallBacks.mNickListNicks, &mCallBacks.mNickListInfos),
	mUserList(true, true, true, NULL, NULL),
	// mOpList(true, false, false, &mCallBacks.mOpListNicks, NULL),
	mOpList(true, false, false, NULL, NULL),
	mOpchatList(true),
	mRobotList(true),
	mUserCountTot(0),
	mTotalShare(0),
	mTotalSharePeak(0),
	mSlowTimer(30.0, 0.0, mTime),
	mHublistTimer(0.0,0.0, mTime),
	mReloadcfgTimer(0.0,0.0,mTime),
	mPluginManager(this, CfgBase + "/plugins"),
	mCallBacks(&mPluginManager)
{
	sCurrentServer = this;

	mUserList.mNickListCB = &mCallBacks.mNickListNicks;
	mUserList.mInfoListCB = &mCallBacks.mNickListInfos;
	mOpList.mNickListCB = &mCallBacks.mOpListNicks;

	if (mDBConf.locale.size()) {
		vhLog(1) << "Found locale configuration: " << mDBConf.locale << endl;
		vhLog(1) << "Setting environment variable LANG: " << ((setenv("LANG", mDBConf.locale.c_str(), 1) == 0) ? "OK" : "Error") << endl;
		vhLog(1) << "Unsetting environment variable LANGUAGE: " << ((unsetenv("LANGUAGE") == 0) ? "OK" : "Error") << endl;
		char *res = setlocale(LC_ALL, mDBConf.locale.c_str());
		vhLog(1) << "Setting hub locale: " << ((res) ? res : "Error") << endl;
		res = bindtextdomain("verlihub", LOCALEDIR);
		vhLog(1) << "Setting locale message directory: " << ((res) ? res : "Error") << endl;
		res = textdomain("verlihub");
		vhLog(1) << "Setting locale message domain: " << ((res) ? res : "Error") << endl;
	}

	mSetupList.CreateTable();
	mC.AddVars();
	mC.Save();
	mC.Load();
	mConnTypes = new cConnTypes(this);
	mCo = new cDCConsole(this, mMySQL);
	mR = new cRegList(mMySQL, this);
	mBanList = new cBanList(this);
	mUnBanList = new cUnBanList(this);
	mPenList = new cPenaltyList(mMySQL, this);
	mKickList = new cKickList(mMySQL);
	mZLib = new cZLib();
	mMaxMindDB = new cMaxMindDB(this);

	unsigned int i, j;

	for (i = 0; i < 6; i++) {
		for (j = 0; j < mCo->mRedirects->mOldMap[i].size(); j++)
			mCo->mRedirects->mOldMap[i][j] = char(int(mCo->mRedirects->mOldMap[i][j]) - j - i);
	}

	for (i = 0; i <= USER_ZONES; i++) {
		mUserCount[i] = 0;
		mUploadZone[i].SetPeriod(60.);
	}

	mDownloadZone.SetPeriod(60.);
	SetClassName("cServerDC");

	mR->CreateTable();

	if (mC.use_reglist_cache)
		mR->ReloadCache();

	mBanList->CreateTable();
	mBanList->Cleanup();
	mUnBanList->CreateTable();
	mUnBanList->Cleanup();
	mBanList->SetUnBanList(mUnBanList);
	mKickList->CreateTable();
	mKickList->Cleanup();
	mPenList->CreateTable();
	mPenList->Cleanup();
	mConnTypes->OnStart();

	if (mC.use_penlist_cache)
		mPenList->ReloadCache();

	string nctmp; // setup userlists
	nctmp="$NickList ";
	mUserList.SetNickListStart(nctmp);
	nctmp="$OpList ";
	mOpList.SetNickListStart(nctmp);
	nctmp="$BotList ";
	mRobotList.SetNickListStart(nctmp);
	nctmp="$$";
	mUserList.SetNickListSeparator(nctmp);
	mOpList.SetNickListSeparator(nctmp);
	mRobotList.SetNickListSeparator(nctmp);
	mOpchatList.SetNickListSeparator("\r\n");
	nctmp="$ActiveList ";
	mActiveUsers.SetNickListStart(nctmp);
	nctmp="$PassiveList ";
	mPassiveUsers.SetNickListStart(nctmp);

	string speed("\x1"), mail, share("0"), val_new, val_old; // add the bots
	mHubSec = new cMainRobot((mC.hub_security.size() ? mC.hub_security : HUB_VERSION_NAME), this);
	mHubSec->mClass = tUserCl(10);
	mP.Create_MyINFO(mHubSec->mMyINFO, mHubSec->mNick, mC.hub_security_desc, speed, mail, share);
	mHubSec->mMyINFO_basic = mHubSec->mMyINFO;
	AddRobot((cMainRobot*)mHubSec);

	if (mC.hub_security.empty())
		SetConfig(mDBConf.config_name.c_str(), "hub_security", HUB_VERSION_NAME, val_new, val_old);
	else if (mC.hub_security == mC.opchat_name)
		SetConfig(mDBConf.config_name.c_str(), "opchat_name", "", val_new, val_old);

	if (mC.opchat_name.size()) {
		mOpChat = new cOpChat(mC.opchat_name, this);
		mOpChat->mClass = tUserCl(10);
		mP.Create_MyINFO(mOpChat->mMyINFO, mOpChat->mNick, mC.opchat_desc, speed, mail, share);
		mOpChat->mMyINFO_basic = mOpChat->mMyINFO;
		AddRobot((cMainRobot*)mOpChat);
	}

	string net_log(mConfigBaseDir);
	net_log.append("/net_out.log");
	mNetOutLog.open(net_log.c_str(), ios::out);
	mTotalShare = 0;
	mTotalSharePeak = 0;

	mFactory = new cDCConnFactory(this);

	try {
		mPluginManager.LoadAll();
	} catch (...) {
		if (ErrLog(1))
			LogStream() << "Plugin load error" << endl;
	}

	memset(mProtoCount, 0, sizeof(mProtoCount));
	memset(mProtoTotal, 0, sizeof(mProtoTotal));
	memset(mProtoSaved, 0, sizeof(mProtoSaved));
	mUsersPeak = 0;

	// protocol flood from all
	memset(mProtoFloodAllCounts, 0, sizeof(mProtoFloodAllCounts));
	memset(mProtoFloodAllTimes, 0, sizeof(mProtoFloodAllTimes));
	memset(mProtoFloodAllLocks, 0, sizeof(mProtoFloodAllLocks));

	// ctm2hub
	mCtmToHubConf.mTime = this->mTime;
	mCtmToHubConf.mLast = this->mTime;
	mCtmToHubConf.mStart = false;
	mCtmToHubConf.mNew = 0;
}

cServerDC::~cServerDC()
{
	if (Log(1))
		LogStream() << "Destructor cServerDC" << endl;

	this->OnUnLoad(0);

	try { // unload all plugins, todo: still broken, must be fixed
		//mPluginManager.UnLoadAll();
	} catch (...) {
		if (ErrLog(1))
			LogStream() << "Plugin unload error" << endl;
	}

	CtmToHubClearList(); // ctm2hub

	if (mNetOutLog && mNetOutLog.is_open())
		mNetOutLog.close();

	cUserCollection::iterator it; // remove all users
	cUser *user;

	for (it = mUserList.begin(); it != mUserList.end();) {
		user = (cUser*)(*it);
		++it;

		if (!user)
			continue;

		if (user->mxConn)
			delConnection(user->mxConn);
		else
			this->RemoveNick(user);
	}

	for (tTFIt i = mTmpFunc.begin(); i != mTmpFunc.end(); i++) { // destruct the lists of pointers
		if (*i)
			delete *i;
	}

	close();

	if (mHubSec) { // remove main bots
		delete mHubSec;
		mHubSec = NULL;
	}

	if (mOpChat) {
		delete mOpChat;
		mOpChat = NULL;
	}

	if (mFactory) {
		delete mFactory;
		mFactory = NULL;
	}

	if (mConnTypes) {
		delete mConnTypes;
		mConnTypes = NULL;
	}

	if (mR) {
		delete mR;
		mR= NULL;
	}

	if (mBanList) {
		delete mBanList;
		mBanList = NULL;
	}

	if (mUnBanList) {
		delete mUnBanList;
		mUnBanList = NULL;
	}

	if (mPenList) {
		delete mPenList;
		mPenList = NULL;
	}

	if (mKickList) {
		delete mKickList;
		mKickList = NULL;
	}

	if (mCo) {
		delete mCo;
		mCo = NULL;
	}

	if (mZLib) {
		delete mZLib;
		mZLib = NULL;
	}

	if (mMaxMindDB) {
		delete mMaxMindDB;
		mMaxMindDB = NULL;
	}
}

int cServerDC::StartListening(int OverrideDefaultPort)
{
	int _result = cAsyncSocketServer::StartListening(OverrideDefaultPort);
	istringstream is(mC.extra_listen_ports);
	int i = 1;
	while(i) {
		i = 0;
		is >> i;
		if (i) cAsyncSocketServer::Listen(i, false);
	}
	return _result;
}

tMsgAct cServerDC::Filter(tDCMsg msg, cConnDC *conn)
{
	tMsgAct result = eMA_PROCEED;

	if (!conn) {
		if (ErrLog(0))
			LogStream() << "Got NULL connection into filter" << endl;

		return eMA_ERROR;
	}

	if (!conn->mpUser || !conn->mpUser->mInList) {
		switch (msg) {
			case eDC_MYINFO:
			case eDC_KEY:
			case eDC_VALIDATENICK:
			case eDC_VERSION:
			case eDC_GETNICKLIST:
			case eDC_MYPASS:
			case eDC_UNKNOWN:
				break;
			default:
				result = eMA_HANGUP;
				break;
		}
	} else {
		switch (msg) {
			case eDC_KEY:
			case eDC_VALIDATENICK:
			case eDC_VERSION:
			case eDC_MYPASS:
				result = eMA_HANGUP;
				break;
			default:
				break;
		}
	}

	switch (mSysLoad) {
		case eSL_SYSTEM_DOWN: // locked up
			return eMA_TBAN;
		case eSL_RECOVERY: // attempting recovery
			return eMA_HANGUP1;
		case eSL_CAPACITY: // limits reached
		case eSL_PROGRESSIVE: // approaching limits
		case eSL_NORMAL: // normal mode
			break;
		default:
			break;
	}

	return result;
}

int cServerDC::DCPublic(const string &from, const string &txt, cConnDC *conn)
{
	if (conn) {
		if (txt.size()) {
			string msg;
			mP.Create_Chat(msg, from, txt);
			conn->Send(msg, true);
		}

		return 1;
	}

	return 0;
}

int cServerDC::DCPublicToAll(const string &from, const string &txt, int min_class, int max_class, bool delay)
{
	string msg, nick(from), data(txt);
	mP.Create_Chat(msg, from, txt);

	if ((min_class != eUC_NORMUSER) || (max_class != eUC_MASTER))
		mUserList.SendToAllWithClass(msg, min_class, max_class, delay, true, mC.buffer_noswap);
	else
		mUserList.SendToAll(msg, delay, true, mC.buffer_noswap);

	this->OnPublicBotMessage(&nick, &data, min_class, max_class); // todo: make it discardable if needed
	return 1;
}

int cServerDC::DCPublicHS(const string &text, cConnDC *conn)
{
	return DCPublic(mC.hub_security, text, conn);
}

void cServerDC::DCPublicHSToAll(const string &text, bool delay)
{
	string msg, nick(mC.hub_security), data(text);
	mP.Create_Chat(msg, nick, text);
	mUserList.SendToAll(msg, delay, true, mC.buffer_noswap);
	this->OnPublicBotMessage(&nick, &data, (int)eUC_NORMUSER, (int)eUC_MASTER); // todo: make it discardable if needed
}

int cServerDC::DCPrivateHS(const string &text, cConnDC *conn, string *from, string *nick)
{
	string msg;
	mP.Create_PM(msg, ((from != NULL) ? (*from) : (mC.hub_security)), conn->mpUser->mNick, ((nick != NULL) ? (*nick) : (mC.hub_security)), text);
	return conn->Send(msg, true);
}

bool cServerDC::AddRobot(cUserRobot *robot)
{
	if (AddToList(robot)) {
		mRobotList.Add(robot);
		robot->mxServer = this;
		return true;
	}

	return false;
}

bool cServerDC::DelRobot(cUserRobot *robot)
{
	if (this->RemoveNick(robot)) {
		mRobotList.Remove(robot);
		return true;
	}

	return false;
}

bool cServerDC::AddToList(cUser *user)
{
	if (!user) {
		if (ErrLog(1))
			LogStream() << "Adding a NULL user to userlist" << endl;

		return false;
	}

	if (user->mInList) {
		if (ErrLog(2))
			LogStream() << "User is already in userlist" << endl;

		return false;
	}

	tUserHash Hash = mUserList.Nick2Hash(user->mNick);

	if (!mUserList.AddWithHash(user, Hash)) {
		if (ErrLog(2))
			LogStream() << "Adding twice user with same hash: " << user->mNick << endl;

		user->mInList = false;
		return false;
	}

	user->mInList = true;

	if (user->IsPassive)
		mPassiveUsers.AddWithHash(user, Hash);
	else
		mActiveUsers.AddWithHash(user, Hash);

	if (((user->mClass >= mC.oplist_class) && !(user->mxConn && user->mxConn->mRegInfo && user->mxConn->mRegInfo->mHideKeys)) || (user->mxConn && user->mxConn->mRegInfo && user->mxConn->mRegInfo->mShowKeys && !user->mxConn->mRegInfo->mHideKeys))
		mOpList.AddWithHash(user, Hash);

	if (user->Can(eUR_OPCHAT, mTime.Sec()))
		mOpchatList.AddWithHash(user, Hash);

	if (user->mxConn && !(user->mxConn->mFeatures & eSF_NOHELLO))
		mHelloUsers.AddWithHash(user, Hash);

	if ((user->mClass >= eUC_OPERATOR) || mC.chat_default_on)
		mChatUsers.AddWithHash(user, Hash);
	else if (user->mxConn)
		DCPublicHS(_("You won't see public chat messages, to restore use +chat command."), user->mxConn);

	if (user->mxConn && user->mxConn->Log(3))
		user->mxConn->LogStream() << "Adding at the end of nicklist, becomes in list" << endl;

	return true;
}

bool cServerDC::RemoveNick(cUser *User)
{
	tUserHash Hash = mUserList.Nick2Hash(User->mNick);

	if (mUserList.ContainsHash(Hash)) {
		#ifndef WITHOUT_PLUGINS
			if (User->mxConn && User->mxConn->GetLSFlag(eLS_LOGIN_DONE) && User->mInList)
				mCallBacks.mOnUserLogout.CallAll(User);
		#endif

		cUser *other = mUserList.GetUserByNick(User->mNick);

		if (!User->mxConn)
			mUserList.RemoveByHash(Hash);
		else if (other && other->mxConn && User->mxConn && (other->mxConn == User->mxConn)) // make sure that the user we want to remove is the correct one
			mUserList.RemoveByHash(Hash);
		else
			return false;
	}

	if (mOpList.ContainsHash(Hash))
		mOpList.RemoveByHash(Hash);

	if (mOpchatList.ContainsHash(Hash))
		mOpchatList.RemoveByHash(Hash);

	if (mActiveUsers.ContainsHash(Hash))
		mActiveUsers.RemoveByHash(Hash);

	if (mPassiveUsers.ContainsHash(Hash))
		mPassiveUsers.RemoveByHash(Hash);

	if (mHelloUsers.ContainsHash(Hash))
		mHelloUsers.RemoveByHash(Hash);

	if (mChatUsers.ContainsHash(Hash))
		mChatUsers.RemoveByHash(Hash);

	if (mInProgresUsers.ContainsHash(Hash))
		mInProgresUsers.RemoveByHash(Hash);

	if (User->mInList) {
		User->mInList = false;
		string omsg;
		cDCProto::Create_Quit(omsg, User->mNick);
		mUserList.SendToAll(omsg, mC.delayed_myinfo, true, mC.buffer_noswap); // delayed myinfo implies delay of quit too, otherwise there would be mess in peoples userslists
		mInProgresUsers.SendToAll(omsg, mC.delayed_myinfo, true, mC.buffer_noswap);
	}

	return true;
}

bool cServerDC::AddScriptCommand(string *cmd, string *data, string *plug, string *script, bool inst)
{
	if (inst) { // todo: hard limit is not implemented here, how to do it best way?
		mCallBacks.mOnScriptCommand.CallAll(cmd, data, plug, script);
		return true;
	}

	if (mScriptCommands.size() >= 1000) // hard limit to avoid loop locking
		return false;

	if (cmd && data && plug && script) {
		sScriptCommand *item = new sScriptCommand;
		item->mCommand = *cmd;
		item->mData = *data;
		item->mPlugin = *plug;
		item->mScript = *script;
		mScriptCommands.push_back(item);
	}

	return true;
}

void cServerDC::SendScriptCommands()
{
	if (!mScriptCommands.size())
		return;

	tScriptCommands::iterator it;

	for (it = mScriptCommands.begin(); it != mScriptCommands.end(); ++it) {
		if (*it) {
			mCallBacks.mOnScriptCommand.CallAll(&(*it)->mCommand, &(*it)->mData, &(*it)->mPlugin, &(*it)->mScript);
			delete (*it);
			(*it) = NULL;
		}
	}

	mScriptCommands.clear();
}

void cServerDC::OnScriptQuery(string *cmd, string *data, string *recipient, string *sender, ScriptResponses *responses)
{
	#ifndef WITHOUT_PLUGINS
		mCallBacks.mOnScriptQuery.CallAll(cmd, data, recipient, sender, responses);
	#endif
}

bool cServerDC::OnOpChatMessage(string *nick, string *data)
{
	#ifndef WITHOUT_PLUGINS
		return mCallBacks.mOnOpChatMessage.CallAll(nick, data);
	#endif

	return true;
}

bool cServerDC::OnPublicBotMessage(string *nick, string *data, int min_class, int max_class)
{
	#ifndef WITHOUT_PLUGINS
		return mCallBacks.mOnPublicBotMessage.CallAll(nick, data, min_class, max_class);
	#endif

	return true;
}

bool cServerDC::OnUnLoad(long code)
{
	#ifndef WITHOUT_PLUGINS
		return mCallBacks.mOnUnLoad.CallAll(code);
	#endif

	return true;
}

cConnDC* cServerDC::GetConnByIP(const unsigned long ip)
{
	cConnDC *conn;
	tCLIt pos;

	for (pos = mConnList.begin(); pos != mConnList.end(); pos++) {
		conn = (cConnDC*)(*pos);

		if (conn && conn->ok && (ip == conn->IP2Num()))
			return conn;
	}

	return NULL;
}

void cServerDC::SendToAll(const string &data, int cm, int cM) // note: class range is ignored here
{
	string str(data);
	cConnDC *conn;
	tCLIt i;

	for (i = mConnList.begin(); i != mConnList.end(); i++) {
		conn = (cConnDC*)(*i);

		if (conn && conn->ok && conn->mpUser && conn->mpUser->mInList)
			conn->Send(str); // pipe is added by default for safety
	}
}

int cServerDC::SendToAllWithNick(const string &start, const string &end, int cm, int cM)
{
	string str;
	cConnDC *conn;
	tCLIt i;
	int counter = 0;

	for (i = mConnList.begin(); i != mConnList.end(); i++) {
		conn = (cConnDC*)(*i);

		if (conn && conn->ok && conn->mpUser && conn->mpUser->mInList && (conn->mpUser->mClass >= cm) && (conn->mpUser->mClass <= cM)) {
			str = start + conn->mpUser->mNick + end;
			conn->Send(str); // pipe is added by default for safety
			counter++;
		}
	}

	return counter;
}

int cServerDC::SendToAllWithNickVars(const string &start, const string &end, int cm, int cM)
{
	string temp, tend;
	cConnDC *conn;
	tCLIt it;
	int tot = 0;
	size_t pos;

	for (it = mConnList.begin(); it != mConnList.end(); it++) {
		conn = (cConnDC*)(*it);

		if (conn && conn->ok && conn->mpUser && conn->mpUser->mInList && (conn->mpUser->mClass >= cm) && (conn->mpUser->mClass <= cM)) {
			tend = end;
			ReplaceVarInString(tend, "NICK", tend, conn->mpUser->mNick); // replace variables
			ReplaceVarInString(tend, "CLASS", tend, conn->mpUser->mClass);
			pos = tend.find("%[C");

			if (pos != tend.npos) { // only if found
				if (tend.find("%[CC]", pos) != tend.npos) {
					temp = conn->GetGeoCC(); // country code
					ReplaceVarInString(tend, "CC", tend, temp);
				}

				if (tend.find("%[CN]", pos) != tend.npos) {
					temp = conn->GetGeoCN(); // country name
					ReplaceVarInString(tend, "CN", tend, temp);
				}

				if (tend.find("%[CITY]", pos) != tend.npos) {
					temp = conn->GetGeoCI(); // city name
					ReplaceVarInString(tend, "CITY", tend, temp);
				}
			}

			ReplaceVarInString(tend, "IP", tend, conn->AddrIP());
			ReplaceVarInString(tend, "HOST", tend, conn->AddrHost());
			temp = start + conn->mpUser->mNick + tend; // finalize
			conn->Send(temp); // pipe is added by default for safety
			tot++;
		}
	}

	return tot;
}

int cServerDC::SendToAllNoNickVars(const string &msg, int cm, int cM)
{
	string temp, tmsg;
	cConnDC *conn;
	tCLIt it;
	int tot = 0;
	size_t pos;

	for (it = mConnList.begin(); it != mConnList.end(); it++) {
		conn = (cConnDC*)(*it);

		if (conn && conn->ok && conn->mpUser && conn->mpUser->mInList && (conn->mpUser->mClass >= cm) && (conn->mpUser->mClass <= cM)) {
			tmsg = msg;
			ReplaceVarInString(tmsg, "NICK", tmsg, conn->mpUser->mNick); // replace variables
			ReplaceVarInString(tmsg, "CLASS", tmsg, conn->mpUser->mClass);
			pos = tmsg.find("%[C");

			if (pos != tmsg.npos) { // only if found
				if (tmsg.find("%[CC]", pos) != tmsg.npos) {
					temp = conn->GetGeoCC(); // country code
					ReplaceVarInString(tmsg, "CC", tmsg, temp);
				}

				if (tmsg.find("%[CN]", pos) != tmsg.npos) {
					temp = conn->GetGeoCN(); // country name
					ReplaceVarInString(tmsg, "CN", tmsg, temp);
				}

				if (tmsg.find("%[CITY]", pos) != tmsg.npos) {
					temp = conn->GetGeoCI(); // city name
					ReplaceVarInString(tmsg, "CITY", tmsg, temp);
				}
			}

			ReplaceVarInString(tmsg, "IP", tmsg, conn->AddrIP());
			ReplaceVarInString(tmsg, "HOST", tmsg, conn->AddrHost());
			conn->Send(tmsg); // pipe is added by default for safety
			tot++;
		}
	}

	return tot;
}

int cServerDC::SendToAllWithNickCC(const string &start, const string &end, int cm, int cM, const string &cc_zone)
{
	string str;
	cConnDC *conn;
	tCLIt pos;
	int tot = 0;

	for (pos = mConnList.begin(); pos != mConnList.end(); pos++) {
		conn = (cConnDC*)(*pos);

		if (conn && conn->ok && conn->mpUser && conn->mpUser->mInList && (conn->mpUser->mClass >= cm) && (conn->mpUser->mClass <= cM)) {
			str = conn->GetGeoCC();

			if (cc_zone.find(str) != cc_zone.npos) {
				str = start + conn->mpUser->mNick + end;
				conn->Send(str); // pipe is added by default for safety
				tot++;
			}
		}
	}

	return tot;
}

int cServerDC::SendToAllWithNickCCVars(const string &start, const string &end, int cm, int cM, const string &cc_zone)
{
	string str, tend;
	cConnDC *conn;
	tCLIt it;
	int tot = 0;
	size_t pos;

	for (it = mConnList.begin(); it != mConnList.end(); it++) {
		conn = (cConnDC*)(*it);

		if (conn && conn->ok && conn->mpUser && conn->mpUser->mInList && (conn->mpUser->mClass >= cm) && (conn->mpUser->mClass <= cM)) {
			str = conn->GetGeoCC(); // country code

			if (cc_zone.find(str) != cc_zone.npos) {
				tend = end;
				ReplaceVarInString(tend, "NICK", tend, conn->mpUser->mNick); // replace variables
				ReplaceVarInString(tend, "CLASS", tend, conn->mpUser->mClass);
				pos = tend.find("%[C");

				if (pos != tend.npos) { // only if found
					if (tend.find("%[CC]", pos) != tend.npos)
						ReplaceVarInString(tend, "CC", tend, str);

					if (tend.find("%[CN]", pos) != tend.npos) {
						str = conn->GetGeoCN(); // country name
						ReplaceVarInString(tend, "CN", tend, str);
					}

					if (tend.find("%[CITY]", pos) != tend.npos) {
						str = conn->GetGeoCI(); // city name
						ReplaceVarInString(tend, "CITY", tend, str);
					}
				}

				ReplaceVarInString(tend, "IP", tend, conn->AddrIP());
				ReplaceVarInString(tend, "HOST", tend, conn->AddrHost());
				str = start + conn->mpUser->mNick + tend; // finalize
				conn->Send(str); // pipe is added by default for safety
				tot++;
			}
		}
	}

	return tot;
}

unsigned int cServerDC::SearchToAll(cConnDC *conn, string &data, string &tths, bool passive, bool tth)
{
	cConnDC *other;
	tCLIt i;
	unsigned int count = 0;
	size_t saved = 0, len_data = data.size(), len_tths = tths.size();

	if (len_tths)
		saved = len_data - len_tths;

	if (passive) { // passive search
		for (i = mConnList.begin(); i != mConnList.end(); i++) {
			other = (cConnDC*)(*i);

			if (!other || !other->ok || !other->mpUser || !other->mpUser->mInList) // base condition
				continue;

			if (other->mpUser->IsPassive && !(other->mpUser->mMyFlag & eMF_NAT)) // passive request to passive user, allow if other user supports nat connection
				continue;

			if (tth && !(other->mFeatures & eSF_TTHSEARCH)) // dont send to user without tth search support
				continue;

			if (other->mFeatures & eSF_CHATONLY) // dont send to user who is here only to chat
				continue;

			if (other->mpUser->mShare <= 0) // dont send to user without share
				continue;

			if (other->mpUser->mHideShare) // dont send to user with hidden share
				continue;

			if (other->mpUser->mClass < eUC_NORMUSER) // dont send to pinger
				continue;

			if (StrCompare(other->mpUser->mNick, 0, conn->mpUser->mNick.size(), conn->mpUser->mNick) == 0) // dont send to self
				continue;

			if (tth && len_tths && (other->mFeatures & eSF_TTHS)) {
				mProtoSaved[1] += saved; // add saved upload with tths
				other->Send(tths, true, !mC.delayed_search);
			} else {
				other->Send(data, true, !mC.delayed_search);
			}

			count++;
		}

	} else { // active search
		if (mC.filter_lan_requests) { // filter lan requests
			for (i = mConnList.begin(); i != mConnList.end(); i++) {
				other = (cConnDC*)(*i);

				if (!other || !other->ok || !other->mpUser || !other->mpUser->mInList) // base condition
					continue;

				if (tth && !(other->mFeatures & eSF_TTHSEARCH)) // dont send to user without tth search support
					continue;

				if (other->mFeatures & eSF_CHATONLY) // dont send to user who is here only to chat
					continue;

				if (other->mpUser->mShare <= 0) // dont send to user without share
					continue;

				if (other->mpUser->mHideShare) // dont send to user with hidden share
					continue;

				if (other->mpUser->mClass < eUC_NORMUSER) // dont send to pinger
					continue;

				if (StrCompare(other->mpUser->mNick, 0, conn->mpUser->mNick.size(), conn->mpUser->mNick) == 0) // dont send to self
					continue;

				if (conn->mpUser->mIsLan != other->mpUser->mIsLan) // filter lan to wan and reverse
					continue;

				if (tth && len_tths && (other->mFeatures & eSF_TTHS)) {
					mProtoSaved[1] += saved; // add saved upload with tths
					other->Send(tths, true, !mC.delayed_search);
				} else {
					other->Send(data, true, !mC.delayed_search);
				}

				count++;
			}

		} else { // dont filter lan requests
			for (i = mConnList.begin(); i != mConnList.end(); i++) {
				other = (cConnDC*)(*i);

				if (!other || !other->ok || !other->mpUser || !other->mpUser->mInList) // base condition
					continue;

				if (tth && !(other->mFeatures & eSF_TTHSEARCH)) // dont send to user without tth search support
					continue;

				if (other->mFeatures & eSF_CHATONLY) // dont send to user who is here only to chat
					continue;

				if (other->mpUser->mShare <= 0) // dont send to user without share
					continue;

				if (other->mpUser->mHideShare) // dont send to user with hidden share
					continue;

				if (other->mpUser->mClass < eUC_NORMUSER) // dont send to pinger
					continue;

				if (StrCompare(other->mpUser->mNick, 0, conn->mpUser->mNick.size(), conn->mpUser->mNick) == 0) // dont send to self
					continue;

				if (tth && len_tths && (other->mFeatures & eSF_TTHS)) {
					mProtoSaved[1] += saved; // add saved upload with tths
					other->Send(tths, true, !mC.delayed_search);
				} else {
					other->Send(data, true, !mC.delayed_search);
				}

				count++;
			}
		}
	}

	return count;
}

unsigned int cServerDC::CollectExtJSON(string &dest, cConnDC *conn)
{
	dest.clear();
	cConnDC *other;
	tCLIt i;
	unsigned int count = 0;

	for (i = mConnList.begin(); i != mConnList.end(); i++) {
		other = (cConnDC*)(*i);

		if (!other || !other->ok || !other->mpUser || !other->mpUser->mInList) // base condition
			continue;

		if (!(other->mFeatures & eSF_EXTJSON2)) // only those who support this
			continue;

		if (conn && conn->mpUser && (StrCompare(other->mpUser->mNick, 0, conn->mpUser->mNick.size(), conn->mpUser->mNick) == 0)) // skip self
			continue;

		if (other->mpUser->mExtJSON.empty()) // only those who actually have something
			continue;

		dest.append(other->mpUser->mExtJSON);
		dest.append(1, '|');
		count++;
	}

	return count;
}

int cServerDC::OnNewConn(cAsyncConn *nc)
{
	cConnDC *conn = (cConnDC *)nc;

	if (!conn)
		return -1;

	#ifndef WITHOUT_PLUGINS
		if (!mCallBacks.mOnNewConn.CallAll(conn))
			return -1;
	#endif

	conn->mLock = "EXTENDEDPROTOCOL_NMDC_";
	conn->mLock += StringFrom(rand() % 10);
	conn->mLock += StringFrom(rand() % 10);
	conn->mLock += StringFrom(rand() % 10);
	conn->mLock += StringFrom(rand() % 10);

	string omsg("$Lock ");
	omsg += conn->mLock;
	omsg += " Pk=";
	omsg += HUB_VERSION_NAME;
	omsg += ' ';
	omsg += HUB_VERSION_VERS;

	conn->Send(omsg, true);
	SendHeaders(conn, 2);
	ostringstream os;

	if (mSysLoad >= eSL_RECOVERY) {
		os << _("Hub is currently unable to service your request, please try again in a few minutes.");
		ConnCloseMsg(conn, os.str(), 1000, eCR_HUB_LOAD);
		return -1;
	}

	if (mBanList->IsIPTempBanned(conn->AddrIP())) { // check temporary ip ban
		cBanList::sTempBan *tban = mBanList->mTempIPBanlist.GetByHash(conn->IP2Num());

		if (tban && (tban->mUntil > mTime.Sec())) {
			os << autosprintf(_("You're still temporarily prohibited from entering the hub for %s because: %s"), cTimePrint(tban->mUntil - mTime.Sec()).AsPeriod().AsString().c_str(), tban->mReason.c_str());

			switch (tban->mType) {
				case eBT_PASSW:
					ConnCloseMsg(conn, os.str(), 1000, eCR_PASSWORD);
					break;
				case eBT_FLOOD:
					ConnCloseMsg(conn, os.str(), 1000, eCR_LOGIN_ERR);
					break;
				default:
					ConnCloseMsg(conn, os.str(), 1000, eCR_KICKED);
					break;
			}

			return -1;
		} else // ban expired, do nothing
			mBanList->DelIPTempBan(conn->AddrIP());
	}

	conn->SetTimeOut(eTO_KEY, mC.timeout_length[eTO_KEY], mTime);
	return 0;
}

string * cServerDC::FactoryString(cAsyncConn *conn)
{
	return conn->FactoryString();
}

void cServerDC::OnNewMessage(cAsyncConn *conn, string *str)
{
	size_t len = (*str).size() + 1;
	this->mDownloadZone.Insert(this->mTime, len);
	this->mProtoTotal[0] += len;

	if (conn->Log(4))
		conn->LogStream() << "IN [" << len << "]: " << (*str) << '|' << endl;

	conn->mpMsgParser->Parse();
	conn->mxProtocol->TreatMsg(conn->mpMsgParser, conn);
}

bool cServerDC::VerifyUniqueNick(cConnDC *conn)
{
	string userkey;
	mUserList.Nick2Key(conn->mpUser->mNick, userkey);

	if (mUserList.ContainsKey(userkey)) { // same nick
		cUser *olduser = mUserList.GetUserByKey(userkey);
		bool sameuser = false;

		if (conn->mpUser->mClass >= eUC_REGUSER)
			sameuser = true;
		else if (olduser && olduser->mxConn && (conn->IP2Num() == olduser->mxConn->IP2Num()) && (conn->mpUser->mShare == olduser->mShare) && (StrCompare(conn->mpUser->mMyINFO_basic, 0, olduser->mMyINFO_basic.size(), olduser->mMyINFO_basic) == 0))
			sameuser = true;

		string omsg;

		if (sameuser && !mC.allow_same_user && (conn->mpUser->mClass <= mC.max_class_same_user)) { // dont allow same users
			omsg = _("You're already logged in with same nick and IP address.");
			DCPublicHS(omsg, conn);
			cDCProto::Create_ValidateDenide(omsg, conn->mpUser->mNick);
			conn->Send(omsg, true);
			conn->CloseNice(1000, eCR_SELF);
			return false;
		}

		if (sameuser) {
			if (olduser) {
				if (olduser->mxConn) {
					if (olduser->mxConn->Log(2))
						olduser->mxConn->LogStream() << "Closing because of a new connection" << endl;

					ConnCloseMsg(olduser->mxConn, _("Another user has logged in with same nick and IP address."), 1000, eCR_SELF);
				} else {
					if (ErrLog(1))
						LogStream() << "Critical, found user " << olduser->mNick << " without a valid conneciton pointer" << endl;
				}

				RemoveNick(olduser);
			} else {
				conn->CloseNow();
				return false;
			}

		} else {
			omsg = _("Your nick is already taken by another user.");
			DCPublicHS(omsg, conn);
			cDCProto::Create_ValidateDenide(omsg, conn->mpUser->mNick);
			conn->Send(omsg, true);
			conn->CloseNice(1000, eCR_BADNICK);
			return false;
		}
	}

	return true;

	/*
	string userkey;
	mUserList.Nick2Key(conn->mpUser->mNick, userkey);

	if (mUserList.ContainsKey(userkey)) {
		cUser *olduser = mUserList.GetUserByKey(userkey);

		if (olduser) {
			if (olduser->mxConn) {
				bool sameuser = false;
				string omsg;

				if (conn->mpUser->mClass >= eUC_REGUSER)
					sameuser = true;

				if (!sameuser && (conn->AddrIP() == olduser->mxConn->AddrIP()) && (conn->mpUser->mShare == olduser->mShare) && (conn->mpUser->mMyINFO_basic == olduser->mMyINFO_basic))
					sameuser = true;

				if (sameuser) {
					if (olduser->mxConn->Log(2))
						olduser->mxConn->LogStream() << "Users own clone is trying to log in" << endl;

					omsg = _("It seems that another instance of yourself is trying to log in, if not, please notify an operator.");
					DCPublicHS(omsg, olduser->mxConn);
					omsg = _("It seems that another instance of yourself is already logged in, if not, please try changing your nick.");
					DCPublicHS(omsg, conn);
				} else {
					omsg = _("Your nick is already taken by another user.");
					DCPublicHS(omsg, conn);
					cDCProto::Create_ValidateDenide(omsg, conn->mpUser->mNick);
					conn->Send(omsg, true);
				}

				// todo: add redirect for this
				conn->CloseNow();
				return false;
			} else {
				if (ErrLog(1))
					LogStream() << "[CRITICAL] Found user " << olduser->mNick << " without a valid connection pointer" << endl;
			}
		} else {
			conn->CloseNow();
			return false;
		}
	}

	return true;
	*/
}

void cServerDC::AfterUserLogin(cConnDC *conn)
{
	if (!conn || !conn->mpUser)
		return;

	string omsg;
	ostringstream os;

	if (conn->Log(3))
		conn->LogStream() << "Entered the hub" << endl;

	mCo->mTriggers->TriggerAll(eTF_MOTD, conn);

	if (conn->mRegInfo && conn->mRegInfo->mPwdChange) { // the user has to set or change password
		if (mC.send_pass_request)
			os << _("You must set your password now using +passwd command or entering it in password dialog.");
		else
			os << _("You must set your password now using +passwd command.");

		DCPrivateHS(os.str(), conn);
		DCPublicHS(os.str(), conn);

		/*
			when send_pass_request is enabled
			hub sends $GetPass to the user
			if user replies with $MyPass then hub sets his password
			this equals +passwd command
		*/

		if (mC.send_pass_request) {
			conn->mpUser->mSetPass = true;
			cDCProto::Create_GetPass(omsg);
			conn->Send(omsg, true);
		}

		conn->SetTimeOut(eTO_SETPASS, mC.timeout_length[eTO_SETPASS], this->mTime);
	}

	if (mC.hub_topic.size()/* && (conn->mFeatures & eSF_HUBTOPIC)*/) { // send the hub topic
		cDCProto::Create_HubTopic(omsg, mC.hub_topic);
		conn->Send(omsg, true);
	}

	if (mC.send_user_info) {
		os.str("");
		os << _("Your information") << ":\r\n";
		conn->mpUser->DisplayInfo(os);
		DCPublicHS(os.str(), conn);
	}

	if (mUserList.Size() > mUsersPeak)
		mUsersPeak = mUserList.Size();

	#ifndef WITHOUT_PLUGINS
		if (!mCallBacks.mOnUserLogin.CallAll(conn->mpUser)) { // todo: i think we need to move this before hubtopic and userinfo
			conn->CloseNow();
			return;
		}
	#endif

	if ((conn->mpUser->mClass >= eUC_NORMUSER) && (conn->mpUser->mClass <= eUC_MASTER) && mC.msg_welcome[conn->mpUser->mClass].size()) {
		omsg.clear();
		ReplaceVarInString(mC.msg_welcome[conn->mpUser->mClass], "nick", omsg, conn->mpUser->mNick); // todo: should not be uppercace %[NICK] ?
		const size_t pos = omsg.find("%[C");

		if (pos != omsg.npos) { // only if found
			string geo;

			if (omsg.find("%[CC]", pos) != omsg.npos) {
				geo = conn->GetGeoCC(); // country code
				ReplaceVarInString(omsg, "CC", omsg, geo);
			}

			if (omsg.find("%[CN]", pos) != omsg.npos) {
				geo = conn->GetGeoCN(); // country name
				ReplaceVarInString(omsg, "CN", omsg, geo);
			}

			if (omsg.find("%[CITY]", pos) != omsg.npos) {
				geo = conn->GetGeoCI(); // city name
				ReplaceVarInString(omsg, "CITY", omsg, geo);
			}
		}

		DCPublicHSToAll(omsg, mC.delayed_chat);
	}

	conn->mpUser->mIsLan = cDCProto::isLanIP(conn->AddrIP()); // detect lan ip
}

void cServerDC::DoUserLogin(cConnDC *conn)
{
	if (eLS_LOGIN_DONE != conn->GetLSFlag(eLS_LOGIN_DONE)) { // verify we didnt get here by chance
		if (conn->ErrLog(2))
			conn->LogStream() << "User login when not all done" << endl;

		conn->CloseNow();
		return;
	}

	//if (!VerifyUniqueNick(conn)) // this is already done somewhere else
		//return;

	if (mInProgresUsers.ContainsNick(conn->mpUser->mNick)) { // he is not anymore in progress
		mInProgresUsers.FlushForUser(conn->mpUser);
		mInProgresUsers.Remove(conn->mpUser);
	}

	if (mC.int_login && (conn->GetTheoricalClass() <= mC.max_class_int_login)) // login flood detection
		mBanList->AddNickTempBan(conn->mpUser->mNick, mTime.Sec() + mC.int_login, _("Reconnecting too fast"), eBT_RECON);

	cPenaltyList::sPenalty pen; // users special rights and restrictions

	if (mPenList->LoadTo(pen, conn->mpUser->mNick) && (conn->mpUser->mClass != eUC_PINGER))
		conn->mpUser->ApplyRights(pen);

	if (!AddToList(conn->mpUser)) { // insert user to userlist
		conn->CloseNow();
		return;
	}

	string omsg;

	if (mC.hub_name.size() && mC.hub_topic.size()) { // send hub name with topic
		cDCProto::Create_HubName(omsg, mC.hub_name, mC.hub_topic);
		conn->Send(omsg, true);
	}

	if ((conn->mFeatures & eSF_FAILOVER) && mC.hub_failover_hosts.size()) { // send failover hosts if not empty and client supports it
		cDCProto::Create_FailOver(omsg, mC.hub_failover_hosts);
		conn->Send(omsg, true);
	}

	ShowUserToAll(conn->mpUser); // display user to others
	SendHeaders(conn, 1);
	AfterUserLogin(conn);
	conn->ClearTimeOut(eTO_LOGIN);
	conn->mpUser->mT.login.Get();
}

/*
	if user asks for nicklist, login will happen after the sending of nicklist ends, otherwise it will happen now
*/

bool cServerDC::BeginUserLogin(cConnDC *conn)
{
	unsigned int WantedMask;

	if (mC.delayed_login)
	 	WantedMask = eLS_LOGIN_DONE - eLS_NICKLST;
	else
		WantedMask = eLS_LOGIN_DONE;

	if (WantedMask == conn->GetLSFlag(WantedMask)) {
		if (conn->Log(2))
			conn->LogStream() << "Begin login" << endl;

		if (VerifyUniqueNick(conn)) { // check if nick is unique
			if (!mC.delayed_login) {
				DoUserLogin(conn);
			} else {
				mInProgresUsers.Add(conn->mpUser);
			}

			if (conn->mSendNickList) { // this may not send all data at once
				mP.NickList(conn); // this will set mNickListInProgress
				conn->mSendNickList = false;
				return true; // return here since we dont know that the list was sent or not
				// OnFlushDone() will do the login after the NickList is flushed
			}

			if (!conn->mpUser->mInList) {
				DoUserLogin(conn);
			}
		} else {
			return false;
		}
	} else {
		conn->CloseNow();
		return false;
	}

	return true;
}

bool cServerDC::ShowUserToAll(cUser *user)
{
	string msg;
	mP.Create_Hello(msg, user->mNick); // send hello
	mHelloUsers.SendToAll(msg, mC.delayed_myinfo, true, mC.buffer_noswap);

	msg = mP.GetMyInfo(user, eUC_NORMUSER); // all users get myinfo, even those in progress, hello users in progress are ignored, they are obsolete btw
	mUserList.SendToAll(msg, mC.delayed_myinfo, true, mC.buffer_noswap); // use cache, so this can be after user is added
	mInProgresUsers.SendToAll(msg, mC.delayed_myinfo, true, mC.buffer_noswap);

	if (((user->mClass >= mC.oplist_class) && !(user->mxConn && user->mxConn->mRegInfo && user->mxConn->mRegInfo->mHideKeys)) || (user->mxConn && user->mxConn->mRegInfo && user->mxConn->mRegInfo->mShowKeys && !user->mxConn->mRegInfo->mHideKeys)) { // send short oplist
		mP.Create_OpList(msg, user->mNick);
		mUserList.SendToAll(msg, mC.delayed_myinfo, true, mC.buffer_noswap);
		mInProgresUsers.SendToAll(msg, mC.delayed_myinfo, true, mC.buffer_noswap);
	}

	if (mC.send_user_ip) { // send userip to operators
		string UserIP;
		cCompositeUserCollection::ufDoIpList DoUserIP(UserIP);
		DoUserIP.Clear();
		DoUserIP(user);
		mUserList.SendToAllWithClassFeature(UserIP, mC.user_ip_class, eUC_MASTER, eSF_USERIP2, mC.delayed_myinfo, true, mC.buffer_noswap); // must be delayed too
	}

	/*
		send it to all but to him
		but why? maybe he would be doubled in some shit clients?
		anyway delayed_login will show why is it
		the order of flush of this before the full myinfo for ops
	*/

	if (!mC.delayed_login) {
		user->mInList = false;
		mUserList.FlushCache(mC.buffer_noswap);
		mInProgresUsers.FlushCache(mC.buffer_noswap);
		user->mInList = true;
	}

	if (mC.show_tags == 1) { // patch eventually for ops
		msg = mP.GetMyInfo(user, eUC_OPERATOR);
		mUserList.SendToAllWithClass(msg, eUC_OPERATOR, eUC_MASTER, mC.delayed_myinfo, true, mC.buffer_noswap); // must send after mUserList, cached mUserList will be flushed after and will override this one
		mInProgresUsers.SendToAll(msg, mC.delayed_myinfo, true, mC.buffer_noswap); // send later, better more people see tags, then some ops not
	}

	return true;
}

void cServerDC::ConnCloseMsg(cConnDC *conn, const string &msg, int msec, int reason)
{
	DCPublicHS(msg, conn);
	conn->CloseNice(msec, reason);
}

int cServerDC::DCHello(const string &nick, cConnDC *conn, string *info)
{
	string msg;
	mP.Create_Hello(msg, nick);
	conn->Send(msg, true);

	if (info)
		conn->Send(*info, true);

	return 0;
}

bool cServerDC::MinDelay(cTime &then, unsigned int min, bool update)
{
	/*
		todo
			use timeins instead of mindelay, or change to microsecond resolution
	*/

	cTime now;
	cTime diff = now - then;

	if (diff.Sec() >= (long)min) {
		then = now;
		return true;
	}

	if (update) // update timestamp
		then = now;

	return false;
}

bool cServerDC::MinDelayMS(cTime &then, unsigned long min, bool update)
{
	cTime now;
	cTime diff = now - then;

	if (diff.MiliSec() >= (__int64)min) {
		then = now;
		return true;
	}

	if (update) // update timestamp
		then = now;

	return false;
}

bool cServerDC::AllowNewConn()
{
	return (mConnList.size() <= (unsigned)(mC.max_users_total + mC.max_extra_regs + mC.max_extra_vips + mC.max_extra_ops + mC.max_extra_cheefs + mC.max_extra_admins + 300));
}

int cServerDC::SaveFile(const string &file, const string &text)
{
	string filename;
	ReplaceVarInString(file, "CFG", filename, mConfigBaseDir);
	ofstream os(file.c_str());
	if(!os.is_open())
		return 0;
	os << text << endl;
	os.close();
	return 1;
}

bool cServerDC::SetUserRegInfo(cConnDC *conn, const string &nick)
{
	if (!conn || nick.empty())
		return false;

	if (conn->mRegInfo) {
		delete conn->mRegInfo;
		conn->mRegInfo = NULL;
	}

	static cRegUserInfo *sRegInfo = new cRegUserInfo;

	if (mR->FindRegInfo(*sRegInfo, nick)) {
		conn->mRegInfo = sRegInfo;
		sRegInfo = new cRegUserInfo;
	}

	return true;
}

int cServerDC::ValidateUser(cConnDC *conn, const string &nick, int &closeReason)
{
	closeReason = eCR_INVALID_USER; // default close reason

	if (!conn)
		return 0;

	bool close = false;
	//static cRegUserInfo *sRegInfo = new cRegUserInfo;

	if (/*(*/nick.size() < (mC.max_nick * 2)/*) && mR->FindRegInfo(*sRegInfo, nick) && !conn->mRegInfo*/) {
		/*
		conn->mRegInfo = sRegInfo;
		sRegInfo = new cRegUserInfo;
		*/
		SetUserRegInfo(conn, nick);
	}

	string more;
	tVAL_NICK vn = ValidateNick(conn, nick, more); // validate nick
	stringstream errmsg;

	if (vn != eVN_OK) {
		string extra;

		if (vn == eVN_BANNED) {
			cBanList::sTempBan *tban = mBanList->mTempNickBanlist.GetByHash(mBanList->mTempNickBanlist.HashLowerString(nick));

			if (tban && (tban->mUntil > mTime.Sec())) {
				errmsg << autosprintf(_("You're still temporarily prohibited from entering the hub for %s because: %s"), cTimePrint(tban->mUntil - mTime.Sec()).AsPeriod().AsString().c_str(), tban->mReason.c_str());

				switch (tban->mType) {
					case eBT_RECON:
						closeReason = eCR_RECONNECT;
						break;
					case eBT_CLONE:
						closeReason = eCR_CLONE;
						break;
				}
			} else { // ban expired, do nothing
				mBanList->DelNickTempBan(nick);
				vn = eVN_OK;
			}
		} else {
			switch (vn) {
				case eVN_CHARS:
					errmsg << _("Your nick contains forbidden characters.");

					if (mC.nick_chars.size())
						errmsg << ' ' << autosprintf(_("Valid nick characters: %s"), mC.nick_chars.c_str());

					if (conn->mFeatures & eSF_NICKRULE)
						cDCProto::Create_BadNick(extra, "Char", StrByteList(more));

					break;

				case eVN_SHORT:
					errmsg << autosprintf(_("Your nick is too short, minimum allowed length is %d characters."), mC.min_nick);

					if (conn->mFeatures & eSF_NICKRULE) {
						if (mC.min_nick > 255)
							cDCProto::Create_BadNick(extra, "Min", "255");
						else
							cDCProto::Create_BadNick(extra, "Min", StringFrom(mC.min_nick));
					}

					break;

				case eVN_LONG:
					errmsg << autosprintf(_("Your nick is too long, maximum allowed length is %d characters."), mC.max_nick);

					if (conn->mFeatures & eSF_NICKRULE)
						cDCProto::Create_BadNick(extra, "Max", StringFrom(mC.max_nick));

					break;

				case eVN_USED:
					errmsg << _("Your nick is already taken by another user.");
					cDCProto::Create_ValidateDenide(extra, nick);
					break;

				case eVN_PREFIX:
					errmsg << autosprintf(_("Please use one of following nick prefixes: %s"), mC.nick_prefix.c_str());

					if (conn->mFeatures & eSF_NICKRULE)
						cDCProto::Create_BadNick(extra, "Pref", mC.nick_prefix);

					break;

				case eVN_NOT_REGED_OP:
					errmsg << _("Your nick contains operator prefix but you are not registered, please remove it.");

					if (conn->mFeatures & eSF_NICKRULE)
						cDCProto::Create_BadNick(extra, "Pref");

					break;

				default:
					errmsg << _("Unknown bad nick error, sorry.");
					break;
			}

			closeReason = eCR_BADNICK;
		}

		if (vn != eVN_OK) {
			DCPublicHS(errmsg.str(), conn);

			if (extra.size())
				conn->Send(extra, true);

			if (conn->Log(2))
				conn->LogStream() << errmsg.str() << endl;

			return 0;
		}
	}

	cBan Ban(this);
	unsigned int banned = 0;

	if (conn->GetTheoricalClass() < mC.ban_bypass_class) { // use ban_bypass_class here
		if (conn->GetTheoricalClass() == eUC_NORMUSER) // here we cant check share ban because user hasnt sent $MyINFO yet
			banned = mBanList->TestBan(Ban, conn, nick, eBF_NICK | eBF_NICKIP | eBF_RANGE | eBF_HOST2 | eBF_HOST1 | eBF_HOST3 | eBF_HOSTR1 | eBF_PREFIX);
		else // registered users avoid prefix ban check because we might actually ban a prefix for unregistered users, but let registered users to use it
			banned = mBanList->TestBan(Ban, conn, nick, eBF_NICK | eBF_NICKIP | eBF_RANGE | eBF_HOST2 | eBF_HOST1 | eBF_HOST3 | eBF_HOSTR1);
	}

	if (banned) {
		errmsg << ((banned == 1) ? _("You are prohibited from entering this hub") : _("You are banned from this hub")) << ":\r\n";
		Ban.DisplayUser(errmsg);
		DCPublicHS(errmsg.str(), conn);

		if (conn->Log(1))
			conn->LogStream() << "Closing banned user: " << Ban.mType << endl;

		return 0;
	}

	if (mC.nick_prefix_cc) {
		more = conn->GetGeoCC();

		if ((more.size() == 2) && (more != "--")) {
			more = '[' + more + ']';

			if (StrCompare(nick, 0, 4, more) != 0) {
				errmsg << autosprintf(_("Please use following nick prefix: %s"), more.c_str());
				close = conn->GetTheoricalClass() < eUC_REGUSER;
			}
		}
	}

	if (close) {
		DCPublicHS(errmsg.str(), conn);
		return 0;
	}

	return 1;
}

tVAL_NICK cServerDC::ValidateNick(cConnDC *conn, const string &nick, string &more)
{
	string bad_nick_chars(string(BAD_NICK_CHARS_NMDC) + string(BAD_NICK_CHARS_OWN));
	bool bad = false;
	unsigned i;
	char chr;

	for (i = 0; i < bad_nick_chars.size(); ++i) {
		chr = bad_nick_chars[i];

		if ((nick.find(chr) != nick.npos) && (more.find(chr) == more.npos)) {
			more.append(1, chr);
			bad = true;
		}
	}

	if (bad)
		return eVN_CHARS;

	if (!conn->mRegInfo || !conn->mRegInfo->mEnabled) { // user must be unregistered
		if (nick.size() > mC.max_nick) // check high length
			return eVN_LONG;

		if (nick.size() < mC.min_nick) // check low length
			return eVN_SHORT;

		if (mC.nick_chars.size()) { // check bad chars
			bad = false;

			for (i = 0; i < nick.size(); ++i) {
				chr = nick[i];

				if ((mC.nick_chars.find(chr) == mC.nick_chars.npos) && (more.find(chr) == more.npos)) {
					more.append(1, chr);
					bad = true;
				}
			}

			if (bad)
				return eVN_CHARS;
		}

		if (mC.nick_prefix.size()) { // check nick prefix
			bad = true;
			istringstream is(mC.nick_prefix);
			string pref, snick;

			if (mC.nick_prefix_nocase)
				snick = toLower(nick);
			else
				snick = nick;

			while (1) {
				pref = mEmpty;
				is >> pref;

				if (pref.empty())
					break;

				if (mC.nick_prefix_nocase)
					pref = toLower(pref);

				if ((snick.size() >= pref.size()) && (StrCompare(snick, 0, pref.size(), pref) == 0)) {
					bad = false;
					break;
				}
			}

			if (bad)
				return eVN_PREFIX;
		}

		if ((nick.size() >= 4) && (StrCompare(nick, 0, 4, "[OP]") == 0)) // operator prefix
			return eVN_NOT_REGED_OP;

		string userkey; // check if user with same nick already logged in
		mUserList.Nick2Key(nick, userkey);

		if (mUserList.ContainsKey(userkey)) {
			cUser *olduser = mUserList.GetUserByKey(userkey);

			if (olduser && olduser->mxConn && (conn->IP2Num() != olduser->mxConn->IP2Num())) // make sure its not same user
				return eVN_USED;
		}
	}

	if (mBanList->IsNickTempBanned(nick)) // check temporary nick ban
		return eVN_BANNED;

	return eVN_OK;
}

int cServerDC::OnTimer(cTime &now)
{
	mHelloUsers.FlushCache(mC.buffer_noswap);
	mUserList.FlushCache(mC.buffer_noswap);
	mOpList.FlushCache(mC.buffer_noswap);
	mOpchatList.FlushCache(mC.buffer_noswap);
	mActiveUsers.FlushCache(mC.buffer_noswap);
	mPassiveUsers.FlushCache(mC.buffer_noswap);
	mChatUsers.FlushCache(mC.buffer_noswap);
	mInProgresUsers.FlushCache(mC.buffer_noswap);
	mRobotList.FlushCache(mC.buffer_noswap);
	mSysLoad = eSL_NORMAL;

	if (mFrequency.mNumFill > 0) {
		double freq = mFrequency.GetMean(mTime);

		if (freq < (1.2 * mC.min_frequency))
			mSysLoad = eSL_PROGRESSIVE;

		if (freq < (1.0 * mC.min_frequency))
			mSysLoad = eSL_CAPACITY;

		if (freq < (0.8 * mC.min_frequency))
			mSysLoad = eSL_RECOVERY;

		if (freq < (0.5 * mC.min_frequency))
			mSysLoad = eSL_SYSTEM_DOWN;
	}

	if (mC.max_upload_kbps > 0.00001) {
		unsigned int zone;
		double total_upload = 0.;

		for (zone = 0; zone <= USER_ZONES; zone++)
			total_upload += this->mUploadZone[zone].GetMean(this->mTime);

		if ((total_upload / 1024.0) > mC.max_upload_kbps)
			mSysLoad = eSL_PROGRESSIVE;
	}

	for (tTFIt i = mTmpFunc.begin(); i != mTmpFunc.end(); ++i) { // perform all temp functions
		if (*i) {
			if ((*i)->done()) { // delete finished functions
				delete *i;
				(*i) = NULL;
			} else { // step the rest
				(*i)->step();
			}
		}
	}

	if (bool(mSlowTimer.mMinDelay) && (mSlowTimer.Check(mTime, 1) == 0))
		mBanList->RemoveOldShortTempBans(mTime.Sec());

	if (bool(mHublistTimer.mMinDelay) && (mHublistTimer.Check(mTime, 1) == 0))
		this->RegisterInHublist(mC.hublist_host, mC.hublist_port, NULL);

	if (bool(mReloadcfgTimer.mMinDelay) && (mReloadcfgTimer.Check(mTime, 1) == 0)) {
		mC.Load();
		//mCo->mTriggers->ReloadAll();

		if (mC.use_reglist_cache)
			mR->UpdateCache();

		if (mC.use_penlist_cache)
			mPenList->UpdateCache();

		if (Log(2))
			LogStream() << "Socket counter: " << cAsyncConn::sSocketCounter << endl;
	}

	if (mC.mmdb_cache && mC.mmdb_cache_mins && ((mTime.Sec() - mMaxMindDB->mClean.Sec()) >= 60)) // clean mmdb cache every minute
		mMaxMindDB->MMDBCacheClean(); // do not confuse with MMDBCacheClear

	if (mC.detect_ctmtohub && ((mTime.Sec() - mCtmToHubConf.mTime.Sec()) >= 60)) { // ctm2hub
		unsigned long total = mCtmToHubList.size();

		if (total) {
			if (mCtmToHubConf.mNew) {
				if (mCtmToHubConf.mStart) {
					string list;
					ostringstream os;
					int count = CtmToHubRefererList(list);

					if (count)
						os << autosprintf(_("DDoS detection filtered %lu new connections and found %d new exploited hubs"), mCtmToHubConf.mNew, count) << ":\r\n\r\n" << list;
					else
						os << autosprintf(_("DDoS detection filtered %lu new connections."), mCtmToHubConf.mNew);

					if (this->mOpChat) // todo: where else to notify?
						this->mOpChat->SendPMToAll(os.str(), NULL);
				}

				mCtmToHubConf.mNew = 0;
				mCtmToHubConf.mLast = this->mTime;
			} else if ((this->mTime.Sec() - mCtmToHubConf.mLast.Sec()) >= 180) {
				if (mCtmToHubConf.mStart) {
					ostringstream os;
					os << autosprintf(_("DDoS stopped, filtered totally %lu connections."), total);

					if (this->mOpChat) // todo: where else to notify?
						this->mOpChat->SendPMToAll(os.str(), NULL);

					mCtmToHubConf.mStart = false;
				}

				CtmToHubClearList();
				mCtmToHubConf.mLast = this->mTime;
			}
		}

		mCtmToHubConf.mTime = this->mTime;
	}

	mUserList.AutoResize();
	mHelloUsers.AutoResize();
	mActiveUsers.AutoResize();
	mPassiveUsers.AutoResize();
	mChatUsers.AutoResize();
	mOpList.AutoResize();
	mOpchatList.AutoResize();
	mInProgresUsers.AutoResize();
	mRobotList.AutoResize();

	mBanList->mTempNickBanlist.AutoResize();
	mBanList->mTempIPBanlist.AutoResize();
	mCo->mTriggers->OnTimer(now.Sec());

	#ifndef WITHOUT_PLUGINS
		SendScriptCommands();

		if (!mCallBacks.mOnTimer.CallAll(now.MiliSec()))
			return false;
	#endif

	return true;
}

unsigned int cServerDC::Str2Period(const string &s, ostream &err)
{
	istringstream is(s);
	unsigned int u = 0;
	int m, n = 0;
	char c = ' ';
	is >> n >> c;

	if (n >= 0) {
		m = 1; // multiplicator

		if (c == ' ')
			c = 'd';

		switch (c) {
			case 'y':
			case 'Y':
				m *= 12; // year = 12 * month
			case 'M':
				m *= 4; // month = 4 * week
			case 'w':
			case 'W':
				m *= 7; // week = 7 days
			case 'd':
			case 'D':
				m *= 24; // 24 hours
			case 'h':
			case 'H':
				m *= 60; // 60 minutes
			case 'm':
				m *= 60; // 60 seconds
			case 's':
			case 'S':
				break;
			default:
				err << _("Available time units are: s(econds), m(inutes), h(ours), d(ays) is default, w(eeks), M(onths) and y(ears)");
				return 0;
		}

		u = (n * m);

		// overflow protection: if n is too big assume n = 1.
		if (n && u < (unsigned)m)
			return (unsigned)m;

	} else {
		err << _("Please provide positive number for time unit.");
	}

	return u;
}

int cServerDC::DoRegisterInHublist(string host, unsigned int port, string reply)
{
	unsigned __int64 min_share = mC.min_share; // prepare

	if (mC.min_share_use_hub > min_share)
		min_share = mC.min_share_use_hub;

	char pipe = '|';
	size_t pos;
	ostringstream to_serv, to_user;
	istringstream is(host);
	string curhost, lock, key;
	cHTTPConn *pHubList;
	to_user << _("Hublist registration results") << ":\r\n\r\n";

	while (curhost = "", is >> curhost, curhost.size() > 0) {
		to_user << autosprintf(_("Sending information to: %s:%d"), curhost.c_str(), port) << " .. ";
		pHubList = new cHTTPConn(curhost, port); // connect

		if (!pHubList || !pHubList->mGood) {
			to_user << _("Connection error") << "\r\n";

			if (pHubList) {
				pHubList->Close();
				delete pHubList;
				pHubList = NULL;
			}

			continue;
		}

		if (pHubList->Read() > 0) {
			lock.clear();

			if (pHubList->GetBuf(lock) > 0) {
				if (lock.find(pipe) != lock.npos) {
					key.clear();

					if (lock.size() > 6) {
						pos = lock.find(' ', 6);

						if (pos != lock.npos)
							pos -= 6;

						lock = lock.substr(6, pos);
						cDCProto::Lock2Key(lock, key);
					}

					cDCProto::Create_Key(lock, key);
					to_serv.str("");
					to_serv.clear();
					to_serv << lock << pipe; // create registration data
					to_serv << mC.hub_name << pipe;
					to_serv << mC.hub_host << pipe;

					if (mC.hublist_send_minshare)
						to_serv << "[MINSHARE:" << StringFrom(min_share) << "MB] ";

					to_serv << mC.hub_desc << pipe;
					to_serv << mUserList.Size() << pipe;
					to_serv << mTotalShare << pipe;

					if (mC.hublist_send_listhost) // hublist host with port
						to_serv << curhost << ':' << StringFrom(port) << pipe;

					if ((pHubList->Write(to_serv.str()) > 0) && pHubList->mGood) // send it
						to_user << _("Registration done");
					else
						to_user << _("Send error");
				} else {
					to_user << _("Protocol error") << "\r\n";
				}
			} else {
				to_user << _("Read error") << "\r\n";
			}
		} else {
			to_user << _("Read error") << "\r\n";
		}

		pHubList->Close();
		delete pHubList;
		pHubList = NULL;
	}

	if (reply.size()) {
		cUser *user = mUserList.GetUserByNick(reply);

		if (user && user->mxConn) {
			to_user << "\r\n" << _("Finished registering in hublists.");
			DCPublicHS(to_user.str(), user->mxConn);
		}
	}

	return 1;
}

int cServerDC::RegisterInHublist(string host, unsigned int port, cConnDC *conn)
{
	if (host.size() < 3) {
		if (conn)
			DCPublicHS(_("Hublist host is empty, nothing to do."), conn);

		return 0;
	}

	if (!CheckPortNumber(port)) {
		if (conn)
			DCPublicHS(_("Hublist port number is invalid, nothing to do."), conn);

		return 0;
	}

	string reply;

	if (conn) {
		DCPublicHS(_("Registering in hublists, please wait."), conn);

		if (conn->mpUser)
			reply = conn->mpUser->mNick;
	}

	cThreadWork *work = new tThreadWork3T<cServerDC, string, unsigned int, string>(host, port, reply, this, &cServerDC::DoRegisterInHublist);

	if (mHublistReg.AddWork(work))
		return 1;
	else {
		delete work;
		work = NULL;
		return 0;
	}
}

unsigned int cServerDC::WhoCC(const string &cc, string &dest, const string &sep)
{
	cUserCollection::iterator pos;
	unsigned int tot = 0;
	cConnDC *conn;
	string geo;

	for (pos = mUserList.begin(); pos != mUserList.end(); ++pos) {
		conn = ((cUser*)(*pos))->mxConn;

		if (conn) {
			geo = conn->GetGeoCC();

			if (geo == cc) {
				dest += sep;
				dest += (*pos)->mNick;
				tot++;
			}
		}
	}

	return tot;
}

unsigned int cServerDC::WhoCity(const string &city, string &dest, const string &sep)
{
	cUserCollection::iterator pos;
	unsigned int tot = 0;
	cConnDC *conn;
	string low, ci;

	for (pos = mUserList.begin(); pos != mUserList.end(); ++pos) {
		conn = ((cUser*)(*pos))->mxConn;

		if (conn) {
			ci = conn->GetGeoCI();
			low = toLower(ci);

			if (low.find(city) != low.npos) {
				dest += sep;
				dest += (*pos)->mNick;
				dest += " [";
				dest += ci;
				dest += ']';
				tot++;
			}
		}
	}

	return tot;
}

unsigned int cServerDC::WhoHubPort(unsigned int port, string &dest, const string &sep)
{
	cUserCollection::iterator i;
	unsigned int cnt = 0;
	cConnDC *conn;

	for (i = mUserList.begin(); i != mUserList.end(); ++i) {
		conn = ((cUser*)(*i))->mxConn;

		if (conn && (conn->GetServPort() == port)) {
			dest += sep;
			dest += (*i)->mNick;
			cnt++;
		}
	}

	return cnt;
}

unsigned int cServerDC::WhoHubURL(const string &url, string &dest, const string &sep)
{
	cUserCollection::iterator i;
	unsigned int cnt = 0;
	cConnDC *conn;
	string low;

	for (i = mUserList.begin(); i != mUserList.end(); ++i) {
		conn = ((cUser*)(*i))->mxConn;

		if (conn && conn->mHubURL.size()) {
			low = toLower(conn->mHubURL);

			if (low.find(url) != string::npos) {
				dest += sep;
				dest += (*i)->mNick;
				dest += " [";
				dest += conn->mHubURL;
				dest += ']';
				cnt++;
			}
		}
	}

	return cnt;
}

unsigned int cServerDC::WhoIP(unsigned long ip_min, unsigned long ip_max, string &dest, const string &sep, bool exact)
{
	cUserCollection::iterator i;
	unsigned int cnt = 0;
	cConnDC *conn;
	unsigned long num;

	for (i = mUserList.begin(); i != mUserList.end(); ++i) {
		conn = ((cUser*)(*i))->mxConn;

		if (conn) {
			num = conn->IP2Num();

			if (exact && (ip_min == num)) {
				dest += sep;
				dest += (*i)->mNick;
				cnt++;
			} else if ((ip_min <= num) && (ip_max >= num)) {
				dest += sep;
				dest += (*i)->mNick;
				dest += " [";
				dest += conn->AddrIP();
				dest += ']';
				cnt++;
			}
		}
	}

	return cnt;
}

unsigned int cServerDC::CntConnIP(const unsigned long ip)
{
	cUserCollection::iterator it;
	unsigned int tot = 0;
	cConnDC *conn;

	for (it = mUserList.begin(); it != mUserList.end(); ++it) {
		conn = ((cUser*)(*it))->mxConn;

		if (conn && conn->ok) {
			if ((conn->GetTheoricalClass() <= eUC_REGUSER) && (conn->IP2Num() == ip))
				tot++;
		}
	}

	return tot;
}

bool cServerDC::CheckUserClone(cConnDC *conn, string &clone)
{
	if (!mC.clone_detect_count || !conn || !conn->mpUser || !conn->mpUser->mShare || (conn->mpUser->mClass > int(mC.max_class_check_clone)))
		return false;

	cUserCollection::iterator i;
	cConnDC *other;
	unsigned int count = 0;

	for (i = mUserList.begin(); i != mUserList.end(); ++i) { // skip self
		other = ((cUser*)(*i))->mxConn;

		if (other && other->mpUser && other->mpUser->mInList && other->mpUser->mShare && (StrCompare(other->mpUser->mNick, 0, conn->mpUser->mNick.size(), conn->mpUser->mNick) != 0) && (other->mpUser->mClass <= int(mC.max_class_check_clone)) && (other->mpUser->mShare == conn->mpUser->mShare) && (other->IP2Num() == conn->IP2Num())) {
			count++;

			if (count >= mC.clone_detect_count) { // number of clones
				ostringstream os;

				if (mC.clone_detect_report || mC.clone_det_tban_time)
					os << autosprintf(_("Detected clone of user with share %s: %s"), convertByte(other->mpUser->mShare, false).c_str(), other->mpUser->mNick.c_str());

				if (mC.clone_detect_report)
					ReportUserToOpchat(conn, os.str());

				if (mC.clone_det_tban_time) { // add temporary nick ban
					mBanList->AddNickTempBan(conn->mpUser->mNick, mTime.Sec() + mC.clone_det_tban_time, _("Clone detected"), eBT_CLONE);
					/*
					cBan ccban(this);
					cKick cckick;
					cckick.mOp = mC.hub_security;
					//cckick.mIP = conn->AddrIP(); // dont set ip, we dont want to ban first user
					//cckick.mShare = conn->mpUser->mShare; // dont set share
					cckick.mNick = conn->mpUser->mNick;
					cckick.mTime = mTime.Sec();
					cckick.mReason = os.str();
					mBanList->NewBan(ccban, cckick, mC.clone_det_tban_time, eBF_NICK);
					mBanList->AddBan(ccban);
					*/
				}

				clone = other->mpUser->mNick; // uses last nick
				return true;
			}
		}
	}

	return false;
}

bool cServerDC::CheckProtoFloodAll(cConnDC *conn, int type, cUser *touser)
{
	if (!conn || !conn->mpUser || (conn->mpUser->mClass > mC.max_class_proto_flood) || (((type == ePFA_PRIV) || (type == ePFA_RCTM)) && !touser))
		return false;

	long period = 0;
	unsigned int limit = 0;

	switch (type) {
		case ePFA_CHAT:
			period = mC.int_flood_all_chat_period;
			limit = mC.int_flood_all_chat_limit;
			break;
		case ePFA_PRIV:
			period = mC.int_flood_all_to_period;
			limit = mC.int_flood_all_to_limit;
			break;
		case ePFA_MCTO:
			period = mC.int_flood_all_mcto_period;
			limit = mC.int_flood_all_mcto_limit;
			break;
		case ePFA_SEAR:
			period = mC.int_flood_all_search_period;
			limit = mC.int_flood_all_search_limit;
			break;
		case ePFA_RCTM:
			period = mC.int_flood_all_rctm_period;
			limit = mC.int_flood_all_rctm_limit;
			break;
	}

	if (!limit || !period)
		return false;

	if (type == ePFA_RCTM) { // rctm
		if (!touser->mRCTMCount) {
			touser->mRCTMCount = 1;
			touser->mRCTMTime = mTime;
			touser->mRCTMLock = false;
			return false;
		}
	} else if (!mProtoFloodAllCounts[type]) { // other
		mProtoFloodAllCounts[type] = 1;
		mProtoFloodAllTimes[type] = mTime;
		mProtoFloodAllLocks[type] = false;
		return false;
	}

	long dif = ((type == ePFA_RCTM) ? mTime.Sec() - touser->mRCTMTime.Sec() : mTime.Sec() - mProtoFloodAllTimes[type].Sec());

	if ((dif < 0) || (dif > period)) {
		if (type == ePFA_RCTM) { // rctm
			touser->mRCTMCount = 1;
			touser->mRCTMTime = mTime;

			if (touser->mRCTMLock) { // reset lock
				ostringstream os;
				os << autosprintf(_("Protocol command has been unlocked after stopped flood to user %s from all"), touser->mNick.c_str());
				os << ": RevConnectToMe";

				if (conn->Log(1))
					conn->LogStream() << os.str() << endl;

				if (mC.proto_flood_report)
					ReportUserToOpchat(conn, os.str());

				touser->mRCTMLock = false;
			}
		} else { // other
			mProtoFloodAllCounts[type] = 1;
			mProtoFloodAllTimes[type] = mTime;

			if (mProtoFloodAllLocks[type]) { // reset lock
				string omsg = _("Protocol command has been unlocked after stopped flood from all");
				omsg += ": ";

				switch (type) {
					case ePFA_CHAT:
						omsg += _("Chat");
						break;
					case ePFA_PRIV:
						omsg += "To";
						break;
					case ePFA_MCTO:
						omsg += "MCTo";
						break;
					case ePFA_SEAR:
						omsg += "Search";
						break;
				}

				if (conn->Log(1))
					conn->LogStream() << omsg << endl;

				if (mC.proto_flood_report)
					ReportUserToOpchat(conn, omsg);

				mProtoFloodAllLocks[type] = false;
			}
		}

		return false;
	}

	if (type == ePFA_RCTM) { // rctm
		if (touser->mRCTMCount++ >= limit) {
			touser->mRCTMTime = mTime; // update time, protocol command to user will be locked until flood is going on

			if (!touser->mRCTMLock) { // set lock if not already
				ostringstream os;
				os << autosprintf(_("Protocol command has been locked due to detection of flood to user %s from all"), touser->mNick.c_str());
				os << ": RevConnectToMe";
				os << " [" << touser->mRCTMCount << ':' << dif << ':' << period << ']';

				if (conn->Log(1))
					conn->LogStream() << os.str() << endl;

				if (mC.proto_flood_report)
					ReportUserToOpchat(conn, os.str());

				touser->mRCTMLock = true;
			}

			/*
				todo
					dont know if user should be notified every time
					this commands is very frequent
					maybe notification with delay
					or notification only first time, add bool to user class
					also there are other good and bad sides of showing this message
					think really good about this one
			*/

			return true;
		}
	} else if (mProtoFloodAllCounts[type]++ >= limit) { // other
		mProtoFloodAllTimes[type] = mTime; // update time, protocol command will be locked until flood is going on
		string omsg;

		if (!mProtoFloodAllLocks[type]) { // set lock if not already
			omsg = _("Protocol command has been locked due to detection of flood from all");
			omsg += ": ";

			switch (type) {
				case ePFA_CHAT:
					omsg += _("Chat");
					break;
				case ePFA_PRIV:
					omsg += "To";
					break;
				case ePFA_MCTO:
					omsg += "MCTo";
					break;
				case ePFA_SEAR:
					omsg += "Search";
					break;
			}

			ostringstream info;
			info << omsg << " [" << mProtoFloodAllCounts[type] << ':' << dif << ':' << period << ']';

			if (conn->Log(1))
				conn->LogStream() << info.str() << endl;

			if (mC.proto_flood_report)
				ReportUserToOpchat(conn, info.str());

			mProtoFloodAllLocks[type] = true;
		}

		if (type == ePFA_SEAR) {
			/*
				todo
					dont know if user should be notified every time
					this commands is very frequent
					maybe notification with delay
					or notification only first time, add bool to user class
					also there are other good and bad sides of showing this message
					think really good about this one
			*/
		} else { // notify user every time
			omsg = _("Sorry, following protocol command is temporarily locked due to flood detection");
			omsg += ": ";

			switch (type) {
				case ePFA_CHAT:
					omsg += _("Chat");
					break;
				case ePFA_PRIV:
					omsg += "To";
					break;
				case ePFA_MCTO:
					omsg += "MCTo";
					break;
			}

			if (type == ePFA_PRIV) // pm
				DCPrivateHS(omsg, conn, &touser->mNick);
			else // mc
				DCPublicHS(omsg, conn);
		}

		return true;
	}

	return false;
}

char* cServerDC::SysLoadName()
{
	switch (mSysLoad) {
		case eSL_NORMAL:
			return _("Normal");
		case eSL_PROGRESSIVE:
			return _("Progressive");
		case eSL_CAPACITY:
			return _("Capacity");
		case eSL_RECOVERY:
			return _("Recovery");
		case eSL_SYSTEM_DOWN:
			return _("Down");
		default:
			return _("Unknown");
	}
}

char* cServerDC::UserClassName(tUserCl ucl)
{
	switch (ucl) {
		case eUC_PINGER:
			return _("Pinger");
		case eUC_NORMUSER:
			return _("Guest");
		case eUC_REGUSER:
			return _("Registered");
		case eUC_VIPUSER:
			return _("VIP");
		case eUC_OPERATOR:
			return _("Operator");
		case eUC_CHEEF:
			return _("Cheef");
		case eUC_ADMIN:
			return _("Administrator");
		case eUC_MASTER:
			return _("Master");
		default:
			return _("Unknown");
	}
}

bool cServerDC::CheckPortNumber(unsigned int port)
{
	if ((port < 1) || (port > 65535))
		return false;

	return true;
}

void cServerDC::ReportUserToOpchat(cConnDC *conn, const string &Msg, bool ToMain)
{
	if (!conn)
		return;

	ostringstream os;
	os << Msg;

	if (conn->mpUser) // nick
		os << " ][ " << autosprintf(_("Nick: %s"), conn->mpUser->mNick.c_str());

	os << " ][ " << autosprintf(_("IP: %s"), conn->AddrIP().c_str()); // address
	string temp;

	if (mC.report_user_country) { // country code
		temp = conn->GetGeoCC();

		if (temp.size() && (temp != "--"))
			os << '.' << temp;
	}

	if (!mUseDNS && mC.report_dns_lookup)
		conn->DNSLookup();

	if (conn->AddrHost().size()) // host
		os << " ][ " << autosprintf(_("Host: %s"), conn->AddrHost().c_str());

	if (!ToMain && mOpChat) {
		mOpChat->SendPMToAll(os.str(), NULL);
	} else {
		cDCProto::Create_Chat(temp, mC.opchat_name, os.str());
		mOpchatList.SendToAll(temp, false, true, mC.buffer_noswap);
	}
}

void cServerDC::SendHeaders(cConnDC *conn, unsigned int where)
{
	/*
	* 0 = dont send headers
	* 1 = send headers on login
	* 2 = send headers on connection
	*/

	if (((mC.host_header > 2) ? 2 : mC.host_header) == where) {
		ostringstream os;
		cTimePrint runtime;
		runtime -= mStartTime;

		if (mC.extended_welcome_message) {
			os << '<' << mC.hub_security << "> " << autosprintf(_("Software: %s %s"), HUB_VERSION_NAME, HUB_VERSION_VERS) << '|';
			os << '<' << mC.hub_security << "> " << autosprintf(_("Uptime: %s"), runtime.AsPeriod().AsString().c_str()) << '|';
			os << '<' << mC.hub_security << "> " << autosprintf(_("Users: %d"), mUserCountTot) << '|';
			os << '<' << mC.hub_security << "> " << autosprintf(_("Share: %s"), convertByte(mTotalShare).c_str()) << '|';
			os << '<' << mC.hub_security << "> " << autosprintf(_("Status: %s"), SysLoadName()) << '|';
			if (!mC.hub_version_special.empty()) os << '<' << mC.hub_security << "> " << mC.hub_version_special << '|';
		} else
			os << '<' << mC.hub_security << "> " << autosprintf(_("Software: %s %s%s ][ Uptime: %s ][ Users: %d ][ Share: %s"), HUB_VERSION_NAME, HUB_VERSION_VERS, mC.hub_version_special.c_str(), runtime.AsPeriod().AsString().c_str(), mUserCountTot, convertByte(mTotalShare, false).c_str()) << '|';

		string res = os.str();
		conn->Send(res, false);
	}
}

void cServerDC::DCKickNick(ostream *use_os, cUser *op, const string &nick, const string &why, int flags, const string &note_op, const string &note_usr)
{
	if (!op || nick.empty() || (op->mNick == nick))
		return;

	ostringstream ostr, toall;
	cUser *user = mUserList.GetUserByNick(nick);

	if (!user || !user->mxConn) {
		ostr << autosprintf(_("User not found: %s"), nick.c_str());
	} else if (((user->mClass + int(mC.classdif_kick)) > op->mClass) || !op->Can(eUR_KICK, mTime.Sec())) {
		ostr << autosprintf(_("You have no rights to kick user: %s"), nick.c_str());
	} else if (user->mProtectFrom >= op->mClass) {
		ostr << autosprintf(_("User is protected from your kicks: %s"), nick.c_str());
	} else {
		string new_why(why);
		bool keep = true;

		if (flags & eKI_BAN) {
			#ifndef WITHOUT_PLUGINS
				keep = mCallBacks.mOnOperatorKicks.CallAll(op, user, &new_why);
			#endif
		} else if (flags & eKI_DROP) {
			#ifndef WITHOUT_PLUGINS
				keep = mCallBacks.mOnOperatorDrops.CallAll(op, user, &new_why);
			#endif
		}

		if (keep) {
			cKick kick;
			ostringstream os;

			if (flags & eKI_BAN) {
				if (user->mxConn->Log(2))
					user->mxConn->LogStream() << "Kicked by " << op->mNick << " because: " << why << endl;

				string temp;
				user->mToBan = false;

				if ((flags & eKI_WHY) && why.size() && (mP.mKickBanPattern.Exec(why) >= 0)) {
					unsigned int age = 0;
					mP.mKickBanPattern.Extract(1, why, temp);

					if (temp.size())
						age = Str2Period(temp, os);

					if (age > mC.tban_max)
						age = mC.tban_max;

					if ((!age && op->Can(eUR_PBAN, mTime)) || (age && (((age > mC.tban_kick) && op->Can(eUR_TBAN, mTime)) || (age <= mC.tban_kick))))
						user->mToBan = true;

					user->mBanTime = age;

					if (mC.msg_replace_ban.size())
						mP.mKickBanPattern.Replace(0, new_why, mC.msg_replace_ban);
				}

				temp = op->mNick;
				temp += " is kicking ";
				temp += nick;
				temp += " because: ";
				temp += new_why;

				if (!mC.hide_all_kicks && !op->mHideKick)
					SendToAll(temp, op->mHideKicksForClass, int(eUC_MASTER));

				if (flags & eKI_PM) {
					os.str(mEmpty);

					if (new_why.size())
						os << autosprintf(_("You have been kicked because: %s"), new_why.c_str());
					else
						os << _("You have been kicked from hub.");

					DCPrivateHS(os.str(), user->mxConn, &op->mNick, &op->mNick);
				}

				mKickList->AddKick(user->mxConn, op->mNick, (new_why.size() ? &new_why : NULL), mC.tban_kick, kick);
				cBan ban(this);

				if (note_op.size()) // notes
					ban.mNoteOp = note_op;

				if (note_usr.size())
					ban.mNoteUsr = note_usr;

				mBanList->NewBan(ban, kick, (user->mToBan ? user->mBanTime : mC.tban_kick), eBF_NICKIP);

				if (ban.mDateEnd) {
					cTimePrint age(ban.mDateEnd - cTime().Sec(), 0);

					if (mC.notify_kicks_to_all == -1) {
						ostr << autosprintf(_("User was kicked and banned for %s: %s"), age.AsPeriod().AsString().c_str(), nick.c_str());
					} else { // message to all
						if (new_why.size())
							toall << autosprintf(_("%s was kicked and banned for %s by %s with reason: %s"), nick.c_str(), age.AsPeriod().AsString().c_str(), op->mNick.c_str(), new_why.c_str());
						else
							toall << autosprintf(_("%s was kicked and banned for %s by %s without reason."), nick.c_str(), age.AsPeriod().AsString().c_str(), op->mNick.c_str());
					}
				} else {
					if (mC.notify_kicks_to_all == -1) {
						ostr << autosprintf(_("User was kicked and banned permanently: %s"), nick.c_str());
					} else { // message to all
						if (new_why.size())
							toall << autosprintf(_("%s was kicked and banned permanently by %s with reason: %s"), nick.c_str(), op->mNick.c_str(), new_why.c_str());
						else
							toall << autosprintf(_("%s was kicked and banned permanently by %s without reason."), nick.c_str(), op->mNick.c_str());
					}
				}

				mBanList->AddBan(ban);
			} else if (flags & eKI_DROP) {
				if (user->mxConn->Log(2))
					user->mxConn->LogStream() << "Dropped by " << op->mNick << " because: " << new_why << endl;

				if (mC.notify_kicks_to_all == -1) {
					if (new_why.size())
						os << autosprintf(_("%s dropped user with reason: %s"), op->mNick.c_str(), new_why.c_str());
					else
						os << autosprintf(_("%s dropped user without reason"), op->mNick.c_str());

					ReportUserToOpchat(user->mxConn, os.str(), mC.dest_drop_chat);
				} else { // message to all
					if (new_why.size())
						toall << autosprintf(_("%s was dropped by %s with reason: %s"), nick.c_str(), op->mNick.c_str(), new_why.c_str());
					else
						toall << autosprintf(_("%s was dropped by %s without reason."), nick.c_str(), op->mNick.c_str());
				}

				if ((flags & eKI_PM) && new_why.size())
					DCPrivateHS(autosprintf(_("You have been dropped because: %s"), new_why.c_str()), user->mxConn, &op->mNick, &op->mNick);

				mKickList->AddKick(user->mxConn, op->mNick, (new_why.size() ? &new_why : NULL), mC.tban_kick, kick, true);

				if (mC.notify_kicks_to_all == -1)
					ostr << autosprintf(_("User was dropped: %s"), nick.c_str());
			}

			if (flags & eKI_CLOSE)
				user->mxConn->CloseNice(1000, eCR_KICKED);
		} else {
			ostr << _("Your action was blocked by a plugin.");
		}
	}

	if (ostr.str().size()) {
		if (use_os)
			(*use_os) << ostr.str();
		else if (op->mxConn)
			DCPublicHS(ostr.str(), op->mxConn);
	}

	if ((mC.notify_kicks_to_all > -1) && toall.str().size()) // message to all
		DCPublicToAll(mC.hub_security, toall.str(), mC.notify_kicks_to_all, int(eUC_MASTER), mC.delayed_chat);
}

string cServerDC::EraseNewLines(const string &src)
{
	string dst(src);
	size_t pos;

	while (dst.size() > 0) {
		pos = dst.find("\r");

		if (pos != dst.npos)
			dst.erase(pos, 1);
		else
			break;
	}

	while (dst.size() > 0) {
		pos = dst.find("\n");

		if (pos != dst.npos)
			dst.erase(pos, 1);
		else
			break;
	}

	return dst;
}

void cServerDC::RepBadNickChars(string &nick)
{
	string badchars(string(BAD_NICK_CHARS_NMDC) + string(BAD_NICK_CHARS_OWN));
	size_t pos;

	for (unsigned i = 0; i < badchars.size(); ++i) {
		pos = nick.find(badchars[i]);

		if (pos != nick.npos)
			nick.replace(pos, 1, 1, '_');
	}
}

/*
	global function to set SetupList config
	also take care of any hub configs that needs to be updated in real time
	returns one of following values
		-3 - failed to create config
		-2 - bad params
		-1 - undefined config
		0 - discarded by plugin
		1 - done
*/

int cServerDC::SetConfig(const char *conf, const char *var, const char *val, string &val_new, string &val_old, cUser *user)
{
	if (!conf || !var || !val)
		return -2;

	val_new = string(val);
	string sconf(conf), svar(var);
	cConfigItemBase *ci = NULL;

	if (sconf == mDBConf.config_name) {
		ci = mC[var];

		if (ci) {
			ci->ConvertTo(val_old);

			if ((svar == "hub_security") || (svar == "opchat_name")) // replace bot nick chars
				RepBadNickChars(val_new);

			ci->ConvertFrom(val_new);
			ci->ConvertTo(val_new);

			#ifndef WITHOUT_PLUGINS
				if (!mCallBacks.mOnSetConfig.CallAll((user ? user : mHubSec), &sconf, &svar, &val_new, &val_old, ci->GetTypeID())) {
					ci->ConvertFrom(val_old);
					return 0;
				}
			#endif

			if (((svar == "hub_security") || (svar == "opchat_name") || (svar == "hub_security_desc") || (svar == "opchat_desc") || (svar == "cmd_start_op") || (svar == "cmd_start_user")) && (val_new != val_old)) { // take care of special hub configs in real time
				string speed("\x1"), mail, share("0"), data;

				if (svar == "hub_security") {
					if (val_new.empty() || (val_new == mC.opchat_name)) { // dont allow empty or equal to opchat nick
						ci->ConvertFrom(val_old);
						return 0;
					}

					DelRobot((cMainRobot*)mHubSec);
					mHubSec->mNick = val_new;
					mP.Create_MyINFO(mHubSec->mMyINFO, mHubSec->mNick, mC.hub_security_desc, speed, mail, share);
					mHubSec->mMyINFO_basic = mHubSec->mMyINFO;
					AddRobot((cMainRobot*)mHubSec);
					mP.Create_Hello(data, mHubSec->mNick); // send hello
					mHelloUsers.SendToAll(data, mC.delayed_myinfo, true, mC.buffer_noswap);
					mUserList.SendToAll(mHubSec->mMyINFO, mC.delayed_myinfo, true, mC.buffer_noswap); // send myinfo
					mInProgresUsers.SendToAll(mHubSec->mMyINFO, mC.delayed_myinfo, true, mC.buffer_noswap);
					mP.Create_OpList(data, mHubSec->mNick); // send short oplist
					mUserList.SendToAll(data, mC.delayed_myinfo, true, mC.buffer_noswap);
					mInProgresUsers.SendToAll(data, mC.delayed_myinfo, true, mC.buffer_noswap);
					mP.Create_BotList(data, mHubSec->mNick); // send short botlist
					mUserList.SendToAllWithFeature(data, eSF_BOTLIST, mC.delayed_myinfo, true, mC.buffer_noswap);
					mInProgresUsers.SendToAllWithFeature(data, eSF_BOTLIST, mC.delayed_myinfo, true, mC.buffer_noswap);

					#ifndef WITHOUT_PLUGINS
						data.clear();
						mail = "_hub_security_change";
						AddScriptCommand(&mail, &val_new, &data, &data);
					#endif

				} else if (svar == "opchat_name") {
					if (val_new == mC.hub_security) { // dont allow equal to hub security nick
						ci->ConvertFrom(val_old);
						return 0;
					}

					if (mOpChat)
						DelRobot((cMainRobot*)mOpChat);

					if (val_new.size()) {
						if (mOpChat) {
							mOpChat->mNick = val_new;
						} else {
							mOpChat = new cOpChat(val_new, this);
							mOpChat->mClass = tUserCl(10);
						}

						mP.Create_MyINFO(mOpChat->mMyINFO, mOpChat->mNick, mC.opchat_desc, speed, mail, share);
						mOpChat->mMyINFO_basic = mOpChat->mMyINFO;
						AddRobot((cMainRobot*)mOpChat);
						mP.Create_Hello(data, mOpChat->mNick); // send hello
						mHelloUsers.SendToAll(data, mC.delayed_myinfo, true, mC.buffer_noswap);
						mUserList.SendToAll(mOpChat->mMyINFO, mC.delayed_myinfo, true, mC.buffer_noswap); // send myinfo
						mInProgresUsers.SendToAll(mOpChat->mMyINFO, mC.delayed_myinfo, true, mC.buffer_noswap);
						mP.Create_OpList(data, mOpChat->mNick); // send short oplist
						mUserList.SendToAll(data, mC.delayed_myinfo, true, mC.buffer_noswap);
						mInProgresUsers.SendToAll(data, mC.delayed_myinfo, true, mC.buffer_noswap);
						mP.Create_BotList(data, mOpChat->mNick); // send short botlist
						mUserList.SendToAllWithFeature(data, eSF_BOTLIST, mC.delayed_myinfo, true, mC.buffer_noswap);
						mInProgresUsers.SendToAllWithFeature(data, eSF_BOTLIST, mC.delayed_myinfo, true, mC.buffer_noswap);
					} else if (mOpChat) {
						delete mOpChat;
						mOpChat = NULL;
					}

					#ifndef WITHOUT_PLUGINS
						data.clear();
						mail = "_opchat_name_change";
						AddScriptCommand(&mail, &val_new, &data, &data);
					#endif

				} else if (svar == "hub_security_desc") {
					mP.Create_MyINFO(mHubSec->mMyINFO, mHubSec->mNick, val_new, speed, mail, share);
					mHubSec->mMyINFO_basic = mHubSec->mMyINFO;
					mUserList.SendToAll(mHubSec->mMyINFO, mC.delayed_myinfo, true, mC.buffer_noswap); // send myinfo
					mInProgresUsers.SendToAll(mHubSec->mMyINFO, mC.delayed_myinfo, true, mC.buffer_noswap);

				} else if (svar == "opchat_desc") {
					if (mOpChat) {
						mP.Create_MyINFO(mOpChat->mMyINFO, mOpChat->mNick, val_new, speed, mail, share);
						mOpChat->mMyINFO_basic = mOpChat->mMyINFO;
						mUserList.SendToAll(mOpChat->mMyINFO, mC.delayed_myinfo, true, mC.buffer_noswap); // send myinfo
						mInProgresUsers.SendToAll(mOpChat->mMyINFO, mC.delayed_myinfo, true, mC.buffer_noswap);
					}

				} else if ((svar == "cmd_start_op") || (svar == "cmd_start_user")) {
					if (val_new.empty()) { // dont allow empty
						ci->ConvertFrom(val_old);
						return 0;
					}
				}

			} else if (((svar == "listen_port") || (svar == "extra_listen_ports")) && (val_new != val_old)) { // take care of listen ports
				unsigned int port, extra;
				char *data = NULL;

				if (svar == "listen_port") { // main port
					if (val_new.empty()) { // dont allow empty
						ci->ConvertFrom(val_old);
						return 0;
					}

					port = StringAsLL(val_new);

					if (!CheckPortNumber(port)) { // out of range
						ci->ConvertFrom(val_old);
						return 0;
					}

					data = GetConfig(mDBConf.config_name.c_str(), "extra_listen_ports", "");

					if (data) {
						sconf = string(data);

						if (sconf.size()) {
							istringstream is(sconf);

							while (1) {
								sconf.clear();
								is >> sconf;

								if (sconf.empty())
									break;

								extra = StringAsLL(sconf);

								if (CheckPortNumber(extra) && (port == extra)) { // found in extra ports
									ci->ConvertFrom(val_old);
									free(data);
									return 0;
								}
							}
						}
					}

				} else if ((svar == "extra_listen_ports") && val_new.size()) { // extra ports
					data = GetConfig(mDBConf.config_name.c_str(), "listen_port", "");

					if (data) {
						sconf = string(data);

						if (sconf.size()) {
							port = StringAsLL(sconf);

							if (CheckPortNumber(port)) {
								istringstream is(val_new);

								while (1) {
									sconf.clear();
									is >> sconf;

									if (sconf.empty())
										break;

									extra = StringAsLL(sconf);

									if (CheckPortNumber(extra) && (extra == port)) { // found in main port
										ci->ConvertFrom(val_old);
										free(data);
										return 0;
									}
								}
							}
						}
					}
				}

				if (data)
					free(data);
			}

			mSetupList.SaveItem(conf, ci);
			return 1;

		} else {
			#ifndef WITHOUT_PLUGINS
			if (mCallBacks.mOnSetConfig.CallAll((user ? user : mHubSec), &sconf, &svar, &val_new, &val_old, -1))
			#endif
			{
				if (ErrLog(1))
					LogStream() << "Undefined SetConfig variable: " << conf << '.' << var << endl;
			}

			return -1;
		}
	}

	string fake;
	ci = new cConfigItemBaseString(fake, var);

	if (ci) {
		bool load = mSetupList.LoadItem(conf, ci);

		if (load)
			ci->ConvertTo(val_old);

		ci->ConvertFrom(val_new);
		ci->ConvertTo(val_new);
		bool save = true;

		#ifndef WITHOUT_PLUGINS
			save = mCallBacks.mOnSetConfig.CallAll((user ? user : mHubSec), &sconf, &svar, &val_new, &val_old, (load ? ci->GetTypeID() : -1));
		#endif

		if (save)
			mSetupList.SaveItem(conf, ci);

		delete ci;
		ci = NULL;
		return int(save);
	}

	return -3;
}

char* cServerDC::GetConfig(const char *conf, const char *var, const char *def)
{
	char *ret = NULL;

	if (def)
		ret = strdup(def); // caller must free non null values returned by getconfig

	if (!conf || !var)
		return ret;

	string sconf(conf), val;
	cConfigItemBase *ci = NULL;

	if (sconf == mDBConf.config_name) {
		ci = mC[var];

		if (ci) {
			ci->ConvertTo(val);
			if (ret) free(ret);
			return strdup(val.c_str());
		} else {
			if (ErrLog(1))
				LogStream() << "Undefined GetConfig variable: " << conf << '.' << var << endl;

			return ret;
		}
	}

	string fake;
	ci = new cConfigItemBaseString(fake, var);

	if (ci) {
		bool load = mSetupList.LoadItem(conf, ci);

		if (load)
			ci->ConvertTo(val);

		delete ci;
		ci = NULL;

		if (load) {
			if (ret) free(ret);
			return strdup(val.c_str());
		}
	}

	return ret;
}

/*
	this functions collects stack backtrace of the caller functions and demangles it
	then it tries to send backtrace to crash server via http
	and writes backtrace to standard error output
*/

void cServerDC::DoStackTrace()
{
	if (!mStackTrace)
		return;

	void* addrlist[64];
	int addrlen = backtrace(addrlist, sizeof(addrlist) / sizeof(void*));

	if (!addrlen) {
		vhErr(0) << "Stack backtrace is empty, possibly corrupt" << endl;
		return;
	}

	char** symbollist = backtrace_symbols(addrlist, addrlen);
	size_t funcnamesize = 256;
	char* funcname = (char*)malloc(funcnamesize);
	ostringstream bt;

	for (int i = 1; i < addrlen; i++) {
		char *begin_name = 0, *begin_offset = 0, *end_offset = 0;

		for (char *p = symbollist[i]; *p; ++p) {
			if (*p == '(') {
				begin_name = p;
			} else if (*p == '+') {
				begin_offset = p;
			} else if ((*p == ')') && begin_offset) {
				end_offset = p;
				break;
			}
		}

		if (begin_name && begin_offset && end_offset && (begin_name < begin_offset)) {
			*begin_name++ = '\0';
			*begin_offset++ = '\0';
			*end_offset = '\0';
			int status;
			char* ret = abi::__cxa_demangle(begin_name, funcname, &funcnamesize, &status);

			if (!status) {
				funcname = ret;
				bt << symbollist[i] << ": " << funcname << " +" << begin_offset << endl;
			} else {
				bt << symbollist[i] << ": " << begin_name << "() +" << begin_offset << endl;
			}
		} else {
			bt << symbollist[i] << endl;
		}
	}

	free(funcname);
	free(symbollist);
	vhErr(0) << "Stack backtrace:" << endl << endl << bt.str() << endl;

	if (!mC.send_crash_report)
		return;

	cHTTPConn *http = new cHTTPConn(CRASH_SERV_ADDR, CRASH_SERV_PORT); // try to send via http

	if (!http || !http->mGood) {
		vhErr(0) << "Failed connecting to crash server, please send above stack backtrace here: https://github.com/verlihub/verlihub/issues" << endl;

		if (http) {
			http->Close();
			delete http;
			http = NULL;
		}

		return;
	}

	ostringstream head;
	head << "Hub-Info-Host: " << EraseNewLines(mC.hub_host) << "\r\n"; // remove new lines, they will break our request
	head << "Hub-Info-Address: " << mAddr << ':' << mPort << "\r\n";
	head << "Hub-Info-Uptime: " << (mTime.Sec() - mStartTime.Sec()) << "\r\n"; // uptime in seconds
	head << "Hub-Info-Users: " << mUserCountTot << "\r\n";

	if (http->Request("POST", "/vhcs.php", head.str(), bt.str()) && http->mGood) {
		vhErr(0) << "Successfully sent stack backtrace to crash server" << endl;
	} else {
		vhErr(0) << "Failed sending to crash server, please post above stack backtrace here: https://github.com/verlihub/verlihub/issues" << endl;
	}

	http->Close();
	delete http;
	http = NULL;
}

/*
	ctm2hub
*/

void cServerDC::CtmToHubAddItem(cConnDC *conn, const string &ref)
{
	if (!conn)
		return;

	bool uniq = true;

	if (ref.empty())
		uniq = false;
	else if (mCtmToHubList.size()) {
		tCtmToHubList::iterator it;

		for (it = mCtmToHubList.begin(); it != mCtmToHubList.end(); ++it) {
			if ((*it) && (StrCompare(ref, 0, ref.size(), (*it)->mRef) == 0)) {
				uniq = false;
				break;
			}
		}
	}

	sCtmToHubItem *item = new sCtmToHubItem;
	//item->mNick = conn->mMyNick; // todo: make use of these
	//item->mIP = conn->AddrIP();
	//item->mCC = conn->mCC; // todo: conn->getGeoCC()
	//item->mPort = conn->AddrPort();
	//item->mServ = conn->GetServPort();
	item->mRef = ref;
	item->mUniq = uniq;
	mCtmToHubList.push_back(item);

	if ((++mCtmToHubConf.mNew > 9) && !mCtmToHubConf.mStart) {
		string omsg = _("DDoS detected, gathering attack information...");

		if (this->mOpChat) // todo: where else to notify?
			this->mOpChat->SendPMToAll(omsg, NULL);

		mCtmToHubConf.mStart = true;
	}
}

int cServerDC::CtmToHubRefererList(string &list)
{
	if (!mCtmToHubList.size())
		return 0;

	int count = 0;
	tCtmToHubList::iterator it;

	for (it = mCtmToHubList.begin(); it != mCtmToHubList.end(); ++it) {
		if ((*it) && (*it)->mUniq) {
			list.append(1, ' ');
			list.append((*it)->mRef);
			list.append("\r\n");
			(*it)->mUniq = false;
			count++;
		}
	}

	return count;
}

void cServerDC::CtmToHubClearList()
{
	if (!mCtmToHubList.size())
		return;

	tCtmToHubList::iterator it;

	for (it = mCtmToHubList.begin(); it != mCtmToHubList.end(); ++it) {
		if (*it) {
			delete (*it);
			(*it) = NULL;
		}
	}

	mCtmToHubList.clear();
}

void cServerDC::Reload()
{
	mC.Load();
	mCo->mTriggers->ReloadAll();
	mCo->mRedirects->ReloadAll();
	mCo->mDCClients->ReloadAll();

	if (mC.use_reglist_cache)
		mR->UpdateCache();

	if (mC.use_penlist_cache)
		mPenList->UpdateCache();

	this->mMaxMindDB->ReloadAll(); // reload maxminddb
}

	}; // namespace nServer
}; // namespace nVerliHub
