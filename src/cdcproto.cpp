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

int cDCProto::TreatMsg(cMessageParser *Msg, cAsyncConn *Conn)
{
	cMessageDC *msg = (cMessageDC *)Msg;
	cConnDC *conn = (cConnDC *)Conn;
	//@todo tMsgAct action = this->mS->Filter(tDCMsg(msg->mType),conn);
	if(strlen(Msg->mStr.data()) < Msg->mStr.size())
	{
		if (mS->mC.nullchars_report) mS->ReportUserToOpchat(conn, _("Probably attempt of an attack with NULL characters")); // Msg->mStr
		conn->CloseNow();
		return -1;
	}

	#ifndef WITHOUT_PLUGINS
	if (msg->mType != eMSG_UNPARSED) {
		if (conn->mpUser != NULL) {
			if (!mS->mCallBacks.mOnParsedMsgAny.CallAll(conn, msg)) return 1;
		} else {
			if (!mS->mCallBacks.mOnParsedMsgAnyEx.CallAll(conn, msg)) {
				conn->CloseNow();
				return -1;
			}
		}
	}
	#endif

	switch (msg->mType)
	{
		case eDC_UNKNOWN: mS->mCallBacks.mOnUnknownMsg.CallAll(conn, msg); return 1; break;
		case eMSG_UNPARSED: msg->Parse(); return TreatMsg(msg, conn); break; // mS->mCallBacks.mOnUnparsedMsg.CallAll(conn, msg)
		case eDC_KEY: this->DC_Key(msg, conn); break;
		case eDC_VALIDATENICK: this->DC_ValidateNick(msg, conn); break;
		case eDC_MYPASS: this->DC_MyPass(msg, conn); break;
		case eDC_VERSION: this->DC_Version(msg, conn); break;
		case eDC_GETNICKLIST: this->DC_GetNickList(msg, conn); break;
		case eDC_MYINFO: this->DC_MyINFO(msg, conn); break;
		case eDC_GETINFO: this->DC_GetINFO(msg, conn); break;
		case eDC_USERIP: this->DC_UserIP(msg, conn); break;
		case eDC_CONNECTTOME: this->DC_ConnectToMe(msg, conn); break;
		case eDC_MCONNECTTOME: this->DC_MultiConnectToMe(msg, conn);break;
		case eDC_RCONNECTTOME: this->DC_RevConnectToMe(msg, conn); break;
		case eDC_TO: this->DC_To(msg, conn); break;
		case eDC_MCTO: this->DC_MCTo(msg, conn); break;
		case eDC_CHAT: this->DC_Chat(msg, conn); break;
		case eDC_OPFORCEMOVE: this->DC_OpForceMove(msg, conn); break;
		case eDC_KICK: this->DC_Kick(msg, conn); break;
		case eDC_SEARCH:
		case eDC_SEARCH_PAS:
		case eDC_MSEARCH:
		case eDC_MSEARCH_PAS: this->DC_Search(msg, conn); break;
		case eDC_SR: this->DC_SR(msg, conn); break;
		case eDC_QUIT: mS->DCPublicHS(_("Goodbye."), conn); conn->CloseNice(2000, eCR_QUIT); break;
		case eDCE_SUPPORTS: this->DCE_Supports(msg, conn); break;
		case eDCO_BAN:
		//case eDCO_TBAN: mP.DCO_TempBan(msg, conn); break;
		case eDCO_UNBAN: this->DCO_UnBan(msg, conn); break;
		case eDCO_GETBANLIST: this->DCO_GetBanList(msg, conn); break;
		case eDCO_WHOIP: this->DCO_WhoIP(msg, conn); break;
		case eDCO_GETTOPIC: this->DCO_GetTopic(msg, conn); break;
		case eDCO_SETTOPIC: this->DCO_SetTopic(msg, conn); break;
		case eDCB_BOTINFO: this->DCB_BotINFO(msg, conn); break;
		default: if(Log(1)) LogStream() << "Incoming untreated event" << endl; break;
	}

	return 0;
}

int cDCProto::DC_ValidateNick(cMessageDC *msg, cConnDC *conn)
{
	if(msg->SplitChunks()) return -1;
	if(conn->GetLSFlag(eLS_VALNICK)) return -1;

	string &nick = msg->ChunkString(eCH_1_PARAM);
	static string omsg;
	ostringstream os;

	// Log new user
	if(conn->Log(3))
		conn->LogStream() << "User " << nick << " tries to login" << endl;

	// Check if nick is valid or close the connection
	int closeReason;
	if(!mS->ValidateUser(conn, nick, closeReason)) {
		conn->CloseNice(1000, closeReason);
		return -1;
	}
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

		if (mS->mC.max_users_total == 0) os << " " << _("This is a registered users only hub.");
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
	string emp;
	Create_HubName(omsg, mS->mC.hub_name, emp);

	#ifndef WITHOUT_PLUGINS
	if (mS->mCallBacks.mOnHubName.CallAll(nick, omsg))
	#endif
	{
		conn->Send(omsg);
	}

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
		NewUser->mFloodPM.SetParams(0.0, 1. * mS->mC.int_flood_pm_period, mS->mC.int_flood_pm_limit);
		NewUser->mFloodMCTo.SetParams(0.0, 1. * mS->mC.int_flood_mcto_period, mS->mC.int_flood_mcto_limit);
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

	if (conn->mRegInfo && (conn->mRegInfo->mClass == eUC_PINGER)) {
		conn->mpUser->Register();
		mS->mR->Login(conn, nick);
	}
	conn->SetLSFlag(eLS_VALNICK|eLS_NICKLST); // set NICKLST because user may want to skip getting userlist
	conn->ClearTimeOut(eTO_VALNICK);
	conn->SetTimeOut(eTO_MYINFO, mS->mC.timeout_length[eTO_MYINFO], mS->mTime);
	return 0;
}

int cDCProto::DC_Key(cMessageDC * msg, cConnDC * conn)
{
	if (msg->SplitChunks()) return -1;

	if (conn->GetLSFlag(eLS_KEYOK)) { // key already sent
		string omsg = _("Invalid login sequence. Key already sent");
		if (conn->Log(1)) conn->LogStream() << omsg << endl;
		mS->ConnCloseMsg(conn, omsg, 1000, eCR_LOGIN_ERR);
		return -1;
	}

	if (mS->mC.drop_invalid_key) {
		string key, lock("EXTENDEDPROTOCOL_" PACKAGE);
		Lock2Key(lock, key);

		if (key != msg->ChunkString(1)) {
			if (conn->Log(1)) conn->LogStream() << "Invalid key" << endl;
			mS->ConnCloseMsg(conn, _("Your client provided an invalid key"), 1000, eCR_INVALID_KEY);
			return -1;
		}
	}

	conn->SetLSFlag(eLS_KEYOK);
	conn->ClearTimeOut(eTO_KEY);
	conn->SetTimeOut(eTO_VALNICK, mS->mC.timeout_length[eTO_VALNICK], mS->mTime);
	conn->mT.key.Get();
	return 0;
}

