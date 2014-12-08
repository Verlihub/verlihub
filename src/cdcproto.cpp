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

#include "cserverdc.h"
#include "cdcproto.h"
#include "cconndc.h"
#include "creglist.h"
#include "cbanlist.h"
#include "cmessagedc.h"
#include "cdctag.h"
#include <string>
#include <string.h>
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#include "stringutils.h"
#include "cdcconsole.h"
#include "i18n.h"

using std::string;

namespace nVerliHub {
	using namespace nUtils;
	using namespace nEnums;
	using namespace nSocket;
	using namespace nSocket;
	namespace nProtocol {

cDCProto::cDCProto(cServerDC *serv):mS(serv)
{
	if(!mKickChatPattern.Compile("^((\\S+) )?is kicking (\\S+) [bB]ecause: (.*)$"))
		throw "error in kickchatpattern";
	if(!mKickBanPattern.Compile("_[bB][aA][nN]_(\\d+[smhdwySHMDWY]?)?"))
		throw "error in kickbanpattern";
	SetClassName("cDCProto");
}

cMessageParser *cDCProto::CreateParser()
{
	return new cMessageDC;
}

void cDCProto::DeleteParser(cMessageParser *OldParser)
{
	if (OldParser != NULL) delete OldParser;
}

int cDCProto::TreatMsg(cMessageParser *pMsg, cAsyncConn *pConn)
{
	if (!pMsg || !pConn)
		return -1;

	cMessageDC *msg = (cMessageDC*)pMsg;
	cConnDC *conn = (cConnDC*)pConn;

	if (!msg || !conn)
		return -1;

	/*
		todo
			tMsgAct action = mS->Filter(tDCMsg(msg->mType), conn);
	*/

	if (strlen(msg->mStr.data()) < msg->mStr.size()) {
		if (conn->Log(1))
			conn->LogStream() << "NULL character message: " << msg->mStr << endl;

		if (mS->mC.nullchars_report)
			mS->ReportUserToOpchat(conn, _("Probably attempt of NULL character attack"));

		conn->CloseNow();
		return -1;
	}

	#ifndef WITHOUT_PLUGINS
		if (msg->mType != eMSG_UNPARSED) {
			if (conn->mpUser) {
				if (!mS->mCallBacks.mOnParsedMsgAny.CallAll(conn, msg))
					return 1;
			} else {
				if (!mS->mCallBacks.mOnParsedMsgAnyEx.CallAll(conn, msg)) {
					conn->CloseNow();
					return -1;
				}
			}
		}
	#endif

	switch (msg->mType) {
		case eDC_UNKNOWN:
			this->DCU_Unknown(msg, conn);
			return 1;
			break;
		case eMSG_UNPARSED:
			msg->Parse();
			return TreatMsg(msg, conn); // mS->mCallBacks.mOnUnparsedMsg.CallAll(conn, msg)
			break;
		case eDC_KEY:
			this->DC_Key(msg, conn);
			break;
		case eDC_VALIDATENICK:
			this->DC_ValidateNick(msg, conn);
			break;
		case eDC_MYPASS:
			this->DC_MyPass(msg, conn);
			break;
		case eDC_VERSION:
			this->DC_Version(msg, conn);
			break;
		case eDC_GETNICKLIST:
			this->DC_GetNickList(msg, conn);
			break;
		case eDC_MYINFO:
			this->DC_MyINFO(msg, conn);
			break;
		case eDC_GETINFO:
			this->DC_GetINFO(msg, conn);
			break;
		case eDCO_USERIP:
			this->DCO_UserIP(msg, conn);
			break;
		case eDC_MCONNECTTOME:
			//this->DC_MultiConnectToMe(msg, conn);
			//break;
		case eDC_CONNECTTOME:
			this->DC_ConnectToMe(msg, conn);
			break;
		case eDC_RCONNECTTOME:
			this->DC_RevConnectToMe(msg, conn);
			break;
		case eDC_TO:
			this->DC_To(msg, conn);
			break;
		case eDC_MCTO:
			this->DC_MCTo(msg, conn);
			break;
		case eDC_CHAT:
			this->DC_Chat(msg, conn);
			break;
		case eDCO_OPFORCEMOVE:
			this->DCO_OpForceMove(msg, conn);
			break;
		case eDCO_KICK:
			this->DCO_Kick(msg, conn);
			break;
		case eDC_SEARCH:
		case eDC_SEARCH_PAS:
		case eDC_MSEARCH:
		case eDC_MSEARCH_PAS:
			this->DC_Search(msg, conn);
			break;
		case eDC_SR:
			this->DC_SR(msg, conn);
			break;
		case eDC_QUIT:
			mS->DCPublicHS(_("See you."), conn);
			conn->CloseNice(1000, eCR_QUIT);
			break;
		case eDC_SUPPORTS:
			this->DC_Supports(msg, conn);
			break;
		case eDCO_BAN:
		case eDCO_TBAN:
			this->DCO_TempBan(msg, conn);
			break;
		case eDCO_UNBAN:
			this->DCO_UnBan(msg, conn);
			break;
		case eDCO_GETBANLIST:
			this->DCO_GetBanList(msg, conn);
			break;
		case eDCO_WHOIP:
			this->DCO_WhoIP(msg, conn);
			break;
		case eDCO_GETTOPIC:
			this->DCO_GetTopic(msg, conn);
			break;
		case eDCO_SETTOPIC:
			this->DCO_SetTopic(msg, conn);
			break;
		case eDCB_BOTINFO:
			this->DCB_BotINFO(msg, conn);
			break;
		case eDCC_MYNICK:
			this->DCC_MyNick(msg, conn);
			break;
		case eDCC_LOCK:
			this->DCC_Lock(msg, conn);
			break;
		default:
			if (Log(1))
				LogStream() << "Incoming untreated event" << endl;

			break;
	}

	return 0;
}

/*
	user commands
*/

int cDCProto::DC_Key(cMessageDC *msg, cConnDC *conn)
{
	ostringstream os;

	if (conn->GetLSFlag(eLS_KEYOK)) { // already sent
		os << _("Invalid login sequence, your client already sent key.");

		if (conn->Log(1))
			conn->LogStream() << os.str() << endl;

		mS->ConnCloseMsg(conn, os.str(), 1000, eCR_LOGIN_ERR);
		return -1;
	}

	if (CheckProtoSyntax(conn, msg))
		return -1;

	if (mS->mC.drop_invalid_key) {
		string key;
		Lock2Key(conn->mLock, key);

		if (key != msg->ChunkString(eCH_1_PARAM)) {
			os << _("Your client provided invalid key in response to lock.");

			if (conn->Log(1))
				conn->LogStream() << os.str() << endl;

			mS->ConnCloseMsg(conn, os.str(), 1000, eCR_INVALID_KEY);
			return -1;
		}
	}

	conn->SetLSFlag(eLS_KEYOK);
	conn->ClearTimeOut(eTO_KEY);
	conn->SetTimeOut(eTO_VALNICK, mS->mC.timeout_length[eTO_VALNICK], mS->mTime);
	conn->mT.key.Get();
	return 0;
}

int cDCProto::DC_Supports(cMessageDC *msg, cConnDC *conn)
{
	ostringstream os;

	if (conn->GetLSFlag(eLS_SUPPORTS)) { // already sent
		os << _("Invalid login sequence, your client already sent supports.");

		if (conn->Log(1))
			conn->LogStream() << os.str() << endl;

		mS->ConnCloseMsg(conn, os.str(), 1000, eCR_LOGIN_ERR);
		return -1;
	}

	if (CheckProtoSyntax(conn, msg))
		return -1;

	string &supports = msg->ChunkString(eCH_1_PARAM);
	conn->mSupportsText = supports; // save user supports in plain format
	istringstream is(supports);
	string feature;

	while (1) {
		feature = mS->mEmpty;
		is >> feature;

		if (!feature.size())
			break;

		if (feature == "OpPlus")
			conn->mFeatures |= eSF_OPPLUS;
		else if (feature == "NoHello")
			conn->mFeatures |= eSF_NOHELLO;
		else if (feature == "NoGetINFO")
			conn->mFeatures |= eSF_NOGETINFO;
		else if (feature == "DHT0")
			conn->mFeatures |= eSF_DHT0;
		else if (feature == "QuickList")
			conn->mFeatures |= eSF_QUICKLIST;
		else if (feature == "BotINFO")
			conn->mFeatures |= eSF_BOTINFO;
		else if ((feature == "ZPipe0") || (feature == "ZPipe"))
			conn->mFeatures |= eSF_ZLIB;
		else if (feature == "ChatOnly")
			conn->mFeatures |= eSF_CHATONLY;
		else if (feature == "MCTo")
			conn->mFeatures |= eSF_MCTO;
		else if (feature == "UserCommand")
			conn->mFeatures |= eSF_USERCOMMAND;
		else if (feature == "BotList")
			conn->mFeatures |= eSF_BOTLIST;
		else if (feature == "HubTopic")
			conn->mFeatures |= eSF_HUBTOPIC;
		else if (feature == "UserIP2")
			conn->mFeatures |= eSF_USERIP2;
		else if (feature == "TTHSearch")
			conn->mFeatures |= eSF_TTHSEARCH;
		else if (feature == "Feed")
			conn->mFeatures |= eSF_FEED;
		else if (feature == "ClientID")
			conn->mFeatures |= eSF_CLIENTID;
		else if (feature == "IN")
			conn->mFeatures |= eSF_IN;
		else if (feature == "BanMsg")
			conn->mFeatures |= eSF_BANMSG;
		else if (feature == "TLS")
			conn->mFeatures |= eSF_TLS;
		else if (feature == "IPv4")
			conn->mFeatures |= eSF_IPV4;
		else if (feature == "IP64")
			conn->mFeatures |= eSF_IP64;
		else if (feature == "FailOver")
			conn->mFeatures |= eSF_FAILOVER;
		else if (feature == "NickChange")
			conn->mFeatures |= eSF_NICKCHANGE;
		else if (feature == "ClientNick")
			conn->mFeatures |= eSF_CLIENTNICK;
		else if (feature == "FeaturedNetworks")
			conn->mFeatures |= eSF_FEATNET;
		else if (feature == "ZLine")
			conn->mFeatures |= eSF_ZLINE;
		else if (feature == "GetZBlock")
			conn->mFeatures |= eSF_GETZBLOCK;
		else if (feature == "ACTM")
			conn->mFeatures |= eSF_ACTM;
		else if (feature == "SaltPass")
			conn->mFeatures |= eSF_SALTPASS;
	}

	#ifndef WITHOUT_PLUGINS
		if (!mS->mCallBacks.mOnParsedMsgSupport.CallAll(conn, msg)) {
			conn->CloseNow();
			return -1;
		}
	#endif

	string omsg("$Supports OpPlus NoGetINFO NoHello UserIP2 HubINFO HubTopic ZPipe0 MCTo BotList FailOver");
	conn->Send(omsg);
	conn->SetLSFlag(eLS_SUPPORTS);
	return 0;
}

int cDCProto::DC_ValidateNick(cMessageDC *msg, cConnDC *conn)
{
	ostringstream os;

	if (conn->GetLSFlag(eLS_VALNICK)) { // already sent
		os << _("Invalid login sequence, your client already validated nick.");

		if (conn->Log(1))
			conn->LogStream() << os.str() << endl;

		mS->ConnCloseMsg(conn, os.str(), 1000, eCR_LOGIN_ERR);
		return -1;
	}

	if (CheckProtoSyntax(conn, msg))
		return -1;

	string &nick = msg->ChunkString(eCH_1_PARAM);

	// Log new user
	if(conn->Log(3))
		conn->LogStream() << "User " << nick << " tries to login" << endl;

	int closeReason; // check if nick is valid or close the connection

	if (!mS->ValidateUser(conn, nick, closeReason)) {
		conn->CloseNice(2000, closeReason); // give it little more time, it seems that some clients with slow connection or ping dont receive ban messages sometimes
		return -1;
	}

	static string omsg;

	// User limit
	int limit = mS->mC.max_users_total;
	int limit_cc = mS->mC.max_users[conn->mGeoZone];
	int limit_extra = 0;

	// Calculate user limits
	if (conn->GetTheoricalClass() == eUC_PINGER)
		limit_extra += mS->mC.max_extra_pings;
	else if (conn->GetTheoricalClass() == eUC_REGUSER)
		limit_extra += mS->mC.max_extra_regs;
	else if (conn->GetTheoricalClass() == eUC_VIPUSER)
		limit_extra += mS->mC.max_extra_vips;
	else if (conn->GetTheoricalClass() == eUC_OPERATOR)
		limit_extra += mS->mC.max_extra_ops;
	else if (conn->GetTheoricalClass() == eUC_CHEEF)
		limit_extra += mS->mC.max_extra_cheefs;
	else if (conn->GetTheoricalClass() == eUC_ADMIN)
		limit_extra += mS->mC.max_extra_admins;

	limit += limit_extra;
	limit_cc += limit_extra;

	// check the max_users limit
	if ((conn->GetTheoricalClass() < eUC_OPERATOR) && ((mS->mUserCountTot >= limit) || (mS->mUserCount[conn->mGeoZone] >= limit_cc))) {
		if (mS->mUserCount[conn->mGeoZone] >= limit_cc) {
			string zonestr, zonedat;

			if (conn->mGeoZone >= 1 && conn->mGeoZone <= 3) { // country zones
				zonestr = _("User limit in country zone %s exceeded at %d/%d online users.");
				zonedat = mS->mC.cc_zone[conn->mGeoZone - 1];
			} else if (conn->mGeoZone >= 4 && conn->mGeoZone <= 6) { // ip range zones
				zonestr = _("User limit in IP zone %s exceeded at %d/%d online users.");

				switch (conn->mGeoZone) {
					case 4:
						zonedat = mS->mC.ip_zone4_min + "-" + mS->mC.ip_zone4_max;
					break;
					case 5:
						zonedat = mS->mC.ip_zone5_min + "-" + mS->mC.ip_zone5_max;
					break;
					case 6:
						zonedat = mS->mC.ip_zone6_min + "-" + mS->mC.ip_zone6_max;
					break;
				}
			} else { // main zone
				zonestr = _("User limit in main zone exceeded at %d/%d online users.");
			}

			if (conn->mGeoZone == 0)
				os << autosprintf(zonestr.c_str(), mS->mUserCount[conn->mGeoZone], mS->mUserCountTot);
			else
				os << autosprintf(zonestr.c_str(), zonedat.c_str(), mS->mUserCount[conn->mGeoZone], mS->mUserCountTot);
		} else
			os << autosprintf(_("User limit exceeded at %d online users."), mS->mUserCountTot);

		if (mS->mC.max_users_total == 0) {
			os << " ";

			if (!mS->mC.hubfull_message.empty())
				os << mS->mC.hubfull_message;
			else
				os << _("This is a registered users only hub.");
		}

		if (conn->Log(2)) conn->LogStream() << "Hub is full: " << mS->mUserCountTot << "/" << limit << " :: " << mS->mUserCount[conn->mGeoZone] << "/" << limit_cc << " :: " << conn->mCC << endl;
		omsg = "$HubIsFull";
		conn->Send(omsg);
		mS->ConnCloseMsg(conn, os.str(), 1000, eCR_USERLIMIT);
		return -1;
	} else {
		conn->SetLSFlag(eLS_ALLOWED);
		mS->mUserCountTot++;
		mS->mUserCount[conn->mGeoZone]++;
	}

	// user limit from single ip
	if ((mS->mC.max_users_from_ip != 0) && (conn->GetTheoricalClass() < eUC_VIPUSER)) {
		int cnt = mS->CntConnIP(conn->mAddrIP);

		if (cnt >= mS->mC.max_users_from_ip) {
			os << autosprintf(_("User limit from IP address %s exceeded at %d online users."), conn->mAddrIP.c_str(), cnt);
			omsg = "$HubIsFull";
			conn->Send(omsg);
			mS->ConnCloseMsg(conn, os.str(), 1000, eCR_USERLIMIT);
			return -1;
		}
	}

	// send hub name without topic
	string emp("");
	Create_HubName(omsg, mS->mC.hub_name, emp);
	conn->Send(omsg);

	// check authorization ip
	if (conn->mRegInfo && !conn->mRegInfo->mAuthIP.empty() && (conn->mRegInfo->mAuthIP != conn->mAddrIP)) {
		mS->mR->LoginError(conn, nick); // important
		if (conn->GetTheoricalClass() >= mS->mC.wrongauthip_report) mS->ReportUserToOpchat(conn, autosprintf(_("Authorization IP mismatch from %s"), nick.c_str()));
		os << autosprintf(_("Authorization IP for this account doesn't match your IP address: %s"), conn->mAddrIP.c_str());
		mS->ConnCloseMsg(conn, os.str(), 1000, eCR_LOGIN_ERR);
		return -1;
	}

	if (conn->NeedsPassword()) {
		omsg="$GetPass";
		conn->Send(omsg);
	} else {
		mS->DCHello(nick, conn);
		conn->SetLSFlag(eLS_PASSWD);
	}

	try {
		cUser *NewUser = new cUser(nick);

		if(!conn->SetUser(NewUser)) {
			conn->CloseNow();
			return -1;
		}

		#ifndef WITHOUT_PLUGINS
		if (!mS->mCallBacks.mOnParsedMsgValidateNick.CallAll(conn, msg)) {
			conn->CloseNow();
			return -2;
		}
		#endif

	} catch(...) {
		if(mS->ErrLog(2))
			mS->LogStream() << "Unhandled exception in cServerDC::DC_ValidateNick" << endl;
		omsg = "Sorry :(";
		if(conn->Log(2))
			conn->LogStream() << "Fatal error calling SetUser; closing..." << endl;
		mS->ConnCloseMsg(conn,omsg,1000);
		return -1;
	}

	if (conn->mpUser) {
		if (conn->mRegInfo && (conn->mRegInfo->mClass == eUC_PINGER)) {
			conn->mpUser->Register();
			mS->mR->Login(conn, nick);
		} else if (conn->mFeatures & eSF_BOTINFO)
			conn->mpUser->mClass = eUC_PINGER;
	}

	conn->SetLSFlag(eLS_VALNICK|eLS_NICKLST); // set NICKLST because user may want to skip getting userlist
	conn->ClearTimeOut(eTO_VALNICK);
	conn->SetTimeOut(eTO_MYINFO, mS->mC.timeout_length[eTO_MYINFO], mS->mTime);
	return 0;
}

int cDCProto::DC_MyPass(cMessageDC *msg, cConnDC *conn)
{
	if (CheckUserLogin(conn, false))
		return -1;

	ostringstream os;

	if (conn->GetLSFlag(eLS_PASSWD) && !conn->mpUser->mSetPass) { // already sent
		os << _("Invalid login sequence, you client already sent password.");

		if (conn->Log(1))
			conn->LogStream() << os.str() << endl;

		mS->ConnCloseMsg(conn, os.str(), 1000, eCR_LOGIN_ERR);
		return -1;
	}

	if (CheckProtoSyntax(conn, msg))
		return -1;

	#ifndef WITHOUT_PLUGINS
		if (!mS->mCallBacks.mOnParsedMsgMyPass.CallAll(conn, msg)) {
			conn->CloseNow();
			return -1;
		}
	#endif

	string &pwd = msg->ChunkString(eCH_1_PARAM);

	if (conn->mpUser->mSetPass) { // set password request
		if (!conn->mRegInfo || !conn->mRegInfo->mPwdChange) {
			conn->mpUser->mSetPass = false;
			return 0;
		}

		if (pwd.size() < (unsigned int)mS->mC.password_min_len) {
			os << autosprintf(_("Minimum password length is %d characters, please retry."), mS->mC.password_min_len);
			mS->DCPrivateHS(os.str(), conn);
			mS->DCPublicHS(os.str(), conn);
			conn->mpUser->mSetPass = false;
			return 0;
		}

		if (!mS->mR->ChangePwd(conn->mpUser->mNick, pwd, 0)) {
			os << _("Error updating password.");
			mS->DCPrivateHS(os.str(), conn);
			mS->DCPublicHS(os.str(), conn);
			conn->mpUser->mSetPass = false;
			return 0;
		}

		os << _("Password updated successfully.");
		mS->DCPrivateHS(os.str(), conn);
		mS->DCPublicHS(os.str(), conn);
		conn->ClearTimeOut(eTO_SETPASS);
		conn->mpUser->mSetPass = false;
		return 0;
	}

	string omsg;

	// Check user password
	if(conn->mpUser->CheckPwd(pwd)) {
		conn->SetLSFlag( eLS_PASSWD );
		// Setup user class
		conn->mpUser->Register();
		mS->mR->Login(conn, conn->mpUser->mNick);
		mS->DCHello(conn->mpUser->mNick,conn);
		// If op send LoggedIn and Oplist
		if(conn->mpUser->mClass >= eUC_OPERATOR) {
			omsg = "$LogedIn ";
			omsg+= conn->mpUser->mNick;
			conn->Send(omsg, true);
		}
	} else { // wrong password
		// user is regged so report it
		if (conn->mRegInfo && conn->mRegInfo->getClass() > 0) {
			omsg = "$BadPass";
			conn->Send(omsg);
		 	if (conn->mRegInfo->getClass() >= mS->mC.wrongpassword_report) mS->ReportUserToOpchat(conn, _("Wrong password"));

			if (!mS->mC.wrongpass_message.empty())
				omsg = mS->mC.wrongpass_message;
			else
				omsg = _("You provided an incorrect password and have been temporarily banned.");

			mS->mBanList->AddNickTempBan(conn->mpUser->mNick, mS->mTime.Sec() + mS->mC.pwd_tmpban, omsg);
			mS->mR->LoginError(conn, conn->mpUser->mNick);
			if (conn->Log(2)) conn->LogStream() << "Wrong password, banned for " << mS->mC.pwd_tmpban <<" seconds" << endl;
			mS->ConnCloseMsg(conn, omsg, 2000, eCR_PASSWORD);
			return -1;
		} else {
			if (conn->Log(3)) conn->LogStream() << "User sent password but he isn't regged" << endl;
			return -1;
		}
	}

	return 0;
}

int cDCProto::DC_Version(cMessageDC *msg, cConnDC *conn)
{
	if (CheckUserLogin(conn, false))
		return -1;

	ostringstream os;

	if (conn->GetLSFlag(eLS_VERSION)) { // already sent
		os << _("Invalid login sequence, you client already sent version.");

		if (conn->Log(1))
			conn->LogStream() << os.str() << endl;

		mS->ConnCloseMsg(conn, os.str(), 1000, eCR_LOGIN_ERR);
		return -1;
	}

	if (CheckProtoSyntax(conn, msg))
		return -1;

	#ifndef WITHOUT_PLUGINS
		if (!mS->mCallBacks.mOnParsedMsgVersion.CallAll(conn, msg)) {
			conn->CloseNow();
			return -1;
		}
	#endif

	string &version = msg->ChunkString(eCH_1_PARAM);

	if (conn->Log(5))
		conn->LogStream() << "NMDC version:" << version << endl;

	conn->mVersion = version;
	conn->SetLSFlag(eLS_VERSION);
	return 1;
}

int cDCProto::DC_GetNickList(cMessageDC *msg, cConnDC *conn)
{
	if (CheckUserLogin(conn, false))
		return -1;

	if (conn->CheckProtoFlood(msg->mStr, ePF_NICKLIST)) // protocol flood
		return -1;

	if (!conn->GetLSFlag(eLS_MYINFO) && mS->mC.nicklist_on_login) {
		if (mS->mC.delayed_login) {
			int LSFlag = conn->GetLSFlag(eLS_LOGIN_DONE);

			if (LSFlag & eLS_NICKLST)
				LSFlag -= eLS_NICKLST;

			conn->ReSetLSFlag(LSFlag);
		}

		conn->mSendNickList = true;
		return 0;
	}

	if ((conn->mpUser->mClass < eUC_OPERATOR) && !mS->MinDelay(conn->mpUser->mT.nicklist, mS->mC.int_nicklist))
		return -1;

	return NickList(conn);
}

int cDCProto::DC_MyINFO(cMessageDC *msg, cConnDC *conn)
{
	if (CheckUserLogin(conn, false))
		return -1;

	if (CheckProtoSyntax(conn, msg))
		return -1;

	string &nick = msg->ChunkString(eCH_MI_NICK);

	if (CheckUserNick(conn, nick))
		return -1;

	if (conn->CheckProtoFlood(msg->mStr, ePF_MYINFO)) // protocol flood
		return -1;

	if (conn->mConnType == NULL) // parse connection
		conn->mConnType = ParseSpeed(msg->ChunkString(eCH_MI_SPEED));

	ostringstream os;
	cDCTag *tag = mS->mCo->mDCClients->ParseTag(msg->ChunkString(eCH_MI_DESC)); // check tag

	if (!mS->mC.tag_allow_none && (mS->mCo->mDCClients->mPositionInDesc < 0) && (conn->mpUser->mClass < mS->mC.tag_min_class_ignore) && (conn->mpUser->mClass != eUC_PINGER)) {
		os << _("Your client didn't specify a tag.");

		if (conn->Log(2))
			conn->LogStream() << os.str() << endl;

		mS->ConnCloseMsg(conn, os.str(), 1000, eCR_TAG_NONE);
		delete tag;
		tag = NULL;
		return -1;
	}

	bool TagValid = true;
	int tag_result = 0;

	if ((!mS->mC.tag_allow_none || (mS->mC.tag_allow_none && (mS->mCo->mDCClients->mPositionInDesc >= 0))) && (conn->mpUser->mClass < mS->mC.tag_min_class_ignore) && (conn->mpUser->mClass != eUC_PINGER)) {
		TagValid = tag->ValidateTag(os, conn->mConnType, tag_result);
	}

	#ifndef WITHOUT_PLUGINS
		TagValid = TagValid && mS->mCallBacks.mOnValidateTag.CallAll(conn, tag);
	#endif

	if (!TagValid) {
		if (conn->Log(2))
			conn->LogStream() << "Invalid tag with result " << tag_result << ": " << tag << endl;

		mS->ConnCloseMsg(conn, os.str(), 1000, eCR_TAG_INVALID);
		delete tag;
		tag = NULL;
		return -1;
	}

	bool WasPassive = conn->mpUser->IsPassive; // user might have changed his mode

	switch (tag->mClientMode) {
		case eCM_NOTAG:
		case eCM_PASSIVE:
		case eCM_SOCK5:
		case eCM_OTHER:
			conn->mpUser->IsPassive = true;
			break;
		case eCM_ACTIVE:
			conn->mpUser->IsPassive = false;
			break;
	}

	if (conn->mpUser->mInList && (WasPassive != conn->mpUser->IsPassive)) { // change user mode if differs and not first time
		if (WasPassive) {
			if (mS->mPassiveUsers.ContainsNick(conn->mpUser->mNick))
				mS->mPassiveUsers.RemoveByNick(conn->mpUser->mNick);

			if (!mS->mActiveUsers.ContainsNick(conn->mpUser->mNick))
				mS->mActiveUsers.AddWithNick(conn->mpUser, conn->mpUser->mNick);
		} else {
			if (mS->mActiveUsers.ContainsNick(conn->mpUser->mNick))
				mS->mActiveUsers.RemoveByNick(conn->mpUser->mNick);

			if (!mS->mPassiveUsers.ContainsNick(conn->mpUser->mNick))
				mS->mPassiveUsers.AddWithNick(conn->mpUser, conn->mpUser->mNick);
		}
	}

	int theoclass = conn->GetTheoricalClass();

	if (conn->mpUser->IsPassive && (mS->mC.max_users_passive != -1) && (theoclass < eUC_OPERATOR) && (mS->mPassiveUsers.Size() > mS->mC.max_users_passive)) { // passive user limit
		os << autosprintf(_("Passive user limit exceeded at %d online passive users, please become active to enter the hub."), mS->mPassiveUsers.Size());

		if (conn->Log(2))
			conn->LogStream() << "Passive user limit exceeded: " << mS->mPassiveUsers.Size() << endl;

		string omsg = "$HubIsFull";
		mS->ConnCloseMsg(conn, os.str(), 1000, eCR_USERLIMIT);
		conn->Send(omsg, true);
		delete tag;
		tag = NULL;
		return -1;
	}

	string &str_share = msg->ChunkString(eCH_MI_SIZE); // check share conditions

	if (str_share.size() > 18) { // share is too big
		conn->CloseNow(); // todo: notify user
		delete tag;
		tag = NULL;
		return -1;
	} else if (str_share.empty() || (str_share[0] == '-')) // missing or negative share
		str_share = "0";

	__int64 share = 0, shareB = 0;
	shareB = StringAsLL(str_share);
	share = shareB / (1024 * 1024);

	if (theoclass <= eUC_OPERATOR) { // calculate minimum and maximum
		__int64 min_share = mS->mC.min_share;
		__int64 max_share = mS->mC.max_share;
		__int64 min_share_p, min_share_a;

		if (theoclass == eUC_PINGER)
			min_share = 0;
		else {
			if (theoclass >= eUC_REGUSER) {
				min_share = mS->mC.min_share_reg;
				max_share = mS->mC.max_share_reg;
			}

			if (theoclass >= eUC_VIPUSER) {
				min_share = mS->mC.min_share_vip;
				max_share = mS->mC.max_share_vip;
			}

			if (theoclass >= eUC_OPERATOR) {
				min_share = mS->mC.min_share_ops;
				max_share = mS->mC.max_share_ops;
			}
		}

		min_share_a = min_share;
		min_share_p = (__int64)(min_share * mS->mC.min_share_factor_passive);

		if (conn->mpUser->IsPassive)
			min_share = min_share_p;

		//if (conn->mpUser->Can(eUR_NOSHARE, mS->mTime.Sec()))
			//min_share = 0;

		if ((share < min_share) || (max_share && (share > max_share))) {
			if (share < min_share)
				os << autosprintf(_("You share %s but minimum allowed is %s, %s for active users, %s for passive users."), convertByte(shareB, false).c_str(), convertByte(min_share * 1024 * 1024, false).c_str(), convertByte(min_share_a * 1024 * 1024, false).c_str(), convertByte(min_share_p * 1024 * 1024, false).c_str());
			else
				os << autosprintf(_("You share %s but maximum allowed is %s."), convertByte(shareB, false).c_str(), convertByte(max_share * 1024 * 1024, false).c_str());

			if (conn->Log(2))
				conn->LogStream() << "Share limit" << endl;

			mS->ConnCloseMsg(conn, os.str(), 4000, eCR_SHARE_LIMIT);
			delete tag;
			tag = NULL;
			return -1;
		}

		if (theoclass <= eUC_VIPUSER) { // second share limit that disables search and download
			__int64 temp_min_share = 0; // todo: rename to min_share_use_hub_guest

			switch (theoclass) {
				case eUC_NORMUSER:
					temp_min_share = mS->mC.min_share_use_hub;
					break;
				case eUC_REGUSER:
					temp_min_share = mS->mC.min_share_use_hub_reg;
					break;
				case eUC_VIPUSER:
					temp_min_share = mS->mC.min_share_use_hub_vip;
					break;
			}

			if (temp_min_share) {
				if (conn->mpUser->IsPassive)
					temp_min_share = (__int64)(temp_min_share * mS->mC.min_share_factor_passive);

				if (share < temp_min_share) {
					conn->mpUser->SetRight(eUR_SEARCH, 0);
					conn->mpUser->SetRight(eUR_CTM, 0);
				}
			}
		}

		int use_hub_class = ((conn->mpUser->IsPassive) ? mS->mC.min_class_use_hub_passive : mS->mC.min_class_use_hub);

		if (theoclass < use_hub_class) {
			conn->mpUser->SetRight(eUR_SEARCH, 0);
			conn->mpUser->SetRight(eUR_CTM, 0);
		}
	}

	if (!conn->mpUser->mHideShare) // only if not hidden
		mS->mTotalShare -= conn->mpUser->mShare;

	conn->mpUser->mShare = shareB;

	if (!conn->mpUser->mHideShare) // update total share
		mS->mTotalShare += conn->mpUser->mShare;

	if (!conn->mpUser->mInList) { // detect clone using ip and share, only when user logs in
		string clone;

		if (mS->CheckUserClone(conn, clone)) {
			os << autosprintf(_("You are already in the hub using another nick: %s"), clone.c_str());

			if (conn->Log(2))
				conn->LogStream() << os.str() << endl;

			mS->ConnCloseMsg(conn, os.str(), 1000, eCR_USERLIMIT);
			delete tag;
			tag = NULL;
			return -1;
		}
	}

	conn->mpUser->mEmail = msg->ChunkString(eCH_MI_MAIL); // set email, not sure where its used

	if (conn->GetLSFlag(eLS_LOGIN_DONE) != eLS_LOGIN_DONE) { // user sent myinfo for the first time
		cBan Ban(mS);
		bool banned = mS->mBanList->TestBan(Ban, conn, conn->mpUser->mNick, eBF_SHARE);

		if (banned && (theoclass <= eUC_REGUSER)) {
			os << _("You are banned from this hub.") << "\r\n";
			Ban.DisplayUser(os);
			mS->DCPublicHS(os.str(), conn);

			if (conn->Log(1))
				conn->LogStream() << "Kicked user due to ban detection" << endl;

			conn->CloseNice(1000, eCR_KICKED);
			delete tag;
			tag = NULL;
			return -1;
		}

		#ifndef WITHOUT_PLUGINS
			if (!mS->mCallBacks.mOnFirstMyINFO.CallAll(conn, msg)) {
				conn->CloseNice(1000, eCR_KICKED);
				delete tag;
				tag = NULL;
				return -2;
			}
		#endif
	}

	#ifndef WITHOUT_PLUGINS
		if (!mS->mCallBacks.mOnParsedMsgMyINFO.CallAll(conn, msg)) {
			delete tag;
			tag = NULL;
			return -2;
		}
	#endif

	if (mS->mTotalShare > mS->mTotalSharePeak) // peak total share
		mS->mTotalSharePeak = mS->mTotalShare;

	if (msg->mModified && msg->SplitChunks() && conn->Log(2)) // plugin has modified message, return here?
		conn->LogStream() << "MyINFO syntax error after plugin modification" << endl;

	string myinfo_full, myinfo_basic, desctag, desc, sTag, speed, email, sShare; // $MyINFO $ALL <nick> <description><tag>$ $<speed><status>$<email>$<share>$
	desctag = msg->ChunkString(eCH_MI_DESC);

	if (!desctag.empty()) {
		mS->mCo->mDCClients->ParsePos(desctag); // get new tag position in case its changed
		desc.assign(desctag, 0, mS->mCo->mDCClients->mPositionInDesc);

		if (mS->mCo->mDCClients->mPositionInDesc >= 0)
			sTag.assign(desctag, mS->mCo->mDCClients->mPositionInDesc, -1);
	}

	if (mS->mC.show_desc_len >= 0) // hide description
		desc.assign(desc, 0, mS->mC.show_desc_len);

	if (mS->mC.desc_insert_mode) { // description insert mode
		if (mS->mC.desc_insert_vars.empty()) { // insert mode only
			switch (tag->mClientMode) {
				case eCM_ACTIVE:
					desc = "[A]" + desc;
					break;
				case eCM_PASSIVE:
					desc = "[P]" + desc;
					break;
				case eCM_SOCK5:
					desc = "[5]" + desc;
					break;
				default: // eCM_OTHER, eCM_NOTAG
					desc = "[?]" + desc;
					break;
			}
		} else { // insert custom variables
			string desc_prefix = mS->mC.desc_insert_vars;

			ReplaceVarInString(desc_prefix, "CC", desc_prefix, conn->mCC);
			ReplaceVarInString(desc_prefix, "CN", desc_prefix, conn->mCN);
			ReplaceVarInString(desc_prefix, "CITY", desc_prefix, conn->mCity);
			ReplaceVarInString(desc_prefix, "CLASS", desc_prefix, conn->mpUser->mClass);

			switch (conn->mpUser->mClass) {
				case eUC_PINGER:
					ReplaceVarInString(desc_prefix, "CLASSNAME", desc_prefix, _("Pinger"));
					break;
				case eUC_NORMUSER:
					ReplaceVarInString(desc_prefix, "CLASSNAME", desc_prefix, _("Guest"));
					break;
				case eUC_REGUSER:
					ReplaceVarInString(desc_prefix, "CLASSNAME", desc_prefix, _("Registered"));
					break;
				case eUC_VIPUSER:
					ReplaceVarInString(desc_prefix, "CLASSNAME", desc_prefix, _("VIP"));
					break;
				case eUC_OPERATOR:
					ReplaceVarInString(desc_prefix, "CLASSNAME", desc_prefix, _("Operator"));
					break;
				case eUC_CHEEF:
					ReplaceVarInString(desc_prefix, "CLASSNAME", desc_prefix, _("Cheef"));
					break;
				case eUC_ADMIN:
					ReplaceVarInString(desc_prefix, "CLASSNAME", desc_prefix, _("Administator"));
					break;
				case eUC_MASTER:
					ReplaceVarInString(desc_prefix, "CLASSNAME", desc_prefix, _("Master"));
					break;
				default:
					ReplaceVarInString(desc_prefix, "CLASSNAME", desc_prefix, _("Unknown"));
					break;
			}

			switch (tag->mClientMode) {
				case eCM_ACTIVE:
					ReplaceVarInString(desc_prefix, "MODE", desc_prefix, "A");
					break;
				case eCM_PASSIVE:
					ReplaceVarInString(desc_prefix, "MODE", desc_prefix, "P");
					break;
				case eCM_SOCK5:
					ReplaceVarInString(desc_prefix, "MODE", desc_prefix, "5");
					break;
				default: // eCM_OTHER, eCM_NOTAG
					ReplaceVarInString(desc_prefix, "MODE", desc_prefix, "?");
					break;
			}

			desc = desc_prefix + desc;
		}
	}

	delete tag; // tag is no longer used
	tag = NULL;
	speed = msg->ChunkString(eCH_MI_SPEED);

	if ((!mS->mC.show_speed) && !speed.empty()) // hide speed but keep status byte
		speed.assign(speed, speed.length() - 1, -1);

	if (!mS->mC.show_email) // hide email
		email = "";
	else
		email = conn->mpUser->mEmail;

	if (conn->mpUser->mHideShare) // hide share
		sShare = "0";
	else
		sShare = str_share;

	Create_MyINFO(myinfo_basic, nick, desc, speed, email, sShare);

	if ((conn->mpUser->mClass >= eUC_OPERATOR) && (mS->mC.show_tags < 3)) // ops have hidden myinfo
		myinfo_full = myinfo_basic;
	else
		Create_MyINFO(myinfo_full, nick, desc + sTag, speed, email, sShare);

	if (conn->mpUser->mInList) { // login or send to all
		/*
			send it to all only if
				its not too often
				it has changed since last time
				send only the version that has changed only to those who want it
		*/

		if (mS->MinDelay(conn->mpUser->mT.info, mS->mC.int_myinfo) && (myinfo_full != conn->mpUser->mMyINFO)) {
			conn->mpUser->mMyINFO = myinfo_full;

			if (myinfo_basic != conn->mpUser->mMyINFO_basic) {
				conn->mpUser->mMyINFO_basic = myinfo_basic;
				string send_info;
				send_info = GetMyInfo(conn->mpUser, eUC_NORMUSER);
				mS->mUserList.SendToAll(send_info, mS->mC.delayed_myinfo, true);
			}

			if (mS->mC.show_tags >= 1)
				mS->mOpchatList.SendToAll(myinfo_full, mS->mC.delayed_myinfo, true);
		}
	} else { // user logs in for the first time
		conn->mpUser->mMyINFO = myinfo_full; // keep it
		conn->mpUser->mMyINFO_basic = myinfo_basic;
		conn->SetLSFlag(eLS_MYINFO);

		if (!mS->BeginUserLogin(conn)) // if all right, add user to userlist, if not yet there
			return -1;
	}

	conn->ClearTimeOut(eTO_MYINFO);
	return 0;
}

int cDCProto::DC_GetINFO(cMessageDC *msg, cConnDC *conn)
{
	if (CheckUserLogin(conn))
		return -1;

	if (CheckProtoSyntax(conn, msg))
		return -1;

	string &nick = msg->ChunkString(eCH_GI_NICK);

	if (CheckUserNick(conn, nick))
		return -1;

	if (conn->CheckProtoFlood(msg->mStr, ePF_GETINFO)) // protocol flood
		return -1;

	string omsg;
	string &other = msg->ChunkString(eCH_GI_OTHER); // find other user
	cUser *user = mS->mUserList.GetUserByNick(other);

	if (!user) {
		if ((other != mS->mC.hub_security) && (other != mS->mC.opchat_name)) {
			Create_Quit(omsg, other);
			conn->Send(omsg, true);
		}

		return -2;
	}

	if ((conn->mpUser->mT.login < user->mT.login) && (mS->mTime < (user->mT.login + 60))) // if user logged in then ignore it, client is dcgui and one myinfo sent already
		return 0;

	if (mS->mC.optimize_userlist == eULO_GETINFO) {
		conn->mpUser->mQueueUL.append(other);
		conn->mpUser->mQueueUL.append("|");
	} else if (!(conn->mFeatures & eSF_NOGETINFO)) { // send it
		omsg = GetMyInfo(user, conn->mpUser->mClass);
		conn->Send(omsg, true, false);
	}

	return 0;
}

int cDCProto::DC_To(cMessageDC *msg, cConnDC *conn)
{
	if (CheckUserLogin(conn))
		return -1;

	if (CheckProtoSyntax(conn, msg))
		return -1;

	string &from = msg->ChunkString(eCH_PM_FROM);
	string &nick = msg->ChunkString(eCH_PM_NICK);

	if (CheckUserNick(conn, from) || CheckUserNick(conn, nick))
		return -1;

	if (conn->CheckProtoFlood(msg->mStr, ePF_PRIV)) // protocol flood
		return -1;

	if (mS->CheckProtoFloodAll(conn, msg, ePFA_PRIV)) // protocol flood from all
		return -1;

	string &to = msg->ChunkString(eCH_PM_TO);
	cUser *other = mS->mUserList.GetUserByNick(to); // find other user

	if (!other) // todo: notify user
		return -2;

	if (!other->mxConn && mS->mRobotList.ContainsNick(to)) { // parse for commands to bot
		((cUserRobot*)mS->mRobotList.GetUserBaseByNick(to))->ReceiveMsg(conn, msg);
		return 0;
	}

	if (!conn->mpUser->Can(eUR_PM, mS->mTime.Sec(), 0)) // todo: notify user
		return -4;

	ostringstream os;

	if (conn->mpUser->mClass < mS->mC.private_class) {
		os << _("Private chat is currently disabled for users with your class.");
		mS->DCPrivateHS(os.str(), conn);
		mS->DCPublicHS(os.str(), conn);
		return 0;
	}

	if ((conn->mpUser->mClass + mS->mC.classdif_pm) < other->mClass) {
		os << _("You can't talk to this user.");
		mS->DCPrivateHS(os.str(), conn);
		mS->DCPublicHS(os.str(), conn);
		return -4;
	}

	string &text = msg->ChunkString(eCH_PM_MSG);
	cUser::tFloodHashType Hash = 0;
	Hash = tHashArray<void*>::HashString(text);

	if (Hash && (conn->mpUser->mClass < eUC_OPERATOR)) { // check for same message flood
		if (Hash == conn->mpUser->mFloodHashes[eFH_PM]) {
			if (conn->mpUser->mFloodCounters[eFC_PM]++ > mS->mC.max_flood_counter_pm) {
				mS->DCPrivateHS(_("Private message flood detected from your client."), conn);
				os << autosprintf(_("Same message flood in PM: %s"), text.c_str());
				mS->ReportUserToOpchat(conn, os.str());
				conn->CloseNow();
				return -5;
			}
		} else
			conn->mpUser->mFloodCounters[eFC_PM] = 0;
	}

	conn->mpUser->mFloodHashes[eFH_PM] = Hash;

	#ifndef WITHOUT_PLUGINS
		if (!mS->mCallBacks.mOnParsedMsgPM.CallAll(conn, msg))
			return 0;
	#endif

	if (other->mxConn) // send it
		other->mxConn->Send(msg->mStr);

	return 0;
}

int cDCProto::DC_MCTo(cMessageDC *msg, cConnDC *conn)
{
	if (CheckUserLogin(conn))
		return -1;

	if (CheckProtoSyntax(conn, msg))
		return -1;

	string &from = msg->ChunkString(eCH_MCTO_FROM);
	string &nick = msg->ChunkString(eCH_MCTO_NICK);

	if (CheckUserNick(conn, from) || CheckUserNick(conn, nick))
		return -1;

	if (conn->CheckProtoFlood(msg->mStr, ePF_MCTO)) // protocol flood
		return -1;

	if (mS->CheckProtoFloodAll(conn, msg, ePFA_MCTO)) // protocol flood from all
		return -1;

	string &to = msg->ChunkString(eCH_MCTO_TO);
	cUser *other = mS->mUserList.GetUserByNick(to); // find other user

	if (!other) // todo: notify user
		return -2;

	if (!conn->mpUser->Can(eUR_PM, mS->mTime.Sec(), 0)) // todo: notify user
		return -4;

	ostringstream os;

	if (conn->mpUser->mClass < mS->mC.private_class) { // pm rules also apply on mcto, messages are also private
		os << _("Private chat is currently disabled for users with your class.");
		mS->DCPrivateHS(os.str(), conn);
		mS->DCPublicHS(os.str(), conn);
		return 0;
	}

	if ((conn->mpUser->mClass + mS->mC.classdif_mcto) < other->mClass) {
		os << _("You can't talk to this user.");
		mS->DCPrivateHS(os.str(), conn);
		mS->DCPublicHS(os.str(), conn);
		return -4;
	}

	string &text = msg->ChunkString(eCH_MCTO_MSG);
	cUser::tFloodHashType Hash = 0;
	Hash = tHashArray<void*>::HashString(text);

	if (Hash && (conn->mpUser->mClass < eUC_OPERATOR)) { // check for same message flood
		if (Hash == conn->mpUser->mFloodHashes[eFH_MCTO]) {
			if (conn->mpUser->mFloodCounters[eFC_MCTO]++ > mS->mC.max_flood_counter_mcto) {
				mS->DCPrivateHS(_("Private message flood detected from your client."), conn);
				os << autosprintf(_("Same message flood in MCTo: %s"), text.c_str());
				mS->ReportUserToOpchat(conn, os.str());
				conn->CloseNow();
				return -5;
			}
		} else
			conn->mpUser->mFloodCounters[eFC_MCTO] = 0;
	}

	conn->mpUser->mFloodHashes[eFH_MCTO] = Hash;

	#ifndef WITHOUT_PLUGINS
		if (!mS->mCallBacks.mOnParsedMsgMCTo.CallAll(conn, msg))
			return 0;
	#endif

	if (other->mxConn) { // send it
		string mcto;

		if (other->mxConn->mFeatures & eSF_MCTO) { // send as is if supported by client
			mcto = msg->mStr;
		} else { // else convert to private main chat message
			mcto.erase();
			Create_Chat(mcto, conn->mpUser->mNick, text);
		}

		other->mxConn->Send(mcto);
	}

	return 0;
}

int cDCProto::DC_Chat(cMessageDC *msg, cConnDC *conn)
{
	if (CheckUserLogin(conn))
		return -1;

	if (CheckProtoSyntax(conn, msg))
		return -1;

	string &nick = msg->ChunkString(eCH_CH_NICK);

	if (CheckUserNick(conn, nick))
		return -1;

	if (conn->CheckProtoFlood(msg->mStr, ePF_CHAT)) // protocol flood
		return -1;

	if (mS->CheckProtoFloodAll(conn, msg, ePFA_CHAT)) // protocol flood from all
		return -1;

	ostringstream os;

	// check if delay is ok
	if (conn->mpUser->mClass < eUC_VIPUSER) {
		long delay = mS->mC.int_chat_ms;

		if (!mS->MinDelayMS(conn->mpUser->mT.chat, delay)) {
			cTime diff = mS->mTime - conn->mpUser->mT.chat;
			os << autosprintf(_("Not sent because minimum chat delay is %lu ms but you made %s."), delay, diff.AsPeriod().AsString().c_str());
			mS->DCPublicHS(os.str(), conn);
			return 0;
		}
	}

	// check for commands
	string &text = msg->ChunkString(eCH_CH_MSG);
	if (ParseForCommands(text, conn, 0)) return 0;

	// check if user is allowed to use main chat
	if (!conn->mpUser->Can(eUR_CHAT, mS->mTime.Sec(), 0)) return -4;

	if (conn->mpUser->mClass < mS->mC.mainchat_class) {
		mS->DCPublicHS(_("Main chat is currently disabled for users with your class."), conn);
		return 0;
	}

	// check flood
	cUser::tFloodHashType Hash = 0;
	Hash = tHashArray<void*>::HashString(msg->mStr);
	if (Hash && (conn->mpUser->mClass < eUC_OPERATOR) && (Hash == conn->mpUser->mFloodHashes[eFH_CHAT])) return -5;
	conn->mpUser->mFloodHashes[eFH_CHAT] = Hash;

	// check message length
	if (conn->mpUser->mClass < eUC_VIPUSER && !cDCProto::CheckChatMsg(text, conn)) return 0;

	// if this is a kick message, process it separately
	if ((mKickChatPattern.Exec(text) >= 4) && (!mKickChatPattern.PartFound(1) || (mKickChatPattern.Compare(2, text, conn->mpUser->mNick) == 0))) {
		if (conn->mpUser->mClass >= eUC_OPERATOR) {
			string kick_reason, other;
			mKickChatPattern.Extract(4, text, kick_reason);
			mKickChatPattern.Extract(3, text, other);
			mS->DCKickNick(NULL, conn->mpUser, other, kick_reason, eKCK_Reason);
		}

		return 0;
	}

	#ifndef WITHOUT_PLUGINS
	if (!mS->mCallBacks.mOnParsedMsgChat.CallAll(conn, msg)) return 0;
	#endif

	// finally send the message
	mS->mChatUsers.SendToAll(msg->mStr);
	return 0;
}

int cDCProto::DC_ConnectToMe(cMessageDC *msg, cConnDC *conn)
{
	if (CheckUserLogin(conn))
		return -1;

	if (CheckProtoSyntax(conn, msg))
		return -1;

	if (conn->CheckProtoFlood(msg->mStr, ePF_CTM)) // protocol flood
		return -1;

	ostringstream os;
	string &nick = msg->ChunkString(eCH_CM_NICK); // find other user
	cUser *other = mS->mUserList.GetUserByNick(nick);

	if (!other || !other->mInList) {
		os << autosprintf(_("You're trying connect to user that is not online: %s"), nick.c_str());
		mS->DCPublicHS(os.str(), conn);
		return -1;
	}

	if (!other->mxConn) {
		os << autosprintf(_("You're trying connect to user that is bot: %s"), nick.c_str());
		mS->DCPublicHS(os.str(), conn);
		return -1;
	}

	if (conn->mpUser->mClass < mS->mC.min_class_use_hub) { // check use hub class
		if (!conn->mpUser->mHideCtmMsg) {
			os << autosprintf(_("You can't download unless you are registered with class: %d"), mS->mC.min_class_use_hub);
			mS->DCPublicHS(os.str(), conn);
		}

		return -4;
	}

	unsigned long use_hub_share = 0; // check use hub share

	switch (conn->mpUser->mClass) {
		case eUC_NORMUSER:
			use_hub_share = mS->mC.min_share_use_hub;
			break;
		case eUC_REGUSER:
			use_hub_share = mS->mC.min_share_use_hub_reg;
			break;
		case eUC_VIPUSER:
			use_hub_share = mS->mC.min_share_use_hub_vip;
			break;
	}

	use_hub_share = use_hub_share * 1024 * 1024;

	if (conn->mpUser->mShare < use_hub_share) {
		if (!conn->mpUser->mHideCtmMsg) {
			os << autosprintf(_("You can't download unless you share: %s"), convertByte(use_hub_share, false).c_str());
			mS->DCPublicHS(os.str(), conn);
		}

		return -4;
	}

	if (!conn->mpUser->Can(eUR_CTM, mS->mTime.Sec(), 0)) // check temporary right
		return -4; // todo: notify user

	if (((conn->mpUser->mClass + mS->mC.classdif_download) < other->mClass) || ((conn->mpUser->mClass < eUC_OPERATOR) && other->mHideShare)) { // check class difference and hidden share
		if (!conn->mpUser->mHideCtmMsg) {
			os << autosprintf(_("You can't download from this user: %s"), nick.c_str());
			mS->DCPublicHS(os.str(), conn);
		}

		return -4;
	}

	if (mS->mC.filter_lan_requests && (isLanIP(conn->mAddrIP) != isLanIP(other->mxConn->mAddrIP))) { // filter lan to wan and reverse
		os << autosprintf(_("You can't download from user because one of you is in LAN: %s"), nick.c_str());
		mS->DCPublicHS(os.str(), conn);
		return -1;
	}

	string &port = msg->ChunkString(eCH_CM_PORT);

	if (port.empty() || (port.size() > 6))
		return -1;

	string extra("");

	if (port[port.size() - 1] == 'S') { // secure connection
		if (conn->mFeatures & eSF_TLS) // only if sender supports it
			extra = 'S';

		port.assign(port, 0, port.size() - 1);
	}

	__int64 iport = StringAsLL(port);

	if ((iport < 1) || (iport > 65535))
		return -1;

	#ifndef WITHOUT_PLUGINS
		if (!mS->mCallBacks.mOnParsedMsgConnectToMe.CallAll(conn, msg))
			return -2;
	#endif

	string &addr = msg->ChunkString(eCH_CM_IP);

	if (!CheckIP(conn, addr)) {
		if (conn->Log(3))
			conn->LogStream() << "Fixed wrong IP in $ConnectToMe: " << addr << endl;

		addr = conn->mAddrIP;
	}

	string ctm = "$ConnectToMe ";
	ctm += nick;
	ctm += ' ';
	ctm += addr;
	ctm += ':';
	ctm += StringFrom(iport);
	ctm += extra;

	other->mxConn->Send(ctm, true); // send it
	return 0;
}

int cDCProto::DC_RevConnectToMe(cMessageDC *msg, cConnDC *conn)
{
	if (CheckUserLogin(conn))
		return -1;

	if (CheckProtoSyntax(conn, msg))
		return -1;

	string &mynick = msg->ChunkString(eCH_RC_NICK);

	if (CheckUserNick(conn, mynick))
		return -1;

	if (conn->CheckProtoFlood(msg->mStr, ePF_RCTM)) // protocol flood
		return -1;

	ostringstream os;
	string &nick = msg->ChunkString(eCH_RC_OTHER); // find other user
	cUser *other = mS->mUserList.GetUserByNick(nick);

	if (!other || !other->mInList) {
		os << autosprintf(_("You're trying connect to user that is not online: %s"), nick.c_str());
		mS->DCPublicHS(os.str(), conn);
		return -2;
	}

	if (!other->mxConn) {
		os << autosprintf(_("You're trying connect to user that is bot: %s"), nick.c_str());
		mS->DCPublicHS(os.str(), conn);
		return -2;
	}

	if (other->IsPassive) { // passive request to passive user
		os << autosprintf(_("You can't download from user because he is in passive mode: %s"), nick.c_str());
		mS->DCPublicHS(os.str(), conn);
		return -2;
	}

	if (conn->mpUser->mHideShare) { // when my share is hidden other users cant connect to me
		os << autosprintf(_("You can't download from user because your share is hidden: %s"), nick.c_str());
		mS->DCPublicHS(os.str(), conn);
		return -2;
	}

	if (conn->mpUser->mClass < mS->mC.min_class_use_hub_passive) { // check use hub class
		if (!conn->mpUser->mHideCtmMsg) {
			os << autosprintf(_("You can't download unless you are registered with class: %d"), mS->mC.min_class_use_hub_passive);
			mS->DCPublicHS(os.str(), conn);
		}

		return -4;
	}

	unsigned long use_hub_share = 0; // check use hub share

	switch (conn->mpUser->mClass) {
		case eUC_NORMUSER:
			use_hub_share = mS->mC.min_share_use_hub;
			break;
		case eUC_REGUSER:
			use_hub_share = mS->mC.min_share_use_hub_reg;
			break;
		case eUC_VIPUSER:
			use_hub_share = mS->mC.min_share_use_hub_vip;
			break;
	}

	use_hub_share = use_hub_share * 1024 * 1024;

	if (conn->mpUser->mShare < use_hub_share) {
		if (!conn->mpUser->mHideCtmMsg) {
			os << autosprintf(_("You can't download unless you share: %s"), convertByte(use_hub_share, false).c_str());
			mS->DCPublicHS(os.str(), conn);
		}

		return -4;
	}

	if (!conn->mpUser->Can(eUR_CTM, mS->mTime.Sec(), 0)) // check temporary right
		return -4; // todo: notify user

	if ((conn->mpUser->mClass + mS->mC.classdif_download < other->mClass) || ((conn->mpUser->mClass < eUC_OPERATOR) && other->mHideShare)) { // check class difference and hidden share
		if (!conn->mpUser->mHideCtmMsg) {
			os << autosprintf(_("You can't download from this user: %s"), nick.c_str());
			mS->DCPublicHS(os.str(), conn);
		}

		return -4;
	}

	#ifndef WITHOUT_PLUGINS
		if (!mS->mCallBacks.mOnParsedMsgRevConnectToMe.CallAll(conn, msg))
			return -2;
	#endif

	other->mxConn->Send(msg->mStr, true); // send it
	return 0;
}

int cDCProto::DC_MultiConnectToMe(cMessageDC *msg, cConnDC *conn)
{
	if (CheckUserLogin(conn))
		return -1;

	// todo
	return 0;
}

int cDCProto::DC_Search(cMessageDC *msg, cConnDC *conn)
{
	if (CheckUserLogin(conn))
		return -1;

	if (CheckProtoSyntax(conn, msg))
		return -1;

	if (conn->CheckProtoFlood(msg->mStr, ePF_SEARCH)) // protocol flood
		return -1;

	ostringstream os;

	if (mS->mSysLoad >= (eSL_CAPACITY + conn->mpUser->mClass)) { // check hub load first of all
		if (mS->Log(3))
			mS->LogStream() << "Hub load is too high for search: " << mS->mSysLoad << endl;

		os << _("Sorry, hub load is too high to process your search request, please try again later.");
		mS->DCPublicHS(os.str(), conn);
		return -2;
	}

	bool passive;

	switch (msg->mType) { // detect search mode once
		case eDC_SEARCH:
		case eDC_MSEARCH:
			passive = false;
			break;
		case eDC_SEARCH_PAS:
		case eDC_MSEARCH_PAS:
			passive = true;
			break;
	}

	string addr;

	if (passive) { // verify sender
		addr = msg->ChunkString(eCH_PS_NICK);

		if (CheckUserNick(conn, addr))
			return -1;

		addr = "Hub:" + addr;
	} else {
		string &port = msg->ChunkString(eCH_AS_PORT);
		port.assign(port, 0, port.find_first_of(' ')); // solution for misparsed port

		if (port.empty() || (port.size() > 5))
			return -1;

		__int64 iport = StringAsLL(port);

		if ((iport < 1) || (iport > 65535))
			return -1;

		addr = msg->ChunkString(eCH_AS_IP);

		if (!CheckIP(conn, addr)) {
			if (conn->Log(3))
				conn->LogStream() << "Fixed wrong IP in $Search: " << addr << endl;

			addr = conn->mAddrIP;
		}

		addr += ':';
		addr += StringFrom(iport);
	}

	int delay;

	switch (conn->mpUser->mClass) { // prepare delay
		case eUC_REGUSER:
			delay = ((passive) ? mS->mC.int_search_reg_pas : mS->mC.int_search_reg);
			break;
		case eUC_VIPUSER:
			delay = mS->mC.int_search_vip;
			break;
		case eUC_OPERATOR:
		case eUC_CHEEF:
		case eUC_ADMIN:
			delay = mS->mC.int_search_op;
			break;
		case eUC_MASTER:
			delay = 0;
			break;
		default:
			delay = ((passive) ? mS->mC.int_search_pas : mS->mC.int_search);
			break;
	}

	if (!mS->MinDelay(conn->mpUser->mT.search, delay)) { // check delay
		if (conn->mpUser->mSearchNumber >= mS->mC.search_number) {
			os << autosprintf(_("Don't search too often, you can do %d searches in %d seconds."), mS->mC.search_number, delay);
			mS->DCPublicHS(os.str(), conn);
			return -1;
		}
	} else
		conn->mpUser->mSearchNumber = 0;

	if (conn->mpUser->mClass < eUC_OPERATOR) { // if not operator
		string patt; // check search length

		if (passive)
			patt = msg->ChunkString(eCH_PS_SEARCHPATTERN);
		else
			patt = msg->ChunkString(eCH_AS_SEARCHPATTERN);

		if (patt.size() < mS->mC.min_search_chars) {
			os << autosprintf(_("Minimum search length is: %d"), mS->mC.min_search_chars);
			mS->DCPublicHS(os.str(), conn);
			return -1;
		}

		cUser::tFloodHashType Hash = 0; // check same message flood
		Hash = tHashArray<void*>::HashString(msg->mStr);

		if (Hash && (Hash == conn->mpUser->mFloodHashes[eFH_SEARCH]))
			return -4; // silent filter, user might be using automatic search

		conn->mpUser->mFloodHashes[eFH_SEARCH] = Hash;
	}

	int use_hub_class = ((passive) ? mS->mC.min_class_use_hub_passive : mS->mC.min_class_use_hub); // check use hub class

	if (conn->mpUser->mClass < use_hub_class) {
		if (!conn->mpUser->mHideCtmMsg) {
			os << autosprintf(_("You can't search unless you are registered with class: %d"), use_hub_class);
			mS->DCPublicHS(os.str(), conn);
		}

		return -4;
	}

	unsigned long use_hub_share = 0; // check use hub share

	switch (conn->mpUser->mClass) {
		case eUC_NORMUSER:
			use_hub_share = mS->mC.min_share_use_hub;
			break;
		case eUC_REGUSER:
			use_hub_share = mS->mC.min_share_use_hub_reg;
			break;
		case eUC_VIPUSER:
			use_hub_share = mS->mC.min_share_use_hub_vip;
			break;
	}

	use_hub_share = use_hub_share * 1024 * 1024;

	if (conn->mpUser->mShare < use_hub_share) {
		if (!conn->mpUser->mHideCtmMsg) {
			os << autosprintf(_("You can't search unless you share: %s"), convertByte(use_hub_share, false).c_str());
			mS->DCPublicHS(os.str(), conn);
		}

		return -4;
	}

	if (!conn->mpUser->Can(eUR_SEARCH, mS->mTime.Sec(), 0)) // check temporary right
		return -4; // todo: notify user

 	#ifndef WITHOUT_PLUGINS
		if (!mS->mCallBacks.mOnParsedMsgSearch.CallAll(conn, msg))
			return -2;
	#endif

	conn->mpUser->mSearchNumber++; // set counter last of all

	string search = "$Search ";
	search += addr;
	search += ' ';

	if (passive) {
		conn->mSRCounter = 0;
		search += msg->ChunkString(eCH_PS_QUERY);
	} else
		search += msg->ChunkString(eCH_AS_QUERY);

	mS->SearchToAll(conn, search, passive); // send it

	/*
		send conditional and filtered search request because
			some users hide their share
			some users dont share anything
			pingers dont need to get any searches
			users dont need to get their own searches
			some users are in lan and others are in wan

		if (passive)
			mS->mActiveUsers.SendToAll(search, mS->mC.delayed_search);
		else
			mS->mUserList.SendToAll(search, mS->mC.delayed_search);
	*/

	return 0;
}

int cDCProto::DC_SR(cMessageDC *msg, cConnDC *conn)
{
	if (CheckUserLogin(conn))
		return -1;

	if (CheckProtoSyntax(conn, msg))
		return -1;

	string &from = msg->ChunkString(eCH_SR_FROM);

	if (CheckUserNick(conn, from))
		return -1;

	if (conn->CheckProtoFlood(msg->mStr, ePF_SR)) // protocol flood
		return -1;

	cUser *other = mS->mUserList.GetUserByNick(msg->ChunkString(eCH_SR_TO)); // find other user

	if (!other || !other->mxConn || !other->mInList)
		return -1; // silent filter

	string sr(msg->mStr, 0, msg->mChunks[eCH_SR_TO].first - 1); // cut the end

	#ifndef WITHOUT_PLUGINS
		if (!mS->mCallBacks.mOnParsedMsgSR.CallAll(conn, msg))
			return -2;
	#endif

	if (!mS->mC.max_passive_sr || (other->mxConn->mSRCounter++ < mS->mC.max_passive_sr)) // send it
		other->mxConn->Send(sr, true, !mS->mC.delayed_search); // part of search, must be delayed too

	return 0;
}

/*
	bot commands
*/

int cDCProto::DCB_BotINFO(cMessageDC *msg, cConnDC *conn)
{
	if (CheckUserLogin(conn, false))
		return -1;

	ostringstream os;

	if (!(conn->mFeatures & eSF_BOTINFO) && (conn->mpUser->mClass != eUC_PINGER)) { // not pinger
		os << _("Invalid login sequence, you didn't identify yourself as pinger.");

		if (conn->Log(1))
			conn->LogStream() << os.str() << endl;

		mS->ConnCloseMsg(conn, os.str(), 1000, eCR_LOGIN_ERR);
		return -1;
	}

	if (conn->GetLSFlag(eLS_BOTINFO)) { // already sent
		os << _("Invalid login sequence, you already sent pinger information.");

		if (conn->Log(1))
			conn->LogStream() << os.str() << endl;

		mS->ConnCloseMsg(conn, os.str(), 1000, eCR_LOGIN_ERR);
		return -1;
	}

	if (CheckProtoSyntax(conn, msg))
		return -1;

	os << autosprintf(_("Pinger entered the hub: %s"), msg->ChunkString(eCH_1_PARAM).c_str());

	if (conn->Log(2))
		conn->LogStream() << os.str() << endl;

	if (mS->mC.botinfo_report)
		mS->ReportUserToOpchat(conn, os.str());

	#ifndef WITHOUT_PLUGINS
		if (!mS->mCallBacks.mOnParsedMsgBotINFO.CallAll(conn, msg)) {
			conn->CloseNow();
			return -1;
		}
	#endif

	os.str("");
	os.clear();
	char sep = '$';
	char pipe = '|';
	cConnType *ConnType = mS->mConnTypes->FindConnType("default");
	__int64 minshare = mS->mC.min_share;

	if (mS->mC.min_share_use_hub > minshare)
		minshare = mS->mC.min_share_use_hub;

	if (mS->mC.hub_icon_url.size())
		os << "$SetIcon " << mS->mC.hub_icon_url << pipe;

	if (mS->mC.hub_logo_url.size())
		os << "$SetLogo " << mS->mC.hub_logo_url << pipe;

	os << "$HubINFO ";
	os << mS->mC.hub_name << sep;
	os << mS->mC.hub_host << sep;
	os << mS->mC.hub_desc << sep;
	os << mS->mC.max_users_total << sep;
	os << StringFrom((__int64)(1024 * 1024) * minshare) << sep;
	os << ((ConnType) ? ConnType->mTagMinSlots : 0) << sep;
	os << mS->mC.tag_max_hubs << sep;
	os << "Verlihub " << VERSION << sep;
	os << mS->mC.hub_owner << sep;
	os << mS->mC.hub_category << sep;
	os << mS->mC.hub_encoding;

	string info = os.str();
	conn->Send(info, true);
	conn->SetLSFlag(eLS_BOTINFO);
	return 0;
}

/*
	operator commands
*/

int cDCProto::DCO_UserIP(cMessageDC *msg, cConnDC *conn)
{
	if (CheckUserLogin(conn))
		return -1;

	if (CheckUserRights(conn, msg, (conn->mpUser->mClass >= mS->mC.user_ip_class)))
		return -1;

	if (CheckProtoSyntax(conn, msg))
		return -1;

	string &lst = msg->ChunkString(eCH_1_PARAM);

	if (lst.empty())
		return -1;

	string sep("$$");
	lst += sep;
	int pos;
	cUser *other;
	string userip;

	while (lst.find(sep) != string::npos) {
		pos = lst.find(sep);
		string nick = lst.substr(0, pos);

		if (!nick.empty()) {
			other = mS->mUserList.GetUserByNick(nick);

			if (other && other->mxConn && other->mxConn->mpUser && other->mxConn->mpUser->mInList) {
				userip.append(nick);
				userip.append(" ");
				userip.append(other->mxConn->AddrIP());
				userip.append(sep);
			}
		}

		lst = lst.substr(pos + sep.length());
	}

	if (!userip.empty()) {
		userip = "$UserIP " + userip;
		conn->Send(userip, true);
	}

	return 0;
}

int cDCProto::DCO_Kick(cMessageDC *msg, cConnDC *conn)
{
	if (CheckUserLogin(conn))
		return -1;

	if (CheckUserRights(conn, msg, conn->mpUser->Can(eUR_KICK, mS->mTime.Sec())))
		return -1;

	if (CheckProtoSyntax(conn, msg))
		return -1;

	mS->DCKickNick(NULL, conn->mpUser, msg->ChunkString(eCH_1_PARAM), mS->mEmpty, eKCK_Drop | eKCK_TBAN); // try to kick user
	return 0;
}

int cDCProto::DCO_OpForceMove(cMessageDC *msg, cConnDC *conn)
{
	if (CheckUserLogin(conn))
		return -1;

	if (CheckUserRights(conn, msg, (conn->mpUser->mClass >= mS->mC.min_class_redir)))
		return -1;

	if (CheckProtoSyntax(conn, msg))
		return -1;

	ostringstream os;
	string &nick = msg->ChunkString(eCH_FM_NICK); // find other user
	cUser *other = mS->mUserList.GetUserByNick(nick);

	if (!other || !other->mInList) {
		os << autosprintf(_("You're trying to redirect user that is not online: %s"), nick.c_str());
		mS->DCPublicHS(os.str(), conn);
		return -2;
	}

	if (!other->mxConn) {
		os << autosprintf(_("You're trying to redirect user that is bot: %s"), nick.c_str());
		mS->DCPublicHS(os.str(), conn);
		return -2;
	}

	if ((other->mClass >= conn->mpUser->mClass) || (other->mProtectFrom >= conn->mpUser->mClass)) { // check protection
		os << autosprintf(_("You're trying to redirect user that is protected: %s"), nick.c_str());
		mS->DCPublicHS(os.str(), conn);
		return -3;
	}

	string &dest = msg->ChunkString(eCH_FM_DEST);

	if (dest.empty()) { // check address
		os << _("Please specify valid redirect address.");
		mS->DCPublicHS(os.str(), conn);
		return -3;
	}

	if (conn->Log(2)) // log it
		conn->LogStream() << "Redirecting " << nick << " to: " << dest << endl;

	string ofm;
	os << autosprintf(_("You are being redirected to %s because: %s"), dest.c_str(), msg->ChunkString(eCH_FM_REASON).c_str());
	Create_PM(ofm, conn->mpUser->mNick, nick, conn->mpUser->mNick, os.str());
	ofm += "|$ForceMove "; // must be last, user might not get reason otherwise
	ofm += dest;

	other->mxConn->Send(ofm, true); // send it
	other->mxConn->CloseNice(5000, eCR_FORCEMOVE); // close after a while, user might not get redirect otherwise
	return 0;
}

int cDCProto::DCO_TempBan(cMessageDC *msg, cConnDC *conn)
{
	if (CheckUserLogin(conn))
		return -1;

	if (CheckUserRights(conn, msg, (conn->mpUser->mClass >= eUC_OPERATOR)))
		return -1;

	if (CheckProtoSyntax(conn, msg))
		return -1;

	ostringstream os;

	// todo: i think ban is not supported here, user will get bad syntax
	long period = 0;
	// calculate time
	if(msg->ChunkString(eCH_NB_TIME).size()) {
		mS->Str2Period(msg->ChunkString(eCH_NB_TIME),os);
		if(!period) {
			mS->DCPublicHS(os.str(),conn);
			return -1;
		}
	}

	cUser *other = mS->mUserList.GetUserByNick(msg->ChunkString(eCH_NB_NICK));
	if(!other) {
		os << autosprintf(_("User %s not found."), msg->ChunkString(eCH_NB_NICK).c_str());
		mS->DCPublicHS(os.str(),conn);
		return -1;
	}

	if(msg->mType == eDCO_TBAN  && !msg->ChunkString(eCH_NB_REASON).size()) {
		os << _("Please provide a valid reason.");
		mS->DCPublicHS(os.str(),conn);
		return -1;
	}

	if(other->mClass >= conn->mpUser->mClass || other->mProtectFrom >= conn->mpUser->mClass) {
		os << _("You can't ban a protected user or with higher privilegies.");
		mS->DCPublicHS(os.str(),conn);
		return -1;
	}

	if(!other->mxConn) {
		os << _("You can't ban a robot.");
		mS->DCPublicHS(os.str(),conn);
		return -1;
	}

	if(period)
		os << autosprintf(_("You are being temporarily banned for %s because %s"), msg->ChunkString(eCH_NB_TIME).c_str(), msg->ChunkString(eCH_NB_REASON).c_str());
	else
		os << autosprintf(_("You are banned because %s"), msg->ChunkString(eCH_NB_REASON).c_str());

	mS->DCPrivateHS(os.str(), other->mxConn, &conn->mpUser->mNick, &conn->mpUser->mNick);
	os.str(mS->mEmpty);

	cBan ban(mS);
	mS->mBanList->NewBan(ban, other->mxConn, conn->mpUser->mNick, msg->ChunkString(eCH_NB_REASON), period, eBF_NICKIP);
	mS->mBanList->AddBan(ban);

	mS->DCKickNick(NULL, conn->mpUser, msg->ChunkString(eCH_NB_NICK), mS->mEmpty, eKCK_Drop);

	ban.DisplayKick(os);
	mS->DCPublicHS(os.str(),conn);
	other->mxConn->CloseNice(1000, eCR_KICKED);
	return 0;
}

int cDCProto::DCO_UnBan(cMessageDC *msg, cConnDC *conn)
{
	if (CheckUserLogin(conn))
		return -1;

	if (CheckUserRights(conn, msg, (conn->mpUser->mClass >= eUC_OPERATOR)))
		return -1;

	if (CheckProtoSyntax(conn, msg))
		return -1;

	ostringstream os;
	string ip, nick/*, host*/;

	if(msg->mType == eDCO_UNBAN)
		ip = msg->ChunkString(eCH_1_PARAM);

	int n = mS->mBanList->DeleteAllBansBy(ip, nick , eBF_NICKIP);

	if(n <= 0) {
		os << autosprintf(_("No banned user found with ip %s."), msg->ChunkString(eCH_1_PARAM).c_str());
		mS->DCPublicHS(os.str().c_str(),conn);
		return -1;
	}
	os << autosprintf(_("Removed %d bans."), n) << endl;
	mS->DCPublicHS(os.str().c_str(),conn);
	return 0;
}

int cDCProto::DCO_GetBanList(cMessageDC *msg, cConnDC *conn)
{
	if (CheckUserLogin(conn))
		return -1;

	if (CheckUserRights(conn, msg, (conn->mpUser->mClass >= eUC_OPERATOR)))
		return -1;

	// todo: mS->mBanList->GetBanList(conn);
	return 0;
}

int cDCProto::DCO_WhoIP(cMessageDC *msg, cConnDC *conn)
{
	if (CheckUserLogin(conn))
		return -1;

	if (CheckUserRights(conn, msg, (conn->mpUser->mClass >= eUC_OPERATOR)))
		return -1;

	if (CheckProtoSyntax(conn, msg))
		return -1;

	string &ip = msg->ChunkString(eCH_1_PARAM);
	string nicklist("$UsersWithIP "), sep("$$");
	nicklist += ip;
	nicklist += "$";
	unsigned long num = cBanList::Ip2Num(ip);
	mS->WhoIP(num, num, nicklist, sep, true);
	conn->Send(nicklist);
	return 0;
}

int cDCProto::DCO_GetTopic(cMessageDC *msg, cConnDC *conn)
{
	if (CheckUserLogin(conn))
		return -1;

	if (CheckUserRights(conn, msg, (conn->mpUser->mClass >= eUC_OPERATOR)))
		return -1;

	string topic("$HubTopic ");

	if (mS->mC.hub_topic.size())
		topic += mS->mC.hub_topic;

	conn->Send(topic);
	return 0;
}

int cDCProto::DCO_SetTopic(cMessageDC *msg, cConnDC *conn)
{
	if (CheckUserLogin(conn))
		return -1;

	if (CheckUserRights(conn, msg, (conn->mpUser->mClass >= mS->mC.topic_mod_class)))
		return -1;

	if (CheckProtoSyntax(conn, msg))
		return -1;

	string &topic = msg->ChunkString(eCH_1_PARAM);
	mS->mC.hub_topic = topic;
	ostringstream os;
	os << autosprintf(_("Topis is set to: %s"), topic.c_str());
	mS->DCPublicHS(os.str(), conn);
	return 0;
}

/*
	unknown commands
*/

int cDCProto::DCU_Unknown(cMessageDC *msg, cConnDC *conn)
{
	if (conn->CheckProtoFlood(msg->mStr, ((msg->mStr.size()) ? ePF_UNKNOWN : ePF_PING))) // protocol flood
		return -1;

	#ifndef WITHOUT_PLUGINS
		// todo: discardable callback
		mS->mCallBacks.mOnUnknownMsg.CallAll(conn, msg);
	#endif

	return 0;
}

/*
	client commands
*/

int cDCProto::DCC_MyNick(cMessageDC *msg, cConnDC *conn)
{
	if (!mS->mC.detect_ctmtohub)
		return this->DCU_Unknown(msg, conn);

	if (!conn->mMyNick.empty()) {
		conn->CloseNow();
		return -1;
	}

	if (msg->SplitChunks()) {
		conn->CloseNow();
		return -1;
	}

	string &nick = msg->ChunkString(eCH_1_PARAM);

	if (nick.empty()) {
		conn->CloseNow();
		return -1;
	}

	conn->mMyNick = nick;
	return 0;
}

int cDCProto::DCC_Lock(cMessageDC *msg, cConnDC *conn)
{
	if (!mS->mC.detect_ctmtohub)
		return this->DCU_Unknown(msg, conn);

	if (conn->mMyNick.empty()) {
		conn->CloseNow();
		return -1;
	}

	if (msg->SplitChunks()) {
		conn->CloseNow();
		return -1;
	}

	string &lock = msg->ChunkString(eCH_1_PARAM);

	if (lock.empty()) {
		conn->CloseNow();
		return -1;
	}

	string ref;
	ParseReferer(lock, ref); // parse referer

	#ifndef WITHOUT_PLUGINS
		if (!mS->mCallBacks.mOnCtmToHub.CallAll(conn, (string*)&ref)) {
			conn->CloseNice(500, eCR_NOREDIR);
			return -1;
		}
	#endif

	mS->CtmToHubAddItem(conn, ref);
	string omsg("$Error CTM2HUB|"); // notify client
	conn->Send(omsg, false, true);
	conn->CloseNice(500, eCR_NOREDIR); // wait before closing
	return 0;
}

/*
	helper functions
*/

bool cDCProto::CheckUserLogin(cConnDC *conn, bool inlist)
{
	if (conn->mpUser && (conn->mpUser->mInList || !inlist))
		return false;

	string rsn = _("Invalid login sequence, your client must validate nick first.");

	if (conn->Log(1))
		conn->LogStream() << rsn << endl;

	mS->ConnCloseMsg(conn, rsn, 1000, eCR_LOGIN_ERR);
	return true;
}

bool cDCProto::CheckUserRights(cConnDC *conn, cMessageDC *msg, bool cond)
{
	if (cond)
		return false;

	string cmd;

	switch (msg->mType) {
		case eDCO_USERIP:
			cmd = "UserIP";
			break;
		case eDCO_OPFORCEMOVE:
			cmd = "OpForceMove";
			break;
		case eDCO_KICK:
			cmd = "Kick";
			break;
		case eDCO_BAN:
			cmd = "Ban";
			break;
		case eDCO_TBAN:
			cmd = "TempBan";
			break;
		case eDCO_UNBAN:
			cmd = "UnBan";
			break;
		case eDCO_GETBANLIST:
			cmd = "GetBanList";
			break;
		case eDCO_WHOIP:
			cmd = "WhoIP";
			break;
		case eDCO_GETTOPIC:
			cmd = "GetTopic";
			break;
		case eDCO_SETTOPIC:
			cmd = "SetTopic";
			break;
		default:
			cmd = _("Unknown");
			break;
	}

	ostringstream os;
	os << autosprintf(_("You have no rights to perform following operator action: %s"), cmd.c_str());

	if (conn->Log(1))
		conn->LogStream() << os.str() << endl;

	mS->ConnCloseMsg(conn, os.str(), 1000, eCR_SYNTAX);
	return true;
}

bool cDCProto::CheckProtoSyntax(cConnDC *conn, cMessageDC *msg)
{
	if (!msg->SplitChunks())
		return false;

	string cmd;

	switch (msg->mType) {
		case eDC_KEY:
			cmd = "Key";
			break;
		case eDC_VALIDATENICK:
			cmd = "ValidateNick";
			break;
		case eDC_MYPASS:
			cmd = "MyPass";
			break;
		case eDC_VERSION:
			cmd = "Version";
			break;
		case eDC_MYINFO:
			cmd = "MyINFO";
			break;
		case eDCO_USERIP:
			cmd = "UserIP";
			break;
		case eDC_MCONNECTTOME:
		case eDC_CONNECTTOME:
			cmd = "ConnectToMe";
			break;
		case eDC_RCONNECTTOME:
			cmd = "RevConnectToMe";
			break;
		case eDC_TO:
			cmd = "To";
			break;
		case eDC_MCTO:
			cmd = "MCTo";
			break;
		case eDC_CHAT:
			cmd = _("Chat");
			break;
		case eDCO_OPFORCEMOVE:
			cmd = "OpForceMove";
			break;
		case eDCO_KICK:
			cmd = "Kick";
			break;
		case eDC_MSEARCH:
		case eDC_MSEARCH_PAS:
		case eDC_SEARCH:
		case eDC_SEARCH_PAS:
			cmd = "Search";
			break;
		case eDC_SR:
			cmd = "SR";
			break;
		case eDC_SUPPORTS:
			cmd = "Supports";
			break;
		case eDCO_BAN:
			cmd = "Ban";
			break;
		case eDCO_TBAN:
			cmd = "TempBan";
			break;
		case eDCO_UNBAN:
			cmd = "UnBan";
			break;
		case eDCO_WHOIP:
			cmd = "WhoIP";
			break;
		case eDCO_SETTOPIC:
			cmd = "SetTopic";
			break;
		case eDCB_BOTINFO:
			cmd = "BotINFO";
			break;
		default:
			cmd = _("Unknown");
			break;
	}

	ostringstream os;
	os << autosprintf(_("Your client sent malformed protocol command: %s"), cmd.c_str());

	if (conn->Log(1))
		conn->LogStream() << os.str() << endl;

	mS->ConnCloseMsg(conn, os.str(), 1000, eCR_SYNTAX);
	return true;
}

bool cDCProto::CheckUserNick(cConnDC *conn, const string &nick)
{
	if (conn->mpUser->mNick == nick)
		return false;

	ostringstream os;
	os << autosprintf(_("Nick spoofing attempt detected from your client: %s"), nick.c_str());

	if (conn->Log(1))
		conn->LogStream() << os.str() << endl;

	mS->ConnCloseMsg(conn, os.str(), 1000, eCR_SYNTAX);
	return true;
}

int cDCProto::NickList(cConnDC *conn)
{
	try {
		bool complete_infolist = false;

		if (mS->mC.show_tags >= 2) // 2 = show to all
			complete_infolist = true;

		if (conn->mpUser && (conn->mpUser->mClass >= eUC_OPERATOR))
			complete_infolist = true;

		if (mS->mC.show_tags == 0) // 0 = hide to all
			complete_infolist = false;

		if (conn->GetLSFlag(eLS_LOGIN_DONE) != eLS_LOGIN_DONE)
			conn->mNickListInProgress = true;

		if (conn->mFeatures & eSF_NOHELLO) {
			if (conn->Log(3))
				conn->LogStream() << "Sending MyINFO list" << endl;

			conn->Send(mS->mUserList.GetInfoList(complete_infolist), true);
		} else if (conn->mFeatures & eSF_NOGETINFO) {
			if (conn->Log(3))
				conn->LogStream() << "Sending MyINFO list" << endl;

			conn->Send(mS->mUserList.GetNickList(), true);
			conn->Send(mS->mUserList.GetInfoList(complete_infolist), true);
		} else {
			if (conn->Log(3))
				conn->LogStream() << "Sending Nicklist" << endl;

			conn->Send(mS->mUserList.GetNickList(), true);
		}

		if (mS->mOpList.Size()) // send $OpList
			conn->Send(mS->mOpList.GetNickList(), true);

		if (mS->mRobotList.Size() && (conn->mFeatures & eSF_BOTLIST)) // send $BotList
			conn->Send(mS->mRobotList.GetNickList(), true);

		if (mS->mC.send_user_ip && (conn->mFeatures & eSF_USERIP2) && conn->mpUser) { // send $UserIP
			if (conn->mpUser->mClass >= eUC_OPERATOR)
				conn->Send(mS->mUserList.GetIPList(), true);
			else {
				string uip;
				cCompositeUserCollection::ufDoIpList DoUserIP(uip);
				DoUserIP.Clear();
				DoUserIP(conn->mpUser);
				conn->Send(uip, true);
			}
		}
	} catch(...) {
		if (conn->ErrLog(2))
			conn->LogStream() << "Exception in cDCProto::NickList" << endl;

		conn->CloseNow();
		return -1;
	}

	return 0;
}

int cDCProto::ParseForCommands(const string &text, cConnDC *conn, int pm)
{
	ostringstream omsg;

	// operator commands

	if (conn->mpUser->mClass >= eUC_OPERATOR && mS->mC.cmd_start_op.find_first_of(text[0]) != string::npos) {
		#ifndef WITHOUT_PLUGINS
		if ((mS->mCallBacks.mOnOperatorCommand.CallAll(conn, (string*)&text)) && (mS->mCallBacks.mOnHubCommand.CallAll(conn, (string*)&text, 1, pm)))
		#endif
		{
			if (!mS->mCo->OpCommand(text, conn) && !mS->mCo->UsrCommand(text, conn)) {
				omsg << autosprintf(_("Unknown hub command: %s"), text.c_str());
				mS->DCPublicHS(omsg.str(), conn);
			}
		}

		return 1;
	}

	// user commands

	if (mS->mC.cmd_start_user.find_first_of(text[0]) != string::npos) {
		#ifndef WITHOUT_PLUGINS
		if ((mS->mCallBacks.mOnUserCommand.CallAll(conn, (string*)&text)) && (mS->mCallBacks.mOnHubCommand.CallAll(conn, (string*)&text, 0, pm)))
		#endif
		{
			if (!mS->mCo->UsrCommand(text, conn)) {
				omsg << autosprintf(_("Unknown hub command: %s"), text.c_str());
				mS->DCPublicHS(omsg.str(), conn);
			}
		}

		return 1;
	}

	return 0;
}

bool cDCProto::CheckIP(cConnDC *conn, string &ip)
{
	if (conn->mAddrIP == ip) {
		return true;
	}

	if (conn->mRegInfo && (conn->mRegInfo->mAlternateIP == ip)) {
		return true;
	}

	return false;
}

bool cDCProto::isLanIP(string ip)
{
	if (ip.substr(0, 4) == "127.")
		return true;

	unsigned long lip = cBanList::Ip2Num(ip);

	if ((lip >= 167772160UL && lip <= 184549375UL) || (lip >= 2886729728UL && lip <= 2887778303UL) || (lip >= 3232235520UL && lip <= 3232301055UL))
		return true;

	/*
		todo
			add list of lan ip ranges defined by hub administrator, external ips that know how to work with lan ips
	*/

	return false;
}

bool cDCProto::CheckChatMsg(const string &text, cConnDC *conn)
{
	if (!conn || !conn->mxServer) return true;
	cServerDC *Server = conn->Server();
	ostringstream errmsg;
	int count = text.size();

	if ((Server->mC.max_chat_msg == 0) || (count > Server->mC.max_chat_msg)) {
		errmsg << autosprintf(_("Your chat message contains %d characters but maximum allowed is %d characters."), count, Server->mC.max_chat_msg);
		Server->DCPublicHS(errmsg.str(), conn);
		return false;
	}

	count = CountLines(text);

	if ((Server->mC.max_chat_lines == 0) || (count > Server->mC.max_chat_lines)) {
		errmsg << autosprintf(_("Your chat message contains %d lines but maximum allowed is %d lines."), count, Server->mC.max_chat_lines);
		Server->DCPublicHS(errmsg.str(), conn);
		return false;
	}

	return true;
}

void cDCProto::Create_MyINFO(string &dest, const string&nick, const string &desc, const string&speed, const string &mail, const string &share)
{
	dest.reserve(dest.size()+nick.size()+desc.size()+speed.size()+mail.size()+share.size()+ 20);
	dest.append("$MyINFO $ALL ");
	dest.append(nick);
	dest.append(" ");
	dest.append(desc);
	dest.append("$ $");
	dest.append(speed);
	dest.append("$");
	dest.append(mail);
	dest.append("$");
	dest.append(share);
	dest.append("$");
}

void cDCProto::Create_Chat(string &dest, const string&nick,const string &text)
{
	dest.reserve(dest.size()+nick.size()+text.size()+ 4);
	dest.append("<");
	dest.append(nick);
	dest.append("> ");
	dest.append(text);
}

void cDCProto::Append_MyInfoList(string &dest, const string &MyINFO, const string &MyINFO_basic, bool DoBasic)
{
	if(dest[dest.size()-1]=='|')
		dest.resize(dest.size()-1);
	if(DoBasic)
		dest.append(MyINFO_basic);
	else
		dest.append(MyINFO);
}

void cDCProto::Create_PM(string &dest,const string &from, const string &to, const string &sign, const string &text)
{
	dest.append("$To: ");
	dest.append(to);
	dest.append(" From: ");
	dest.append(from);
	dest.append(" $<");
	dest.append(sign);
	dest.append("> ");
	dest.append(text);
}


void cDCProto::Create_PMForBroadcast(string &start, string &end, const string &from, const string &sign, const string &text)
{
	start.append("$To: ");
	end.append(" From: ");
	end.append(from);
	end.append(" $<");
	end.append(sign);
	end.append("> ");
	end.append(text);
}

void cDCProto::Create_HubName(string &dest, string &name, string &topic)
{
	dest = "$HubName " + name;

	if (topic.length()) {
		dest += " - ";
		dest += topic;
	}
}

void cDCProto::Create_Quit(string &dest, const string &nick)
{
	dest.append("$Quit ");
	dest.append(nick);
}

void cDCProto::Create_Hello(string &dest, const string &nick)
{
	dest.append("$Hello ");
	dest.append(nick);
}

void cDCProto::Create_OpList(string &dest, const string &nick)
{
	dest.append("$OpList ");
	dest.append(nick);
}

void cDCProto::Create_BotList(string &dest, const string &nick)
{
	dest.append("$BotList ");
	dest.append(nick);
}

cConnType *cDCProto::ParseSpeed(const string &uspeed)
{
	string speed(uspeed, 0, uspeed.size() -1);
	return mS->mConnTypes->FindConnType(speed);
}

const string &cDCProto::GetMyInfo(cUserBase * User, int ForClass)
{
	if((mS->mC.show_tags + int (ForClass >= eUC_OPERATOR) >= 2))
		return User->mMyINFO;
	else
		return User->mMyINFO_basic;
}

void cDCProto::ParseReferer(const string &lock, string &ref)
{
	size_t pos = lock.find("Ref=");

	if (pos != lock.npos) {
		ref = toLower(lock.substr(pos + 4));

		while (ref.size() > 7) {
			pos = ref.find("dchub://");

			if (pos != ref.npos)
				ref.erase(pos, 8);
			else
				break;
		}

		while (ref.size() > 2) {
			pos = ref.find("...");

			if (pos != ref.npos)
				ref.erase(pos, 3);
			else
				break;
		}

		while (ref.size() > 0) {
			pos = ref.find('/');

			if (pos != ref.npos)
				ref.erase(pos, 1);
			else
				break;
		}

		while (ref.size() > 0) {
			pos = ref.find(' ');

			if (pos != ref.npos)
				ref.erase(pos, 1);
			else
				break;
		}

		if (ref.size() > 3) {
			pos = ref.find(":411", ref.size() - 4);

			if (pos != ref.npos)
				ref.erase(pos, 4);
		}
	}
}

void cDCProto::UnEscapeChars(const string &src, string &dst, bool WithDCN)
{
	size_t pos;
	dst = src;
	pos = dst.find("&#36;");
	while (pos != dst.npos) {
		dst.replace(pos,5, "$");
		pos = dst.find("&#36;", pos);
	}

	pos = dst.find("&#124;");
	while (pos != dst.npos) {
		dst.replace(pos,6, "|");
		pos = dst.find("&#124;", pos);
	}
}

void cDCProto::UnEscapeChars(const string &src, char *dst, int &len ,bool WithDCN)
{
	size_t pos, pos2 = 0;
	string start, end;
	unsigned char c;
	int i = 0;

	if(!WithDCN) {
		start = "&#";
		end =";";
	} else {
		start = "/%DCN";
		end = "%/";
	}

	pos = src.find(start);
	while ((pos != src.npos) && (i < src.size())) {
		if (pos > pos2) {
			memcpy(dst + i, src.c_str() + pos2, pos - pos2);
			i += pos - pos2;
		}
		pos2 = src.find(end, pos);
		if ((pos2 != src.npos) && ((pos2 - pos) <= (start.size()+3))) {
				c = atoi(src.substr(pos + start.size(), 3).c_str());
				dst[i++] = c;
				pos2 += end.size();
		}
		pos = src.find(start, pos + 1);
	}
	if (pos2 < src.size()) {
		memcpy(dst + i, src.c_str() + pos2, src.size() - pos2 + 1);
		i += src.size() - pos2;
	}
	len = i;
}

void cDCProto::EscapeChars(const string &src, string &dst, bool WithDCN)
{
	dst = src;
	size_t pos;
	ostringstream os;
	pos = dst.find_first_of("\x05\x24\x60\x7C\x7E\x00"); // 0, 5, 36, 96, 124, 126
	while (pos != dst.npos) {
		os.str("");
		if (! WithDCN) os << "&#" << unsigned(dst[pos]) << ";";
		else os << "/%DCN" << unsigned(dst[pos]) << "%/";
		dst.replace(pos,1, os.str());
		pos = dst.find_first_of("\x05\x24\x60\x7C\x7E\x00", pos);
	}
}

void cDCProto::EscapeChars(const char *buf, int len, string &dest, bool WithDCN)
{
	dest ="";
	unsigned char c;
	unsigned int olen = 0;
	ostringstream os;
	while(len-- > 0) {
		c = *(buf++);
		olen = 0;
		switch(c) {
			case 0: case 5: case 36: case 96: case 124: case 126:
				os.str("");
				if(!WithDCN)
					os << "&#" << unsigned(c) << ";";
				else {
					if(c < 10) olen = 7;
					else if(c > 10 && c < 100) olen = 6;
					os.width(olen);
					os.fill('0');
					os << left << "/%DCN" << unsigned(c);
					os.width(0);
					os << "%/";
				}
				dest += os.str();
			break;
			default: dest += c; break;
		};
	}
}

void cDCProto::Lock2Key(const string &Lock, string &fkey)
{
	int count = 0, len = Lock.size();
	char *key = 0;
	char * lock = new char[len+1];
	UnEscapeChars(Lock, lock, len, true);

	key = new char[len+1];

	key[0] = lock[0] ^ lock[len - 1] ^ lock[len - 2] ^ 5;
	while(++count < len)
		key[count] = lock[count] ^ lock[count - 1];
	key[len]=0;

	count = 0;
	while(count++ < len)
		key[count - 1] = ((key[count - 1] << 4)) | ((key[count - 1] >> 4));


	cDCProto::EscapeChars(key, len, fkey, true);
	delete [] key;
	delete [] lock;
}

	}; // namespace nProtocol
}; // namespace nVerliHub
