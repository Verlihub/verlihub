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

#include "cconndc.h"
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
	memset(&mTO, 0, sizeof(mTO));
	mFeatures = 0;
	mSendNickList = false;
	//mNickListInProgress = false;
	//mSkipNickList = false;
	mConnType = NULL;
	mCloseReason = eCR_DEFAULT;
	SetTimeOut(eTO_LOGIN, Server()->mC.timeout_length[eTO_LOGIN], server->mTime); // default login timeout
	mGeoZone = -1;
	mRegInfo = NULL;
	mSRCounter = 0;
	memset(mProtoFloodCounts, 0, sizeof(mProtoFloodCounts)); // protocol flood
	//memset(&mProtoFloodTimes, 0, sizeof(mProtoFloodTimes));
	//memset(&mProtoFloodReports, 0, sizeof(mProtoFloodReports));
}

cConnDC::~cConnDC()
{
	if (mRegInfo) {
		delete mRegInfo;
		mRegInfo = NULL;
	}
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
		usr = NULL;
		return false;
	}
	mpUser = usr;
	mpUser->mxConn = this;
	mpUser->mxServer = (cServerDC*)mxServer;
	if(Log(3)) LogStream() << "User " << usr->mNick << " connected ... " << endl;
	return true;
}

int cConnDC::Send(string &data, bool AddPipe, bool Flush)
{
	if (!mWritable)
		return 0;

	if (AddPipe)
		data.append(1, '|');

	size_t len = data.size();

	if (len > 1) { // write only if we really got anything excluding pipe
		if (Log(5))
			LogStream() << "OUT [" << len << "]: " << data.substr(0, 100) << endl;

		if ((msLogLevel >= 3) && Server()->mNetOutLog && Server()->mNetOutLog.is_open())
			Server()->mNetOutLog << len << ": " << data.substr(0, 100) << endl;
	}

	int ret = Write(data, Flush);

	if ((Server()->mTime.Sec() - mTimeLastAttempt.Sec()) >= 2) // delay 2 seconds
		mTimeLastAttempt = Server()->mTime;

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
	if (cObj::StrLog(ostr, level)) {
		LogStream() << '(' << mAddrIP; // << ':' << mAddrPort;

		if (mAddrHost.size())
			LogStream() << ' ' << mAddrHost;

		LogStream() << ") ";

		if (mpUser)
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
	if (!mRegInfo || !mRegInfo->mEnabled) {
		if (mFeatures & eSF_BOTINFO)
			return -1;

		return 0;
	}

	return mRegInfo->mClass;
}

int cConnDC::OnTimer(const cTime &now)
{
	ostringstream os;
	int i;

	for (i = 0; i < eTO_MAXTO; i++) { // check the timeouts
		if (!CheckTimeOut(tTimeOut(i), now)) {
			os << autosprintf(_("Operation timeout: %s"), this->GetTimeOutType(tTimeOut(i)));

			if (Log(2))
				LogStream() << "Operation timeout (" << tTimeOut(i) << ')' << endl;

			Server()->ConnCloseMsg(this, os.str(), 6000, eCR_TIMEOUT);
			break;
		}
	}

	if (mTimeLastIOAction.Sec() < (mTimeLastAttempt.Sec() - 270)) {
		os << _("General timeout");

		if (Log(2))
			LogStream() << "Any action timeout" << endl;

		Server()->ConnCloseMsg(this, os.str(), 6000, eCR_TO_ANYACTION);
	}

	if (mpUser && mpUser->mInList && Server()->mC.delayed_ping && Server()->MinDelay(mT.ping, Server()->mC.delayed_ping)) { // check frozen users, every minute by default
		string omsg("|");
		Send(omsg, false);
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

int cConnDC::CheckTimeOut(tTimeOut timeout, const cTime &now)
{
	if (timeout >= eTO_MAXTO)
		return 0;

	return 0 == mTO[timeout].Check(now);
}

/*
void cConnDC::OnFlushDone()
{
	//mBufFlush.clear(); // all buffers were already resized and shrinked in cAsyncConn::Write(), else i dont understand when this is called or if its called at all
	//mBufSend.clear();
	//ShrinkStringToFit(mBufFlush);
	//ShrinkStringToFit(mBufSend);

	if (mNickListInProgress) {
		SetLSFlag(eLS_NICKLST);
		mNickListInProgress = false;

		if (!ok || !mWritable) {
			if (Log(2))
				LogStream() << "Connection closed during nick list send" << endl;
		} else {
			if (Log(2))
				LogStream() << "Login after nick list send" << endl;

			Server()->DoUserLogin(this);
		}
	}
}
*/

int cConnDC::OnCloseNice()
{
	string omsg;

	if (this->mCloseRedirect.size()) {
		Server()->mP.Create_ForceMove(omsg, this->mCloseRedirect, true);
		Send(omsg, true);

	} else if (mxServer) {
		const string &cc = GetGeoCC();
		unsigned __int64 shar = 0;

		if (mpUser && (mpUser->mShare > 0))
			shar = mpUser->mShare;

		char *addr = Server()->mCo->mRedirects->MatchByType(this->mCloseReason, cc, (this->mTLSVer.size() && (this->mTLSVer != "0.0")), shar);

		if (addr) {
			Server()->mP.Create_ForceMove(omsg, addr, true);
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
		mProtoFloodReports[type] = cTime(serv->mTime.Sec() - signed(serv->mC.proto_flood_report_time));
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

	if ((dif < 0) || (dif >= signed(serv->mC.proto_flood_report_time))) {
		mProtoFloodReports[type] = serv->mTime;

		if (serv->mC.proto_flood_report && (
			serv->mC.proto_flood_report_locked ||
			((type != ePF_CHAT) && (type != ePF_PRIV) && (type != ePF_MCTO) && (type != ePF_SEARCH) && (type != ePF_RCTM)) || // not lockable types
			((type == ePF_CHAT) && !serv->mProtoFloodAllLocks[ePFA_CHAT]) ||
			((type == ePF_PRIV) && !serv->mProtoFloodAllLocks[ePFA_PRIV]) ||
			((type == ePF_MCTO) && !serv->mProtoFloodAllLocks[ePFA_MCTO]) ||
			((type == ePF_SEARCH) && !serv->mProtoFloodAllLocks[ePFA_SEAR]) ||
			((type == ePF_RCTM) && !serv->mProtoFloodAllLocks[ePFA_RCTM])
		))
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

	if ((action >= 3) && serv->mC.proto_flood_tban_time) // add temporary ban
		serv->mBanList->AddIPTempBan(AddrToNumber(), serv->mTime.Sec() + serv->mC.proto_flood_tban_time, to_user.str(), eBT_FLOOD);

	serv->ConnCloseMsg(this, to_user.str(), 1000, eCR_LOGIN_ERR);
	return true;
}

cDCConnFactory::cDCConnFactory(cServerDC *server):
	cConnFactory(&(server->mP)),
	mServer(server)
{}

cAsyncConn *cDCConnFactory::CreateConn(tSocket sd)
{
	if (!mServer)
		return NULL;

	cConnDC *conn = new cConnDC(sd, mServer);

	/* todo: use try instead
	if (!conn) {
		if (mServer->Log(0))
			mServer->LogStream() << "Failed to create new connection" << endl;

		return NULL;
	}
	*/

	conn->mxMyFactory = this;
	conn->mxProtocol = mProtocol;
	return (cAsyncConn*)conn;
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

string cConnDC::GetGeoCC()
{
	if (mCC.empty() && mxServer) { // if not set
		nVerliHub::cServerDC *serv = (nVerliHub::cServerDC*)mxServer;
		serv->mMaxMindDB->GetCCC(mCC, mCN, mCity, AddrIP()); // lookup all at once
	}

	return mCC;
}

string cConnDC::GetGeoCN()
{
	if (mCN.empty() && mxServer) { // if not set
		nVerliHub::cServerDC *serv = (nVerliHub::cServerDC*)mxServer;
		serv->mMaxMindDB->GetCCC(mCC, mCN, mCity, AddrIP()); // lookup all at once
	}

	return mCN;
}

string cConnDC::GetGeoCI()
{
	if (mCity.empty() && mxServer) { // if not set
		nVerliHub::cServerDC *serv = (nVerliHub::cServerDC*)mxServer;
		serv->mMaxMindDB->GetCCC(mCC, mCN, mCity, AddrIP()); // lookup all at once
	}

	return mCity;
}

void cConnDC::ResetGeo()
{
	mCC.clear();
	mCN.clear();
	mCity.clear();

	if (mGeoZone == -1) // only if not set
		return;

	if (!mxServer) {
		mGeoZone = 0;
		return;
	}

	nVerliHub::cServerDC *serv = (nVerliHub::cServerDC*)mxServer;
	serv->mUserCount[mGeoZone]--;
	mGeoZone = -1;
	SetGeoZone();
	serv->mUserCount[mGeoZone]++;
}

void cConnDC::SetGeoZone()
{
	if (mGeoZone > -1) // already set
		return;

	if (!mxServer) {
		mGeoZone = 0;
		return;
	}

	nVerliHub::cServerDC *serv = (nVerliHub::cServerDC*)mxServer;
	string cc;

	for (unsigned int pos = 0; pos < 3; pos++) {
		if (serv->mC.cc_zone[pos].size()) { // only if enabled
			if (cc.empty())
				cc = GetGeoCC();

			if ((cc == serv->mC.cc_zone[pos]) || (serv->mC.cc_zone[pos].find(cc) != string::npos)) {
				mGeoZone = pos + 1;
				return;
			}
		}
	}

	unsigned long inum = AddrToNumber(), imin = 0, imax = 0;

	if (serv->mC.ip_zone4_min.size() && serv->mC.ip_zone4_max.size()) {
		cBanList::Ip2Num(serv->mC.ip_zone4_min, imin); // validated on set
		cBanList::Ip2Num(serv->mC.ip_zone4_max, imax);

		if ((imin <= inum) && (imax >= inum)) {
			mGeoZone = 4;
			return;
		}
	}

	if (serv->mC.ip_zone5_min.size() && serv->mC.ip_zone5_max.size()) {
		cBanList::Ip2Num(serv->mC.ip_zone5_min, imin); // validated on set
		cBanList::Ip2Num(serv->mC.ip_zone5_max, imax);

		if ((imin <= inum) && (imax >= inum)) {
			mGeoZone = 5;
			return;
		}
	}

	if (serv->mC.ip_zone6_min.size() && serv->mC.ip_zone6_max.size()) {
		cBanList::Ip2Num(serv->mC.ip_zone6_min, imin); // validated on set
		cBanList::Ip2Num(serv->mC.ip_zone6_max, imax);

		if ((imin <= inum) && (imax >= inum)) {
			mGeoZone = 6;
			return;
		}
	}

	mGeoZone = 0; // default zone
}

	}; // namespace nSocket
}; // namespace nVerliHub
