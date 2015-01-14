/*
	Copyright (C) 2003-2005 Daniel Muller, dan at verliba dot cz
	Copyright (C) 2006-2012 Verlihub Team, devs at verlihub-project dot org
	Copyright (C) 2013-2015 RoLex, webmaster at feardc dot net

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
#include "curr_date_time.h"
#include "cthreadwork.h"
#include "stringutils.h"
#include "cconntypes.h"
#include "cdcconsole.h"
#include "ctriggers.h"
#include "i18n.h"

#define HUB_VERSION_CLASS __CURR_DATE_TIME__
#define HUB_VERSION_NAME "Verlihub"
#define PADDING 25

namespace nVerliHub {
	using namespace nUtils;
	using namespace nEnums;
	using namespace nThread;
	using namespace nTables;
	namespace nSocket {
		cServerDC * cServerDC::sCurrentServer = NULL;

		#if HAVE_LIBGEOIP
			cGeoIP cServerDC::sGeoIP;
		#endif

cServerDC::cServerDC( string CfgBase , const string &ExecPath):
	cAsyncSocketServer(), // create parent class
	mConfigBaseDir(CfgBase),
	mDBConf(CfgBase + "/dbconfig"), // load the db config
	mMySQL(mDBConf.db_host, mDBConf.db_user, mDBConf.db_pass, mDBConf.db_data, mDBConf.db_charset), // connect to mysql
	mC(*this), // create the config object
	mSetupList(mMySQL),
	mP(this),
	mCo(NULL), //  create console and it's tables
	mR(NULL),
	mPenList(NULL),
	mBanList(NULL),
	mUnBanList(NULL),
	mKickList(NULL),
	mOpChat(NULL),
	mExecPath(ExecPath),
	mSysLoad(eSL_NORMAL),
	mUserList(true,true, true, &mCallBacks.mNickListNicks, &mCallBacks.mNickListInfos),
	mOpList(true, false, false, &mCallBacks.mOpListNicks, NULL),
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
	mSetupList.CreateTable();
	mC.AddVars();
	mC.Save();
	mC.Load();
	mConnTypes = new cConnTypes(this);
	mCo = new cDCConsole(this,mMySQL);
	mR = new cRegList(mMySQL,this);
	mBanList = new cBanList(this);
	mUnBanList = new cUnBanList(this);
	mPenList = new cPenaltyList(mMySQL);
	mKickList = new cKickList(mMySQL);
	mZLib = new cZLib();
	int i;
	for ( i = 0; i <= USER_ZONES; i++ ) mUserCount[i]=0;
	for ( i = 0; i <= USER_ZONES; i++ ) mUploadZone[i].SetPeriod(60.);

	SetClassName("cServerDC");

	mR->CreateTable();
	if(mC.use_reglist_cache) mR->ReloadCache();
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
	if(mC.use_penlist_cache) mPenList->ReloadCache();

	// setup userlists
	string nctmp;
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

	// add the users
	string speed(/*"Hub\x9"*/"\x1"),mail(""),share("0");
	cUser *VerliHub;
	VerliHub=new cMainRobot(mC.hub_security, this);
	VerliHub->mClass=tUserCl(10);
	mP.Create_MyINFO(VerliHub->mMyINFO ,VerliHub->mNick,mC.hub_security_desc,speed,mail,share);
	VerliHub->mMyINFO_basic = VerliHub->mMyINFO;
	AddRobot((cMainRobot*)VerliHub);

	if(mC.opchat_name.size()) {
		mOpChat=new cOpChat(this);
		mOpChat->mClass=tUserCl(10);
		mP.Create_MyINFO(mOpChat->mMyINFO, mOpChat->mNick,mC.opchat_desc,speed,mail,share);
		mOpChat->mMyINFO_basic = mOpChat->mMyINFO;
		AddRobot((cMainRobot*)mOpChat);
	}
	string net_log(mConfigBaseDir);
	net_log.append( "/net_out.log");
	mNetOutLog.open(net_log.c_str(), ios::out);
	mTotalShare = 0;
	mTotalSharePeak = 0;

	mFactory = new cDCConnFactory(this);

	try {
		mPluginManager.LoadAll();
	} catch (...)
	{
		if(ErrLog(1)) LogStream() << "Plugin loading error" << endl;
	}
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

	CtmToHubClearList(); // ctm2hub

	if (mNetOutLog && mNetOutLog.is_open())
		mNetOutLog.close();

	// remove all users
	cUserCollection::iterator it;
	cUser *user;
	for (it=mUserList.begin(); it != mUserList.end();) {
		user = (cUser*)*it;
		++it;
		if (user->mxConn) {
			delConnection(user->mxConn);
		} else {
			this->RemoveNick(user);
		}
	}

	// destruct the lists of pointers
	for (tTFIt i = mTmpFunc.begin(); i != mTmpFunc.end(); i++ )
		if(*i)
			delete *i;
	close();
	if (mFactory) delete mFactory;
	mFactory = NULL;
	if (mConnTypes) delete mConnTypes;
	mConnTypes = NULL;
	if (mR) delete mR;
	mR= NULL;
	if (mBanList) delete mBanList;
	mBanList = NULL;
	if (mUnBanList) delete mUnBanList;
	mUnBanList = NULL;
	if (mPenList) delete mPenList;
	mPenList = NULL;
	if (mKickList) delete mKickList;
	mKickList = NULL;
	if (mCo) delete mCo;
	mCo = NULL;
	if(mZLib)
		delete mZLib;
	mZLib = NULL;
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

tMsgAct cServerDC::Filter( tDCMsg msg, cConnDC *conn )
{
	tMsgAct result = eMA_PROCEED;
	if(!conn) {
		if(ErrLog(0)) LogStream() << "Got NULL conn into filter" << endl;
		return eMA_ERROR;
	}
	if(!conn->mpUser || !conn->mpUser->mInList) {
		switch(msg) {
			case eDC_KEY:
			case eDC_VALIDATENICK:
			case eDC_MYPASS:
			case eDC_VERSION:
			case eDC_GETNICKLIST:
			case eDC_MYINFO:
			case eDC_UNKNOWN:
			break;
			default: result = eMA_HANGUP;
		}
	} else {
		switch(msg) {
			case eDC_KEY:
			case eDC_VALIDATENICK:
			case eDC_MYPASS:
			case eDC_VERSION:
				result=eMA_HANGUP;
			break;
			default: break;
		}
	}

	switch(mSysLoad) {
		case eSL_SYSTEM_DOWN: return eMA_TBAN; break; // locked up
		case eSL_RECOVERY: return eMA_HANGUP1; break; // attempting recovery
		case eSL_CAPACITY: break; // limits reached
		case eSL_PROGRESSIVE: break; // approaching limits
		case eSL_NORMAL: break; // normal mode
		default: break;
	}
	return result;
}

int cServerDC::DCPublic(const string &from, const string &txt, cConnDC *conn)
{
	if (conn) {
		if (!txt.empty()) {
			static string msg;
			msg.erase();
			mP.Create_Chat(msg, from, txt);
			conn->Send(msg, true);
		}

		return 1;
	}

	return 0;
}

int cServerDC::DCPublicToAll(const string &from, const string &txt, int min_class, int max_class)
{
	static string msg;
	msg.erase();
	mP.Create_Chat(msg, from, txt);

	if ((min_class != eUC_NORMUSER) || (max_class != eUC_MASTER))
		mUserList.SendToAllWithClass(msg, min_class, max_class, true, true);
	else
		mUserList.SendToAll(msg, true, true);

	return 1;
}

