/*
	Copyright (C) 2003-2005 Daniel Muller, dan at verliba dot cz
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
#include "curr_date_time.h"
#include "cthreadwork.h"
#include "stringutils.h"
#include "cconntypes.h"
#include "cdcconsole.h"
#include "ctriggers.h"
#include "i18n.h"

#define HUB_VERSION_CLASS __CURR_DATE_TIME__
#define LOCK_VERSION PACKAGE
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
	mDBConf(CfgBase +"/dbconfig"), // load the db config
	mMySQL(
		mDBConf.db_host,
		mDBConf.db_user,
		mDBConf.db_pass,
		mDBConf.db_data), // connect to mysql
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
	mNetOutLog.open(net_log.c_str(),ios::out);
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
}

cServerDC::~cServerDC()
{
	if(Log(1)) LogStream() << "Destructor cServerDC" << endl;
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
			cDCProto::Create_Chat(msg, from, txt);
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
	cDCProto::Create_Chat(msg, from, txt);
	//TODO: Use constant instead of num
	if(min_class !=0 && max_class != 10)
		mUserList.SendToAllWithClass(msg, min_class, max_class, true, true);
	else
		mUserList.SendToAll(msg, true, true);
	return 1;
}

int cServerDC::DCPublicHS(const string &text, cConnDC *conn)
{
	return DCPublic( mC.hub_security, text,  conn );
}

void cServerDC::DCPublicHSToAll(const string &text)
{
	static string msg;
	msg.erase();
	cDCProto::Create_Chat(msg, mC.hub_security , text);
	mUserList.SendToAll(msg, true, true);
}

int cServerDC::DCPrivateHS(const string & text, cConnDC * conn, string *from)
{
	string msg;
	mP.Create_PM(msg, mC.hub_security, conn->mpUser->mNick, (from!=NULL)?(*from):(mC.hub_security), text);
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

	if ((usr->mClass >= eUC_OPERATOR) || mC.chat_default_on)
		mChatUsers.AddWithHash(usr, Hash);
	else
		DCPublicHS(_("You won't see public chat messages, to restore use +chat command."), usr->mxConn);

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

void cServerDC::OnScriptCommand(string cmd, string data, string plug, string script)
{
	mCallBacks.mOnScriptCommand.CallAll(cmd, data, plug, script);
}

void cServerDC::SendToAll(string &data, int cm,int cM)
{
	cConnDC *conn;
	tCLIt i;
	int size = data.size();

	// prepare data
	if(size >= MAX_SEND_SIZE-1) {
		if(Log(2))
			LogStream() << "Truncating too long message from: "
				<< data.size() << " to " << MAX_SEND_SIZE -1 << " Message starts with: " << data.substr(0,10) << endl;
		data.resize( MAX_SEND_SIZE -1,' ');
		size = MAX_SEND_SIZE -1;
	}
	if(data[data.size()-1] !='|') {
		data.append("|");
		size ++;
	}

	int count = 0;
	for(i=mConnList.begin(); i!= mConnList.end(); i++) {
		conn=(cConnDC *)(*i);
		if(conn && conn->ok && conn->mWritable && conn->mpUser && conn->mpUser->mInList) {
			conn->Write(data, true);
			mUploadZone[conn->mGeoZone].Insert(mTime,data.size());
			++count;
		}
	}

	if(Log(5))
		LogStream() << "ALL << " << data.substr(0,100) << endl;
	if(msLogLevel >= 3)
		mNetOutLog << ((unsigned long)count) *data.size() << " "
			<< data.size() << " "
			<< count << " " << data.substr(0,10) << endl;
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

int cServerDC::OnNewConn(cAsyncConn *nc)
{
	cConnDC *conn = (cConnDC *)nc;
	if (!conn) return -1;

	#ifndef WITHOUT_PLUGINS
	if (!mCallBacks.mOnNewConn.CallAll(conn)) return -1;
	#endif

	string omsg;
	omsg = "$Lock EXTENDEDPROTOCOL_" LOCK_VERSION " Pk=v" VERSION;
	conn->Send(omsg, true);
	SendHeaders(conn, 2);

	if (mSysLoad >= eSL_RECOVERY) {
		stringstream os;
		os << _("The hub is currently unable to service your request. Please try again in a few minutes.");
		DCPublicHS(os.str(), conn);
		conn->CloseNice(500, eCR_HUB_LOAD);
		return -1;
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

		if (!sameuser && olduser->mxConn && (conn->AddrIP() == olduser->mxConn->AddrIP()) && (conn->mpUser->mShare == olduser->mShare) && (conn->mpUser->mMyINFO_basic == olduser->mMyINFO_basic))
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

	// anti login flood temp bans
	if (conn->GetTheoricalClass() <= mC.max_class_int_login) {
		mBanList->AddNickTempBan(conn->mpUser->mNick, mTime.Sec() + mC.int_login, "login later");
		mBanList->AddIPTempBan(conn->GetSockAddress(), mTime.Sec() + mC.int_login, "login later");
	}

	// users special rights and restrictions
	cPenaltyList::sPenalty pen;
	if (mPenList->LoadTo(pen, conn->mpUser->mNick) && (conn->mpUser->mClass != eUC_PINGER))
		conn->mpUser->ApplyRights(pen);

	// insert user to userlist
	if(!AddToList(conn->mpUser)) {
		conn->CloseNow();
		return;
	}

	// display user to others
	ShowUserToAll(conn->mpUser);

	if(mC.send_user_ip) {
		if(conn->mpUser->mClass >= eUC_OPERATOR) {
 			conn->Send(mUserList.GetIPList(),true);
		} else {
			string UserIP;
			cCompositeUserCollection::ufDoIpList DoUserIP(UserIP);
			DoUserIP.Clear();
			DoUserIP(conn->mpUser);
			mOpchatList.SendToAll(UserIP, true, true);
			conn->Send(UserIP);
		}
	}

	if (!mC.hub_topic.empty()) { // send hub name with topic
		static string omsg;
		cDCProto::Create_HubName(omsg, mC.hub_name, mC.hub_topic);

		#ifndef WITHOUT_PLUGINS
		if (mCallBacks.mOnHubName.CallAll(conn->mpUser->mNick, omsg))
		#endif
		{
			conn->Send(omsg);
		}
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

#define UseChache true
#define OpList mOpList.GetNickList()
bool cServerDC::ShowUserToAll(cUserBase *user)
{
	string omsg;

	// only Hello users get hello message
	omsg = "$Hello ";
	omsg+= user->mNick;
	mHelloUsers.SendToAll(omsg, mC.delayed_myinfo);

	// all users get myinfo, event hose in progress
	// (hello users in progres are ignored, they are obsolete btw)
	omsg = mP.GetMyInfo(user, eUC_NORMUSER);
	mUserList.SendToAll(omsg, mC.delayed_myinfo); // use cache -> so this can be after user is added
	mInProgresUsers.SendToAll(omsg, mC.delayed_myinfo);

	// distribute oplist
	if(user->mClass >= eUC_OPERATOR) {
		mUserList.SendToAll(OpList, UseChache);
		mInProgresUsers.SendToAll(OpList, UseChache);
	}

	// send it to all but to him
	// but why ? maybe he would be doubled in some shi** clients ?
	// anyway delayed_login will show why is it..
	// the order of flush of this before the full myinfo for ops
	if(!mC.delayed_login)	{
		user->mInList = false;
		mUserList.FlushCache();
		mInProgresUsers.FlushCache();
		user->mInList = true;
	}

	/// patch eventually for ops
	if(mC.show_tags == 1)	{
		omsg = mP.GetMyInfo(user, eUC_OPERATOR);
		mOpchatList.SendToAll(omsg, mC.delayed_myinfo); // must send after mUserList! Cached mUserList will be flushed after and will override this one!
		mInProgresUsers.SendToAll(omsg, mC.delayed_myinfo); // send later, better more people see tags, then some ops not,
		// ops are dangerous creatures, they may have idea to kick people for not seeing their tags
	}
	return true;
}

void cServerDC::ConnCloseMsg(cConnDC *conn, const string &msg, int msec, int reason)
{
	DCPublicHS(msg,conn);
	conn->CloseNice(msec, reason);
}

int cServerDC::DCHello(const string & nick, cConnDC * conn, string *info)
{
	string str("$Hello ");
	str+= nick + "|";
	conn->Send(str, false);
	if(info)
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
	if (!conn)
		return 0;

	stringstream errmsg;
	closeReason = eCR_INVALID_USER; // default close reason
	bool close = false; // time_t n;

	// first validate ip and host
	// phase 1: test nick validity
	// phase 2: test ip and host ban (registered users pass)
	// phase 3: test nickban
	// then we're done

	static cRegUserInfo *sRegInfo = new cRegUserInfo;

	if ((nick.size() < mC.max_nick * 2) && mR->FindRegInfo(*sRegInfo, nick) && !conn->mRegInfo) {
		conn->mRegInfo = sRegInfo;
		sRegInfo = new cRegUserInfo;
	}

	// validate nick
	tVAL_NICK vn = ValidateNick(nick, (conn->GetTheoricalClass() >= eUC_REGUSER));

	if (vn != eVN_OK) {
		string extra;

		switch (vn) {
			case eVN_CHARS:
				errmsg << _("Your nick contains forbidden characters.");

				if (mC.nick_chars.size())
					errmsg << " " << autosprintf(_("Valid nick characters: %s"), mC.nick_chars.c_str());

				closeReason = eCR_BADNICK;
				break;

			case eVN_SHORT:
				errmsg << autosprintf(_("Your nick is too short, minimum allowed length is %d characters."), mC.min_nick);
				closeReason = eCR_BADNICK;
				break;

			case eVN_LONG:
				errmsg << autosprintf(_("Your nick is too long, maximum allowed length is %d characters."), mC.max_nick);
				closeReason = eCR_BADNICK;
				break;

			case eVN_USED: // never happens here
				errmsg << _("Your nick is already in use, please use something else.");
				closeReason = eCR_BADNICK;
				extra = "$ValidateDenide";
				break;

			case eVN_PREFIX:
				errmsg << autosprintf(_("Please use one of following nick prefixes: %s"), mC.nick_prefix.c_str());
				closeReason = eCR_BADNICK;
				break;

			case eVN_NOT_REGED_OP:
				errmsg << _("Your nick contains operator prefix but you are not registered, please remove it.");
				closeReason = eCR_BADNICK;
				break;

			case eVN_BANNED:
				errmsg << autosprintf(_("You are reconnecting too fast, please wait %s."), cTime(mBanList->IsNickTempBanned(nick) - cTime().Sec()).AsPeriod().AsString().c_str());
				closeReason = eCR_RECONNECT;
				break;

			default:
				errmsg << _("Unknown bad nick error, sorry.");
				//closeReason = eCR_BADNICK;
				break;
		}

		if (extra.size())
			conn->Send(extra);

		DCPublicHS(errmsg.str(), conn);

		if (conn->Log(2))
			conn->LogStream() << "Bad nick: " << nick << " (" << errmsg.str() << ")" << endl;

		return 0;
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

tVAL_NICK cServerDC::ValidateNick(const string &nick, bool registered)
{
	cTime now;
	string ProhibitedChars("$| ");
	//ProhibitedChars.append("\0",1);

	if (!registered) {
		if(nick.size() > mC.max_nick ) return eVN_LONG;
		if(nick.size() < mC.min_nick ) return eVN_SHORT;
		if(nick.npos != nick.find_first_of(ProhibitedChars)) return eVN_CHARS;
		if((mC.nick_chars.size()>0) && (nick.npos != nick.find_first_not_of(mC.nick_chars.c_str()))) return eVN_CHARS;

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

		if(StrCompare(nick,0,4,"[OP]") == 0) return eVN_NOT_REGED_OP;
	}
	if(mBanList->IsNickTempBanned(nick) > now.Sec())
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
	for (tTFIt i = mTmpFunc.begin(); i != mTmpFunc.end(); i++) {
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

	if (bool(mSlowTimer.mMinDelay) && mSlowTimer.Check(mTime , 1) == 0)
		mBanList->RemoveOldShortTempBans(mTime.Sec());
	if (bool(mHublistTimer.mMinDelay) && mHublistTimer.Check(mTime , 1) == 0)
		this->RegisterInHublist(mC.hublist_host, mC.hublist_port, NULL);
	if (bool(mReloadcfgTimer.mMinDelay) && mReloadcfgTimer.Check(mTime , 1) == 0) {
		mC.Load();
		mCo->mTriggers->ReloadAll();
		if (mC.use_reglist_cache)
			mR->UpdateCache();
		if (Log(2))
			LogStream() << "Socket counter : " << cAsyncConn::sSocketCounter << endl;
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

	unsigned u=0;
	int m,n=0;
	char c=' ';
	is >> n >> c;
	if(n >= 0) {
		m = 1; // multiplicator
		if(c==' ')
			c = 'd';
		switch(c) {
			case 'y':
			case 'Y': m*= 12; // year = 12* month
			case 'M': m*=  4; // month = 4 * week
			case 'w':
			case 'W': m*=  7; // week = 7 days
			case 'd':
			case 'D': m*= 24; // 24 hours
			case 'h':
			case 'H': m*= 60; // 60mniutes
			case 'm': m*= 60; // 60 seconds
			case 's':
			case 'S': break;
			default:
				err << _("Error. Available units are: "
					 "s(econd), m(inute), h(our), d(ay), w(eek), M(onth), Y(ear).\n"
					 "Default is 'd'.") << endl;
				return 0;
				m=1;
			break;
		}
		u= n*m;
	} else
		err << _("Please provide a positive number.") << endl;
	return u;
}

int cServerDC::DoRegisterInHublist(string host, int port, string NickForReply)
{
	ostringstream os, os2;
	char pipe='|';
#ifndef _WIN32
	unsigned long long buf = GetTotalShareSize();
#else
	char buf[32];
	sprintf(buf,"%ui64d",GetTotalShareSize());
#endif

	istringstream is(host);
	string CurHost;
	string lock, key;
	size_t pos_space;
	cAsyncConn *pHubList;

	os2 << _("Hublist registration results:") << "\n";
	while (CurHost = "", is >> CurHost, CurHost.size() > 0) {
		os2 << autosprintf(_("Sending to %s:%d..."), CurHost.c_str(), port);
		pHubList = new cAsyncConn(CurHost,port);

		if(!pHubList->ok) {
			os2 << " " << _("connection failed") << "\n";
			pHubList->Close();
			delete pHubList;
			pHubList = 0;
			continue;
		}

		key = "";
		pHubList->SetLineToRead(&lock,pipe,1024);
		pHubList->ReadAll();
		pHubList->ReadLineLocal();
		if(lock.size() > 6) {
			pos_space = lock.find(' ',6);
			if (pos_space != lock.npos) pos_space -= 6;
			lock = lock.substr(6,pos_space);
			cDCProto::Lock2Key(lock, key);
		}

		// Create the registration string
		os.str(mEmpty);
		os << "$Key " << key << pipe <<  mC.hub_name // removed pipe before name
			<< pipe << mC.hub_host
			<< pipe;
		__int64 hl_minshare = mC.min_share;
		if (mC.min_share_use_hub > hl_minshare)
			hl_minshare = mC.min_share_use_hub;
		if (mC.hublist_send_minshare)
			os << "[MINSHARE:" << StringFrom(hl_minshare) << "MB] ";
		os << mC.hub_desc
			<< pipe << mUserList.Size()
			<< pipe << buf
			<< pipe;


		// send it
		if(Log(2))
			LogStream() << os.str() << endl;
		pHubList->Write(os.str(), true);
		if(!pHubList->ok)
			os2 << " " << _("error sending info") << endl;
		pHubList->Close();
		delete pHubList;
		pHubList = NULL;
		os2 << " " << _(".. OK") << "\n";
	}

	os2 << _("Done");
	CurHost = os2.str();
	if (NickForReply.size() > 0) {
		cUser * user = mUserList.GetUserByNick(NickForReply);
		if(user && user->mxConn)
			DCPublicHS(CurHost, user->mxConn);
	}
	return 1;
}

int cServerDC::RegisterInHublist(string host, int port, cConnDC *conn)
{
	string NickForReply;
	DCPublicHS(_("Registering the hub in hublists. This may take a while, please wait..."), conn);
	if(conn && conn->mpUser) NickForReply = conn->mpUser->mNick;
	cThreadWork *work = new tThreadWork3T<cServerDC, string, int, string>( host, port, NickForReply, this, &cServerDC::DoRegisterInHublist);
	if(mHublistReg.AddWork(work)) {
		return 1;
	} else  {
		delete work;
		return 0;
	}
}

__int64 cServerDC::GetTotalShareSize()
{
	__int64 total =0;
	cUserCollection::iterator i;
	for(i=mUserList.begin(); i!= mUserList.end(); ++i) total += ((cUser *)(*i))->mShare;
	return total;
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

// todo: add whocity function, like one above

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

void cServerDC::ReportUserToOpchat(cConnDC *conn, const string &Msg, bool ToMain)
{
	if (conn) {
		ostringstream os;
		os << Msg;

		if (conn->mpUser) os << " ][ " << _("Nickname") << ": " << conn->mpUser->mNick;
		os << " ][ " << _("IP") << ": " << conn->AddrIP().c_str();
		if (!mUseDNS && mC.report_dns_lookup) conn->DNSLookup();
		if (!conn->AddrHost().empty()) os << " ][ " << _("Host") << ": " << conn->AddrHost().c_str();

		if (!ToMain && this->mOpChat)
			this->mOpChat->SendPMToAll(os.str(), NULL);
		else {
			static string ChatMsg;
			ChatMsg.erase();
			cDCProto::Create_Chat(ChatMsg, mC.opchat_name, os.str());
			this->mOpchatList.SendToAll(ChatMsg, false, true);
		}
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
						DCPrivateHS(ostr.str(), user->mxConn, &OP->mNick);
					}
				}
			}

			if(flags & eKCK_Drop) {
				// Send the message to the kicker
				ostr.str(mEmpty);
				if(flags & eKCK_TBAN)
					ostr << autosprintf(_("Kicked user %s"), Nick.c_str());
				else
					ostr << autosprintf(_("Dropped user %s"), Nick.c_str());
				ostr << " (IP: " << user->mxConn->AddrIP();
				if(user->mxConn->AddrHost().length())
					ostr << ", host: " << user->mxConn->AddrHost();
				ostr << ")";

				if(user->mxConn->Log(2))
					user->mxConn->LogStream() << "Kicked by " << OP->mNick << ", ban " << mC.tban_kick << "s"<< endl;
				if(OP->Log(3))
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
					ostr << "\n" << _("User cannot be kicked.");

				// temp ban kicked user
				if (flags & eKCK_TBAN) {
					cBan Ban(this);
					cKick Kick;

					mKickList->FindKick(Kick, user->mNick, OP->mNick, 30, true, true);

					if(user->mToBan) {
						mBanList->NewBan(Ban, Kick, user->mBanTime, eBF_NICKIP);
						ostr << " " << _("and banned") << " ";
						Ban.DisplayKick(ostr);
					} else {
						mBanList->NewBan(Ban, Kick, mC.tban_kick, eBF_NICKIP);
						ostr << " " << _("and banned") << " ";
						Ban.DisplayKick(ostr);
					}
					mBanList->AddBan(Ban);
				}
				if (!use_os)
					DCPublicHS(ostr.str(),OP->mxConn);
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
	}; // namespace nServer
}; // namespace nVerliHub
