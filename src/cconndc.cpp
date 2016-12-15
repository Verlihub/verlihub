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

#include "cconndc.h"
#ifdef HAVE_LIBGEOIP
#include "cgeoip.h"
#endif
#include "creglist.h"
#include "creguserinfo.h"
#include "cbanlist.h"
#include "cserverdc.h"
#include "ccustomredirects.h"
#include "i18n.h"

namespace nVerliHub {
	using namespace nTables;
	using namespace nEnums;
	namespace nSocket {
cConnDC::cConnDC(int sd, cAsyncSocketServer *server):
	cAsyncConn(sd, server)
{
	mpUser = NULL;
	SetClassName("ConnDC");
	mLogStatus = 0;
	memset(&mTO,0, sizeof(mTO));
	mFeatures = 0;
	mSendNickList = false;
	mNickListInProgress = false;
	mConnType = NULL;
	mCloseReason = 0;
	// Set default login timeout
	SetTimeOut(eTO_LOGIN, Server()->mC.timeout_length[eTO_LOGIN], server->mTime);
	// Default zone
	mGeoZone = 0;
	mRegInfo = NULL;
	mSRCounter = 0;

	// protocol flood
	memset(mProtoFloodCounts, 0, sizeof(mProtoFloodCounts));
	memset(mProtoFloodTimes, 0, sizeof(mProtoFloodTimes));
	memset(mProtoFloodReports, 0, sizeof(mProtoFloodReports));
}

cConnDC::~cConnDC()
{
	if(mRegInfo)
		delete mRegInfo;
	mRegInfo = NULL;
}

bool cConnDC::SetUser(cUser *usr)
{
	if(!usr) {
		if(ErrLog(0))
			LogStream() << "Trying to add a NULL user" << endl;
		return false;
	}

	if(mpUser) {
		if(ErrLog(1))
			LogStream() << "Trying to add user when it's actually done" << endl;
		delete usr;
		return false;
	}
	mpUser = usr;
	mpUser->mxConn = this;
	mpUser->mxServer = (cServerDC *) mxServer;
	if(Log(3)) LogStream() << "User " << usr->mNick << " connected ... " << endl;
	return true;
}

int cConnDC::Send(string &data, bool AddPipe, bool Flush)
{
	if (!mWritable)
		return 0;

	if (AddPipe)
		data.append("|");

	size_t len = data.size();

	if ((len > 1) && Log(5))
		LogStream() << "OUT [" << len << "]: " << data.substr(0, 100) << endl;

	if ((msLogLevel >= 3) && Server()->mNetOutLog && Server()->mNetOutLog.is_open())
		Server()->mNetOutLog << len << ": " << data.substr(0, 100) << endl;

	int ret = Write(data, Flush);
	mTimeLastAttempt.Get();

	if (ret > 0) { // calculate upload bandwidth in real time
		//Server()->mUploadZone[mGeoZone].Dump();
		Server()->mUploadZone[mGeoZone].Insert(Server()->mTime, ret);
		//Server()->mUploadZone[mGeoZone].Dump();
		Server()->mProtoTotal[1] += ret; // add total upload
	}

	if (AddPipe)
		data.erase(len - 1, 1);

	return ret;
}

int cConnDC::StrLog(ostream & ostr, int level)
{
	if(cObj::StrLog(ostr,level))
	{
		LogStream()   << "(" << mAddrIP ;//<< ":" << mAddrPort;
		if(mAddrHost.length())
			LogStream() << " " << mAddrHost;
		LogStream()   << ") ";
		if(mpUser)
			LogStream() << "[ " << mpUser->mNick << " ] ";
		return 1;
	}
	return 0;
}

void cConnDC::SetLSFlag(unsigned int statusFlag)
{
	mLogStatus |= statusFlag;
}

void cConnDC::ReSetLSFlag(unsigned int statusFlag)
{
	mLogStatus = statusFlag;
}

unsigned int cConnDC::GetLSFlag(unsigned int statusFlag)
{
	return mLogStatus & statusFlag;
}

const char *cConnDC::GetTimeOutType(tTimeOut timeout)
{
	static const char *timeoutType [] = {_("Key"), _("ValidateNick"), _("Login"), _("MyINFO"), _("Password")};
	return timeoutType[timeout];
}

int cConnDC::GetTheoricalClass()
{
	if (mFeatures & eSF_BOTINFO)
		return -1;

	if (!mRegInfo || !mRegInfo->mEnabled)
		return 0;

	return mRegInfo->mClass;
}

int cConnDC::OnTimer(cTime &now)
{
	ostringstream os;
	string omsg;
	// check the timeouts
	int i;
	for(i=0; i < eTO_MAXTO; i++) {
		if(!CheckTimeOut(tTimeOut(i), now)) {
			os << autosprintf(_("Operation timeout: %s"), this->GetTimeOutType(tTimeOut(i)));
			if(Log(2))
				LogStream() << "Operation timeout (" << tTimeOut(i) << ")" << endl;
			Server()->ConnCloseMsg(this,os.str(),6000, eCR_TIMEOUT);
			break;
		}
	}
	if (mTimeLastIOAction.Sec() < (mTimeLastAttempt.Sec() - 270)) {
		os << _("General timeout");
		if(Log(2))
			LogStream() << "Any action timeout.." << endl;
		Server()->ConnCloseMsg(this,os.str(),6000, eCR_TO_ANYACTION);
	}

	cTime ten_min_ago; // check frozen users, send to every user, every minute an empty message
	ten_min_ago = ten_min_ago - 600;

	if (Server()->MinDelay(mT.ping, Server()->mC.delayed_ping) && mpUser && mpUser->mInList && (mpUser->mT.login < ten_min_ago)) {
		omsg = "";
		Send(omsg, true);
	}

	// upload line optimisation  - upload userlist slowlier
	if(mpUser && mpUser->mQueueUL.size()) {
		unsigned long pos = 0,ppos=0;
		string buf, nick;
		cUser *other;

		for(unsigned int i = 0; i < Server()->mC.ul_portion; i++) {
			pos=mpUser->mQueueUL.find_first_of('|',ppos);
			if(pos == mpUser->mQueueUL.npos) break;

			nick = mpUser->mQueueUL.substr(ppos, pos-ppos);
			other = Server()->mUserList.GetUserByNick(nick);
			ppos=pos+1;

			// check if user found
			if(!other) {
				if ((nick != Server()->mC.hub_security) && (nick != Server()->mC.opchat_name)) {
					cDCProto::Create_Quit(buf, nick);
				}
			} else {
				buf.clear(); // only if nothing was added before
				buf.append(Server()->mP.GetMyInfo(other, mpUser->mClass));
			}
		}

		if (buf.size())
			Send(buf, true);

		if(pos != mpUser->mQueueUL.npos)
			pos++;
		// I can spare some RAM here by copying it to intermediate buffer and back
		mpUser->mQueueUL.erase(0,pos);
		mpUser->mQueueUL.reserve(0);
	}

	return 0;
}

int cConnDC::ClearTimeOut(tTimeOut timeout)
{
	if(timeout >= eTO_MAXTO)
		return 0;
	mTO[timeout].Disable();
	return 1;
}

int cConnDC::SetTimeOut(tTimeOut timeout, double seconds, cTime &now)
{
	if(timeout >= eTO_MAXTO)
		return 0;
	if(seconds == 0.)
		return 0;
	mTO[timeout].mMaxDelay = seconds;
	mTO[timeout].Reset(now);
	return 1;
}

int cConnDC::CheckTimeOut(tTimeOut timeout, cTime &now)
{
	if(timeout >= eTO_MAXTO)
		return 0;
	return 0 == mTO[timeout].Check(now);
}

void cConnDC::OnFlushDone()
{
	mBufSend.erase(0,mBufSend.size());
	if(mNickListInProgress) {
		SetLSFlag(eLS_NICKLST);
		mNickListInProgress = false;
		if(!ok || !mWritable) {
			if(Log(2)) LogStream() << "Connection closed during nicklist" << endl;
		} else {
			if(Log(2)) LogStream() << "Login after nicklist" << endl;
			Server()->DoUserLogin(this);
		}
	}
}

int cConnDC::OnCloseNice()
{
	string omsg;

	if (this->mCloseRedirect.size()) {
		cDCProto::Create_ForceMove(omsg, this->mCloseRedirect);
		Send(omsg, true);
	} else if (mxServer) {
		char *addr = Server()->mCo->mRedirects->MatchByType(this->mCloseReason);

		if (addr) {
			cDCProto::Create_ForceMove(omsg, addr);
			Send(omsg, true);
			free(addr);
		}
	}

	return 0;
}

void cConnDC::CloseNow(int reason)
{
	this->mCloseReason = reason;
	this->cAsyncConn::CloseNow();
}

void cConnDC::CloseNice(int msec, int reason)
{
	this->mCloseReason = reason;
	this->cAsyncConn::CloseNice(msec);
}

bool cConnDC::NeedsPassword()
{
	return
	   (
		mRegInfo &&
		(mRegInfo->mEnabled || mRegInfo->mClass == eUC_MASTER )&&
		(mRegInfo->mClass != eUC_PINGER) &&
		(!mRegInfo->mPwdChange ||
		 (
		   mRegInfo->mPasswd.size() &&
		   Server()->mC.always_ask_password
		 )
		)
	);
}

/*
	protocol flood
*/

bool cConnDC::CheckProtoFlood(const string &data, int type)
{
	if (!mxServer)
		return false;

	nVerliHub::cServerDC *serv = (nVerliHub::cServerDC*)mxServer;

	if (!serv)
		return false;

	if (GetTheoricalClass() > serv->mC.max_class_proto_flood)
		return false;

	long period = 0;
	unsigned int limit = 0;
	unsigned int action = 3;

	switch (type) {
		case ePF_CTM:
			period = serv->mC.int_flood_ctm_period;
			limit = serv->mC.int_flood_ctm_limit;
			action = serv->mC.proto_flood_ctm_action;
			break;

		case ePF_RCTM:
			period = serv->mC.int_flood_rctm_period;
			limit = serv->mC.int_flood_rctm_limit;
			action = serv->mC.proto_flood_rctm_action;
			break;

		case ePF_SR:
			period = serv->mC.int_flood_sr_period;
			limit = serv->mC.int_flood_sr_limit;
			action = serv->mC.proto_flood_sr_action;
			break;

		case ePF_SEARCH:
			period = serv->mC.int_flood_search_period;
			limit = serv->mC.int_flood_search_limit;
			action = serv->mC.proto_flood_search_action;
			break;

		case ePF_MYINFO:
			period = serv->mC.int_flood_myinfo_period;
			limit = serv->mC.int_flood_myinfo_limit;
			action = serv->mC.proto_flood_myinfo_action;
			break;

		case ePF_EXTJSON:
			period = serv->mC.int_flood_extjson_period;
			limit = serv->mC.int_flood_extjson_limit;
			action = serv->mC.proto_flood_extjson_action;
			break;

		case ePF_NICKLIST:
			period = serv->mC.int_flood_nicklist_period;
			limit = serv->mC.int_flood_nicklist_limit;
			action = serv->mC.proto_flood_nicklist_action;
			break;

		case ePF_PRIV:
			period = serv->mC.int_flood_to_period;
			limit = serv->mC.int_flood_to_limit;
			action = serv->mC.proto_flood_to_action;
			break;

		case ePF_CHAT:
			period = serv->mC.int_flood_chat_period;
			limit = serv->mC.int_flood_chat_limit;
			action = serv->mC.proto_flood_chat_action;
			break;

		case ePF_GETINFO:
			if (!mFeatures || (mFeatures & eSF_NOGETINFO)) { // dont check old clients that dont have NoGetINFO support flag because they will send this command for every user on hub
				period = serv->mC.int_flood_getinfo_period;
				limit = serv->mC.int_flood_getinfo_limit;
				action = serv->mC.proto_flood_getinfo_action;
			}

			break;

		case ePF_MCTO:
			period = serv->mC.int_flood_mcto_period;
			limit = serv->mC.int_flood_mcto_limit;
			action = serv->mC.proto_flood_mcto_action;
			break;

		case ePF_IN:
			period = serv->mC.int_flood_in_period;
			limit = serv->mC.int_flood_in_limit;
			action = serv->mC.proto_flood_in_action;
			break;

		case ePF_PING:
			period = serv->mC.int_flood_ping_period;
			limit = serv->mC.int_flood_ping_limit;
			action = serv->mC.proto_flood_ping_action;
			break;

		case ePF_UNKNOWN:
			period = serv->mC.int_flood_unknown_period;
			limit = serv->mC.int_flood_unknown_limit;
			action = serv->mC.proto_flood_unknown_action;
			break;

		default:
			period = serv->mC.int_flood_unknown_period;
			limit = serv->mC.int_flood_unknown_limit;
			action = serv->mC.proto_flood_unknown_action;
			break;
	}

	if (!limit || !period)
		return false;

	if (!mProtoFloodCounts[type]) {
		mProtoFloodCounts[type] = 1;
		mProtoFloodTimes[type] = serv->mTime;
		mProtoFloodReports[type] = cTime(serv->mTime.Sec() - (signed)serv->mC.proto_flood_report_time);
		return false;
	}

	long dif = serv->mTime.Sec() - mProtoFloodTimes[type].Sec();

	if ((dif < 0) || (dif > period)) {
		mProtoFloodCounts[type] = 1;
		mProtoFloodTimes[type] = serv->mTime;
		return false;
	}

	if (mProtoFloodCounts[type]++ < limit)
		return false;

	ostringstream to_user, to_feed;
	to_user << _("Protocol flood detected") << ": ";

	switch (type) {
		case ePF_CTM:
			to_user << "ConnectToMe";
			break;

		case ePF_RCTM:
			to_user << "RevConnectToMe";
			break;

		case ePF_SR:
			to_user << "SR";
			break;

		case ePF_SEARCH:
			to_user << "Search";
			break;

		case ePF_MYINFO:
			to_user << "MyINFO";
			break;

		case ePF_EXTJSON:
			to_user << "ExtJSON";
			break;

		case ePF_NICKLIST:
			to_user << "GetNickList";
			break;

		case ePF_PRIV:
			to_user << "To";
			break;

		case ePF_CHAT:
			to_user << _("Chat");
			break;

		case ePF_GETINFO:
			to_user << "GetINFO";
			break;

		case ePF_MCTO:
			to_user << "MCTo";
			break;

		case ePF_IN:
			to_user << "IN";
			break;

		case ePF_PING:
			to_user << "Ping";
			break;

		case ePF_UNKNOWN:
			to_user << _("Unknown");
			break;

		default:
			to_user << _("Unknown");
			break;
	}

	string pref;

	if ((type == ePF_UNKNOWN) && data.size()) {
		pref = data.substr(0, data.find_first_of(' '));

		if (pref.size())
			cDCProto::EscapeChars(pref, pref);
	}

	to_feed << to_user.str();

	if (pref.size())
		to_feed << ": " << pref;

	to_feed << " [" << mProtoFloodCounts[type] << ':' << dif << ':' << period << ']';
	dif = serv->mTime.Sec() - mProtoFloodReports[type].Sec(); // report if enabled and not too often

	if ((dif < 0) || (dif >= (signed)serv->mC.proto_flood_report_time)) {
		mProtoFloodReports[type] = serv->mTime;

		if (serv->mC.proto_flood_report)
			serv->ReportUserToOpchat(this, to_feed.str());

		if (Log(1))
			LogStream() << to_feed.str() << endl;
	}

	switch (action) { // protocol flood actions
		case 0: // notify only
			return false;
		case 1: // skip
			return true;
		//case 2: // drop
			//break;
		//case 3: // ban
			//break;
	}

	if ((action >= 3) && serv->mC.proto_flood_tban_time) { // add temporary ban
		if (mpUser) { // we have user, create full ban
			cBan pfban(serv);
			cKick pfkick;
			pfkick.mOp = serv->mC.hub_security;
			pfkick.mIP = AddrIP();
			pfkick.mNick = mpUser->mNick;
			pfkick.mTime = serv->mTime.Sec();
			pfkick.mReason = to_user.str();
			serv->mBanList->NewBan(pfban, pfkick, serv->mC.proto_flood_tban_time, eBF_NICKIP);
			serv->mBanList->AddBan(pfban);
		} else // user missing, create short ban
			serv->mBanList->AddIPTempBan(AddrIP(), serv->mTime.Sec() + serv->mC.proto_flood_tban_time, _("Protocol flood"), eBT_FLOOD);
	}

	serv->ConnCloseMsg(this, to_user.str(), 1000, eCR_LOGIN_ERR);
	return true;
}

cDCConnFactory::cDCConnFactory(cServerDC *server):
	cConnFactory(&(server->mP)),
	mServer(server)
{}

cAsyncConn *cDCConnFactory::CreateConn(tSocket sd)
{
	cConnDC *conn;
	if(!mServer)
		return NULL;

	conn = new cConnDC(sd, mServer);
	conn->mxMyFactory = this;
#ifdef HAVE_LIBGEOIP
	if (
		mServer->sGeoIP.GetCC(conn->AddrIP(),conn->mCC) &&
		mServer->mC.cc_zone[0].size()
	){
		for (int i = 0; i < 3; i ++)  {
			if((conn->mCC == mServer->mC.cc_zone[i]) || (mServer->mC.cc_zone[i].find(conn->mCC) != mServer->mC.cc_zone[i].npos)) {
				conn->mGeoZone = i+1;
				break;
			}
		}
	}

	mServer->sGeoIP.GetCN(conn->AddrIP(), conn->mCN); // get country name
	mServer->sGeoIP.GetCity(conn->mCity, conn->AddrIP()); // get city name
#endif
	long IPConn, IPMin, IPMax;
	IPConn = cBanList::Ip2Num(conn->AddrIP());
	if(mServer->mC.ip_zone4_min.size()) {
		IPMin = cBanList::Ip2Num(mServer->mC.ip_zone4_min);
		IPMax = cBanList::Ip2Num(mServer->mC.ip_zone4_max);
		if((IPMin <= IPConn) && (IPMax >= IPConn))
			conn->mGeoZone = 4;
	}
	if(mServer->mC.ip_zone5_min.size()) {
		IPMin = cBanList::Ip2Num(mServer->mC.ip_zone5_min);
		IPMax = cBanList::Ip2Num(mServer->mC.ip_zone5_max);
		if((IPMin <= IPConn) && (IPMax >= IPConn))
			conn->mGeoZone = 5;
	}
	if(mServer->mC.ip_zone6_min.size()) {
		IPMin = cBanList::Ip2Num(mServer->mC.ip_zone6_min);
		IPMax = cBanList::Ip2Num(mServer->mC.ip_zone6_max);
		if((IPMin <= IPConn) && (IPMax >= IPConn))
			conn->mGeoZone = 6;
	}
	conn->mxProtocol = mProtocol;
	return (cAsyncConn*) conn;
}

void cDCConnFactory::DeleteConn(cAsyncConn * &connection)
{
	cConnDC *conn = (cConnDC*)connection;

	if (conn) {
		#ifndef WITHOUT_PLUGINS
			mServer->mCallBacks.mOnCloseConn.CallAll(conn);
			mServer->mCallBacks.mOnCloseConnEx.CallAll(conn);
		#endif

		if (conn->GetLSFlag(eLS_ALLOWED)) {
			mServer->mUserCountTot--;
			mServer->mUserCount[conn->mGeoZone]--;

			if (conn->mpUser && !conn->mpUser->mHideShare) // only if not hidden
				mServer->mTotalShare -= conn->mpUser->mShare;
		}

		if (conn->mpUser) {
			mServer->RemoveNick(conn->mpUser);

			if (conn->mpUser->mClass)
				mServer->mR->Logout(conn->mpUser->mNick);

			delete conn->mpUser;
			conn->mpUser = NULL;
		}
	}

	cConnFactory::DeleteConn(connection);
}

	}; // namespace nSocket
}; // namespace nVerliHub