int cServerDC::DCPublicHS(const string &text, cConnDC *conn)
{
	return DCPublic(mC.hub_security, text, conn);
}

void cServerDC::DCPublicHSToAll(const string &text)
{
	static string msg;
	msg.erase();
	mP.Create_Chat(msg, mC.hub_security, text);
	mUserList.SendToAll(msg, true, true);
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
	} else return false;
}

bool cServerDC::DelRobot(cUserRobot *robot)
{
	if (this->RemoveNick(robot)) {
		mRobotList.Remove(robot);
		return true;
	} else return false;
}

bool cServerDC::AddToList(cUser *usr)
{
	if(!usr) {
		if(ErrLog(1)) LogStream() << "Adding a NULL user to userlist" << endl;
		return false;
	}
	if(usr->mInList) {
		if(ErrLog(2)) LogStream() << "User is already in the user list, he says it " << endl;
		return false;
	}

	tUserHash Hash = mUserList.Nick2Hash(usr->mNick);

	if(!mUserList.AddWithHash(usr, Hash)) {
		if(ErrLog(2)) LogStream() << "Adding twice user with same hash " << usr->mNick << endl;
		usr->mInList = false;
		return false;
	}

	usr->mInList = true;
	if(!usr->IsPassive)
		mActiveUsers.AddWithHash(usr, Hash);
	if(usr->IsPassive)
		mPassiveUsers.AddWithHash(usr, Hash);
	if(usr->mClass >= eUC_OPERATOR && ! (usr->mxConn && usr->mxConn->mRegInfo && usr->mxConn->mRegInfo->mHideKeys))
		mOpList.AddWithHash(usr, Hash);
	if(usr->Can(eUR_OPCHAT, mTime.Sec()))
		mOpchatList.AddWithHash(usr, Hash);
	if(usr->mxConn && !(usr->mxConn->mFeatures & eSF_NOHELLO))
		mHelloUsers.AddWithHash(usr, Hash);

	if ((usr->mClass >= eUC_OPERATOR) || mC.chat_default_on) {
		mChatUsers.AddWithHash(usr, Hash);
	} else if (usr->mxConn) {
		DCPublicHS(_("You won't see public chat messages, to restore use +chat command."), usr->mxConn);
	}

	if(usr->mxConn && usr->mxConn->Log(3))
		usr->mxConn->LogStream() << "Adding at the end of Nicklist" << endl;

	if(usr->mxConn && usr->mxConn->Log(3))
		usr->mxConn->LogStream() << "Becomes in list" << endl;

	return true;
}

bool cServerDC::RemoveNick(cUser *User)
{
	tUserHash Hash = mUserList.Nick2Hash(User->mNick);

	if(mUserList.ContainsHash(Hash)) {
		#ifndef WITHOUT_PLUGINS
		if (User->mxConn && User->mxConn->GetLSFlag(eLS_LOGIN_DONE) && User->mInList) mCallBacks.mOnUserLogout.CallAll(User);
		#endif
                // make sure that the user we want to remove is the correct one!
                cUser *other = mUserList.GetUserByNick(User->mNick);
		if(!User->mxConn) {
			//cout << "Removing robot user" << endl;
			mUserList.RemoveByHash(Hash);
		} else if(other && other->mxConn && User->mxConn && other->mxConn == User->mxConn) {
            		//cout << "Leave: " << User->mNick << " count = " << mUserList.size() << endl;
			mUserList.RemoveByHash(Hash);
		} else {
			// this may cause problems when user is robot with 0 connection
			//cout << "NOT found the correct user!, skip removing: " << User->mNick << endl;
	                return false;
		}
	}
	if(mOpList.ContainsHash(Hash))
		mOpList.RemoveByHash(Hash);
	if(mOpchatList.ContainsHash(Hash))
		mOpchatList.RemoveByHash(Hash);
	if(mActiveUsers.ContainsHash(Hash))
		mActiveUsers.RemoveByHash(Hash);
	if(mPassiveUsers.ContainsHash(Hash))
		mPassiveUsers.RemoveByHash(Hash);
	if(mHelloUsers.ContainsHash(Hash))
		mHelloUsers.RemoveByHash(Hash);
	if(mChatUsers.ContainsHash(Hash))
		mChatUsers.RemoveByHash(Hash);
	if(mInProgresUsers.ContainsHash(Hash))
		mInProgresUsers.RemoveByHash(Hash);

	if(User->mInList) {
		User->mInList=false;
		static string omsg;
		omsg = "";
		cDCProto::Create_Quit(omsg, User->mNick);

		// delayed myinfo implies delay of quit too, otherwise there would be mess in peoples userslists
		mUserList.SendToAll(omsg,mC.delayed_myinfo, true);
		if(mC.show_tags == 1) {
			mOpchatList.SendToAll(omsg,mC.delayed_myinfo, true);
		}
	}
	return true;
}

void cServerDC::OnScriptCommand(string *cmd, string *data, string *plug, string *script)
{
	mCallBacks.mOnScriptCommand.CallAll(cmd, data, plug, script);
}

/*
bool cServerDC::OnOpChatMessage(string *nick, string *data)
{
	return mCallBacks.mOnOpChatMessage.CallAll(nick, data);
}
*/

void cServerDC::SendToAll(string &data, int cm, int cM) // class range is ignored here
{
	cConnDC *conn;
	tCLIt i;

	for (i = mConnList.begin(); i != mConnList.end(); i++) {
		conn = (cConnDC*)(*i);

		if (conn && conn->ok && conn->mpUser && conn->mpUser->mInList)
			conn->Send(data, true);
	}
}

int cServerDC::SendToAllWithNick(const string &start,const string &end, int cm,int cM)
{
	static string str;
	cConnDC *conn;
	tCLIt i;
	int counter = 0;
	for(i=mConnList.begin(); i!= mConnList.end(); i++) {
		conn=(cConnDC *)(*i);
		if(
			conn &&
			conn->ok &&
			conn->mpUser &&
			conn->mpUser->mInList &&
			conn->mpUser->mClass >= cm &&
			conn->mpUser->mClass <= cM
			)
		{
			str=start + conn->mpUser->mNick + end + "|";
			conn->Send(str, false);
			counter ++;
		}
	}
	return counter;
}

int cServerDC::SendToAllWithNickVars(const string &start, const string &end, int cm, int cM)
{
	static string str;
	string tend;
	cConnDC *conn;
	tCLIt i;
	int counter = 0;

	for (i = mConnList.begin(); i != mConnList.end(); i++) {
		conn = (cConnDC *)(*i);

		if (conn && conn->ok && conn->mpUser && conn->mpUser->mInList && conn->mpUser->mClass >= cm && conn->mpUser->mClass <= cM) {
			// replace variables
			tend = end;
			ReplaceVarInString(tend, "NICK", tend, conn->mpUser->mNick);
			ReplaceVarInString(tend, "CLASS", tend, conn->mpUser->mClass);
			ReplaceVarInString(tend, "CC", tend, conn->mCC);
			ReplaceVarInString(tend, "CN", tend, conn->mCN);
			ReplaceVarInString(tend, "CITY", tend, conn->mCity);
			ReplaceVarInString(tend, "IP", tend, conn->AddrIP());
			ReplaceVarInString(tend, "HOST", tend, conn->AddrHost());

			// finalize
			str = start + conn->mpUser->mNick + tend + "|";
			conn->Send(str, false);
			counter++;
		}
	}

	return counter;
}