int cDCProto::DC_MyPass(cMessageDC * msg, cConnDC * conn)
{
	if(msg->SplitChunks()) return -1;
	string &pwd=msg->ChunkString(eCH_1_PARAM);
	string omsg;

	if(!conn->mpUser) {
		omsg = _("Bad login sequence; you must provide a valid nick first.");
		if(conn->Log(1))
			conn->LogStream() << "Mypass before validatenick" << endl;
		mS->ConnCloseMsg(conn,omsg,1000, eCR_LOGIN_ERR);
		return -1;
	}

	#ifndef WITHOUT_PLUGINS
	if (!mS->mCallBacks.mOnParsedMsgMyPass.CallAll(conn, msg)) {
		conn->CloseNow();
		return -1;
	}
	#endif

	// set password request
	if (conn->mpUser->mSetPass) {
		if (!conn->mRegInfo || !conn->mRegInfo->mPwdChange) {
			conn->mpUser->mSetPass = false;
			return 0;
		}

		ostringstream ostr;

		if (pwd.size() < (unsigned int)mS->mC.password_min_len) {
			ostr << autosprintf(_("Minimum password length is %d characters, please retry."), mS->mC.password_min_len);
			mS->DCPrivateHS(ostr.str(), conn);
			mS->DCPublicHS(ostr.str(), conn);
			conn->mpUser->mSetPass = false;
			return 0;
		}

		if (!mS->mR->ChangePwd(conn->mpUser->mNick, pwd, 0)) {
			ostr << _("Error updating password.");
			mS->DCPrivateHS(ostr.str(), conn);
			mS->DCPublicHS(ostr.str(), conn);
			conn->mpUser->mSetPass = false;
			return 0;
		}

		ostr << _("Password updated successfully.");
		mS->DCPrivateHS(ostr.str(), conn);
		mS->DCPublicHS(ostr.str(), conn);
		conn->ClearTimeOut(eTO_SETPASS);
		conn->mpUser->mSetPass = false;
		return 0;
	}

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

int cDCProto::DC_Version(cMessageDC * msg, cConnDC * conn)
{
	if(msg->SplitChunks()) return -1;

	#ifndef WITHOUT_PLUGINS
	if (!mS->mCallBacks.mOnParsedMsgVersion.CallAll(conn, msg)) {
		conn->CloseNow();
		return -1;
	}
	#endif

	conn->SetLSFlag( eLS_VERSION );
	string &version=msg->ChunkString(eCH_1_PARAM);
	if(conn->Log(5))
		conn->LogStream() << "Version:" << version << endl;
	//TODO Check protocol version
	conn->mVersion = version;
	return 1;
}

int cDCProto::DC_GetNickList(cMessageDC * , cConnDC * conn)
{
	if(!conn) return -1;
	if(!conn->GetLSFlag(eLS_MYINFO) && mS->mC.nicklist_on_login)
	{
		if (mS->mC.delayed_login)
		{
			int LSFlag = conn->GetLSFlag(eLS_LOGIN_DONE);
			if (LSFlag & eLS_NICKLST) LSFlag -= eLS_NICKLST;
			conn->ReSetLSFlag(LSFlag);
		}
		conn->mSendNickList = true;
		return 0;
	}
	if (conn->mpUser && (conn->mpUser->mClass < eUC_OPERATOR)) {
		if(!mS->MinDelay(conn->mpUser->mT.nicklist,mS->mC.int_nicklist)) {
			return -1;
		}
	}
	return NickList(conn);
}

int cDCProto::DC_MyINFO(cMessageDC *msg, cConnDC *conn)
{
	string cmsg;

	// server gets this once on login, and then yet many times
	if (msg->SplitChunks()) {
		if (conn->Log(2)) conn->LogStream() << "MyINFO syntax error, closing connection." << endl;
		mS->ConnCloseMsg(conn, cmsg, 4000, eCR_SYNTAX);
		return -1;
	}

	string &nick = msg->ChunkString(eCH_MI_NICK);

	// this cant happen without having created user object
	if (!conn->mpUser) {
		cmsg = _("Bad login sequence.");
		if (conn->Log(2)) conn->LogStream() << "MyINFO without nick: " << nick << endl;
		mS->ConnCloseMsg(conn, cmsg, 1000, eCR_LOGIN_ERR);
		if (mS->ErrLog(0)) mS->LogStream() << "MyINFO without nick: " << nick << endl;
		return -1;
	}

	// check syntax
	// check nick
	if (nick != conn->mpUser->mNick) {
		cmsg = _("Wrong nick in MyINFO.");
		if (conn->Log(1)) conn->LogStream() << "Claims to be someone else in MyINFO." << endl;
		mS->ConnCloseMsg(conn, cmsg, 1500, eCR_SYNTAX);
		return -1;
	}

	// begin tag verification
	// parse conention type
	if (conn->mConnType == NULL) conn->mConnType = ParseSpeed(msg->ChunkString(eCH_MI_SPEED));

	// check users tag
	cDCTag *tag = mS->mCo->mDCClients->ParseTag(msg->ChunkString(eCH_MI_DESC));

	if (!mS->mC.tag_allow_none && (mS->mCo->mDCClients->mPositionInDesc < 0) && (conn->mpUser->mClass < mS->mC.tag_min_class_ignore) && (conn->mpUser->mClass != eUC_PINGER)) {
		cmsg = _("Turn on your tag.");
		if (conn->Log(2)) conn->LogStream() << "No tag." << endl;
		mS->ConnCloseMsg(conn, cmsg, 1000, eCR_TAG_NONE);
		delete tag;
		return -1;
	}

	bool TagValid = true;
	int tag_result = 0;
	ostringstream os;

	if (((!mS->mC.tag_allow_none) || ((mS->mC.tag_allow_none) && (mS->mCo->mDCClients->mPositionInDesc >= 0))) && (conn->mpUser->mClass < mS->mC.tag_min_class_ignore) && (conn->mpUser->mClass != eUC_PINGER)) {
		TagValid = tag->ValidateTag(os, conn->mConnType, tag_result);
	}

	#ifndef WITHOUT_PLUGINS
	TagValid = TagValid && mS->mCallBacks.mOnValidateTag.CallAll(conn, tag);
	#endif

	if (!TagValid) {
		if(conn->Log(2)) conn->LogStream() << "Invalid tag: " << tag_result << " Tag info: " << tag << endl;
		mS->ConnCloseMsg(conn, os.str(), 1000, eCR_TAG_INVALID);
		return -1;
	}

	if ((tag->mClientMode == eCM_PASSIVE) || (tag->mClientMode == eCM_SOCK5) || (tag->mClientMode == eCM_OTHER)) conn->mpUser->IsPassive = true;
	//delete tag; // tag is still used below, could this cause crash?
	// end tag verification

	// passive user limit
	if (conn->mpUser->IsPassive && (mS->mC.max_users_passive != -1) && (conn->GetTheoricalClass() < eUC_OPERATOR) && (mS->mPassiveUsers.Size() >= mS->mC.max_users_passive)) {
		os << autosprintf(_("Passive user limit exceeded at %d online passive users, please become active to enter the hub."), mS->mPassiveUsers.Size());
		if (conn->Log(2)) conn->LogStream() << "Passive user limit exceeded: " << mS->mPassiveUsers.Size() << endl;
		static string omsg;
		omsg = "$HubIsFull";
		conn->Send(omsg);
		mS->ConnCloseMsg(conn, os.str(), 1000, eCR_USERLIMIT);
		return -1;
	}

	// check min and max share conditions
	string &str_share = msg->ChunkString(eCH_MI_SIZE);

	if (str_share.size() > 18) { // share is too big
		conn->CloseNow();
		return -1;
	} else if (str_share.empty() || (str_share[0] == '-')) { // missing or negative share
		str_share = "0";
	}

	__int64 share = 0, shareB = 0;
	shareB = StringAsLL(str_share);
	share = shareB / (1024 * 1024);

	if (conn->GetTheoricalClass() <= eUC_OPERATOR) {
		// calculate minimax
		__int64 min_share = mS->mC.min_share;
		__int64 max_share = mS->mC.max_share;
		__int64 min_share_p, min_share_a;

		if (conn->GetTheoricalClass() == eUC_PINGER) {
			min_share = 0;
		} else {
			if (conn->GetTheoricalClass() >= eUC_REGUSER) {
				min_share = mS->mC.min_share_reg;
				max_share = mS->mC.max_share_reg;
			}

			if (conn->GetTheoricalClass() >= eUC_VIPUSER) {
				min_share = mS->mC.min_share_vip;
				max_share = mS->mC.max_share_vip;
			}

			if (conn->GetTheoricalClass() >= eUC_OPERATOR) {
				min_share = mS->mC.min_share_ops;
				max_share = mS->mC.max_share_ops;
			}
		}

		min_share_a = min_share;
		min_share_p = (__int64)(min_share * mS->mC.min_share_factor_passive);
		if (conn->mpUser->IsPassive) min_share = min_share_p;
		//if (conn->mpUser->Can(eUR_NOSHARE, mS->mTime.Sec())) min_share = 0;

		if ((share < min_share) || (max_share && (share > max_share))) {
			if (share < min_share)
				os << autosprintf(_("You share %s but minimum allowed is %s, %s for active users, %s for passive users."), convertByte(share * 1024 * 1024, false).c_str(), convertByte(min_share * 1024 * 1024, false).c_str(), convertByte(min_share_a * 1024 * 1024, false).c_str(), convertByte(min_share_p * 1024 * 1024, false).c_str());
			else
				os << autosprintf(_("You share %s but maximum allowed is %s."), convertByte(share * 1024 * 1024, false).c_str(), convertByte(max_share * 1024 * 1024, false).c_str());

			if (conn->Log(2)) conn->LogStream() << "Share limit."<< endl;
			mS->ConnCloseMsg(conn, os.str(), 4000, eCR_SHARE_LIMIT);
			return -1;
		}

		// this is second share limit, if non-zero disables search and download
		if (conn->GetTheoricalClass() <= eUC_VIPUSER) {
			__int64 temp_min_share = 0; // todo: rename to min_share_use_hub_guest

			if (mS->mC.min_share_use_hub && conn->GetTheoricalClass() == eUC_NORMUSER) {
				temp_min_share = mS->mC.min_share_use_hub;
			} else if (mS->mC.min_share_use_hub_reg && conn->GetTheoricalClass() == eUC_REGUSER) {
				temp_min_share = mS->mC.min_share_use_hub_reg;
			} else if (mS->mC.min_share_use_hub_vip && conn->GetTheoricalClass() == eUC_VIPUSER) {
				temp_min_share = mS->mC.min_share_use_hub_vip;
			}

			// found hub limit
			if (temp_min_share) {
				if (conn->mpUser->IsPassive) temp_min_share = (__int64)(temp_min_share * mS->mC.min_share_factor_passive);

				if (share < temp_min_share) {
					conn->mpUser->SetRight(eUR_SEARCH, 0);
					conn->mpUser->SetRight(eUR_CTM, 0);
				}
			}
		}

		if (conn->GetTheoricalClass() < mS->mC.min_class_use_hub) {
			conn->mpUser->SetRight(eUR_SEARCH, 0);
			conn->mpUser->SetRight(eUR_CTM, 0);
		}

		if ((conn->GetTheoricalClass() < mS->mC.min_class_use_hub_passive) && !(conn->mpUser->IsPassive == false)) {
			conn->mpUser->SetRight(eUR_SEARCH, 0);
			conn->mpUser->SetRight(eUR_CTM, 0);
		}
	}

	// update totalshare
	mS->mTotalShare -= conn->mpUser->mShare;
	conn->mpUser->mShare = shareB;
	mS->mTotalShare += conn->mpUser->mShare;

	// peak total share
	if (mS->mTotalShare > mS->mTotalSharePeak)
		mS->mTotalSharePeak = mS->mTotalShare;

	conn->mpUser->mEmail = msg->ChunkString(eCH_MI_MAIL);

	// user sent myinfo for the first time
	if (conn->GetLSFlag(eLS_LOGIN_DONE) != eLS_LOGIN_DONE) {
		cBan Ban(mS);
		bool banned = false;
		banned = mS->mBanList->TestBan(Ban, conn, conn->mpUser->mNick, eBF_SHARE);

		if (banned && conn->GetTheoricalClass() <= eUC_REGUSER) {
			stringstream msg;
			msg << _("You are banned from this hub.") << "\r\n";
			Ban.DisplayUser(msg);
			mS->DCPublicHS(msg.str(), conn);
			conn->LogStream() << "Kicked user due ban detection." << endl;
			conn->CloseNice(1000, eCR_KICKED);
			return -1;
		}

		#ifndef WITHOUT_PLUGINS
		if (!mS->mCallBacks.mOnFirstMyINFO.CallAll(conn, msg)) {
			conn->CloseNice(1000, eCR_KICKED);
			return -2;
		}
		#endif
	}

 	#ifndef WITHOUT_PLUGINS
		if (!mS->mCallBacks.mOnParsedMsgMyINFO.CallAll(conn, msg))
			return -2;
	#endif

	// $MyINFO $ALL <nick> <description><tag>$ $<speed><status>$<email>$<share>$
	string myinfo_full, myinfo_basic, desctag, desc, sTag, email, speed, sShare;
	desctag = msg->ChunkString(eCH_MI_DESC);

	if (!desctag.empty()) {
		mS->mCo->mDCClients->ParsePos(desctag);
		desc.assign(desctag, 0, mS->mCo->mDCClients->mPositionInDesc); // todo: tag.mPositionInDesc may be incorrect after the description has been modified by a plugin

		if (mS->mCo->mDCClients->mPositionInDesc >= 0)
			sTag.assign(desctag, mS->mCo->mDCClients->mPositionInDesc, -1);
	}

	if (mS->mC.show_desc_len >= 0)
		desc.assign(desc, 0, mS->mC.show_desc_len);

	if (mS->mC.desc_insert_mode) {
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
	}

	delete tag; // now we can delete it

	if (mS->mC.show_email == 0)
		email= "";
	else
		email = msg->ChunkString(eCH_MI_MAIL);

	speed = msg->ChunkString(eCH_MI_SPEED);

	if ((mS->mC.show_speed == 0) && !speed.empty())
		speed = speed[speed.size() - 1]; // hide speed but keep status byte

	if (conn->mpUser->mHideShare)
		sShare = "0";
	else
		sShare = str_share; // msg->ChunkString(eCH_MI_SIZE)

	Create_MyINFO(myinfo_basic, nick, desc, speed, email, sShare); // msg->ChunkString(eCH_MI_NICK)

	if ((conn->mpUser->mClass >= eUC_OPERATOR) && (mS->mC.show_tags < 3)) // ops have hidden myinfo
		myinfo_full = myinfo_basic;
	else
		Create_MyINFO(myinfo_full, nick, desc + sTag, speed, email, sShare); // msg->mStr

	// login or send to all
	if (conn->mpUser->mInList) {
		// send it to all only if: its not too often, it has changed since last time, send only the version that has changed only to those who want it
		if (mS->MinDelay(conn->mpUser->mT.info, mS->mC.int_myinfo) && (myinfo_full != conn->mpUser->mMyINFO)) {
			conn->mpUser->mMyINFO = myinfo_full;

			if (myinfo_basic != conn->mpUser->mMyINFO_basic) {
				conn->mpUser->mMyINFO_basic = myinfo_basic;
				string send_info;
				send_info = GetMyInfo(conn->mpUser, eUC_NORMUSER);
				mS->mUserList.SendToAll(send_info, mS->mC.delayed_myinfo, true);
			}

			if (mS->mC.show_tags >= 1) mS->mOpchatList.SendToAll(myinfo_full, mS->mC.delayed_myinfo, true);
		}
	} else { // user logs in for the first time
		conn->mpUser->mMyINFO = myinfo_full; // keep it
		conn->mpUser->mMyINFO_basic = myinfo_basic;
		conn->SetLSFlag(eLS_MYINFO);
		if (!mS->BeginUserLogin(conn)) return -1; // if all right, add user to userlist, if not yet there
	}

	conn->ClearTimeOut(eTO_MYINFO);
	return 0;
}

int cDCProto::DC_GetINFO(cMessageDC * msg, cConnDC * conn)
{
	if(msg->SplitChunks()) return -1;

	if(!conn->mpUser || !conn->mpUser->mInList)
		return -1;

	string buf;
	string str=msg->ChunkString(eCH_GI_OTHER);

	cUser *other = mS->mUserList.GetUserByNick ( str );

	// check if user found
	if(!other) {
		if(str != mS->mC.hub_security && str != mS->mC.opchat_name) {
			cDCProto::Create_Quit(buf, str);
			conn->Send(buf, true);
		}
		return -2;
	}

	// if user just logged in ignore it, conn is dcgui, and already one myinfo sent
	if(conn->mpUser->mT.login < other->mT.login && cTime() < (other->mT.login + 60))
		return 0;

	if(mS->mC.optimize_userlist == eULO_GETINFO) {
		conn->mpUser->mQueueUL.append(str);
		conn->mpUser->mQueueUL.append("|");
	} else {
		// send it
		if(!(conn->mFeatures & eSF_NOGETINFO)) {
			buf = GetMyInfo(other, conn->mpUser->mClass );
			conn->Send(buf, true, false);
		}
	}
	return 0;
}

int cDCProto::DC_UserIP(cMessageDC * msg, cConnDC * conn)
{
	/*
	* @rolex
	* not sure about this,
	* but if UserIP2 is present in client supports,
	* and hub supports it aswell,
	* then client should never send this command,
	* and we should ignore it
	*/

	if (!msg || !conn || !conn->mpUser || !conn->mpUser->mInList) return -1;
	if (conn->mpUser->mClass < eUC_OPERATOR) return -1;
	if (msg->SplitChunks()) return -1;
	string lst = msg->ChunkString(eCH_1_PARAM);
	if (lst.empty()) return -1;
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

int cDCProto::DC_To(cMessageDC * msg, cConnDC * conn)
{
	if(msg->SplitChunks()) return -1;
	string &str=msg->ChunkString(eCH_PM_TO);
	ostringstream os;

	if(!conn->mpUser) return -1;
	if(!conn->mpUser->mInList) return -2;
	if(!conn->mpUser->Can(eUR_PM, mS->mTime.Sec(), 0)) return -4;

	// verify sender's nick
	if(msg->ChunkString(eCH_PM_FROM) != conn->mpUser->mNick || msg->ChunkString(eCH_PM_NICK) != conn->mpUser->mNick) {
		if(conn->Log(2))
			conn->LogStream() << "Pretend to be someone else in PM (" << msg->ChunkString(eCH_PM_FROM) << ")." <<endl;
		conn->CloseNow();
		return -1;
	}
	cTime now;
	now.Get();
	int fl = 0;
	fl = - conn->mpUser->mFloodPM.Check(now);
	if((conn->mpUser->mClass < eUC_OPERATOR) && fl) {
		if(conn->Log(1)) conn->LogStream() << "Floods PM (" << msg->ChunkString(eCH_PM_FROM) << ")." <<endl;
		if(fl >= 3) {
			mS->DCPrivateHS(_("Flooding PM"), conn);
			ostringstream reportMessage;
			reportMessage << autosprintf(_("*** PM Flood: %s"), msg->ChunkString(eCH_PM_MSG).c_str());
			mS->ReportUserToOpchat(conn, reportMessage.str());
			conn->CloseNow();
		}
		return -1;
	}

	cUser::tFloodHashType Hash = 0;
	Hash = tHashArray<void*>::HashString(msg->ChunkString(eCH_PM_MSG));
	if (Hash && (conn->mpUser->mClass < eUC_OPERATOR)) {
		if(Hash == conn->mpUser->mFloodHashes[eFH_PM]) {
			if( conn->mpUser->mFloodCounters[eFC_PM]++ > mS->mC.max_flood_counter_pm) {
					mS->DCPrivateHS(_("Flooding PM"), conn);
					ostringstream reportMessage;
					reportMessage << autosprintf(_("*** PM same message flood detected: %s"), msg->ChunkString(eCH_PM_MSG).c_str());
					mS->ReportUserToOpchat(conn, reportMessage.str());
					conn->CloseNow();
					return -5;
			}
		} else {
			conn->mpUser->mFloodCounters[eFC_PM]=0;
		}
	}
	conn->mpUser->mFloodHashes[eFH_PM] = Hash;

	if (conn->mpUser->mClass < mS->mC.private_class) {
		mS->DCPrivateHS(_("Private chat is currently disabled for users with your class."), conn);
		mS->DCPublicHS(_("Private chat is currently disabled for users with your class."), conn);
		return 0;
	}

	// Find other user
	cUser *other = mS->mUserList.GetUserByNick(str);
	if(!other)
		return -2;
	//NOTE: It seems to be there a crash on Windows when using Lua plugin and a Lua script calls DelRobot
	if(conn->mpUser->mClass + mS->mC.classdif_pm < other->mClass) {
		mS->DCPrivateHS(_("You cannot talk to this user."), conn);
		mS->DCPublicHS(_("You cannot talk to this user."), conn);
		return -4;
	}

	// Log it
	if(mS->Log(5))
		mS->LogStream() << "PM from:" << conn->mpUser->mNick << " To: " << msg->ChunkString(eCH_PM_TO) << endl;

	#ifndef WITHOUT_PLUGINS
	if (!mS->mCallBacks.mOnParsedMsgPM.CallAll(conn, msg)) return 0;
	#endif

	// Send it
	if(other->mxConn) {
		other->mxConn->Send(msg->mStr);
	} else if (mS->mRobotList.ContainsNick(str)) {
		((cUserRobot*)mS->mRobotList.GetUserBaseByNick(str))->ReceiveMsg(conn, msg);
	}
	return 0;
}

int cDCProto::DC_MCTo(cMessageDC * msg, cConnDC * conn)
{
	if (msg->SplitChunks()) return -1;
	if (!conn->mpUser) return -1;
	if (!conn->mpUser->mInList) return -2;
	if (!conn->mpUser->Can(eUR_PM, mS->mTime.Sec(), 0)) return -4;

	// verify senders nick
	if ((msg->ChunkString(eCH_MCTO_FROM) != conn->mpUser->mNick) || (msg->ChunkString(eCH_MCTO_NICK) != conn->mpUser->mNick)) {
		if (conn->Log(2)) conn->LogStream() << "User pretend to be someone else in MCTo: " << msg->ChunkString(eCH_MCTO_FROM) << endl;
		conn->CloseNow();
		return -1;
	}

	cTime now;
	now.Get();
	int fl = 0;
	fl =- conn->mpUser->mFloodMCTo.Check(now);

	if ((conn->mpUser->mClass < eUC_OPERATOR) && fl) {
		if (conn->Log(1)) conn->LogStream() << "MCTo flood: " << msg->ChunkString(eCH_MCTO_FROM) << endl;

		if (fl >= 3) {
			mS->DCPrivateHS(_("MCTo flood."), conn);
			ostringstream reportMessage;
			reportMessage << autosprintf(_("*** MCTo flood: %s"), msg->ChunkString(eCH_MCTO_MSG).c_str());
			mS->ReportUserToOpchat(conn, reportMessage.str());
			conn->CloseNow();
		}

		return -1;
	}

	cUser::tFloodHashType Hash = 0;
	Hash = tHashArray<void*>::HashString(msg->ChunkString(eCH_MCTO_MSG));

	if (Hash && (conn->mpUser->mClass < eUC_OPERATOR)) {
		if (Hash == conn->mpUser->mFloodHashes[eFH_MCTO]) {
			if (conn->mpUser->mFloodCounters[eFC_MCTO]++ > mS->mC.max_flood_counter_mcto) {
				mS->DCPrivateHS(_("MCTo flood."), conn);
				ostringstream reportMessage;
				reportMessage << autosprintf(_("*** MCTo same message flood detected: %s"), msg->ChunkString(eCH_MCTO_MSG).c_str());
				mS->ReportUserToOpchat(conn, reportMessage.str());
				conn->CloseNow();
				return -5;
			}
		} else
			conn->mpUser->mFloodCounters[eFC_MCTO] = 0;
	}

	conn->mpUser->mFloodHashes[eFH_MCTO] = Hash;
	// find other user
	string &str=msg->ChunkString(eCH_MCTO_TO);
	cUser *other = mS->mUserList.GetUserByNick(str);
	if (!other) return -2;

	if (conn->mpUser->mClass + mS->mC.classdif_mcto < other->mClass) {
		mS->DCPrivateHS(_("You can't talk to this user."), conn);
		mS->DCPublicHS(_("You can't talk to this user."), conn);
		return -4;
	}

	// log
	if (mS->Log(5)) mS->LogStream() << "MCTo from: " << conn->mpUser->mNick << " to: " << msg->ChunkString(eCH_MCTO_TO) << endl;

	#ifndef WITHOUT_PLUGINS
	if (!mS->mCallBacks.mOnParsedMsgMCTo.CallAll(conn, msg)) return 0;
	#endif

	// send
	if (other->mxConn) {
		string mcto;

		if (other->mxConn->mFeatures & eSF_MCTO) { // send as is if supported by client
			mcto = msg->mStr;
		} else { // else convert to private main chat message
			mcto.erase();
			Create_Chat(mcto, conn->mpUser->mNick, msg->ChunkString(eCH_MCTO_MSG));
		}

		other->mxConn->Send(mcto);
	}

	return 0;
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

int cDCProto::DC_Chat(cMessageDC *msg, cConnDC *conn)
{
	if (!msg) return -1;
	if (msg->SplitChunks()) return -1;
	if (!conn) return -2;
	if (!conn->mpUser) return -2;
	if (!conn->mpUser->mInList) return -3;
	stringstream omsg;

	// check if nick is ok
	if ((msg->ChunkString(eCH_CH_NICK) != conn->mpUser->mNick)) {
		omsg << autosprintf(_("You nick is not %s but %s."), msg->ChunkString(eCH_CH_NICK).c_str(), conn->mpUser->mNick.c_str());
		mS->DCPublicHS(omsg.str(), conn);
		conn->CloseNice(1000, eCR_CHAT_NICK);
		return -2;
	}

	// check if delay is ok
	if (conn->mpUser->mClass < eUC_VIPUSER) {
		long delay = mS->mC.int_chat_ms;

		if (!mS->MinDelayMS(conn->mpUser->mT.chat, delay)) {
			cTime now;
			cTime diff = now - conn->mpUser->mT.chat;
			omsg << autosprintf(_("Not sent because minimum chat delay is %lu ms but you made %s."), delay, diff.AsPeriod().AsString().c_str());
			mS->DCPublicHS(omsg.str(), conn);
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
			string kick_reason;
			mKickChatPattern.Extract(4, text, kick_reason);
			string nick;
			mKickChatPattern.Extract(3, text, nick);
			mS->DCKickNick(NULL, conn->mpUser, nick, kick_reason, eKCK_Reason);
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

int cDCProto::DC_Kick(cMessageDC * msg, cConnDC * conn)
{
	if(msg->SplitChunks()) return -1;
	if(!conn->mpUser || !conn->mpUser->mInList) return -2;
	string &nick = msg->ChunkString(eCH_1_PARAM);

	// check rights
	if(conn->mpUser->Can(eUR_KICK, mS->mTime.Sec()))
	{
		mS->DCKickNick(NULL, conn->mpUser, nick, mS->mEmpty, eKCK_Drop | eKCK_TBAN);
		return 0;
	}
	else
	{
		conn->CloseNice(2000, eCR_KICKED);
		return -1;
	}
}

bool cDCProto::CheckIP(cConnDC * conn, string &ip)
{
	bool WrongIP = true;
	if(WrongIP && conn->mAddrIP == ip) {
		WrongIP = false;
	}
	if (WrongIP && (conn->mRegInfo && conn->mRegInfo->mAlternateIP == ip)) {
		WrongIP = false;
	}
	return ! WrongIP;
}

bool cDCProto::isLanIP(string ip)
{

	unsigned long senderIP = cBanList::Ip2Num(ip);
	// see RFC 1918
	if( (senderIP > 167772160UL && senderIP < 184549375UL) || (senderIP > 2886729728UL && senderIP < 2887778303UL) || (senderIP > 3232235520UL && senderIP < 3232301055UL)) return true;
	return false;
}

int cDCProto::DC_ConnectToMe(cMessageDC *msg, cConnDC *conn)
{
	if (msg->SplitChunks())
		return -1;

	if (!conn)
		return -1;

	if (!conn->mpUser)
		return -1;

	if (!conn->mpUser->mInList)
		return -1;

	ostringstream os;

	if (!conn->mpUser->Can(eUR_CTM, mS->mTime.Sec(), 0)) {
		if (!mS->mC.min_x_use_hub_message || conn->mpUser->mHideCtmMsg)
			return -4;

		unsigned long use_hub_share = 0;

		if (mS->mC.min_share_use_hub && conn->GetTheoricalClass() == eUC_NORMUSER) {
			use_hub_share = mS->mC.min_share_use_hub;
		} else if (mS->mC.min_share_use_hub_reg && conn->GetTheoricalClass() == eUC_REGUSER) {
			use_hub_share = mS->mC.min_share_use_hub_reg;
		} else if (mS->mC.min_share_use_hub_vip && conn->GetTheoricalClass() == eUC_VIPUSER) {
			use_hub_share = mS->mC.min_share_use_hub_vip;
		}

		use_hub_share = use_hub_share * 1024 * 1024;

		if (conn->mpUser->mShare < use_hub_share) {
			os << autosprintf(_("You can't download unless you share: %s"), convertByte(use_hub_share, false).c_str());
			mS->DCPublicHS(os.str(), conn);
			return -4;
		}

		int use_hub_class = 0;

		if (mS->mC.min_class_use_hub && conn->mpUser->IsPassive == false) {
			use_hub_class = mS->mC.min_class_use_hub;
		} else if (mS->mC.min_class_use_hub_passive && conn->mpUser->IsPassive == true) {
			use_hub_class = mS->mC.min_class_use_hub_passive;
		}

		if (conn->mpUser->mClass < use_hub_class) {
			os << autosprintf(_("You can't download unless you are registered with class: %d"), use_hub_class);
			mS->DCPublicHS(os.str(), conn);
			return -4;
		}

		// any other reason that should be notified?
		return -4;
	}

	string &nick = msg->ChunkString(eCH_CM_NICK);
	cUser *other = mS->mUserList.GetUserByNick(nick);

	// check nick and connection

	if (!other) {
		os << autosprintf(_("User not found: %s"), nick.c_str());
		mS->DCPublicHS(os.str(), conn);
		return -1;
	}

	if (!other->mxConn) {
		mS->DCPublicHS(_("Robots don't share."), conn);
		return -1;
	}

	// check if user can download and if other user hides share

	if ((conn->mpUser->mClass + mS->mC.classdif_download < other->mClass) || other->mHideShare) {
		if (!mS->mC.hide_noctm_message && !conn->mpUser->mHideCtmMsg) {
			os << autosprintf(_("You can't download from: %s"), nick.c_str());
			mS->DCPublicHS(os.str(), conn);
		}

		return -4;
	}

	#ifndef WITHOUT_PLUGINS
		if (!mS->mCallBacks.mOnParsedMsgConnectToMe.CallAll(conn, msg))
			return -2;
	#endif

	string ctm, addr, port, extra;
	addr = msg->ChunkString(eCH_CM_IP);

	if (!CheckIP(conn, addr)) {
		if (isLanIP(conn->mAddrIP) && !isLanIP(other->mxConn->mAddrIP)) { // lan to wan
			os << _("You can't connect to an external IP because you are in LAN.");
			string toSend = os.str();
			conn->Send(toSend);
			return -1;
		}

		if (conn->Log(3))
			LogStream() << "Fixed wrong IP in $ConnectToMe from " << addr << " to " << conn->mAddrIP << endl;

		addr = conn->mAddrIP;
	}

	port = msg->ChunkString(eCH_CM_PORT);

	if (port.empty() || (port.size() > 6))
		return -1;

	if (port[port.size() - 1] == 'S') { // secure connection, todo: add more stuff
		extra = "S";
		port.assign(port, 0, port.size() - 1);
	}

	__int64 iport = StringAsLL(port);

	if ((iport < 1) || (iport > 65535))
		return -1;

	ctm = "$ConnectToMe ";
	ctm += nick;
	ctm += ' ';
	ctm += addr;
	ctm += ':';
	ctm += StringFrom(iport);
	ctm += extra;

	other->mxConn->Send(ctm);
	return 0;
}

int cDCProto::DC_MultiConnectToMe(cMessageDC *, cConnDC *)
{
	// todo
	return 0;
}

int cDCProto::DC_RevConnectToMe(cMessageDC *msg, cConnDC *conn)
{
	if (msg->SplitChunks())
		return -1;

	if (!conn)
		return -1;

	if (!conn->mpUser)
		return -1;

	if (!conn->mpUser->mInList)
		return -2;

	if (!conn->mpUser->IsPassive) {
		mS->DCPublicHS(_("You can't send this request because you're not in passive mode."), conn);
		return -2;
	}

	ostringstream os;

	if (!conn->mpUser->Can(eUR_CTM, mS->mTime.Sec(), 0)) {
		if (!mS->mC.min_x_use_hub_message || conn->mpUser->mHideCtmMsg)
			return -4;

		unsigned long use_hub_share = 0;

		if (mS->mC.min_share_use_hub && conn->GetTheoricalClass() == eUC_NORMUSER) {
			use_hub_share = mS->mC.min_share_use_hub;
		} else if (mS->mC.min_share_use_hub_reg && conn->GetTheoricalClass() == eUC_REGUSER) {
			use_hub_share = mS->mC.min_share_use_hub_reg;
		} else if (mS->mC.min_share_use_hub_vip && conn->GetTheoricalClass() == eUC_VIPUSER) {
			use_hub_share = mS->mC.min_share_use_hub_vip;
		}

		use_hub_share = use_hub_share * 1024 * 1024;

		if (conn->mpUser->mShare < use_hub_share) {
			os << autosprintf(_("You can't download unless you share: %s"), convertByte(use_hub_share, false).c_str());
			mS->DCPublicHS(os.str(), conn);
			return -4;
		}

		int use_hub_class = 0;

		if (mS->mC.min_class_use_hub && conn->mpUser->IsPassive == false) {
			use_hub_class = mS->mC.min_class_use_hub;
		} else if (mS->mC.min_class_use_hub_passive && conn->mpUser->IsPassive == true) {
			use_hub_class = mS->mC.min_class_use_hub_passive;
		}

		if (conn->mpUser->mClass < use_hub_class) {
			os << autosprintf(_("You can't download unless you are registered with class: %d"), use_hub_class);
			mS->DCPublicHS(os.str(), conn);
			return -4;
		}

		// any other reason that should be notified?
		return -4;
	}

	// check nick

	if (msg->ChunkString(eCH_RC_NICK) != conn->mpUser->mNick) {
		os << autosprintf(_("Your nick is not %s but %s."), msg->ChunkString(eCH_RC_NICK).c_str(), conn->mpUser->mNick.c_str());
		mS->ConnCloseMsg(conn, os.str(), 1500, eCR_SYNTAX);
		return -1;
	}

	// find and check the other nickname

	string &onick = msg->ChunkString(eCH_RC_OTHER);
	cUser *other = mS->mUserList.GetUserByNick(onick);

	if (!other) {
		os << autosprintf(_("User not found: %s"), onick.c_str());
		mS->DCPublicHS(os.str(), conn);
		return -2;
	}

	if (!other->mxConn) {
		os << autosprintf(_("User is a bot: %s"), onick.c_str());
		mS->DCPublicHS(os.str(), conn);
		return -2;
	}

	if ((conn->mpUser->mClass + mS->mC.classdif_download < other->mClass) || other->mHideShare) { // check if user can download and if other user hides share
		if (!mS->mC.hide_noctm_message && !conn->mpUser->mHideCtmMsg) {
			os << autosprintf(_("You can't download from: %s"), onick.c_str());
			mS->DCPublicHS(os.str(), conn);
		}

		return -4;
	}

	#ifndef WITHOUT_PLUGINS
		if (!mS->mCallBacks.mOnParsedMsgRevConnectToMe.CallAll(conn, msg))
			return -2;
	#endif

	other->mxConn->Send(msg->mStr);
	return 0;
}

int cDCProto::DC_Search(cMessageDC *msg, cConnDC *conn)
{
	if (msg->SplitChunks())
		return -1;

	if (!conn)
		return -1;

	if (!conn->mpUser)
		return -1;

	if (!conn->mpUser->mInList)
		return -2;

	ostringstream os;

	if (!conn->mpUser->Can(eUR_SEARCH, mS->mTime.Sec(), 0)) {
		if (!mS->mC.min_x_use_hub_message || conn->mpUser->mHideCtmMsg)
			return -4;

		unsigned long use_hub_share = 0;

		if (mS->mC.min_share_use_hub && conn->GetTheoricalClass() == eUC_NORMUSER) {
			use_hub_share = mS->mC.min_share_use_hub;
		} else if (mS->mC.min_share_use_hub_reg && conn->GetTheoricalClass() == eUC_REGUSER) {
			use_hub_share = mS->mC.min_share_use_hub_reg;
		} else if (mS->mC.min_share_use_hub_vip && conn->GetTheoricalClass() == eUC_VIPUSER) {
			use_hub_share = mS->mC.min_share_use_hub_vip;
		}

		use_hub_share = use_hub_share * 1024 * 1024;

		if (conn->mpUser->mShare < use_hub_share) {
			os << autosprintf(_("You can't search unless you share: %s"), convertByte(use_hub_share, false).c_str());
			mS->DCPublicHS(os.str(), conn);
			return -4;
		}

		int use_hub_class = 0;

		if (mS->mC.min_class_use_hub && conn->mpUser->IsPassive == false) {
			use_hub_class = mS->mC.min_class_use_hub;
		} else if (mS->mC.min_class_use_hub_passive && conn->mpUser->IsPassive == true) {
			use_hub_class = mS->mC.min_class_use_hub_passive;
		}

		if (conn->mpUser->mClass < use_hub_class) {
			os << autosprintf(_("You can't search unless you are registered with class: %d"), use_hub_class);
			mS->DCPublicHS(os.str(), conn);
			return -4;
		}

		// any other reason that should be notified?
		return -4;
	}

	if (conn->mpUser->mClass < eUC_OPERATOR)  {
		switch (msg->mType) {
			case eDC_MSEARCH:
			case eDC_SEARCH:
				if (msg->ChunkString(eCH_AS_SEARCHPATTERN).size() < mS->mC.min_search_chars) {
					os << autosprintf(_("Minimum search length is: %d"), mS->mC.min_search_chars);
					mS->DCPublicHS(os.str(), conn);
					return -1;
				}

				break;
			case eDC_MSEARCH_PAS:
			case eDC_SEARCH_PAS:
				if (msg->ChunkString(eCH_PS_SEARCHPATTERN).size() < mS->mC.min_search_chars) {
					os << autosprintf(_("Minimum search length is: %d"), mS->mC.min_search_chars);
					mS->DCPublicHS(os.str(), conn);
					return -1;
				}

				break;
			default:
				break;
		}
	}

	if (mS->mSysLoad >= (eSL_CAPACITY + conn->mpUser->mClass)) {
		if (mS->Log(3))
			mS->LogStream() << "Skipping search, system is: " << mS->mSysLoad << endl;

		os << _("Sorry, hub is too busy now, no search is available, please try later.");
		mS->DCPublicHS(os.str(), conn);
		return -2;
	}

	cUser::tFloodHashType Hash = 0;
	Hash = tHashArray<void*>::HashString(msg->mStr);

	if (Hash && (conn->mpUser->mClass < eUC_OPERATOR) && (Hash == conn->mpUser->mFloodHashes[eFH_SEARCH])) {
		return -4;
	}

	conn->mpUser->mFloodHashes[eFH_SEARCH] = Hash;

	// calculate delay and do some checks

	int delay = 10;
	string addr;

	switch (msg->mType) {
		case eDC_MSEARCH:
		case eDC_SEARCH:
			addr = msg->ChunkString(eCH_AS_IP);

			if (!CheckIP(conn, addr)) {
				if (conn->Log(3))
					LogStream() << "Fixed wrong IP in $Search from " << addr << " to " << conn->mAddrIP << endl;

				addr = conn->mAddrIP;
			}

			addr += ':';
			addr += StringFrom(StringAsLL(msg->ChunkString(eCH_AS_PORT)));

			if (conn->mpUser->mClass == eUC_REGUSER)
				delay = mS->mC.int_search_reg;
			else if (conn->mpUser->mClass == eUC_VIPUSER)
				delay = mS->mC.int_search_vip;
			else if (conn->mpUser->mClass == eUC_OPERATOR)
				delay = mS->mC.int_search_op;
			else
				delay = mS->mC.int_search;

			break;
		case eDC_MSEARCH_PAS:
		case eDC_SEARCH_PAS:
			addr = msg->ChunkString(eCH_PS_NICK);

			if (conn->mpUser->mNick != addr) {
				os << autosprintf(_("Your nick is not %s but %s."), addr.c_str(), conn->mpUser->mNick.c_str());
				mS->ConnCloseMsg(conn, os.str(), 4000, eCR_SYNTAX);
				return -1;
			}

			addr = "Hub:" + addr;

			if (conn->mpUser->mClass == eUC_REGUSER)
				delay = mS->mC.int_search_reg_pass;
			else if (conn->mpUser->mClass > eUC_REGUSER)
				delay = int(1.5 * mS->mC.int_search_reg);
			else
				delay = mS->mC.int_search_pas;

			break;
		default:
			return -5;
			break;
	}

	if (conn->mpUser->mClass >= eUC_VIPUSER)
		delay = mS->mC.int_search_vip;
	else if (conn->mpUser->mClass >= eUC_OPERATOR)
		delay = mS->mC.int_search_op;

	// verify the delay and the number of search

	if (!mS->MinDelay(conn->mpUser->mT.search, delay)) {
		if (conn->mpUser->mSearchNumber >= mS->mC.search_number) {
			os << autosprintf(_("You can do %d searches in %d seconds."), mS->mC.search_number, delay);
			mS->DCPublicHS(os.str(), conn);
			return -1;
		}
	} else {
		conn->mpUser->mSearchNumber = 0;
	}

 	#ifndef WITHOUT_PLUGINS
		if (!mS->mCallBacks.mOnParsedMsgSearch.CallAll(conn, msg))
			return -2;
	#endif

	conn->mpUser->mSearchNumber++;

	// send message

	string search("$Search ");
	search += addr;
	search += ' ';

	if (msg->mType == eDC_SEARCH_PAS) {
		conn->mSRCounter = 0;
		search += msg->ChunkString(eCH_PS_QUERY);
		mS->mActiveUsers.SendToAll(search, mS->mC.delayed_search);
	} else {
		search += msg->ChunkString(eCH_AS_QUERY);
		mS->mUserList.SendToAll(search, mS->mC.delayed_search);
	}

	return 0;
}

int cDCProto::DC_SR(cMessageDC * msg, cConnDC * conn)
{
	if(msg->SplitChunks())
		return -1;
	if(!conn->mpUser || !conn->mpUser->mInList) return -2;
	ostringstream os;
	// Check nickname
	if(conn->mpUser->mNick != msg->ChunkString(eCH_SR_FROM)) {
		if(conn->Log(1))
			conn->LogStream() << "Claims to be someone else in search response. Dropping connection." << endl;
		if(conn->mpUser)
			os << autosprintf(_("Your nick is not %s but %s. Disconnecting."), msg->ChunkString(eCH_SR_FROM).c_str(), conn->mpUser->mNick.c_str());
		mS->ConnCloseMsg(conn, os.str(),4000, eCR_SYNTAX);
		return -1;
	}
	string &str = msg->ChunkString(eCH_SR_TO);
	cUser *other = mS->mUserList.GetUserByNick(str);
	// Check other nick
	if(!other)
		return -1;
	// Cut the end
	string ostr(msg->mStr,0 ,msg->mChunks[eCH_SR_TO].first - 1);

	#ifndef WITHOUT_PLUGINS
	if (!mS->mCallBacks.mOnParsedMsgSR.CallAll(conn, msg)) return -2;
	#endif

	// Send it
	if((other->mxConn) && (!mS->mC.max_passive_sr || (other->mxConn->mSRCounter++ < mS->mC.max_passive_sr))) {
		other->mxConn->Send(ostr, true);
	}
	return 0;
}

int cDCProto::DC_OpForceMove(cMessageDC * msg, cConnDC * conn)
//$ForceMove <newIp>
//$To: <victimNick> From: <senderNick> $<<senderNick>> You are being re-directed to <newHub> because: <reasonMsg>
{
	if(msg->SplitChunks())
		return -1;
	if(!conn->mpUser || !conn->mpUser->mInList)
		return -2;
	ostringstream ostr;

	string &str = msg->ChunkString(eCH_FM_NICK);

	// check rights
	if(!conn->mpUser || conn->mpUser->mClass < mS->mC.min_class_redir) {
		if(conn->Log(1))
			conn->LogStream() << "Tried to redirect " << str  << endl;
		ostr << _("You have not rights to use redirects.");
		mS->ConnCloseMsg(conn,ostr.str(),2000, eCR_SYNTAX);
		return -1;
	}

	cUser *other = mS->mUserList.GetUserByNick ( str );

	// Check other nick
	if(!other) {
		ostr << autosprintf(_("User %s not found."), str.c_str());
		mS->DCPublicHS(ostr.str(),conn);
		return -2;
	}

	// check privileges
	if(other->mClass >= conn->mpUser->mClass || other->mProtectFrom >= conn->mpUser->mClass) {
		ostr << autosprintf(_("User %s is protected or you have no rights on him"), str.c_str());
		mS->DCPublicHS(ostr.str(),conn);
		return -3;
	}

	// create the message
	string omsg("$ForceMove ");
	omsg += msg->ChunkString(eCH_FM_DEST);
	omsg += "|";


	ostringstream redirectReason;
	redirectReason << autosprintf(_("You are being redirected to %s because: %s"), msg->ChunkString(eCH_FM_DEST).c_str(), msg->ChunkString(eCH_FM_REASON).c_str());
	Create_PM(omsg,conn->mpUser->mNick, msg->ChunkString(eCH_FM_NICK), conn->mpUser->mNick, redirectReason.str());

	if(other->mxConn) {
		// Send $ForceMove and PM
		other->mxConn->Send(omsg);
		// Close user connection
		other->mxConn->CloseNice(3000, eCR_FORCEMOVE);
		if(conn->Log(2))
			conn->LogStream() << "ForceMove " << str  << " to: " << msg->ChunkString(eCH_FM_DEST)<< " because : " << msg->ChunkString(eCH_FM_REASON) << endl;
	} else {
		mS->DCPrivateHS(_("User is not online or you tried to redirect a robot."),conn);
	}
	return 0;
}


int cDCProto::DCE_Supports(cMessageDC * msg, cConnDC * conn)
{
	if (msg->SplitChunks()) return -1;
	string &supports = msg->ChunkString(eCH_1_PARAM);
	conn->mSupportsText = supports; // save user supports in text format

	istringstream is(msg->mStr);
	string feature;
	is >> feature;

	while (1) {
		feature = this->mS->mEmpty;
		is >> feature;
		if (!feature.size()) break;
		if (feature == "OpPlus") conn->mFeatures |= eSF_OPPLUS;
		else if (feature == "NoHello") conn->mFeatures |= eSF_NOHELLO;
		else if (feature == "NoGetINFO") conn->mFeatures |= eSF_NOGETINFO;
		else if (feature == "DHT0") conn->mFeatures |= eSF_DHT0;
		else if (feature == "QuickList") conn->mFeatures |= eSF_QUICKLIST;
		else if (feature == "BotINFO") conn->mFeatures |= eSF_BOTINFO;
		else if (feature == "ZPipe0" || feature == "ZPipe") conn->mFeatures |= eSF_ZLIB;
		else if (feature == "ChatOnly") conn->mFeatures |= eSF_CHATONLY;
		else if (feature == "MCTo") conn->mFeatures |= eSF_MCTO;
		else if (feature == "UserCommand") conn->mFeatures |= eSF_USERCOMMAND;
		else if (feature == "BotList") conn->mFeatures |= eSF_BOTLIST;
		else if (feature == "HubTopic") conn->mFeatures |= eSF_HUBTOPIC;
		else if (feature == "UserIP2") conn->mFeatures |= eSF_USERIP2;
		else if (feature == "TTHSearch") conn->mFeatures |= eSF_TTHSEARCH;
		else if (feature == "Feed") conn->mFeatures |= eSF_FEED;
		else if (feature == "ClientID") conn->mFeatures |= eSF_CLIENTID;
		else if (feature == "IN") conn->mFeatures |= eSF_IN;
		else if (feature == "BanMsg") conn->mFeatures |= eSF_BANMSG;
		else if (feature == "TLS") conn->mFeatures |= eSF_TLS;
		else if (feature == "IPv4") conn->mFeatures |= eSF_IPV4;
		else if (feature == "IP64") conn->mFeatures |= eSF_IP64;
	}

	#ifndef WITHOUT_PLUGINS
	if (!mS->mCallBacks.mOnParsedMsgSupport.CallAll(conn, msg)) {
		conn->CloseNow();
		return -1;
	}
	#endif

	string omsg("$Supports OpPlus NoGetINFO NoHello UserIP2 HubINFO HubTopic ZPipe MCTo BotList");
	conn->Send(omsg);
	return 0;
}

int cDCProto::DCO_TempBan(cMessageDC * msg, cConnDC * conn)
{
	if(!conn || !conn->mpUser || !conn->mpUser->mInList || conn->mpUser->mClass < eUC_OPERATOR)
		return -1;
	if(msg->SplitChunks())
		return -1;

	ostringstream os;
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
		os << _("You cannot ban a protected user or with higher privilegies.");
		mS->DCPublicHS(os.str(),conn);
		return -1;
	}

	if(!other->mxConn) {
		os << _("You cannot ban a robot.");
		mS->DCPublicHS(os.str(),conn);
		return -1;
	}

	if(period)
		os << autosprintf(_("You are being temporarily banned for %s because %s"), msg->ChunkString(eCH_NB_TIME).c_str(), msg->ChunkString(eCH_NB_REASON).c_str());
	else
		os << autosprintf(_("You are banned because %s"), msg->ChunkString(eCH_NB_REASON).c_str());

	mS->DCPrivateHS(os.str(), other->mxConn, &conn->mpUser->mNick);
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

int cDCProto::NickList(cConnDC *conn)
{
	try {
		bool complete_infolist = false;
		// 2 = show to all
		if (mS->mC.show_tags >= 2) complete_infolist = true;
		if (conn->mpUser && (conn->mpUser->mClass >= eUC_OPERATOR)) complete_infolist = true;
		// 0 = hide to all
		if (mS->mC.show_tags == 0) complete_infolist = false;
		if (conn->GetLSFlag(eLS_LOGIN_DONE) != eLS_LOGIN_DONE) conn->mNickListInProgress = true;

		if (conn->mFeatures & eSF_NOHELLO) {
			if (conn->Log(3)) conn->LogStream() << "Sending MyINFO list" << endl;
			conn->Send(mS->mUserList.GetInfoList(complete_infolist), true);
		} else if (conn->mFeatures & eSF_NOGETINFO) {
			if (conn->Log(3)) conn->LogStream() << "Sending MyINFO list" << endl;
			conn->Send(mS->mUserList.GetNickList(), true);
			conn->Send(mS->mUserList.GetInfoList(complete_infolist), true);
		} else {
			if (conn->Log(3)) conn->LogStream() << "Sending Nicklist" << endl;
			conn->Send(mS->mUserList.GetNickList(), true);
		}

		conn->Send(mS->mOpList.GetNickList(), true); // send $OpList
		conn->Send(mS->mRobotList.GetNickList(), true); // send $BotList
	} catch(...) {
		if (conn->ErrLog(2)) conn->LogStream() << "Exception in cDCProto::NickList" << endl;
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

int cDCProto::DCO_UnBan(cMessageDC * msg, cConnDC * conn)
{
	if(!conn || !conn->mpUser || !conn->mpUser->mInList || conn->mpUser->mClass < eUC_OPERATOR)
		return -1;
	if(msg->SplitChunks())
		return -1;

	string ip, nick, host;
	ostringstream os;
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
	return 1;

	return 0;
}

int cDCProto::DCO_GetBanList(cMessageDC * msg, cConnDC * conn)
{
	if(!conn || !conn->mpUser || !conn->mpUser->mInList || conn->mpUser->mClass < eUC_OPERATOR)
		return -1;
	//@todo mS->mBanList->GetBanList(conn);
	return 0;
}

int cDCProto::DCB_BotINFO(cMessageDC * msg, cConnDC * conn)
{
	if (msg->SplitChunks())
		return -1;

	ostringstream os;

	if (!(conn->mFeatures & eSF_BOTINFO)) {
		if (conn->Log(2))
			conn->LogStream() << "User " << conn->mpUser->mNick << " sent $BotINFO but BotINFO extension is not set in $Supports" << endl;

		os << _("You can't send $BotINFO because BotINFO extension is not set in $Supports.");
		mS->DCPublicHS(os.str(), conn);
		return 0;
	}

	if (conn->Log(2))
		conn->LogStream() << "Bot visit: " << msg->ChunkString(eCH_1_PARAM) << endl;

	if (mS->mC.botinfo_report) {
		os << autosprintf(_("Pinger entered the hub: %s"), msg->ChunkString(eCH_1_PARAM).c_str());
		mS->ReportUserToOpchat(conn, os.str());
		os.str("");
	}

	#ifndef WITHOUT_PLUGINS
		if (!mS->mCallBacks.mOnParsedMsgBotINFO.CallAll(conn, msg)) {
			conn->CloseNow();
			return -1;
		}
	#endif

	char S = '$';
	cConnType *ConnType = mS->mConnTypes->FindConnType("default");
	__int64 hl_minshare = mS->mC.min_share;

	if (mS->mC.min_share_use_hub > hl_minshare)
		hl_minshare = mS->mC.min_share_use_hub;

	if (!mS->mC.hub_icon_url.empty())
		os << "$SetIcon " << mS->mC.hub_icon_url.c_str() << "|";

	if (!mS->mC.hub_logo_url.empty())
		os << "$SetLogo " << mS->mC.hub_logo_url.c_str() << "|";

	os << "$HubINFO "
	<< mS->mC.hub_name << S
	<< mS->mC.hub_host << S
	<< mS->mC.hub_desc << S
	<< mS->mC.max_users_total << S
	<< StringFrom((__int64)(1024 * 1024) * hl_minshare) << S
	<< ConnType->mTagMinSlots << S
	<< mS->mC.tag_max_hubs << S
	<< "Verlihub" << S
	<< mS->mC.hub_owner << S
	<< mS->mC.hub_category << S
	<< mS->mC.hub_encoding;

	string str = os.str();
	conn->Send(str);
	return 0;
}

int cDCProto::DCO_WhoIP(cMessageDC * msg, cConnDC * conn)
{
	if(msg->SplitChunks()) return -1;
	if(!conn || !conn->mpUser || !conn->mpUser->mInList || conn->mpUser->mClass < eUC_OPERATOR) return -1;
	string nicklist("$UsersWithIp ");
	string sep("$$");
	nicklist += msg->ChunkString(eCH_1_PARAM);
	nicklist += "$";
	unsigned long num = cBanList::Ip2Num(msg->ChunkString(eCH_1_PARAM));
	mS->WhoIP(num ,num , nicklist, sep, true);
	conn->Send(nicklist);
	return 0;
}

int cDCProto::DCO_GetTopic(cMessageDC *, cConnDC *conn)
{
	if (!conn->mpUser) return -1;
	if (!conn->mpUser->mInList) return -2;

	if (!mS->mC.hub_topic.empty()) {
		string topic("$HubTopic ");
		topic += mS->mC.hub_topic;
		conn->Send(topic);
	}

	return 0;
}

/** Change the hub's topic by sending $SetTopic; if the user has no rights it returns an error */
int cDCProto::DCO_SetTopic(cMessageDC * msg, cConnDC * conn)
{
	if(msg->SplitChunks())
		return -1;
	if(!conn->mpUser || !conn->mpUser->mInList || conn->mpUser->mClass < eUC_OPERATOR)
		return -2;
	// check rights
	if(conn->mpUser->mClass < mS->mC.topic_mod_class) {
		mS->DCPublicHS(_("You do not have permissions to change the hub topic."),conn);
		return 0;
	}
	string &str = msg->ChunkString(eCH_1_PARAM);
	mS->mC.hub_topic = str;

	ostringstream os;
	os << autosprintf(_("Topis is set to: %s"), str.c_str());
	mS->DCPublicHS(os.str(), conn);
	return 0;
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
