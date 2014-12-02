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

#include "cconndc.h"
#if HAVE_LIBGEOIP
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
cConnDC::cConnDC(int sd, cAsyncSocketServer *server)
: cAsyncConn(sd,server)
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

int cConnDC::Send(const string &data, bool AddPipe, bool Flush)
{
	if (!mWritable)
		return 0;

	string outData(data);
	size_t outSize = outData.size();

	if (outSize >= (MAX_SEND_SIZE - 1)) {
		if (Log(2))
			LogStream() << "Truncating too long message from " << outSize << " to " << (MAX_SEND_SIZE - 1) << ", message starts with: " << outData.substr(0, 10) << endl;

		outData.resize(MAX_SEND_SIZE - 1, ' ');
		outSize = MAX_SEND_SIZE - 1;
	}

	if (Log(5))
		LogStream() << "OUT: " << outData.substr(0, 100) << endl;

	if (msLogLevel >= 3)
		Server()->mNetOutLog << outSize << ": " << outData.substr(0, 10) << endl;

	if (AddPipe) {
		outData.append("|");
		outSize++;
	}

	if (!Server()->mC.disable_zlib && (mFeatures & eSF_ZLIB)) {
		/*
		if (!Flush) { // if data should be buffered, append content to zlib buffer
			Server()->mZLib->AppendData(outData.c_str(), outSize);
			return 1;
		}
		*/

		size_t zlibDataLen = 0;
		char *zlibData = Server()->mZLib->Compress(outData.c_str(), outSize, zlibDataLen);

		if (zlibData == NULL) {
			if (Log(5))
				LogStream() << "Error compressing data with ZLib, fall back to uncompressed data" << endl;
		} else {
			outData.assign(zlibData, zlibDataLen);
			outSize = zlibDataLen;
		}
	}

	int ret = Write(outData, Flush);
	mTimeLastAttempt.Get();

	if (mxServer) {
		//Server()->mUploadZone[mGeoZone].Dump();
		Server()->mUploadZone[mGeoZone].Insert(Server()->mTime, (int)outSize);
		//Server()->mUploadZone[mGeoZone].Dump();
	}

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

	// check frozen users, send to every user, every minute an empty message
	cTime ten_min_ago;
	ten_min_ago = ten_min_ago - 600;
	if(Server()->MinDelay(mT.ping,Server()->mC.delayed_ping) && mpUser && mpUser->mInList && mpUser->mT.login < ten_min_ago) {
		omsg="";
		Send(omsg,true);
	}

	// upload line optimisation  - upload userlist slowlier
	if(mpUser && mpUser->mQueueUL.size()) {
		unsigned long pos = 0,ppos=0;
		string buf,nick;
		cUser *other;
		for(i=0; i < Server()->mC.ul_portion; i++) {
			pos=mpUser->mQueueUL.find_first_of('|',ppos);
			if(pos == mpUser->mQueueUL.npos) break;

			nick = mpUser->mQueueUL.substr(ppos, pos-ppos);
			other = Server()->mUserList.GetUserByNick(nick);
			ppos=pos+1;

			// check if user found
			if(!other) {
				if(nick != Server()->mC.hub_security && nick != Server()->mC.opchat_name) {
					cDCProto::Create_Quit(buf, nick);
				}
			} else {
				buf.append(Server()->mP.GetMyInfo(other, mpUser->mClass));
			}
		}
		Send(buf,true);

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
	if (!this->mCloseRedirect.empty()) {
		string omsg = "$ForceMove " + this->mCloseRedirect;
		Send(omsg, true);
	} else if (mxServer) {
		string address = Server()->mCo->mRedirects->MatchByType(this->mCloseReason);

		if (!address.empty()) {
			string omsg = "$ForceMove " + address;
			Send(omsg, true);
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

cDCConnFactory::cDCConnFactory(cServerDC *server) :
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
#if HAVE_LIBGEOIP
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