int cServerDC::SendToAllNoNickVars(const string &msg, int cm, int cM)
{
	string tmsg;
	cConnDC *conn;
	tCLIt i;
	int counter = 0;

	for (i = mConnList.begin(); i != mConnList.end(); i++) {
		conn = (cConnDC *)(*i);

		if (conn && conn->ok && conn->mpUser && conn->mpUser->mInList && conn->mpUser->mClass >= cm && conn->mpUser->mClass <= cM) {
			// replace variables
			tmsg = msg;
			ReplaceVarInString(tmsg, "NICK", tmsg, conn->mpUser->mNick);
			ReplaceVarInString(tmsg, "CLASS", tmsg, conn->mpUser->mClass);
			ReplaceVarInString(tmsg, "CC", tmsg, conn->mCC);
			ReplaceVarInString(tmsg, "CN", tmsg, conn->mCN);
			ReplaceVarInString(tmsg, "CITY", tmsg, conn->mCity);
			ReplaceVarInString(tmsg, "IP", tmsg, conn->AddrIP());
			ReplaceVarInString(tmsg, "HOST", tmsg, conn->AddrHost());

			// finalize
			conn->Send(tmsg);
			counter++;
		}
	}

	return counter;
}

int cServerDC::SendToAllWithNickCC(const string &start,const string &end, int cm,int cM, const string &cc_zone)
{
	static string str;
	cConnDC *conn;
	tCLIt i;
	int counter = 0;
	for(i=mConnList.begin(); i!= mConnList.end(); i++) {
		conn=(cConnDC *)(*i);
		if(
			conn &&
			conn->ok &&
			conn->mpUser &&
			conn->mpUser->mInList &&
			conn->mpUser->mClass >= cm &&
			conn->mpUser->mClass <= cM &&
			cc_zone.npos != cc_zone.find(conn->mCC)
			)
		{
			str=start + conn->mpUser->mNick + end + "|";
			conn->Send(str, false);
			counter++;
		}
	}
	return counter;
}

int cServerDC::SendToAllWithNickCCVars(const string &start, const string &end, int cm, int cM, const string &cc_zone)
{
	static string str;
	string tend;
	cConnDC *conn;
	tCLIt i;
	int counter = 0;

	for (i = mConnList.begin(); i != mConnList.end(); i++) {
		conn = (cConnDC *)(*i);

		if (conn && conn->ok && conn->mpUser && conn->mpUser->mInList && conn->mpUser->mClass >= cm && conn->mpUser->mClass <= cM && cc_zone.npos != cc_zone.find(conn->mCC)) {
			// replace variables
			tend = end;
			ReplaceVarInString(tend, "NICK", tend, conn->mpUser->mNick);
			ReplaceVarInString(tend, "CLASS", tend, conn->mpUser->mClass);
			ReplaceVarInString(tend, "CC", tend, conn->mCC);
			ReplaceVarInString(tend, "CN", tend, conn->mCN);
			ReplaceVarInString(tend, "CITY", tend, conn->mCity);
			ReplaceVarInString(tend, "IP", tend, conn->AddrIP());
			ReplaceVarInString(tend, "HOST", tend, conn->AddrHost());

			// finalize
			str = start + conn->mpUser->mNick + tend + "|";
			conn->Send(str, false);
			counter++;
		}
	}

	return counter;
}

int cServerDC::SearchToAll(cConnDC *conn, string &data, bool passive)
{
	cConnDC *other;
	tCLIt i;
	int count = 0;

	if (passive) { // passive search
		for (i = mConnList.begin(); i != mConnList.end(); i++) {
			other = (cConnDC*)(*i);

			if (!other || !other->ok || !other->mpUser || !other->mpUser->mInList) // base condition
				continue;

			if (other->mpUser->IsPassive) // dont send to passive user
				continue;

			if (other->mpUser->mShare <= 0) // dont send to user without share
				continue;

			if (other->mpUser->mHideShare) // dont send to user with hidden share
				continue;

			if (other->mpUser->mClass < eUC_NORMUSER) // dont send to pinger
				continue;

			if (other->mpUser->mNick == conn->mpUser->mNick) // dont send to self
				continue;

			other->Send(data, true, !mC.delayed_search);
			count++;
		}
	} else { // active search
		if (mC.filter_lan_requests) { // filter lan requests
			bool lan = cDCProto::isLanIP(conn->AddrIP());

			for (i = mConnList.begin(); i != mConnList.end(); i++) {
				other = (cConnDC*)(*i);

				if (!other || !other->ok || !other->mpUser || !other->mpUser->mInList) // base condition
					continue;

				if (other->mpUser->mShare <= 0) // dont send to user without share
					continue;

				if (other->mpUser->mHideShare) // dont send to user with hidden share
					continue;

				if (other->mpUser->mClass < eUC_NORMUSER) // dont send to pinger
					continue;

				if (other->mpUser->mNick == conn->mpUser->mNick) // dont send to self
					continue;

				if (lan != cDCProto::isLanIP(other->AddrIP())) // filter lan to wan and reverse
					continue;

				other->Send(data, true, !mC.delayed_search);
				count++;
			}
		} else { // dont filter lan requests
			for (i = mConnList.begin(); i != mConnList.end(); i++) {
				other = (cConnDC*)(*i);

				if (!other || !other->ok || !other->mpUser || !other->mpUser->mInList) // base condition
					continue;

				if (other->mpUser->mShare <= 0) // dont send to user without share
					continue;

				if (other->mpUser->mHideShare) // dont send to user with hidden share
					continue;

				if (other->mpUser->mClass < eUC_NORMUSER) // dont send to pinger
					continue;

				if (other->mpUser->mNick == conn->mpUser->mNick) // dont send to self
					continue;

				other->Send(data, true, !mC.delayed_search);
				count++;
			}
		}
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
	omsg += VERSION;

	conn->Send(omsg, true);
	SendHeaders(conn, 2);
	ostringstream os;

	if (mSysLoad >= eSL_RECOVERY) {
		os << _("Hub is currently unable to service your request, please try again in a few minutes.");
		ConnCloseMsg(conn, os.str(), 1000, eCR_HUB_LOAD);
		return -1;
	}

	if (mBanList->IsIPTempBanned(conn->AddrIP())) { // check temporary ip ban
		cBanList::sTempBan *tban = mBanList->mTempIPBanlist.GetByHash(cBanList::Ip2Num(conn->AddrIP()));

		if (tban && (tban->mUntil > mTime.Sec())) {
			os << autosprintf(_("You're still temporarily banned for %s because: %s"), cTime(tban->mUntil - mTime.Sec()).AsPeriod().AsString().c_str(), tban->mReason.c_str());

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
	if(conn->Log(4))
		conn->LogStream() << "IN: " << (*str) << "|" << endl;
	conn->mpMsgParser->Parse();
	conn->mxProtocol->TreatMsg(conn->mpMsgParser, conn);
}

bool cServerDC::VerifyUniqueNick(cConnDC *conn)
{
	string userkey;
	mUserList.Nick2Key(conn->mpUser->mNick, userkey);

	if (mUserList.ContainsKey(userkey)) {
		cUser *olduser = mUserList.GetUserByKey(userkey);
		bool sameuser = false;
		string omsg;

		if (conn->mpUser->mClass >= eUC_REGUSER)
			sameuser = true;

		if (!sameuser && olduser && olduser->mxConn && (conn->AddrIP() == olduser->mxConn->AddrIP()) && (conn->mpUser->mShare == olduser->mShare) && (conn->mpUser->mMyINFO_basic == olduser->mMyINFO_basic))
			sameuser = true;

		if (sameuser) {
			if (olduser) {
				if (olduser->mxConn) {
					if (olduser->mxConn->Log(2))
						olduser->mxConn->LogStream() << "Closing because of a new connection" << endl;

					omsg = _("Another user has logged in with same nick and IP address.");
					DCPublicHS(omsg, olduser->mxConn);
					olduser->mxConn->CloseNow();
				} else {
					if (ErrLog(1))
						LogStream() << "[CRITICAL] Found user " << olduser->mNick << " without a valid conneciton pointer" << endl;
				}

				RemoveNick(olduser);
			} else {
				conn->CloseNow();
				return false;
			}
		} else {
			omsg = _("Your nick is already taken by another user.");
			DCPublicHS(omsg, conn);
			omsg = "$ValidateDenide " + conn->mpUser->mNick;
			conn->Send(omsg);
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
					omsg = "$ValidateDenide " + conn->mpUser->mNick;
					conn->Send(omsg);
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
	ostringstream os;
	if(conn->Log(3))
		conn->LogStream() << "Entered the hub." << endl;
	mCo->mTriggers->TriggerAll(eTF_MOTD, conn);

	// the user has to set or change password
	if (conn->mRegInfo && conn->mRegInfo->mPwdChange) {
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
			string getpass("$GetPass");
			conn->Send(getpass);
		}

		conn->SetTimeOut(eTO_SETPASS, mC.timeout_length[eTO_SETPASS], this->mTime);
		os.str("");
	}

	// send the hub topic
	if (!mC.hub_topic.empty()) {
		string topic("$HubTopic ");
		topic += mC.hub_topic + "|";
		conn->Send(topic, false);
	}

	if (mC.send_user_info) {
		os << _("Your information") << ":\r\n";
		conn->mpUser->DisplayInfo(os, eUC_OPERATOR);
		DCPublicHS(os.str(), conn);
	}

	if(mUserList.Size() > mUsersPeak)
		mUsersPeak = mUserList.Size();
	#ifndef WITHOUT_PLUGINS
	if (!mCallBacks.mOnUserLogin.CallAll(conn->mpUser)) { // todo: i think we need to move this before hubtopic and userinfo
		conn->CloseNow();
		return;
	}
	#endif

	if ((conn->mpUser->mClass >= eUC_NORMUSER) && (conn->mpUser->mClass <= eUC_MASTER)) {
		if (mC.msg_welcome[conn->mpUser->mClass].size()) {
			string ToSend;
			ReplaceVarInString(mC.msg_welcome[conn->mpUser->mClass], "nick", ToSend, conn->mpUser->mNick);
			ReplaceVarInString(ToSend, "CC", ToSend, conn->mCC);
			ReplaceVarInString(ToSend, "CN", ToSend, conn->mCN);
			ReplaceVarInString(ToSend, "CITY", ToSend, conn->mCity);
			DCPublicHSToAll(ToSend);
		}
	}
}

void cServerDC::DoUserLogin(cConnDC *conn)
{
	// verify we didn't get here by chance
	if(eLS_LOGIN_DONE != conn->GetLSFlag(eLS_LOGIN_DONE)) {
		if(conn->ErrLog(2))
			conn->LogStream() << "User Login when not all done"<<endl;
		conn->CloseNow();
		return;
	}

	// check if same nick already exists
	if (!VerifyUniqueNick(conn))
		return;

	// he is not anymore in progress
	if (mInProgresUsers.ContainsNick(conn->mpUser->mNick)) {
		mInProgresUsers.FlushForUser(conn->mpUser);
		mInProgresUsers.Remove(conn->mpUser);
	}

	if (mC.int_login && (conn->GetTheoricalClass() <= mC.max_class_int_login)) // login flood detection
		mBanList->AddNickTempBan(conn->mpUser->mNick, mTime.Sec() + mC.int_login, _("Reconnecting too fast"), eBT_RECON);

	// users special rights and restrictions
	cPenaltyList::sPenalty pen;
	if (mPenList->LoadTo(pen, conn->mpUser->mNick) && (conn->mpUser->mClass != eUC_PINGER))
		conn->mpUser->ApplyRights(pen);

	// insert user to userlist
	if(!AddToList(conn->mpUser)) {
		conn->CloseNow();
		return;
	}

	ShowUserToAll(conn->mpUser); // display user to others

	if (!mC.hub_topic.empty()) { // send hub name with topic
		static string omsg;
		omsg.erase();
		cDCProto::Create_HubName(omsg, mC.hub_name, mC.hub_topic);
		conn->Send(omsg);
	}

	if ((conn->mFeatures & eSF_FAILOVER) && !mC.hub_failover_hosts.empty()) { // send failover hosts if not empty and client supports it
		static string omsg;
		omsg.erase();
		omsg.append("$FailOver ");
		omsg.append(mC.hub_failover_hosts);
		conn->Send(omsg, true);
	}

	SendHeaders(conn, 1);
	AfterUserLogin(conn);
	conn->ClearTimeOut(eTO_LOGIN);
	conn->mpUser->mT.login.Get();
}

bool cServerDC::BeginUserLogin(cConnDC *conn)
{
	// If user asks for nicklist, then login will happen after the sending of nicklist ends
	// otherwise it will happen now
	unsigned int WantedMask;
	if (mC.delayed_login)
	 	WantedMask = eLS_LOGIN_DONE - eLS_NICKLST;
	else
		WantedMask = eLS_LOGIN_DONE;

	if(WantedMask == conn->GetLSFlag(WantedMask)) {
		if(conn->Log(2))
			conn->LogStream() << "Begin login" << endl;
		// Check if nick is unique
		if(VerifyUniqueNick(conn)) {
			if (!mC.delayed_login)  {
				DoUserLogin(conn);
			} else {
				mInProgresUsers.Add(conn->mpUser);
			}

			if (conn->mSendNickList) {
				// this may won't send all data at once...
				mP.NickList(conn);	// this will set mNickListInProgress
				conn->mSendNickList = false;
				return true;	// return here since we don't know that the list was sent or not
						// OnFlushDone() will do the login after the NickList is flushed
			}
			if(!conn->mpUser->mInList) {
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
	static string msg;
	msg.erase();
	mP.Create_Hello(msg, user->mNick); // send hello
	mHelloUsers.SendToAll(msg, mC.delayed_myinfo, true);

	msg.erase();
	msg = mP.GetMyInfo(user, eUC_NORMUSER); // all users get myinfo, even those in progress, hello users in progress are ignored, they are obsolete btw
	mUserList.SendToAll(msg, mC.delayed_myinfo, true); // use cache, so this can be after user is added
	mInProgresUsers.SendToAll(msg, mC.delayed_myinfo, true);

	if ((user->mClass >= eUC_OPERATOR) && !(user->mxConn && user->mxConn->mRegInfo && user->mxConn->mRegInfo->mHideKeys)) { // send short oplist
		msg.erase();
		mP.Create_OpList(msg, user->mNick);
		mUserList.SendToAll(msg, mC.delayed_myinfo, true);
		mInProgresUsers.SendToAll(msg, mC.delayed_myinfo, true);
	}

	if (mC.send_user_ip) { // send userip to operators
		string UserIP;
		cCompositeUserCollection::ufDoIpList DoUserIP(UserIP);
		DoUserIP.Clear();
		DoUserIP(user);
		mUserList.SendToAllWithClassFeature(UserIP, mC.user_ip_class, eUC_MASTER, eSF_USERIP2, mC.delayed_myinfo, true); // must be delayed too
	}

	/*
		send it to all but to him
		but why? maybe he would be doubled in some shit clients?
		anyway delayed_login will show why is it
		the order of flush of this before the full myinfo for ops
	*/

	if (!mC.delayed_login) {
		user->mInList = false;
		mUserList.FlushCache();
		mInProgresUsers.FlushCache();
		user->mInList = true;
	}

	if (mC.show_tags == 1) { // patch eventually for ops
		msg.erase();
		msg = mP.GetMyInfo(user, eUC_OPERATOR);
		mOpchatList.SendToAll(msg, mC.delayed_myinfo, true); // must send after mUserList, cached mUserList will be flushed after and will override this one
		mInProgresUsers.SendToAll(msg, mC.delayed_myinfo, true); // send later, better more people see tags, then some ops not, ops are dangerous creatures, they may have idea to kick people for not seeing their tags
	}

	return true;
}

void cServerDC::ConnCloseMsg(cConnDC *conn, const string &msg, int msec, int reason)
{
	DCPublicHS(msg,conn);
	conn->CloseNice(msec, reason);
}

int cServerDC::DCHello(const string &nick, cConnDC *conn, string *info)
{
	static string msg;
	msg.erase();
	mP.Create_Hello(msg, nick);
	conn->Send(msg, true);

	if (info)
		conn->Send(*info, true);

	return 0;
}

bool cServerDC::MinDelay(cTime &then, int min)
{
	// @todo use timeins instead of mindelay, or change to microsecond resolution
	cTime now;
	cTime diff=now-then;
	if(diff.Sec() >= min) {
		then = now;
		return true;
	}
	return false;
}

bool cServerDC::MinDelayMS(cTime& what, long unsigned int min)
{
	cTime now;
	cTime diff=now-what;
	if(diff.MiliSec() >= min) {
		what = now;
		return true;
	}
	return false;
}

bool cServerDC::AllowNewConn()
{
	return mConnList.size() <= (unsigned) mC.max_users_total + mC.max_extra_regs + mC.max_extra_vips + mC.max_extra_ops + mC.max_extra_cheefs + mC.max_extra_admins + 300;
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

int cServerDC::ValidateUser(cConnDC *conn, const string &nick, int &closeReason)
{
	closeReason = eCR_INVALID_USER; // default close reason

	if (!conn)
		return 0;

	bool close = false;
	static cRegUserInfo *sRegInfo = new cRegUserInfo;

	if ((nick.size() < (mC.max_nick * 2)) && mR->FindRegInfo(*sRegInfo, nick) && !conn->mRegInfo) {
		conn->mRegInfo = sRegInfo;
		sRegInfo = new cRegUserInfo;
	}

	tVAL_NICK vn = ValidateNick(conn, nick); // validate nick
	stringstream errmsg;

	if (vn != eVN_OK) {
		string extra;

		if (vn == eVN_BANNED) {
			cBanList::sTempBan *tban = mBanList->mTempNickBanlist.GetByHash(mBanList->mTempNickBanlist.HashLowerString(nick));

			if (tban && (tban->mUntil > mTime.Sec())) {
				errmsg << autosprintf(_("You're still temporarily banned for %s because: %s"), cTime(tban->mUntil - mTime.Sec()).AsPeriod().AsString().c_str(), tban->mReason.c_str());

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
						errmsg << " " << autosprintf(_("Valid nick characters: %s"), mC.nick_chars.c_str());

					break;
				case eVN_SHORT:
					errmsg << autosprintf(_("Your nick is too short, minimum allowed length is %d characters."), mC.min_nick);
					break;
				case eVN_LONG:
					errmsg << autosprintf(_("Your nick is too long, maximum allowed length is %d characters."), mC.max_nick);
					break;
				case eVN_USED: // never appears here
					errmsg << _("Your nick is already taken by another user.");
					extra = "$ValidateDenide ";
					extra += nick;
					break;
				case eVN_PREFIX:
					errmsg << autosprintf(_("Please use one of following nick prefixes: %s"), mC.nick_prefix.c_str());
					break;
				case eVN_NOT_REGED_OP:
					errmsg << _("Your nick contains operator prefix but you are not registered, please remove it.");
					break;
				default:
					errmsg << _("Unknown bad nick error, sorry.");
					break;
			}

			closeReason = eCR_BADNICK;
		}

		if (vn != eVN_OK) {
			if (extra.size())
				conn->Send(extra, true);

			DCPublicHS(errmsg.str(), conn);

			if (conn->Log(2))
				conn->LogStream() << errmsg.str() << endl;

			return 0;
		}
	}

	cBan Ban(this);
	bool banned = false;

	if (conn->GetTheoricalClass() < mC.ban_bypass_class) { // use ban_bypass_class here
		// here we cant check share ban because user hasnt sent $MyINFO yet
		if (conn->GetTheoricalClass() == eUC_NORMUSER)
			banned = mBanList->TestBan(Ban, conn, nick, eBF_NICK | eBF_NICKIP | eBF_RANGE | eBF_HOST2 | eBF_HOST1 | eBF_HOST3 | eBF_HOSTR1 | eBF_PREFIX);
		else // registered users avoid prefix ban check because we might actually ban a prefix for unregistered users, but let registered users to use it
			banned = mBanList->TestBan(Ban, conn, nick, eBF_NICK | eBF_NICKIP | eBF_RANGE | eBF_HOST2 | eBF_HOST1 | eBF_HOST3 | eBF_HOSTR1);
	}

	if (banned) {
		errmsg << _("You are banned from this hub") << ":\r\n";
		Ban.DisplayUser(errmsg);
		DCPublicHS(errmsg.str(), conn);

		if (conn->Log(1))
			conn->LogStream() << "Closing banned user: " << Ban.mType << endl;

		return 0;
	}

	if (mC.nick_prefix_cc && conn->mCC.size() && (conn->mCC != "--")) {
		string Prefix("[");
		Prefix += conn->mCC;
		Prefix += "]";

		if (StrCompare(nick, 0, 4, Prefix) != 0) {
			errmsg << autosprintf(_("Please use following nick prefix: %s"), Prefix.c_str());
			close = conn->GetTheoricalClass() < eUC_REGUSER;
		}
	}

	if (close) {
		DCPublicHS(errmsg.str(), conn);
		return 0;
	}

	return 1;
}

tVAL_NICK cServerDC::ValidateNick(cConnDC *conn, const string &nick)
{
	cTime now;
	string ProhibitedChars("$|<> ");
	//ProhibitedChars.append("\0", 1);

	if (nick.npos != nick.find_first_of(ProhibitedChars)) // check all for special nick chars
		return eVN_CHARS;

	if (!conn->mRegInfo || !conn->mRegInfo->mEnabled) {
		if (nick.size() > mC.max_nick)
			return eVN_LONG;

		if (nick.size() < mC.min_nick)
			return eVN_SHORT;

		if ((mC.nick_chars.size() > 0) && (nick.npos != nick.find_first_not_of(mC.nick_chars.c_str())))
			return eVN_CHARS;

		if (mC.nick_prefix.size() > 0) { // check nick prefix
			bool ok = false;
			istringstream is(mC.nick_prefix);
			string pref, snick;

			if (mC.nick_prefix_nocase)
				snick = toLower(nick);
			else
				snick = nick;

			while (1) {
				pref = mEmpty;
				is >> pref;

				if (!pref.size())
					break;

				if (mC.nick_prefix_nocase)
					pref = toLower(pref);

				if (StrCompare(snick, 0, pref.length(), pref) == 0) {
					ok = true;
					break;
				}
			}

			if (!ok)
				return eVN_PREFIX;
		}

		if (StrCompare(nick, 0, 4, "[OP]") == 0)
			return eVN_NOT_REGED_OP;
	}

	if (mBanList->IsNickTempBanned(nick)) // check temporary nick ban
		return eVN_BANNED;

	return eVN_OK;
}

int cServerDC::OnTimer(cTime &now)
{
	mHelloUsers.FlushCache();
	mUserList.FlushCache();
	mOpList.FlushCache();
	mOpchatList.FlushCache();
	mActiveUsers.FlushCache();
	mPassiveUsers.FlushCache();
	mChatUsers.FlushCache();
	mInProgresUsers.FlushCache();
	mRobotList.FlushCache();

	mSysLoad = eSL_NORMAL;
	if(mFrequency.mNumFill > 0) {
		double freq = mFrequency.GetMean(mTime);

		if(freq < 1.2 * mC.min_frequency) mSysLoad = eSL_PROGRESSIVE;
		if(freq < 1.0 * mC.min_frequency) mSysLoad = eSL_CAPACITY;
		if(freq < 0.8 * mC.min_frequency) mSysLoad = eSL_RECOVERY;
		if(freq < 0.5 * mC.min_frequency) mSysLoad = eSL_SYSTEM_DOWN;
	}

	if(mC.max_upload_kbps > 0.00001) {
		int zone;
		double total_upload=0.;
		for(zone = 0; zone <= USER_ZONES; zone++)
			total_upload += this->mUploadZone[zone].GetMean(this->mTime);
		if ((total_upload / 1024.0) > mC.max_upload_kbps) {
			mSysLoad = eSL_PROGRESSIVE;
		}
	}

	// perform all temp functions
	for (tTFIt i = mTmpFunc.begin(); i != mTmpFunc.end(); ++i) {
		if (*i) {
			// delete finished functions
			if((*i)->done()) {
				delete *i;
				(*i) = NULL;
			} else {
				// step the rest
				(*i)->step();
			}
		}
	}

	if (bool(mSlowTimer.mMinDelay) && mSlowTimer.Check(mTime, 1) == 0)
		mBanList->RemoveOldShortTempBans(mTime.Sec());

	if (bool(mHublistTimer.mMinDelay) && mHublistTimer.Check(mTime, 1) == 0)
		this->RegisterInHublist(mC.hublist_host, mC.hublist_port, NULL);

	if (bool(mReloadcfgTimer.mMinDelay) && mReloadcfgTimer.Check(mTime, 1) == 0) {
		mC.Load();
		//mCo->mTriggers->ReloadAll();

		if (mC.use_reglist_cache)
			mR->UpdateCache();

		if (Log(2))
			LogStream() << "Socket counter: " << cAsyncConn::sSocketCounter << endl;
	}

	if (mC.detect_ctmtohub && ((this->mTime.Sec() - mCtmToHubConf.mTime.Sec()) >= 60)) { // ctm2hub
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

					this->mOpChat->SendPMToAll(os.str(), NULL);
				}

				mCtmToHubConf.mNew = 0;
				mCtmToHubConf.mLast = this->mTime;
			} else if ((this->mTime.Sec() - mCtmToHubConf.mLast.Sec()) >= 180) {
				if (mCtmToHubConf.mStart) {
					ostringstream os;
					os << autosprintf(_("DDoS stopped, filtered totally %lu connections."), total);
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
	if (!mCallBacks.mOnTimer.CallAll(now.MiliSec())) return false;
	#endif
	return true;
}

unsigned cServerDC::Str2Period(const string &s, ostream &err)
{
	istringstream is(s);
	unsigned u = 0;
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
	} else
		err << _("Please provide positive number for time unit.");

	return u;
}

int cServerDC::DoRegisterInHublist(string host, int port, string reply)
{
	__int64 min_share = mC.min_share; // prepare

	if (mC.min_share_use_hub > min_share)
		min_share = mC.min_share_use_hub;

	char pipe = '|';
	ostringstream to_serv, to_user;
	istringstream is(host);
	string curhost, lock, key;
	size_t pos_space;
	cAsyncConn *pHubList;

	if (reply.size())
		to_user << _("Hublist registration results") << ":\r\n\r\n";

	while (curhost = "", is >> curhost, curhost.size() > 0) {
		if (reply.size())
			to_user << autosprintf(_("Sending information to: %s:%d"), curhost.c_str(), port) << " .. ";

		pHubList = new cAsyncConn(curhost, port); // connect

		if (!pHubList || !pHubList->ok) {
			if (reply.size())
				to_user << _("Error connecting") << "\r\n";

			if (pHubList) {
				pHubList->Close();
				delete pHubList;
				pHubList = NULL;
			}

			continue;
		}

		to_serv.str("");
		to_serv.clear();
		key = "";
		pHubList->SetLineToRead(&lock, pipe, 1024);
		pHubList->ReadAll();
		pHubList->ReadLineLocal();

		if (lock.size() > 6) {
			pos_space = lock.find(' ', 6);

			if (pos_space != lock.npos)
				pos_space -= 6;

			lock = lock.substr(6, pos_space);
			cDCProto::Lock2Key(lock, key);
		}

		to_serv << "$Key " << key << pipe; // create registration data
		to_serv << mC.hub_name << pipe;
		to_serv << mC.hub_host << pipe;

		if (mC.hublist_send_minshare)
			to_serv << "[MINSHARE:" << StringFrom(min_share) << "MB] ";

		to_serv << mC.hub_desc << pipe;
		to_serv << mUserList.Size() << pipe;
		to_serv << mTotalShare << pipe;

		pHubList->Write(to_serv.str(), true); // send it

		if (reply.size()) {
			if (pHubList->ok)
				to_user << _("Done");
			else
				to_user << _("Error sending");

			to_user << "\r\n";
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

int cServerDC::RegisterInHublist(string host, int port, cConnDC *conn)
{
	if (host.size() < 3) {
		if (conn)
			DCPublicHS(_("Hublist host is empty, nothing to do."), conn);

		return 0;
	}

	string reply;

	if (conn) {
		DCPublicHS(_("Registering in hublists, please wait..."), conn);

		if (conn->mpUser)
			reply = conn->mpUser->mNick;
	}

	cThreadWork *work = new tThreadWork3T<cServerDC, string, int, string>(host, port, reply, this, &cServerDC::DoRegisterInHublist);

	if (mHublistReg.AddWork(work))
		return 1;
	else {
		delete work;
		work = NULL;
		return 0;
	}
}

int cServerDC::WhoCC(string CC, string &dest, const string&separator)
{
	cUserCollection::iterator i;
	int cnt=0;
	cConnDC *conn;
	for(i=mUserList.begin(); i != mUserList.end(); ++i) {
		conn = ((cUser*)(*i))->mxConn;
		if(conn && conn->mCC == CC) {
			dest += separator;
			dest += (*i)->mNick;
			cnt++;
		}
	}
	return cnt;
}

int cServerDC::WhoCity(string city, string &dest, const string &separator)
{
	cUserCollection::iterator i;
	int cnt = 0;
	cConnDC *conn;

	for (i = mUserList.begin(); i != mUserList.end(); ++i) {
		conn = ((cUser*)(*i))->mxConn;

		if (conn && conn->mCity == city) {
			dest += separator;
			dest += (*i)->mNick;
			cnt++;
		}
	}

	return cnt;
}

int cServerDC::WhoIP(unsigned long ip_min, unsigned long ip_max, string &dest, const string&separator, bool exact)
{
	cUserCollection::iterator i;
	int cnt=0;
	cConnDC *conn;
	for(i=mUserList.begin(); i!= mUserList.end(); ++i) {
		conn = ((cUser*)(*i))->mxConn;
		if(conn) {
			unsigned long num = cBanList::Ip2Num(conn->AddrIP());
			if(exact && (ip_min == num)) {
				dest += separator;
				dest += (*i)->mNick;
				cnt++;
			} else if ((ip_min <= num) && (ip_max >= num)) {
				dest += separator;
				dest += (*i)->mNick;
				dest += " (";
				dest += conn->AddrIP();
				dest += ")";
				cnt++;
			}
		}
	}
	return cnt;
}

int cServerDC::CntConnIP(string ip)
{
	cUserCollection::iterator i;
	int cnt = 0;
	cConnDC *conn;

	for (i = mUserList.begin(); i != mUserList.end(); ++i) {
		conn = ((cUser*)(*i))->mxConn;

		if (conn) {
			if ((conn->GetTheoricalClass() <= eUC_REGUSER) && (conn->AddrIP() == ip))
				cnt++;
		}
	}

	return cnt;
}

bool cServerDC::CheckUserClone(cConnDC *conn, string &clone)
{
	if ((mC.max_class_check_clone < 0) || !conn || !conn->mpUser || !conn->mpUser->mShare || (conn->mpUser->mClass > mC.max_class_check_clone))
		return false;

	cUserCollection::iterator i;
	cConnDC *other;

	for (i = mUserList.begin(); i != mUserList.end(); ++i) { // skip self
		other = ((cUser*)(*i))->mxConn;

		if (other && other->mpUser && other->mpUser->mInList && other->mpUser->mShare && (other->mpUser->mNick != conn->mpUser->mNick) && (other->mpUser->mClass <= mC.max_class_check_clone) && (other->mpUser->mShare == conn->mpUser->mShare) && (other->AddrIP() == conn->AddrIP())) {
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

			clone = other->mpUser->mNick;
			return true;
		}
	}

	return false;
}

bool cServerDC::CheckProtoFloodAll(cConnDC *conn, cMessageDC *msg, int type)
{
	if (!conn || !conn->mpUser || (conn->mpUser->mClass > mC.max_class_proto_flood))
		return false;

	unsigned long period = 0;
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
	}

	if (!limit || !period)
		return false;

	if (!mProtoFloodAllCounts[type]) {
		mProtoFloodAllCounts[type] = 1;
		mProtoFloodAllTimes[type] = mTime;
		mProtoFloodAllLocks[type] = false;
		return false;
	}

	long dif = mTime.Sec() - mProtoFloodAllTimes[type].Sec();

	if ((dif < 0) || (dif > period)) {
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
			}

			if (conn->Log(1))
				conn->LogStream() << omsg << endl;

			if (mC.proto_flood_report)
				ReportUserToOpchat(conn, omsg);

			mProtoFloodAllLocks[type] = false;
		}

		return false;
	}

	if (mProtoFloodAllCounts[type]++ >= limit) {
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
			}

			ostringstream info;
			info << omsg << " [" << mProtoFloodAllCounts[type] << ':' << dif << ':' << period << ']';

			if (conn->Log(1))
				conn->LogStream() << info.str() << endl;

			if (mC.proto_flood_report)
				ReportUserToOpchat(conn, info.str());

			mProtoFloodAllLocks[type] = true;
		}

		omsg = _("Sorry, following protocol command is temporarily locked due to flood detection"); // notify user every time
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

		if (type == ePFA_PRIV) { // pm
			string to_nick;
			cUser *to_user = NULL;

			if (msg && msg->mStr.size()) {
				to_nick = msg->ChunkString(eCH_PM_TO);

				if (to_nick.size())
					to_user = mUserList.GetUserByNick(to_nick);
			}

			if (to_user)
				DCPrivateHS(omsg, conn, &to_nick);
			else
				DCPrivateHS(omsg, conn);
		} else // mc
			DCPublicHS(omsg, conn);

		return true;
	}

	return false;
}

void cServerDC::ReportUserToOpchat(cConnDC *conn, const string &Msg, bool ToMain)
{
	if (!conn)
		return;

	ostringstream os;
	os << Msg;

	if (conn->mpUser) // nick
		os << " ][ " << autosprintf(_("Nick: %s"), conn->mpUser->mNick.c_str());

	os << " ][ " << autosprintf(_("IP: %s"), conn->AddrIP().c_str()); // ip

	if (mC.report_user_country && conn->mCC.size() && (conn->mCC != "--")) // country code
		os << "." << conn->mCC;

	if (!mUseDNS && mC.report_dns_lookup)
		conn->DNSLookup();

	if (conn->AddrHost().size()) // host
		os << " ][ " << autosprintf(_("Host: %s"), conn->AddrHost().c_str());

	if (!ToMain && this->mOpChat)
		this->mOpChat->SendPMToAll(os.str(), NULL);
	else {
		static string ChatMsg;
		ChatMsg.erase();
		cDCProto::Create_Chat(ChatMsg, mC.opchat_name, os.str());
		this->mOpchatList.SendToAll(ChatMsg, false, true);
	}
}

void cServerDC::SendHeaders(cConnDC * conn, int where)
{
	/*
	* 0 = dont send headers
	* 1 = send headers on login
	* 2 = send headers on connection
	*/

	if (((mC.host_header > 2) ? 2 : mC.host_header) == where) {
		ostringstream os;
		cTime runtime;
		runtime -= mStartTime;

		if (mC.extended_welcome_message) {
			mStatus = _("Not available");

			if (mFrequency.mNumFill > 0) {
				if (mSysLoad == eSL_RECOVERY) mStatus = _("Recovery mode");
				else if (mSysLoad == eSL_CAPACITY) mStatus = _("Near capacity");
				else if (mSysLoad == eSL_PROGRESSIVE) mStatus = _("Progressive mode");
				else if (mSysLoad == eSL_NORMAL) mStatus = _("Normal mode");
			}

			os << "<" << mC.hub_security << "> " << autosprintf(_("Software: %s %s @ %s"), HUB_VERSION_NAME, VERSION, HUB_VERSION_CLASS) << "|";
			os << "<" << mC.hub_security << "> " << autosprintf(_("Uptime: %s"), runtime.AsPeriod().AsString().c_str()) << "|";
			os << "<" << mC.hub_security << "> " << autosprintf(_("Users: %d"), mUserCountTot) << "|";
			os << "<" << mC.hub_security << "> " << autosprintf(_("Share: %s"), convertByte(mTotalShare, false).c_str()) << "|";
			os << "<" << mC.hub_security << "> " << autosprintf(_("Status: %s"), mStatus.c_str()) << "|";
			if (!mC.hub_version_special.empty()) os << "<" << mC.hub_security << "> " << mC.hub_version_special << "|";
		} else
			os << "<" << mC.hub_security << "> " << autosprintf(_("Software: %s %s @ %s%s ][ Uptime: %s ][ Users: %d ][ Share: %s"), HUB_VERSION_NAME, VERSION, HUB_VERSION_CLASS, mC.hub_version_special.c_str(), runtime.AsPeriod().AsString().c_str(), mUserCountTot, convertByte(mTotalShare, false).c_str()) << "|";

		string res = os.str();
		conn->Send(res, false);
	}
}

void cServerDC::DCKickNick(ostream *use_os,cUser *OP, const string &Nick, const string &Reason, int flags)
{
	ostringstream ostr;
	cUser *user = mUserList.GetUserByNick(Nick);
	string NewReason(Reason);
	cKick OldKick;

	// Check if it is possible to kick the user
	if(user && user->mxConn &&
	    (user->mClass + (int) mC.classdif_kick <= OP->mClass) &&
	    (OP->Can(eUR_KICK, mTime.Sec())) &&
		//mClass >= eUC_OPERATOR) &&
	    (OP->mNick != Nick))
	{
		if (user->mProtectFrom < OP->mClass) {
			if(flags & eKCK_Reason) {
				user->mToBan = false;
				// auto kickban
				if(mP.mKickBanPattern.Exec(Reason) >= 0) {
					unsigned u = 0;
					string bantime;
					mP.mKickBanPattern.Extract(1,Reason,bantime);
					if(bantime.size()) {
						ostringstream os;
						if(!(u=Str2Period(bantime,os))) DCPublicHS(os.str(),OP->mxConn);
					}
					if (u > mC.tban_max)
						u = mC.tban_max;
					if (
						(!u && OP->Can(eUR_PBAN, this->mTime)) ||
						((u > 0) &&
						(((u > mC.tban_kick) && OP->Can(eUR_TBAN, this->mTime)) ||
						(u <= mC.tban_kick)
						)
						)
					  )
						user->mToBan = true;

					user->mBanTime = u;
					if ( mC.msg_replace_ban.size())
						mP.mKickBanPattern.Replace(0, NewReason, mC.msg_replace_ban);
				}
				mKickList->AddKick(user->mxConn, OP->mNick, &Reason, OldKick);

				if(Reason.size()) {
					string omsg;
					ostr << OP->mNick << " is kicking " << Nick << " because: " << NewReason;
					omsg = ostr.str();
					if(!mC.hide_all_kicks && !OP->mHideKick)
						SendToAll(omsg, OP->mHideKicksForClass ,int(eUC_MASTER));

					if(flags & eKCK_PM) {
						ostr.str(mEmpty);
						ostr << autosprintf(_("You have been kicked because %s"), NewReason.c_str());
						DCPrivateHS(ostr.str(), user->mxConn, &OP->mNick, &OP->mNick);
					}
				}
			}

			if (flags & eKCK_Drop) {
				ostr.str(mEmpty); // send the message to the kicker

				if (flags & eKCK_TBAN)
					ostr << autosprintf(_("Kicked user %s"), Nick.c_str());
				else
					ostr << autosprintf(_("Dropped user %s"), Nick.c_str());

				ostr << " " << autosprintf(_("with IP %s"), user->mxConn->AddrIP().c_str());

				if (user->mxConn->AddrHost().length())
					ostr << " " << autosprintf(_("and host %s"), user->mxConn->AddrHost().c_str());

				ostr << ".";

				if (user->mxConn->Log(2))
					user->mxConn->LogStream() << "Kicked by " << OP->mNick << ", ban " << mC.tban_kick << "s" << endl;

				if (OP->Log(3))
					OP->LogStream() << "Kicking " << Nick << endl;

				bool Disconnect = true;
				mKickList->AddKick(user->mxConn, OP->mNick, NULL, OldKick);

				if (OldKick.mReason.size()) {
					#ifndef WITHOUT_PLUGINS
						Disconnect = mCallBacks.mOnOperatorKicks.CallAll(OP, user, &OldKick.mReason);
					#endif
				} else {
					#ifndef WITHOUT_PLUGINS
						Disconnect = mCallBacks.mOnOperatorDrops.CallAll(OP, user);
					#endif
				}

				if (Disconnect) {
					user->mxConn->CloseNice(1000, eCR_KICKED);

					if (!(flags & eKCK_TBAN)) {
						string msg(OP->mNick);
						msg += " ";
						msg += _("dropped");
						ReportUserToOpchat(user->mxConn,msg, mC.dest_drop_chat);
					}
				} else
					ostr << " " << _("Disconnect prevented by a plugin.");

				if (flags & eKCK_TBAN) { // temporarily ban kicked user
					cBan Ban(this);
					cKick Kick;
					mKickList->FindKick(Kick, user->mNick, OP->mNick, 30, true, true);
					mBanList->NewBan(Ban, Kick, (user->mToBan ? user->mBanTime : mC.tban_kick), eBF_NICKIP);
					ostr << " ";

					if (Ban.mDateEnd) {
						cTime HowLong(Ban.mDateEnd - cTime().Sec(), 0);
						ostr << autosprintf(_("User banned for %s."), HowLong.AsPeriod().AsString().c_str());
					} else
						ostr << _("User banned permanently.");

					mBanList->AddBan(Ban);
				}

				if (!use_os)
					DCPublicHS(ostr.str(), OP->mxConn);
				else
					(*use_os) << ostr.str();
			}
		} else if(flags & eKCK_Drop) {
			ostr.str(mEmpty);
			ostr << autosprintf(_("Error kicking user %s because he is protected against all classes below %d"), Nick.c_str(), user->mProtectFrom);
			DCPublicHS(ostr.str(),OP->mxConn);
		}
	}
}

/*
	this functions collects stack backtrace of the caller functions and demangles it
	then it tries to send backtrace to crash server via http
	and writes backtrace to standard error output
*/

void cServerDC::DoStackTrace()
{
	void* addrlist[64];
	int addrlen = backtrace(addrlist, sizeof(addrlist) / sizeof(void*));

	if (!addrlen) {
		cerr << "Stack backtrace is empty, possibly corrupt" << endl;
		return;
	}

	char** symbollist = backtrace_symbols(addrlist, addrlen);
	size_t funcnamesize = 256;
	char* funcname = (char*)malloc(funcnamesize);
	ostringstream bt;

	for (int i = 1; i < addrlen; i++) {
		char *begin_name = 0, *begin_offset = 0, *end_offset = 0;

		for (char *p = symbollist[i]; *p; ++p) {
			if (*p == '(')
				begin_name = p;
			else if (*p == '+')
				begin_offset = p;
			else if ((*p == ')') && begin_offset) {
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
			} else
				bt << symbollist[i] << ": " << begin_name << "() +" << begin_offset << endl;
		} else
			bt << symbollist[i] << endl;
	}

	free(funcname);
	free(symbollist);
	cerr << "Stack backtrace:" << endl << endl << bt.str() << endl;
	cAsyncConn *http = new cAsyncConn("www.te-home.net", 80); // try to send via http

	if (!http || !http->ok) {
		cerr << "Failed connecting to crash server, please send above stack backtrace to developers" << endl;

		if (http) {
			http->Close();
			delete http;
			http = NULL;
		}

		return;
	}

	ostringstream hub_info, http_req;
	cTime uptime;
	uptime -= mStartTime;
	hub_info << "Address: " << mAddr << ':' << mPort << endl;
	hub_info << "Uptime: " << uptime.AsPeriod().AsString() << endl;
	hub_info << "Users: " << mUserCountTot << endl;
	hub_info << "Stack backtrace:" << endl << endl;
	http_req << "POST /vhcs.php HTTP/1.1\n";
	http_req << "Host: www.te-home.net\n";
	http_req << "User-Agent: " << HUB_VERSION_NAME << '/' << VERSION << '/' << HUB_VERSION_CLASS << "\n";
	http_req << "Content-Type: text/plain\n";
	http_req << "Content-Length: " << (hub_info.str().size() + bt.str().size()) << "\n\n"; // end of headers
	http_req << hub_info.str();
	http_req << bt.str();
	http->Write(http_req.str(), true);

	if (http->ok)
		cerr << "Successfully sent stack backtrace to crash server" << endl;
	else
		cerr << "Failed sending to crash server, please send above stack backtrace to developers" << endl;

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
	//item->mCC = conn->mCC;
	//item->mPort = conn->AddrPort();
	//item->mServ = conn->GetServPort();
	item->mRef = ref;
	item->mUniq = uniq;
	mCtmToHubList.push_back(item);

	if ((++mCtmToHubConf.mNew > 9) && !mCtmToHubConf.mStart) {
		string omsg = _("DDoS detected, gathering attack information...");
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
			list.append(" ");
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

	}; // namespace nServer
}; // namespace nVerliHub
