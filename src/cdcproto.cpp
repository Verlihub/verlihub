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

#include "cserverdc.h"
#include "cdcproto.h"
#include "cconndc.h"
#include "creglist.h"
#include "cbanlist.h"
#include "cmessagedc.h"
#include "cdctag.h"
#include "stringutils.h"
#include "cdcconsole.h"
#include "i18n.h"

#include <string>
#include <string.h>
#include <stdio.h>

#ifdef HAVE_CONFIG_H
	#include <config.h>
#endif

using std::string;

namespace nVerliHub {
	using namespace nUtils;
	using namespace nEnums;
	using namespace nSocket;
	using namespace nSocket;

	namespace nProtocol {

cDCProto::cDCProto(cServerDC *serv):
	mS(serv)
{
	if (!mKickChatPattern.Compile("^((\\S+) )?is kicking (\\S+) [bB]ecause: (.*)$"))
		throw "error in kickchatpattern";

	if (!mKickBanPattern.Compile("_[bB][aA][nN]_(\\d+[smhdwySHMDWY]?)?"))
		throw "error in kickbanpattern";

	SetClassName("cDCProto");
}

cDCProto::~cDCProto()
{}

cMessageParser *cDCProto::CreateParser()
{
	return new cMessageDC;
}

void cDCProto::DeleteParser(cMessageParser *OldParser)
{
	if (OldParser != NULL) {
		delete OldParser;
		OldParser = NULL;
	}
}

int cDCProto::TreatMsg(cMessageParser *pMsg, cAsyncConn *pConn)
{
	if (!pMsg || !pConn)
		return -1;

	cMessageDC *msg = (cMessageDC*)pMsg;
	cConnDC *conn = (cConnDC*)pConn;

	/*
		todo
			tMsgAct action = mS->Filter(tDCMsg(msg->mType), conn);
	*/

	size_t msg_strlen = strlen(msg->mStr.data()); // note: we use strlen here because it might contain null character

	if (msg_strlen < msg->mLen) { // look for null character, message is already parsed by this moment
		if (msg->mType == eDCC_LOCK) { // bug is some clients that send null character in referer
			msg->mStr.resize(msg_strlen); // resize message until null character and continue

			if (conn->Log(1))
				conn->LogStream() << "NULL character removed from message: " << msg->mStr << endl;
		} else { // other messages
			int end_cnt = 0;

			while (msg->mStr.size() && (msg->mStr[msg->mStr.size() - 1] == '\0')) { // try to fix message by removing all null characters in the end
				msg->mStr.resize(msg->mStr.size() - 1);
				end_cnt++;
			}

			if (end_cnt)
				msg_strlen = strlen(msg->mStr.data());

			if (!end_cnt || (msg_strlen < msg->mStr.size())) { // look for null character again
				msg->mStr.resize(msg_strlen); // resize message until null character so we can use mStr below

				if (conn->Log(1))
					conn->LogStream() << "NULL character found in message: " << msg->mStr << endl;

				if (mS->mC.nullchars_report) {
					ostringstream os;
					os << autosprintf(_("Probably attempt of NULL character attack: %s"), msg->mStr.c_str());
					mS->ReportUserToOpchat(conn, os.str());
				}

				conn->CloseNow(eCR_SYNTAX);
				return -1;
			}

			if (conn->Log(1))
				conn->LogStream() << "NULL character removed from the end of message " << end_cnt << " times: " << msg->mStr << endl;
		}

		msg->Parse(); // parse message again because parser sets state variables
	}

	#ifndef WITHOUT_PLUGINS
		if (msg->mType != eMSG_UNPARSED) {
			if (conn->mpUser) {
				if (!mS->mCallBacks.mOnParsedMsgAny.CallAll(conn, msg))
					return 1;
			} else {
				if (!mS->mCallBacks.mOnParsedMsgAnyEx.CallAll(conn, msg)) {
					conn->CloseNow(eCR_PLUGIN);
					return -1;
				}
			}
		}
	#endif

	switch (msg->mType) {
		case eDC_UNKNOWN: // this must be first
			if (msg->mStr.size()) {
				if (Log(2)) // dont log pings
					LogStream() << "Incoming unknown command: " << msg->mStr << endl;

				mS->mProtoCount[eDC_UNKNOWN]++;
			} else {
				mS->mProtoCount[eDC_UNKNOWN + 1]++;
			}

			this->DCU_Unknown(msg, conn);
			return 1;

		case eMSG_UNPARSED:
			msg->Parse();
			return TreatMsg(msg, conn); // todo: mS->mCallBacks.mOnUnparsedMsg.CallAll(conn, msg)

		case eDC_CONNECTTOME:
		case eDC_MCONNECTTOME: // todo: this->DC_MultiConnectToMe(msg, conn);
			this->DC_ConnectToMe(msg, conn);
			break;

		case eDC_RCONNECTTOME:
			this->DC_RevConnectToMe(msg, conn);
			break;

		case eDC_SR:
			this->DC_SR(msg, conn);
			break;

		case eDC_SEARCH_PAS:
		case eDC_SEARCH:
		case eDC_MSEARCH_PAS:
		case eDC_MSEARCH:
			this->DC_Search(msg, conn);
			break;

		case eDC_TTHS:
			this->DC_SA(msg, conn);
			break;

		case eDC_TTHS_PAS:
			this->DC_SP(msg, conn);
			break;

		case eDC_MYINFO:
			this->DC_MyINFO(msg, conn);
			break;

		case eDC_EXTJSON:
			this->DC_ExtJSON(msg, conn);
			break;

		case eDC_KEY:
			this->DC_Key(msg, conn);
			break;

		case eDC_SUPPORTS:
			this->DC_Supports(msg, conn);
			break;

		case eDC_VALIDATENICK:
			this->DC_ValidateNick(msg, conn);
			break;

		case eDC_VERSION:
			this->DC_Version(msg, conn);
			break;

		case eDC_GETNICKLIST:
			this->DC_GetNickList(msg, conn);
			break;

		case eDC_MYHUBURL:
			this->DC_MyHubURL(msg, conn);
			break;

		case eDC_MYPASS:
			this->DC_MyPass(msg, conn);
			break;

		case eDC_TO:
			this->DC_To(msg, conn);
			break;

		case eDC_CHAT:
			this->DC_Chat(msg, conn);
			break;

		case eDCB_BOTINFO:
			this->DCB_BotINFO(msg, conn);
			break;

		case eDC_GETINFO:
			this->DC_GetINFO(msg, conn);
			break;

		case eDCO_USERIP:
			this->DCO_UserIP(msg, conn);
			break;

		case eDCO_KICK:
			this->DCO_Kick(msg, conn);
			break;

		case eDCO_OPFORCEMOVE:
			this->DCO_OpForceMove(msg, conn);
			break;

		case eDC_MCTO:
			this->DC_MCTo(msg, conn);
			break;

		case eDC_QUIT:
			mS->DCPublicHS(_("See you."), conn);
			conn->CloseNice(1000, eCR_QUIT);
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

		case eDCC_MYIP:
			this->DCC_MyIP(msg, conn);
			break;

		case eDCC_MYNICK:
			this->DCC_MyNick(msg, conn);
			break;

		case eDCC_LOCK:
			this->DCC_Lock(msg, conn);
			break;

		case eDC_IN:
			this->DC_IN(msg, conn);
			break;

		default:
			if (Log(1))
				LogStream() << "Incoming untreated command: " << msg->mStr << endl;

			if (msg->mStr.size())
				mS->mProtoCount[eDC_UNKNOWN]++;
			else
				mS->mProtoCount[eDC_UNKNOWN + 1]++;

			return 0;
	}

	mS->mProtoCount[msg->mType]++;
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
		string key, par = msg->ChunkString(eCH_1_PARAM);
		Lock2Key(conn->mLock, key);

		if (StrCompare(key, 0, par.size(), par) != 0) {
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
	//conn->mT.key = mS->mTime;
	conn->mLock.clear(); // not needed anymore
	ShrinkStringToFit(conn->mLock);
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

	if (CheckProtoLen(conn, msg))
		return -1;

	if (CheckProtoSyntax(conn, msg))
		return -1;

	const string &supports = msg->ChunkString(eCH_1_PARAM);
	conn->mSupportsText = supports; // save user supports in plain text format
	istringstream is(supports);
	string feature, omsg, pars;

	while (1) {
		feature = mS->mEmpty;
		is >> feature;

		if (feature.empty())
			break;

		if ((feature.size() == 6) && (StrCompare(feature, 0, 6, "OpPlus") == 0)) {
			conn->mFeatures |= eSF_OPPLUS;
			pars.append("OpPlus ");

		} else if ((feature.size() == 7) && (StrCompare(feature, 0, 7, "NoHello") == 0)) {
			conn->mFeatures |= eSF_NOHELLO;
			pars.append("NoHello ");

		} else if ((feature.size() == 9) && (StrCompare(feature, 0, 9, "NoGetINFO") == 0)) {
			conn->mFeatures |= eSF_NOGETINFO;
			pars.append("NoGetINFO ");

		} else if ((feature.size() == 4) && (StrCompare(feature, 0, 4, "DHT0") == 0)) {
			conn->mFeatures |= eSF_DHT0;

		} else if ((feature.size() == 9) && (StrCompare(feature, 0, 9, "QuickList") == 0)) {
			conn->mFeatures |= eSF_QUICKLIST;

		} else if ((feature.size() == 7) && (StrCompare(feature, 0, 7, "BotINFO") == 0)) {
			conn->mFeatures |= eSF_BOTINFO;
			pars.append("HubINFO ");

		} else if (((feature.size() == 6) && (StrCompare(feature, 0, 6, "ZPipe0") == 0)) || ((feature.size() == 5) && (StrCompare(feature, 0, 5, "ZPipe") == 0))) {
			conn->mFeatures |= eSF_ZLIB;
			conn->mZLibFlag = true;
			pars.append("ZPipe0 "); // send even if disabled, we might want to turn it on any time, todo: which clients actually use ZPipe without 0?

		} else if ((feature.size() == 8) && (StrCompare(feature, 0, 8, "ChatOnly") == 0)) {
			conn->mFeatures |= eSF_CHATONLY;
			pars.append("ChatOnly ");

		} else if ((feature.size() == 4) && (StrCompare(feature, 0, 4, "MCTo") == 0)) {
			conn->mFeatures |= eSF_MCTO;
			pars.append("MCTo ");

		} else if ((feature.size() == 11) && (StrCompare(feature, 0, 11, "UserCommand") == 0)) {
			conn->mFeatures |= eSF_USERCOMMAND;

		} else if ((feature.size() == 7) && (StrCompare(feature, 0, 7, "BotList") == 0)) {
			conn->mFeatures |= eSF_BOTLIST;
			pars.append("BotList ");

		} else if ((feature.size() == 8) && (StrCompare(feature, 0, 8, "HubTopic") == 0)) {
			conn->mFeatures |= eSF_HUBTOPIC;
			pars.append("HubTopic ");

		} else if ((feature.size() == 7) && (StrCompare(feature, 0, 7, "UserIP2") == 0)) {
			conn->mFeatures |= eSF_USERIP2;
			pars.append("UserIP2 ");

		} else if ((feature.size() == 9) && (StrCompare(feature, 0, 9, "TTHSearch") == 0)) {
			conn->mFeatures |= eSF_TTHSEARCH;
			pars.append("TTHSearch ");

		} else if ((feature.size() == 4) && (StrCompare(feature, 0, 4, "Feed") == 0)) {
			conn->mFeatures |= eSF_FEED;

		/*
		} else if ((feature.size() == 8) && (StrCompare(feature, 0, 8, "ClientID") == 0)) {
			conn->mFeatures |= eSF_CLIENTID;
		*/

		} else if ((feature.size() == 4) && (StrCompare(feature, 0, 4, "TTHS") == 0)) {
			conn->mFeatures |= eSF_TTHS;
			pars.append("TTHS ");

		} else if ((feature.size() == 2) && (StrCompare(feature, 0, 2, "IN") == 0)) {
			conn->mFeatures |= eSF_IN;
			//pars.append("IN "); // todo

		} else if ((feature.size() == 6) && (StrCompare(feature, 0, 6, "BanMsg") == 0)) {
			conn->mFeatures |= eSF_BANMSG;

		} else if ((feature.size() == 3) && (StrCompare(feature, 0, 3, "TLS") == 0)) {
			conn->mFeatures |= eSF_TLS;
			pars.append("TLS ");

		} else if ((feature.size() == 8) && (StrCompare(feature, 0, 8, "FailOver") == 0)) {
			conn->mFeatures |= eSF_FAILOVER;
			pars.append("FailOver ");

		} else if ((feature.size() == 10) && (StrCompare(feature, 0, 10, "NickChange") == 0)) {
			conn->mFeatures |= eSF_NICKCHANGE;

		} else if ((feature.size() == 10) && (StrCompare(feature, 0, 10, "ClientNick") == 0)) {
			conn->mFeatures |= eSF_CLIENTNICK;

		} else if ((feature.size() == 5) && (StrCompare(feature, 0, 5, "ZLine") == 0)) {
			conn->mFeatures |= eSF_ZLINE;

		} else if ((feature.size() == 9) && (StrCompare(feature, 0, 9, "GetZBlock") == 0)) {
			conn->mFeatures |= eSF_GETZBLOCK;

		} else if ((feature.size() == 4) && (StrCompare(feature, 0, 4, "ACTM") == 0)) {
			conn->mFeatures |= eSF_ACTM;

		} else if ((feature.size() == 8) && (StrCompare(feature, 0, 8, "SaltPass") == 0)) {
			conn->mFeatures |= eSF_SALTPASS;

		} else if ((feature.size() == 8) && (StrCompare(feature, 0, 8, "NickRule") == 0)) {
			conn->mFeatures |= eSF_NICKRULE;
			pars.append("NickRule ");

		} else if ((feature.size() == 10) && (StrCompare(feature, 0, 10, "SearchRule") == 0)) {
			conn->mFeatures |= eSF_SEARRULE;
			pars.append("SearchRule ");

		} else if ((feature.size() == 6) && (StrCompare(feature, 0, 6, "HubURL") == 0)) {
			conn->mFeatures |= eSF_HUBURL;
			pars.append("HubURL ");

		} else if ((feature.size() == 8) && (StrCompare(feature, 0, 8, "ExtJSON2") == 0)) {
			conn->mFeatures |= eSF_EXTJSON2;

			if (!mS->mC.disable_extjson)
				pars.append("ExtJSON2 ");
		}
	}

	#ifndef WITHOUT_PLUGINS
	string copy(pars);

	if (mS->mCallBacks.mOnParsedMsgSupports.CallAll(conn, msg, &copy))
	#endif
	{
		Create_Supports(omsg, pars); // send our supports based on client supports
		conn->Send(omsg, true);
	}

	if (conn->mFeatures & eSF_NICKRULE) { // send nick rule command
		pars.clear();
		pars.append("Min ");

		if (mS->mC.min_nick > 255)
			pars.append("255");
		else
			pars.append(StringFrom(mS->mC.min_nick));

		pars.append("$$Max ");
		pars.append(StringFrom(mS->mC.max_nick));
		pars.append("$$Char ");
		pars.append(StrByteList(string(BAD_NICK_CHARS_OWN))); // dont list all forbidden nmdc characters
		pars.append("$$");

		if (mS->mC.nick_prefix.size()) {
			pars.append("Pref ");
			pars.append(mS->mC.nick_prefix);
			pars.append("$$");
		}

		Create_NickRule(omsg, pars);
		conn->Send(omsg, true);
	}

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

	const string &nick = msg->ChunkString(eCH_1_PARAM);

	if (conn->Log(3)) // log new user
		conn->LogStream() << "User " << nick << " tries to login" << endl;

	int closeReason; // check if nick is valid or close the connection

	if (!mS->ValidateUser(conn, nick, closeReason)) {
		conn->CloseNice(2000, closeReason); // give it little more time, it seems that some clients with slow connection or ping dont receive ban messages sometimes
		return -1;
	}

	unsigned int limit = mS->mC.max_users_total; // user limits
	unsigned int limit_cc = mS->mC.max_users[conn->mGeoZone];
	unsigned int limit_extra = 0;

	if (conn->GetTheoricalClass() == eUC_PINGER) // calculate user limits
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
	string omsg;

	if ((conn->GetTheoricalClass() < eUC_OPERATOR) && ((mS->mUserCountTot >= limit) || (mS->mUserCount[conn->mGeoZone] >= limit_cc))) { // check the max_users limit
		if (mS->mUserCount[conn->mGeoZone] >= limit_cc) {
			string zonestr, zonedat;

			if ((conn->mGeoZone >= 1) && (conn->mGeoZone <= 3)) { // country zones
				zonestr = _("User limit in country zone %s exceeded at %d/%d online users.");
				zonedat = mS->mC.cc_zone[conn->mGeoZone - 1];
			} else if ((conn->mGeoZone >= 4) && (conn->mGeoZone <= 6)) { // ip range zones
				zonestr = _("User limit in IP zone %s exceeded at %d/%d online users.");

				switch (conn->mGeoZone) {
					case 4:
						zonedat = mS->mC.ip_zone4_min + '-' + mS->mC.ip_zone4_max;
					break;
					case 5:
						zonedat = mS->mC.ip_zone5_min + '-' + mS->mC.ip_zone5_max;
					break;
					case 6:
						zonedat = mS->mC.ip_zone6_min + '-' + mS->mC.ip_zone6_max;
					break;
				}
			} else { // main zone
				zonestr = _("User limit in main zone exceeded at %d/%d online users.");
			}

			if (conn->mGeoZone == 0)
				os << autosprintf(zonestr.c_str(), mS->mUserCount[conn->mGeoZone], mS->mUserCountTot);
			else
				os << autosprintf(zonestr.c_str(), zonedat.c_str(), mS->mUserCount[conn->mGeoZone], mS->mUserCountTot);
		} else {
			os << autosprintf(_("User limit exceeded at %d online users."), mS->mUserCountTot);
		}

		if (mS->mC.max_users_total == 0) {
			os << ' ';

			if (!mS->mC.hubfull_message.empty())
				os << mS->mC.hubfull_message;
			else
				os << _("This is a registered users only hub.");
		}

		if (conn->Log(2)) { // this is because GetGeoCC also writes logs
			conn->LogStream() << "Hub is full, see results on next line" << endl;
			omsg = conn->GetGeoCC();
		}

		if (conn->Log(2))
			conn->LogStream() << mS->mUserCountTot << '/' << limit << " :: " << mS->mUserCount[conn->mGeoZone] << '/' << limit_cc << " :: " << omsg << endl;

		mS->ConnCloseMsg(conn, os.str(), 1000, eCR_USERLIMIT);
		Create_HubIsFull(omsg); // must be sent after chat message
		conn->Send(omsg, true);
		return -1;

	} else {
		conn->SetLSFlag(eLS_ALLOWED);
		mS->mUserCountTot++;
		mS->mUserCount[conn->mGeoZone]++;
	}

	if ((mS->mC.max_users_from_ip != 0) && (conn->GetTheoricalClass() < eUC_VIPUSER) && mS->CntConnIP(conn->AddrToNumber(), mS->mC.max_users_from_ip)) { // user limit from single ip
		os << autosprintf(_("User limit from IP address %s exceeded at %d online users."), conn->mAddrIP.c_str(), mS->mC.max_users_from_ip);
		mS->ConnCloseMsg(conn, os.str(), 1000, eCR_USERLIMIT);
		Create_HubIsFull(omsg); // must be sent after chat message
		conn->Send(omsg, true);
		return -1;
	}

	if (mS->mC.hub_name.size()) { // send hub name without topic
		Create_HubName(omsg, mS->mC.hub_name, "");
		conn->Send(omsg, true);
	}

	if (conn->mRegInfo && conn->mRegInfo->mAuthIP.size() && (StrCompare(conn->mRegInfo->mAuthIP, 0, conn->mAddrIP.size(), conn->mAddrIP) != 0)) { // check authorization ip
		mS->mR->LoginError(conn, nick); // important

		if (conn->GetTheoricalClass() >= mS->mC.wrongauthip_report)
			mS->ReportUserToOpchat(conn, autosprintf(_("Authorization IP mismatch from %s"), nick.c_str()));

		os << autosprintf(_("Authorization IP for this account doesn't match your IP address: %s"), conn->mAddrIP.c_str());
		mS->mBanList->AddIPTempBan(conn->AddrToNumber(), mS->mTime.Sec() + mS->mC.pwd_tmpban, os.str(), eBT_PASSW); // ban ip for pwd_tmpban
		mS->ConnCloseMsg(conn, os.str(), 1000, eCR_LOGIN_ERR);
		return -1;
	}

	if (conn->mRegInfo && conn->mRegInfo->mFakeIP.size()) { // set fake ip
		if (conn->SetUserIP(conn->mRegInfo->mFakeIP))
			conn->ResetGeo();
	}

	if (conn->NeedsPassword()) {
		Create_GetPass(omsg);
		conn->Send(omsg, true);
	} else {
		mS->mP.Create_Hello(omsg, nick);
		conn->Send(omsg, true);
		conn->SetLSFlag(eLS_PASSWD);

		if (conn->mFeatures & eSF_HUBURL) { // send hub url command
			Create_GetHubURL(omsg);
			conn->Send(omsg, true);
		}
	}

	//try {
		cUser *NewUser = new cUser(nick);

		if (!conn->SetUser(NewUser)) {
			conn->CloseNow(eCR_INVALID_USER);
			return -1;
		}

		#ifndef WITHOUT_PLUGINS
			if (!mS->mCallBacks.mOnParsedMsgValidateNick.CallAll(conn, msg)) {
				conn->CloseNow(eCR_PLUGIN);
				return -2;
			}
		#endif
	/*
	} catch (...) {
		if (mS->ErrLog(2))
			mS->LogStream() << "Unhandled exception in cServerDC::DC_ValidateNick" << endl;

		if (conn->Log(2))
			conn->LogStream() << "Fatal error calling SetUser, closing" << endl;

		mS->ConnCloseMsg(conn, _("Undefined error occurred."), 1000);
		return -1;
	}
	*/

	if (conn->mpUser) {
		if (conn->mRegInfo && (conn->mRegInfo->mClass == eUC_PINGER)) {
			conn->mpUser->Register();
			mS->mR->Login(conn, nick);
		} else if (conn->mFeatures & eSF_BOTINFO) {
			conn->mpUser->mClass = eUC_PINGER;
		}
	}

	conn->SetLSFlag(eLS_VALNICK | eLS_NICKLST); // set eLS_NICKLST because user may want to skip getting userlist
	conn->ClearTimeOut(eTO_VALNICK);
	conn->SetTimeOut(eTO_MYINFO, mS->mC.timeout_length[eTO_MYINFO], mS->mTime);
	return 0;
}

int cDCProto::DC_MyPass(cMessageDC *msg, cConnDC *conn)
{
	if (CheckUserLogin(conn, msg, false))
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
			conn->CloseNow(eCR_PLUGIN);
			return -1;
		}
	#endif

	const string &pwd = msg->ChunkString(eCH_1_PARAM);

	if (conn->mpUser->mSetPass) { // set password request
		if (!conn->mRegInfo || !conn->mRegInfo->mPwdChange) {
			conn->mpUser->mSetPass = false;
			return 0;
		}

		if (pwd.size() < mS->mC.password_min_len) {
			os << autosprintf(_("Minimum password length is %d characters. Please retry."), mS->mC.password_min_len);
			mS->DCPrivateHS(os.str(), conn);
			mS->DCPublicHS(os.str(), conn);
			conn->mpUser->mSetPass = false;
			return 0;
		}

		if (!mS->mR->ChangePwd(conn->mpUser->mNick, pwd, conn)) {
			os << _("Error updating password.");
			mS->DCPrivateHS(os.str(), conn);
			mS->DCPublicHS(os.str(), conn);
			conn->mpUser->mSetPass = false;
			return 0;
		}

		conn->mpUser->mClass = tUserCl(conn->mRegInfo->getClass());

		if ((conn->mpUser->mClass >= mS->mC.opchat_class) && !mS->mOpchatList.ContainsHash(conn->mpUser->mNickHash)) // opchat list
			mS->mOpchatList.AddWithHash(conn->mpUser, conn->mpUser->mNickHash);

		if ((conn->mpUser->mClass >= mS->mC.oplist_class) && !mS->mOpList.ContainsHash(conn->mpUser->mNickHash)) { // oplist
			mS->mOpList.AddWithHash(conn->mpUser, conn->mpUser->mNickHash);
			string str;
			mS->mP.Create_OpList(str, conn->mpUser->mNick); // send short oplist
			mS->mUserList.SendToAll(str, mS->mC.delayed_myinfo, true);
		}

		mS->SetUserRegInfo(conn, conn->mpUser->mNick); // update registration information in real time aswell
		os << _("Password updated successfully.");
		mS->DCPrivateHS(os.str(), conn);
		mS->DCPublicHS(os.str(), conn);
		conn->ClearTimeOut(eTO_SETPASS);
		conn->mpUser->mSetPass = false;
		return 0;
	}

	string omsg;

	if (conn->mpUser->CheckPwd(pwd)) { // check password
		conn->SetLSFlag(eLS_PASSWD);
		conn->mpUser->Register(); // set class
		mS->mR->Login(conn, conn->mpUser->mNick);
		mS->mP.Create_Hello(omsg, conn->mpUser->mNick);
		conn->Send(omsg, true);

		if (conn->mFeatures & eSF_HUBURL) { // send hub url command
			Create_GetHubURL(omsg);
			conn->Send(omsg, true);
		}

		if (conn->mpUser->mClass >= eUC_OPERATOR) { // operators get $LogedIn
			Create_LogedIn(omsg, conn->mpUser->mNick);
			conn->Send(omsg, true);
		}

	} else { // wrong password
		if (conn->mRegInfo && (tUserCl(conn->mRegInfo->getClass()) != eUC_NORMUSER)) { // user is regged
			omsg = _("Incorrect password");

			if (mS->mC.wrongpassword_report)
				mS->ReportUserToOpchat(conn, omsg);

			if (mS->mC.wrongpass_message.size())
				omsg = mS->mC.wrongpass_message;

			#ifndef WITHOUT_PLUGINS
			if (mS->mCallBacks.mOnBadPass.CallAll(conn->mpUser))
			#endif
			{
				mS->mBanList->AddIPTempBan(conn->AddrToNumber(), mS->mTime.Sec() + mS->mC.pwd_tmpban, omsg, eBT_PASSW); // ban ip instead of nick
			}

			if (conn->Log(2))
				conn->LogStream() << omsg << endl;

			mS->mR->LoginError(conn, conn->mpUser->mNick);
			mS->ConnCloseMsg(conn, _("You've been temporarily banned due to incorrect password."), 1000, eCR_PASSWORD);
			Create_BadPass(omsg); // must be sent after chat message
			conn->Send(omsg, true);
		} else {
			if (conn->Log(3))
				conn->LogStream() << "Password without reg" << endl;
		}

		return -1;
	}

	return 0;
}

int cDCProto::DC_MyHubURL(cMessageDC *msg, cConnDC *conn)
{
	if (CheckUserLogin(conn, msg, false))
		return -1;

	ostringstream os;

	if (!(conn->mFeatures & eSF_HUBURL)) { // check support
		os << _("Invalid login sequence, you didn't specify support for hub URL command.");

		if (conn->Log(1))
			conn->LogStream() << os.str() << endl;

		mS->ConnCloseMsg(conn, os.str(), 1000, eCR_LOGIN_ERR);
		return -1;
	}

	if (conn->GetLSFlag(eLS_MYHUBURL)) { // already sent
		os << _("Invalid login sequence, your client already sent hub URL.");

		if (conn->Log(1))
			conn->LogStream() << os.str() << endl;

		mS->ConnCloseMsg(conn, os.str(), 1000, eCR_LOGIN_ERR);
		return -1;
	}

	if (CheckProtoLen(conn, msg))
		return -1;

	if (CheckProtoSyntax(conn, msg))
		return -1;

	const string &url = msg->ChunkString(eCH_1_PARAM);
	ParseReferer(url, conn->mHubURL, false);

	/*
		todo
			perform planned work with hub url
			if enabled, check hub url, iterate list by space
			if enabled, send $SetHubURL with correct url including port
	*/

	#ifndef WITHOUT_PLUGINS
		if (!mS->mCallBacks.mOnParsedMsgMyHubURL.CallAll(conn, msg)) {
			conn->CloseNow(eCR_PLUGIN);
			return -1;
		}
	#endif

	conn->SetLSFlag(eLS_MYHUBURL);
	return 0;
}

int cDCProto::DC_Version(cMessageDC *msg, cConnDC *conn)
{
	if (CheckUserLogin(conn, msg, false))
		return -1;

	ostringstream os;

	if (conn->GetLSFlag(eLS_VERSION)) { // already sent
		os << _("Invalid login sequence, you client already sent version.");

		if (conn->Log(1))
			conn->LogStream() << os.str() << endl;

		mS->ConnCloseMsg(conn, os.str(), 1000, eCR_LOGIN_ERR);
		return -1;
	}

	if (CheckProtoLen(conn, msg))
		return -1;

	if (CheckProtoSyntax(conn, msg))
		return -1;

	#ifndef WITHOUT_PLUGINS
		if (!mS->mCallBacks.mOnParsedMsgVersion.CallAll(conn, msg)) {
			conn->CloseNow(eCR_PLUGIN);
			return -1;
		}
	#endif

	const string &version = msg->ChunkString(eCH_1_PARAM);

	if (conn->Log(5))
		conn->LogStream() << "NMDC version:" << version << endl;

	conn->mVersion = version;
	conn->SetLSFlag(eLS_VERSION);
	return 1;
}

int cDCProto::DC_GetNickList(cMessageDC *msg, cConnDC *conn)
{
	if (CheckUserLogin(conn, msg, false))
		return -1;

	if (conn->CheckProtoFlood(msg->mStr, ePF_NICKLIST)) // protocol flood
		return -1;

	if (!conn->GetLSFlag(eLS_MYINFO)) {
		conn->mSendNickList = true;
		return 1;
	}

	if ((conn->mpUser->mClass < eUC_OPERATOR) && !mS->MinDelay(conn->mpUser->mT.nicklist, mS->mC.int_nicklist))
		return -1;

	return NickList(conn);
}

int cDCProto::DC_MyINFO(cMessageDC *msg, cConnDC *conn)
{
	if (CheckUserLogin(conn, msg, false))
		return -1;

	if (CheckProtoLen(conn, msg))
		return -1;

	if (CheckProtoSyntax(conn, msg))
		return -1;

	const string &nick = msg->ChunkString(eCH_MI_NICK);

	if (CheckUserNick(conn, nick))
		return -1;

	if (conn->CheckProtoFlood(msg->mStr, ePF_MYINFO)) // protocol flood
		return -1;

	string myinfo_desc = msg->ChunkString(eCH_MI_DESC);
	cDCTag *dc_tag = mS->mCo->mDCClients->ParseTag(myinfo_desc);
	ostringstream os;

	if (!mS->mC.tag_allow_none && (mS->mCo->mDCClients->mPositionInDesc < 0) && (conn->mpUser->mClass < mS->mC.tag_min_class_ignore) && (conn->mpUser->mClass != eUC_PINGER)) { // check for empty tag if required
		os << _("Your client didn't specify a tag.");

		if (conn->Log(2))
			conn->LogStream() << os.str() << endl;

		mS->ConnCloseMsg(conn, os.str(), 1000, eCR_TAG_NONE);
		delete dc_tag;
		dc_tag = NULL;
		return -1;
	}

	string myinfo_speed = msg->ChunkString(eCH_MI_SPEED);

	if (myinfo_speed.empty())
		myinfo_speed = "\x1";

	if (!conn->mConnType) // parse connection, it will be used below
		conn->mConnType = ParseSpeed(myinfo_speed);

	bool check = true;
	int result = 0;

	if ((!mS->mC.tag_allow_none || (mS->mCo->mDCClients->mPositionInDesc >= 0)) && (conn->mpUser->mClass < mS->mC.tag_min_class_ignore) && (conn->mpUser->mClass != eUC_PINGER)) // validate tag
		check = dc_tag->ValidateTag(os, conn, result);

	#ifndef WITHOUT_PLUGINS
		check = (check && mS->mCallBacks.mOnValidateTag.CallAll(conn, dc_tag));
	#endif

	if (!check) {
		if (conn->Log(2))
			conn->LogStream() << "Invalid tag with result " << result << ": " << dc_tag << endl;

		mS->ConnCloseMsg(conn, os.str(), 1000, eCR_TAG_INVALID);
		delete dc_tag;
		dc_tag = NULL;
		return -1;
	}

	check = conn->mpUser->mPassive; // user might have changed his mode

	switch (dc_tag->mClientMode) {
		case eCM_NOTAG:
		case eCM_PASSIVE:
		case eCM_SOCK5:
		case eCM_OTHER:
			conn->mpUser->mPassive = true;
			break;
		case eCM_ACTIVE:
			conn->mpUser->mPassive = false;
			break;
	}

	string myinfo_share = msg->ChunkString(eCH_MI_SIZE); // check share conditions

	if ((myinfo_share.size() > 18) || !IsNumber(myinfo_share.c_str()))
		myinfo_share = "0";

	__int64 share_check = 0, share_byte = 0;
	share_byte = StringAsLL(myinfo_share);
	share_check = share_byte / (1024 * 1024);

	if (!conn->mpUser->mHideShare) // minus old share, only if not hidden
		mS->mTotalShare -= conn->mpUser->mShare;

	conn->mpUser->mShare = share_byte; // note: from here on share limited redirects are available

	if (conn->mpUser->mHideShare) // update total share
		myinfo_share = "0";
	else
		mS->mTotalShare += share_byte;

	if (conn->mpUser->mClass <= eUC_OPERATOR) { // calculate minimum and maximum
		__int64 min_share = mS->mC.min_share;
		__int64 max_share = mS->mC.max_share;
		__int64 min_share_p, min_share_a;

		if (conn->mpUser->mClass == eUC_PINGER) {
			min_share = 0;

		} else {
			if (conn->mpUser->mClass >= eUC_REGUSER) {
				min_share = mS->mC.min_share_reg;
				max_share = mS->mC.max_share_reg;
			}

			if (conn->mpUser->mClass >= eUC_VIPUSER) {
				min_share = mS->mC.min_share_vip;
				max_share = mS->mC.max_share_vip;
			}

			if (conn->mpUser->mClass >= eUC_OPERATOR) {
				min_share = mS->mC.min_share_ops;
				max_share = mS->mC.max_share_ops;
			}
		}

		min_share_a = min_share;
		min_share_p = (__int64)(min_share * mS->mC.min_share_factor_passive);

		if (conn->mpUser->mPassive)
			min_share = min_share_p;

		//if (conn->mpUser->Can(eUR_NOSHARE, mS->mTime.Sec()))
			//min_share = 0;

		if ((share_check < min_share) || (max_share && (share_check > max_share))) {
			if (share_check < min_share)
				os << autosprintf(_("You share %s but minimum allowed is %s (%s for active users, %s for passive users)."), convertByte(share_byte, false).c_str(), convertByte(min_share * 1024 * 1024, false).c_str(), convertByte(min_share_a * 1024 * 1024, false).c_str(), convertByte(min_share_p * 1024 * 1024, false).c_str());
			else
				os << autosprintf(_("You share %s but maximum allowed is %s."), convertByte(share_byte, false).c_str(), convertByte(max_share * 1024 * 1024, false).c_str());

			if (conn->Log(2))
				conn->LogStream() << "Share limit" << endl;

			mS->ConnCloseMsg(conn, os.str(), 4000, eCR_SHARE_LIMIT);
			delete dc_tag;
			dc_tag = NULL;
			return -1;
		}

		if (conn->mpUser->mClass <= eUC_VIPUSER) { // second share limit that disables search and download
			__int64 temp_min_share = 0; // todo: rename to min_share_use_hub_guest

			switch (conn->mpUser->mClass) {
				case eUC_REGUSER:
					temp_min_share = mS->mC.min_share_use_hub_reg;
					break;
				case eUC_VIPUSER:
					temp_min_share = mS->mC.min_share_use_hub_vip;
					break;
				default: // eUC_NORMUSER
					temp_min_share = mS->mC.min_share_use_hub;
					break;
			}

			if (temp_min_share) {
				if (conn->mpUser->mPassive)
					temp_min_share = (__int64)(temp_min_share * mS->mC.min_share_factor_passive);

				if (share_check < temp_min_share) { // todo: notify user
					conn->mpUser->SetRight(eUR_SEARCH, 0);
					conn->mpUser->SetRight(eUR_CTM, 0);
				}
			}
		}

		result = ((conn->mpUser->mPassive) ? mS->mC.min_class_use_hub_passive : mS->mC.min_class_use_hub);

		if (conn->mpUser->mClass < result) { // todo: notify user
			conn->mpUser->SetRight(eUR_SEARCH, 0);
			conn->mpUser->SetRight(eUR_CTM, 0);
		}
	}

	if (conn->mpUser->mInList && (check != conn->mpUser->mPassive)) { // change user mode if differs and not first time
		if (check) {
			if (mS->mPassiveUsers.ContainsHash(conn->mpUser->mNickHash))
				mS->mPassiveUsers.RemoveByHash(conn->mpUser->mNickHash);

			if (!mS->mActiveUsers.ContainsHash(conn->mpUser->mNickHash))
				mS->mActiveUsers.AddWithHash(conn->mpUser, conn->mpUser->mNickHash);
		} else {
			if (mS->mActiveUsers.ContainsHash(conn->mpUser->mNickHash))
				mS->mActiveUsers.RemoveByHash(conn->mpUser->mNickHash);

			if (!mS->mPassiveUsers.ContainsHash(conn->mpUser->mNickHash))
				mS->mPassiveUsers.AddWithHash(conn->mpUser, conn->mpUser->mNickHash);
		}
	}

	string temp;

	if (conn->mpUser->mPassive && (conn->mpUser->mClass != eUC_PINGER) && (conn->mpUser->mClass < eUC_OPERATOR) && (mS->mC.max_users_passive > -1) && (mS->mPassiveUsers.Size() > (unsigned int)mS->mC.max_users_passive)) { // passive user limit
		os << autosprintf(_("Passive user limit exceeded at %d users. Try again later or set up an active connection."), mS->mPassiveUsers.Size());

		if (conn->Log(2))
			conn->LogStream() << "Passive user limit exceeded: " << mS->mPassiveUsers.Size() << endl;

		mS->ConnCloseMsg(conn, os.str(), 1000, eCR_USERLIMIT);
		Create_HubIsFull(temp); // must be sent after chat message
		conn->Send(temp, true);
		delete dc_tag;
		dc_tag = NULL;
		return -1;
	}

	conn->mpUser->mMyFlag = myinfo_speed[myinfo_speed.size() - 1]; // set status flags, note: we set it before executing callbacks, so plugins have option to change it before showing user to all

	if (conn->GetLSFlag(eLS_LOGIN_DONE) != eLS_LOGIN_DONE) { // user sent myinfo for the first time
		if ((conn->mpUser->mShare > 0) && (conn->mpUser->mClass <= eUC_REGUSER)) { // check share ban if non zero
			cBan Ban(mS);
			result = mS->mBanList->TestBan(Ban, conn, conn->mpUser->mNick, eBF_SHARE);

			if (result) {
				os << ((result == 1) ? _("You are prohibited from entering this hub") : _("You are banned from this hub")) << ":\r\n";
				Ban.DisplayUser(os);
				mS->DCPublicHS(os.str(), conn);

				if (conn->Log(1))
					conn->LogStream() << "Kicked user due to ban detection" << endl;

				conn->CloseNice(1000, eCR_KICKED);
				delete dc_tag;
				dc_tag = NULL;
				return -1;
			}
		}

		#ifndef WITHOUT_PLUGINS
			if (!mS->mCallBacks.mOnFirstMyINFO.CallAll(conn, msg)) {
				conn->CloseNice(1000, eCR_KICKED);
				delete dc_tag;
				dc_tag = NULL;
				return -2;
			}
		#endif
	}

	#ifndef WITHOUT_PLUGINS
		if (!mS->mCallBacks.mOnParsedMsgMyINFO.CallAll(conn, msg)) {
			delete dc_tag;
			dc_tag = NULL;
			return -2;
		}
	#endif

	if (mS->mTotalShare > mS->mTotalSharePeak) // peak total share
		mS->mTotalSharePeak = mS->mTotalShare;

	if (msg->mModified && msg->SplitChunks() && conn->Log(2)) // plugin has modified message, return here?
		conn->LogStream() << "MyINFO syntax error after plugin modification" << endl;

	string myinfo, myinfo_tag, myinfo_email; // $MyINFO $ALL <nick> <description><tag>$ $<speed><status>$<email>$<share>$

	if (myinfo_desc.size()) {
		temp = myinfo_desc;
		mS->mCo->mDCClients->ParsePos(temp); // get new tag position in case its changed

		if (mS->mC.show_desc_len >= 0) // hide description
			myinfo_desc.assign(temp, 0, mS->mC.show_desc_len);
		else
			myinfo_desc.assign(temp, 0, mS->mCo->mDCClients->mPositionInDesc);

		if (mS->mC.show_tags && mS->mCo->mDCClients->mPositionInDesc >= 0)
			myinfo_tag.assign(temp, mS->mCo->mDCClients->mPositionInDesc, -1);
	}

	if (mS->mC.desc_insert_mode) { // description insert mode
		if (mS->mC.desc_insert_vars.empty()) { // insert mode only
			switch (dc_tag->mClientMode) {
				case eCM_ACTIVE:
					myinfo_desc = "[A]" + myinfo_desc;
					break;
				case eCM_PASSIVE:
					myinfo_desc = "[P]" + myinfo_desc;
					break;
				case eCM_SOCK5:
					myinfo_desc = "[5]" + myinfo_desc;
					break;
				default: // eCM_OTHER, eCM_NOTAG
					myinfo_desc = "[?]" + myinfo_desc;
					break;
			}

		} else { // insert custom variables
			temp = mS->mC.desc_insert_vars;
			ReplaceVarInString(temp, "CLASS", temp, conn->mpUser->mClass);
			ReplaceVarInString(temp, "CLASSNAME", temp, mS->UserClassName(conn->mpUser->mClass));
			size_t pos = temp.find("%[C");

			if (pos != temp.npos) { // only if found
				string geo;

				if (temp.find("%[CC]", pos) != temp.npos) {
					geo = conn->GetGeoCC(); // country code
					ReplaceVarInString(temp, "CC", temp, geo);
				}

				if (temp.find("%[CN]", pos) != temp.npos) {
					geo = conn->GetGeoCN(); // country name
					ReplaceVarInString(temp, "CN", temp, geo);
				}

				if (temp.find("%[CITY]", pos) != temp.npos) {
					geo = conn->GetGeoCI(); // city name
					ReplaceVarInString(temp, "CITY", temp, geo);
				}
			}

			switch (dc_tag->mClientMode) {
				case eCM_ACTIVE:
					ReplaceVarInString(temp, "MODE", temp, 'A');
					break;

				case eCM_PASSIVE:
					ReplaceVarInString(temp, "MODE", temp, 'P');
					break;

				case eCM_SOCK5:
					ReplaceVarInString(temp, "MODE", temp, '5');
					break;

				default: // eCM_OTHER, eCM_NOTAG
					ReplaceVarInString(temp, "MODE", temp, '?');
					break;
			}

			myinfo_desc = temp + myinfo_desc;
		}
	}

	delete dc_tag; // note: dc tag is no longer used
	dc_tag = NULL;

	if (!mS->mC.show_speed && (myinfo_speed.size() > 1)) // hide speed but keep status byte
		myinfo_speed.assign(myinfo_speed, myinfo_speed.size() - 1, -1);

	if (mS->mC.show_email) // hide email
		myinfo_email = msg->ChunkString(eCH_MI_MAIL);

	myinfo_speed[myinfo_speed.size() - 1] = conn->mpUser->mMyFlag; // get possibly modified status flags
	Create_MyINFO(myinfo, nick, myinfo_desc + myinfo_tag, myinfo_speed, myinfo_email, myinfo_share); // generate fake myinfo

	if (conn->mpUser->mInList) { // login or send to all
		/*
			send it to all only if
				its not too often
				it has changed since last time
				send only the version that has changed only to those who want it
		*/

		if (mS->MinDelay(conn->mpUser->mT.info, mS->mC.int_myinfo) && (StrCompare(conn->mpUser->mMyINFO, 0, conn->mpUser->mMyINFO.size(), msg->mStr) != 0)) { // compare real myinfo
			conn->mpUser->mMyINFO = msg->mStr; // set real myinfo
			conn->mpUser->mFakeMyINFO = myinfo; // set fake myinfo

			if (mS->mC.myinfo_tls_filter && conn->mpUser->GetMyFlag(eMF_TLS)) { // myinfo tls filter
				mS->mUserList.SendToAllWithMyFlag(myinfo, eMF_TLS, mS->mC.delayed_myinfo, true);
				mS->RemoveMyINFOFlag(temp, myinfo, eMF_TLS);
				mS->mUserList.SendToAllWithoutMyFlag(temp, eMF_TLS, mS->mC.delayed_myinfo, true);

			} else {
				mS->mUserList.SendToAll(myinfo, mS->mC.delayed_myinfo, true); // send fake myinfo
			}
		}

	} else { // user sends myinfo for the first time
		if ((mS->mC.clone_detect_count) && (conn->mpUser->mClass <= int(mS->mC.max_class_check_clone))) { // detect clone using all myinfo and ip parameters
			temp.assign(msg->mStr, 14 + nick.size(), -1); // "$MyINFO $ALL <nick> ", note: same in cserverdc.cpp
			size_t posh = temp.find(",M:"), poss = temp.find(",H:"); // workaround for flylinkdc that sets passive mode for its second clone

			if ((posh != temp.npos) && (poss != temp.npos) && (poss > posh))
				temp.erase(posh + 3, poss - posh - 3);

			posh = temp.find(",H:"); // workaround for clients that cant predict hub count before sending myinfo
			poss = temp.find(",S:");

			if ((posh != temp.npos) && (poss != temp.npos) && (poss > posh))
				temp.erase(posh + 3, poss - posh - 3);

			if (mS->CheckUserClone(conn, temp, temp)) {
				os << autosprintf(_("You are already in the hub using another nick: %s"), temp.c_str());

				if (conn->Log(2))
					conn->LogStream() << os.str() << endl;

				mS->ConnCloseMsg(conn, os.str(), 1000, eCR_CLONE);
				return -1;
			}
		}

		conn->mpUser->mMyINFO = msg->mStr; // set real myinfo
		conn->mpUser->mFakeMyINFO = myinfo; // set fake myinfo
		conn->SetLSFlag(eLS_MYINFO);

		if (conn->mFeatures & eSF_SEARRULE) { // send search rule command
			string pars;
			pars.append("Min ");
			pars.append(StringFrom(mS->mC.min_search_chars));
			pars.append("$$Max ");
			pars.append(StringFrom(mS->mC.max_len_search));
			pars.append("$$Num ");
			pars.append(StringFrom(mS->mC.search_number));
			pars.append("$$Int ");

			if ((conn->mpUser->mClass >= int(eUC_NORMUSER)) && (conn->mpUser->mClass >= mS->mC.min_class_use_hub)) {
				switch (conn->mpUser->mClass) {
					case eUC_REGUSER:
						pars.append(StringFrom(mS->mC.int_search_reg));
						break;

					case eUC_VIPUSER:
						pars.append(StringFrom(mS->mC.int_search_vip));
						break;

					case eUC_OPERATOR:
					case eUC_CHEEF:
					case eUC_ADMIN:
						pars.append(StringFrom(mS->mC.int_search_op));
						break;

					case eUC_MASTER:
						pars.append(1, '0');
						break;

					default:
						pars.append(StringFrom(mS->mC.int_search));
						break;
				}
			} else {
				pars.append("-1");
			}

			pars.append("$$IntPas ");

			if ((conn->mpUser->mClass >= int(eUC_NORMUSER)) && (conn->mpUser->mClass >= mS->mC.min_class_use_hub_passive)) {
				switch (conn->mpUser->mClass) {
					case eUC_REGUSER:
						pars.append(StringFrom(mS->mC.int_search_reg_pas));
						break;

					case eUC_VIPUSER:
						pars.append(StringFrom(mS->mC.int_search_vip));
						break;

					case eUC_OPERATOR:
					case eUC_CHEEF:
					case eUC_ADMIN:
						pars.append(StringFrom(mS->mC.int_search_op));
						break;

					case eUC_MASTER:
						pars.append(1, '0');
						break;

					default:
						pars.append(StringFrom(mS->mC.int_search_pas));
						break;
				}
			} else {
				pars.append("-1");
			}

			share_check = 0;

			switch (conn->mpUser->mClass) {
				case eUC_NORMUSER:
					share_check = mS->mC.min_share_use_hub;
					break;

				case eUC_REGUSER:
					share_check = mS->mC.min_share_use_hub_reg;
					break;

				case eUC_VIPUSER:
					share_check = mS->mC.min_share_use_hub_vip;
					break;

				default:
					break;
			}

			if (share_check > 0) { // todo: mS->mC.min_share_factor_passive
				pars.append("$$Share ");
				pars.append(StringFrom(share_check * 1024 * 1024));
			}

			Create_SearchRule(temp, pars);
			conn->Send(temp, true);
		}

		if (!mS->BeginUserLogin(conn)) // if all right, add user to userlist, if not yet there
			return -1;
	}

	conn->ClearTimeOut(eTO_MYINFO);
	return 0;
}

int cDCProto::DC_IN(cMessageDC *msg, cConnDC *conn)
{
	if (CheckUserLogin(conn, msg))
		return -1;

	ostringstream os;

	if (!(conn->mFeatures & eSF_IN)) { // check support
		os << _("Invalid login sequence, you didn't specify support for IN command.");

		if (conn->Log(1))
			conn->LogStream() << os.str() << endl;

		mS->ConnCloseMsg(conn, os.str(), 1000, eCR_LOGIN_ERR);
		return -1;
	}

	if (CheckProtoLen(conn, msg))
		return -1;

	if (CheckProtoSyntax(conn, msg))
		return -1;

	const string &nick = msg->ChunkString(eCH_IN_NICK);

	if (CheckUserNick(conn, nick))
		return -1;

	if (conn->CheckProtoFlood(msg->mStr, ePF_IN)) // protocol flood
		return -1;

	string &data = msg->ChunkString(eCH_IN_DATA);

	if (data.empty())
		return 0;

	string back_in, /* todo: back_full, */part, body, sep("$$");
	bool empt;
	data.append(sep);
	size_t pos = data.find(sep);

	while (pos != string::npos) {
		part = data.substr(0, pos);
		data = data.substr(pos + 2); // size of sep
		pos = data.find(sep);

		if (part.size() < 2) // id + data
			continue;

		body = part.substr(1);
		empt = ((body[1] == '$') ? true : false);

		if (!empt && (body.find('$') != string::npos)) // make sure body is free from bad characters
			continue;

		switch (part[0]) {
			case 'D': // description
				// todo
				break;

			case 'T': // tag
				// todo
					// todo
				break;

			case 'C': // connection
				// todo
				break;

			case 'F': // status
				// todo
				break;

			case 'E': // email
				// todo
				break;

			case 'S': // share
				// todo
				break;

			default: // unknown
				// todo: add more
				break;
		}
	}

	if (back_in.empty()) // todo: Expression 'back_in.empty()' is always true - this needs to be implemented for it to work
		return 0;

	/*
		todo
			finish this command
			use iteration for broadcast like in search
			users who support it get shrot command
			users who dont get full command
			dont forget IN supports flag
	*/

	return 0;
}

int cDCProto::DC_ExtJSON(cMessageDC *msg, cConnDC *conn)
{
	if (CheckUserLogin(conn, msg))
		return -1;

	ostringstream os;

	if (!(conn->mFeatures & eSF_EXTJSON2)) { // check support
		os << _("Invalid login sequence, you didn't specify support for ExtJSON command.");

		if (conn->Log(1))
			conn->LogStream() << os.str() << endl;

		mS->ConnCloseMsg(conn, os.str(), 1000, eCR_LOGIN_ERR);
		return -1;
	}

	if (CheckProtoLen(conn, msg))
		return -1;

	if (CheckProtoSyntax(conn, msg))
		return -1;

	const string &nick = msg->ChunkString(eCH_EJ_NICK);

	if (CheckUserNick(conn, nick))
		return -1;

	if (conn->CheckProtoFlood(msg->mStr, ePF_EXTJSON)) // protocol flood
		return -1;

	if ((conn->mpUser->mClass < eUC_OPERATOR) && (mS->mSysLoad >= eSL_CAPACITY)) { // check hub load first of all
		if (mS->Log(3))
			mS->LogStream() << "Hub load is too high for ExtJSON: " << mS->mSysLoad << endl;

		/*
		os << _("Sorry, hub load is too high to process your ExtJSON. Please try again later."); // dont notify user, this feature is not important
		mS->DCPublicHS(os.str(), conn);
		*/
		return -2;
	}

	if (!mS->mC.disable_extjson) { // forward to users who support this, if enabled and if changed
		#ifndef WITHOUT_PLUGINS
		if (mS->mCallBacks.mOnParsedMsgExtJSON.CallAll(conn, msg))
		#endif
		{
			if (conn->mpUser->mExtJSON.empty() || (StrCompare(msg->mStr, 0, conn->mpUser->mExtJSON.size(), conn->mpUser->mExtJSON) != 0)) {
				string omsg(msg->mStr);
				mS->mUserList.SendToAllWithFeature(omsg, eSF_EXTJSON2, mS->mC.delayed_myinfo, true);
				conn->mpUser->mExtJSON = msg->mStr;
			}
		}
	}

	return 0;
}

int cDCProto::DC_GetINFO(cMessageDC *msg, cConnDC *conn)
{
	if (CheckUserLogin(conn, msg))
		return -1;

	if (CheckProtoSyntax(conn, msg))
		return -1;

	const string &nick = msg->ChunkString(eCH_GI_NICK);

	if (CheckUserNick(conn, nick))
		return -1;

	if (conn->CheckProtoFlood(msg->mStr, ePF_GETINFO)) // protocol flood
		return -1;

	string omsg;
	const string &other = msg->ChunkString(eCH_GI_OTHER); // find other user
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

	if (!(conn->mFeatures & eSF_NOGETINFO)) { // send it
		omsg = user->mFakeMyINFO;
		conn->Send(omsg, true, false); // must be flushed, else user can wait for data forever
	}

	return 0;
}

int cDCProto::DC_To(cMessageDC *msg, cConnDC *conn)
{
	if (CheckUserLogin(conn, msg))
		return -1;

	if (CheckProtoSyntax(conn, msg))
		return -1;

	const string &from = msg->ChunkString(eCH_PM_FROM);
	const string &nick = msg->ChunkString(eCH_PM_NICK);

	if (CheckUserNick(conn, from) || CheckUserNick(conn, nick))
		return -1;

	if (conn->CheckProtoFlood(msg->mStr, ePF_PRIV)) // protocol flood
		return -1;

	ostringstream os;
	string &to = msg->ChunkString(eCH_PM_TO);
	cUser *other = mS->mUserList.GetUserByNick(to); // find other user

	if (!other) {
		os << autosprintf(_("You're trying to send private message to an offline user: %s"), to.c_str());
		mS->DCPublicHS(os.str(), conn);
		return -2;
	}

	if (!other->mxConn) {
		if (mS->mRobotList.ContainsNick(to)) { // parse for commands to bot
			#ifndef WITHOUT_PLUGINS
				if (!mS->mCallBacks.mOnParsedMsgPM.CallAll(conn, msg))
					return 0;
			#endif

			cUserRobot *robot = (cUserRobot*)mS->mRobotList.GetUserBaseByNick(to);

			if (robot)
				robot->ReceiveMsg(conn, msg);

			return 0;
		} else {
			os << autosprintf(_("You're trying to send private message to a bot: %s"), to.c_str());
			mS->DCPublicHS(os.str(), conn);
			return -2;
		}
	}

	string &text = msg->ChunkString(eCH_PM_MSG);

	if (!conn->mpUser->Can(eUR_PM, mS->mTime.Sec(), 0)) {
		mS->DCPrivateHS(_("You're not allowed to use private chat right now."), conn, &to);

		if (mS->mC.notify_gag_chats)
			mS->ReportUserToOpchat(conn, autosprintf(_("User tries to speak with gag in PM to %s: %s"), to.c_str(), text.c_str()));

		return -4;
	}

	if (conn->mpUser->mClass < mS->mC.private_class) {
		os << _("Private chat is currently disabled for users with your class, please consider registering on the hub or contact an operator.");
		mS->DCPrivateHS(os.str(), conn);
		mS->DCPublicHS(os.str(), conn);
		return 0;
	}

	if ((conn->mpUser->mClass + (int)mS->mC.classdif_pm) < other->mClass) {
		os << _("You can't talk to this user.");
		mS->DCPrivateHS(os.str(), conn);
		mS->DCPublicHS(os.str(), conn);
		return -4;
	}

	cUser::tFloodHashType Hash = 0;
	Hash = tHashArray<void*>::HashString(text);

	if (Hash && (conn->mpUser->mClass < eUC_OPERATOR)) { // check for same message flood
		if (Hash == conn->mpUser->mFloodHashes[eFH_PM]) {
			if (conn->mpUser->mFloodCounters[eFC_PM]++ > mS->mC.max_flood_counter_pm) {
				os << autosprintf(_("Same message flood in PM: %s"), text.c_str());
				mS->ReportUserToOpchat(conn, os.str());
				mS->DCPrivateHS(os.str(), conn);

				if (mS->mC.same_flood_ban_time) // add temporary ban
					mS->mBanList->AddIPTempBan(conn->AddrToNumber(), mS->mTime.Sec() + mS->mC.same_flood_ban_time, os.str(), eBT_FLOOD);

				conn->CloseNice(1000, eCR_KICKED);
				return -5;
			}
		} else {
			conn->mpUser->mFloodCounters[eFC_PM] = 0;
		}
	}

	conn->mpUser->mFloodHashes[eFH_PM] = Hash;
	unsigned int count = text.size(); // check length

	if ((conn->mpUser->mClass < eUC_VIPUSER) && mS->mC.max_pm_msg && (count > mS->mC.max_pm_msg)) {
		os << autosprintf(_("Your PM contains %d characters but maximum allowed is %d characters."), count, mS->mC.max_pm_msg);
		mS->DCPrivateHS(os.str(), conn);
		return -1;
	}

	if (mS->CheckProtoFloodAll(conn, ePFA_PRIV, other)) // protocol flood from all
		return -1;

	#ifndef WITHOUT_PLUGINS
		if (!mS->mCallBacks.mOnParsedMsgPM.CallAll(conn, msg))
			return 0;
	#endif

	string _str(msg->mStr);
	other->mxConn->Send(_str, true); // send it
	return 0;
}

int cDCProto::DC_MCTo(cMessageDC *msg, cConnDC *conn)
{
	if (CheckUserLogin(conn, msg))
		return -1;

	if (CheckProtoSyntax(conn, msg))
		return -1;

	const string &nick = msg->ChunkString(eCH_MCTO_NICK);

	if (CheckUserNick(conn, nick))
		return -1;

	if (conn->CheckProtoFlood(msg->mStr, ePF_MCTO)) // protocol flood
		return -1;

	ostringstream os;
	string &to = msg->ChunkString(eCH_MCTO_TO);
	cUser *other = mS->mUserList.GetUserByNick(to); // find other user

	if (!other) {
		os << autosprintf(_("You're trying to send private message to an offline user: %s"), to.c_str());
		mS->DCPublicHS(os.str(), conn);
		return -2;
	}

	if (!other->mxConn) {
		os << autosprintf(_("You're trying to send private message to a bot: %s"), to.c_str());
		mS->DCPublicHS(os.str(), conn);
		return -2;
	}

	const string &text = msg->ChunkString(eCH_MCTO_MSG);

	if (!conn->mpUser->Can(eUR_PM, mS->mTime.Sec(), 0)) {
		mS->DCPrivateHS(_("You're not allowed to use private chat right now."), conn, &to);

		if (mS->mC.notify_gag_chats)
			mS->ReportUserToOpchat(conn, autosprintf(_("User tries to speak with gag in PM to %s: %s"), to.c_str(), text.c_str()));

		return -4;
	}

	if (conn->mpUser->mClass < mS->mC.private_class) { // pm rules apply to mcto
		os << _("Private chat is currently disabled for users with your class, please consider registering on the hub or contact an operator.");
		mS->DCPrivateHS(os.str(), conn);
		mS->DCPublicHS(os.str(), conn);
		return 0;
	}

	if ((conn->mpUser->mClass + (int)mS->mC.classdif_mcto) < other->mClass) {
		os << _("You can't talk to this user.");
		mS->DCPrivateHS(os.str(), conn);
		mS->DCPublicHS(os.str(), conn);
		return -4;
	}

	cUser::tFloodHashType Hash = 0;
	Hash = tHashArray<void*>::HashString(text);

	if (Hash && (conn->mpUser->mClass < eUC_OPERATOR)) { // check for same message flood
		if (Hash == conn->mpUser->mFloodHashes[eFH_MCTO]) {
			if (conn->mpUser->mFloodCounters[eFC_MCTO]++ > mS->mC.max_flood_counter_mcto) {
				os << autosprintf(_("Same message flood in MCTo: %s"), text.c_str());
				mS->ReportUserToOpchat(conn, os.str());
				mS->DCPrivateHS(os.str(), conn);

				if (mS->mC.same_flood_ban_time) // add temporary ban
					mS->mBanList->AddIPTempBan(conn->AddrToNumber(), mS->mTime.Sec() + mS->mC.same_flood_ban_time, os.str(), eBT_FLOOD);

				conn->CloseNice(1000, eCR_KICKED);
				return -5;
			}
		} else {
			conn->mpUser->mFloodCounters[eFC_MCTO] = 0;
		}
	}

	conn->mpUser->mFloodHashes[eFH_MCTO] = Hash;
	unsigned int count = text.size(); // check length

	if ((conn->mpUser->mClass < eUC_VIPUSER) && mS->mC.max_mcto_msg && (count > mS->mC.max_mcto_msg)) {
		os << autosprintf(_("Your chat message contains %d characters but maximum allowed is %d characters."), count, mS->mC.max_mcto_msg);
		mS->DCPublicHS(os.str(), conn);
		return -1;
	}

	if (mS->CheckProtoFloodAll(conn, ePFA_MCTO)) // protocol flood from all
		return -1;

	#ifndef WITHOUT_PLUGINS
		if (!mS->mCallBacks.mOnParsedMsgMCTo.CallAll(conn, msg))
			return 0;
	#endif

	string mcto;

	if (other->mxConn->mFeatures & eSF_MCTO) // send as is if supported by client
		mcto = msg->mStr;
	else // convert to private main chat message
		Create_Chat(mcto, conn->mpUser->mNick, text);

	other->mxConn->Send(mcto, true); // send it
	return 0;
}

int cDCProto::DC_Chat(cMessageDC *msg, cConnDC *conn)
{
	if (CheckUserLogin(conn, msg))
		return -1;

	if (CheckProtoSyntax(conn, msg))
		return -1;

	const string &nick = msg->ChunkString(eCH_CH_NICK);

	if (CheckUserNick(conn, nick))
		return -1;

	if (conn->CheckProtoFlood(msg->mStr, ePF_CHAT)) // protocol flood
		return -1;

	ostringstream os;

	if ((conn->mpUser->mClass < eUC_VIPUSER) && mS->mC.int_chat_ms && !mS->MinDelayMS(conn->mpUser->mT.chat, mS->mC.int_chat_ms, true)) { // check chat delay, keep resetting timer when user dont read what we say
		os << autosprintf(_("Your message wasn't sent because minimum chat delay is %lu ms but you waited only %s."), mS->mC.int_chat_ms, cTimePrint(mS->mTime - conn->mpUser->mT.chat).AsPeriod().AsString().c_str());
		mS->DCPublicHS(os.str(), conn);
		return 0;
	}

	string &text = msg->ChunkString(eCH_CH_MSG);

	if (ParseForCommands(text, conn, 0)) // check for commands
		return 0;

	if (!conn->mpUser->Can(eUR_CHAT, mS->mTime.Sec(), 0)) { // check if user is allowed to use main chat
		mS->DCPublicHS(_("You're not allowed to use main chat right now."), conn);

		if (mS->mC.notify_gag_chats)
			mS->ReportUserToOpchat(conn, autosprintf(_("User tries to speak with gag in MC: %s"), text.c_str()));

		return -4;
	}

	if (conn->mpUser->mClass < mS->mC.mainchat_class) {
		mS->DCPublicHS(_("Main chat is currently disabled for users with your class, please consider registering on the hub or contact an operator."), conn);
		return 0;
	}

	cUser::tFloodHashType Hash = 0; // check same message flood
	Hash = tHashArray<void*>::HashString(msg->mStr);

	if (Hash && (conn->mpUser->mClass < eUC_OPERATOR) && (Hash == conn->mpUser->mFloodHashes[eFH_CHAT])) {
		mS->DCPublicHS(_("Your message wasn't sent because it equals your previous message."), conn);
		return -5;
	}

	conn->mpUser->mFloodHashes[eFH_CHAT] = Hash;

	if ((conn->mpUser->mClass < eUC_VIPUSER) && !CheckChatMsg(text, conn)) // check message length and other conditions
		return 0;

	if (text.size() && (mKickChatPattern.Exec(text) >= 4) && (!mKickChatPattern.PartFound(1) || (mKickChatPattern.Compare(2, text, conn->mpUser->mNick) == 0))) { // if this is a kick message, process it separately
		if (conn->mpUser->mClass >= eUC_OPERATOR) {
			string other, why;
			mKickChatPattern.Extract(3, text, other);
			mKickChatPattern.Extract(4, text, why);
			mS->DCKickNick(NULL, conn->mpUser, other, why, (eKI_WHY | eKI_BAN));
		}

		return 0;
	}

	if (mS->CheckProtoFloodAll(conn, ePFA_CHAT)) // protocol flood from all
		return -1;

	#ifndef WITHOUT_PLUGINS
		if (!mS->mCallBacks.mOnParsedMsgChat.CallAll(conn, msg))
			return 0;
	#endif

	string omsg(msg->mStr);
	mS->mChatUsers.SendToAll(omsg, mS->mC.delayed_chat, true); // send it
	return 0;
}

int cDCProto::DC_ConnectToMe(cMessageDC *msg, cConnDC *conn)
{
	if (CheckUserLogin(conn, msg))
		return -1;

	if (CheckProtoSyntax(conn, msg))
		return -1;

	if (conn->CheckProtoFlood(msg->mStr, ePF_CTM)) // protocol flood
		return -1;

	ostringstream os;
	string &nick = msg->ChunkString(eCH_CM_NICK); // find other user
	const string &port = msg->ChunkString(eCH_CM_PORT);
	size_t pize = port.size();

	if (!pize)
		return -1;

	cUser *other = mS->mUserList.GetUserByNick(nick);
	string temp;

	if (!other) {
		/*
		if (!mS->mC.hide_msg_badctm && !conn->mpUser->mHideCtmMsg) {

			todo
				this ban check works only with protocol flood action 3 for rctm commands
				other actions and regular disconnects right after request are not detectable
				we need a proper way of detecting if user sent rctm request right before he left the hub
				can we take back last protocol messages from user who is receiving them?

			if (mS->mC.int_flood_rctm_period && mS->mC.int_flood_rctm_limit && (mS->mC.proto_flood_rctm_action >= 3) && mS->mC.proto_flood_tban_time) {
				cBan ban(mS);

				if (mS->mBanList->TestBan(ban, NULL, nick, eBF_NICK)) {
					os << _("Protocol flood detected") << ": " << "RevConnectToMe";

					if (StrCompare(os.str(), 0, os.str().size(), ban.mReason) != 0) {
						os.str("");
						*//*
						os << autosprintf(_("You're trying to connect to an offline user: %s"), nick.c_str());
						mS->DCPublicHS(os.str(), conn);
						*//*
					}
				}
			}
		}
		*/

		if (((pize > 2) && (port.substr(pize - 2) == "RS")) || ((pize > 1) && (port[pize - 1] == 'R'))) { // note: try to fix utf8 bug in nat reverse command
			if (!mS->mICUConvert->Convert(nick.c_str(), nick.size(), temp)) // failed to convert
				return -1;

			other = mS->mUserList.GetUserByNick(temp);

			if (!other || !other->mInList)
				return -1;

			nick.assign(temp, 0, temp.size()); // set converted nick for further use

		} else {
			return -1;
		}

	} else if (!other->mInList) {
		return -1;
	}

	if (!other->mxConn) {
		if (!mS->mC.hide_msg_badctm && !conn->mpUser->mHideCtmMsg) {
			os << autosprintf(_("You're trying to connect to a bot: %s"), other->mNick.c_str());
			mS->DCPublicHS(os.str(), conn);
		}

		return -1;
	}

	if (other->mNickHash == conn->mpUser->mNickHash) {
		if (!mS->mC.hide_msg_badctm && !conn->mpUser->mHideCtmMsg)
			mS->DCPublicHS(_("You're trying to connect to yourself."), conn);

		return -1;
	}

	if (conn->mpUser->mClass < mS->mC.min_class_use_hub) { // check use hub class
		if (!conn->mpUser->mHideCtmMsg && (!mS->mC.use_hub_msg_time || mS->MinDelay(conn->mpUser->mT.msg_ctm, mS->mC.use_hub_msg_time))) { // check message delay
			os << autosprintf(_("You can't download unless you are registered with class %d."), mS->mC.min_class_use_hub);
			mS->DCPublicHS(os.str(), conn);
		}

		return -4;
	}

	unsigned __int64 use_hub_share = 0; // check use hub share

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
		default:
			break;
	}

	use_hub_share = use_hub_share * 1024 * 1024;

	if (conn->mpUser->mShare < use_hub_share) {
		if (!conn->mpUser->mHideCtmMsg && (!mS->mC.use_hub_msg_time || mS->MinDelay(conn->mpUser->mT.msg_ctm, mS->mC.use_hub_msg_time))) { // check message delay
			os << autosprintf(_("You can't download unless you share %s."), convertByte(use_hub_share, false).c_str());
			mS->DCPublicHS(os.str(), conn);
		}

		return -4;
	}

	if (!conn->mpUser->Can(eUR_CTM, mS->mTime.Sec(), 0)) { // check temporary right
		if (!mS->mC.hide_msg_badctm && !conn->mpUser->mHideCtmMsg)
			mS->DCPublicHS(_("You're not allowed to download from anyone."), conn);

		return -4;
	}

	if (((conn->mpUser->mClass + (int)mS->mC.classdif_download) < other->mClass) || ((conn->mpUser->mClass < eUC_OPERATOR) && other->mHideShare)) { // check class difference and hidden share
		if (!mS->mC.hide_msg_badctm && !conn->mpUser->mHideCtmMsg) {
			os << autosprintf(_("You can't download from this user: %s"), nick.c_str());
			mS->DCPublicHS(os.str(), conn);
		}

		return -4;
	}

	if (mS->mC.filter_lan_requests && (conn->mpUser->mLan != other->mLan)) { // filter lan to wan and reverse
		if (!mS->mC.hide_msg_badctm && !conn->mpUser->mHideCtmMsg) {
			os << autosprintf(_("You can't download from this user because one of you is in a LAN: %s"), nick.c_str());
			mS->DCPublicHS(os.str(), conn);
		}

		return -1;
	}

	if ((other->mxConn->mFeatures & eSF_CHATONLY) && (conn->mpUser->mClass < mS->mC.chatonly_bypass_class)) { // user is here only to chat
		if (!mS->mC.hide_msg_badctm && !conn->mpUser->mHideCtmMsg) {
			os << autosprintf(_("You can't download from this user because he is in chat only mode: %s"), nick.c_str());
			mS->DCPublicHS(os.str(), conn);
		}

		return -1;
	}

	temp = port;
	string extra;
	size_t pos = temp.find_first_of(' ');

	if (pos != string::npos) { // nat
		const string natnick = temp.substr(pos + 1);

		if (natnick.size()) {
			if (CheckUserNick(conn, natnick))
				return -1;

			temp.assign(temp, 0, pos);
			pize = temp.size();

			if (!pize || (pize > 7))
				return -1;

			if ((pize > 2) && (temp.substr(pize - 2) == "NS")) { // + tls
				if (conn->mpUser->GetMyFlag(eMF_NAT) && other->GetMyFlag(eMF_NAT)) { // only if both support nat
					if (CheckCompatTLS(conn, other->mxConn)) // note: we dont strip secure flag because it wont work anyway, instead we notify user and ignore the request
						extra = "NS " + natnick;
					else
						return -1; //extra = "N " + natnick;
				}

				temp.assign(temp, 0, pize - 2);

			} else if (temp[pize - 1] == 'N') {
				if (conn->mpUser->GetMyFlag(eMF_NAT) && other->GetMyFlag(eMF_NAT)) // only if both support nat
					extra = "N " + natnick;

				temp.assign(temp, 0, pize - 1);
			}

		} else {
			temp.assign(temp, 0, pos);
		}

	} else if ((pize > 2) && (temp.substr(pize - 2) == "RS")) { // nat + tls
		if (conn->mpUser->GetMyFlag(eMF_NAT) && other->GetMyFlag(eMF_NAT)) { // only if both support nat
			if (CheckCompatTLS(conn, other->mxConn)) // note: we dont strip secure flag because it wont work anyway, instead we notify user and ignore the request
				extra = "RS";
			else
				return -1; //extra = 'R';
		}

		temp.assign(temp, 0, pize - 2);

	} else if (temp[pize - 1] == 'R') { // nat
		if (conn->mpUser->GetMyFlag(eMF_NAT) && other->GetMyFlag(eMF_NAT)) // only if both support nat
			extra = 'R';

		temp.assign(temp, 0, pize - 1);

	} else if (temp[pize - 1] == 'S') { // tls
		if (CheckCompatTLS(conn, other->mxConn)) // note: we dont strip secure flag because it wont work anyway, instead we notify user and ignore the request
			extra = 'S';
		else
			return -1;

		temp.assign(temp, 0, pize - 1);
	}

	if (temp.empty() || (temp.size() > 5))
		return -1;

	unsigned int iport = StringAsLL(temp);

	if (!mS->CheckPortNumber(iport))
		return -1;

	#ifndef WITHOUT_PLUGINS
		if (!mS->mCallBacks.mOnParsedMsgConnectToMe.CallAll(conn, msg))
			return -2;
	#endif

	const string &addr = msg->ChunkString(eCH_CM_IP);
	temp = addr;

	if (!CheckIP(conn, temp)) {
		if (conn->Log(3))
			conn->LogStream() << "Fixed wrong IP in $ConnectToMe: " << temp << endl;

		if (mS->mC.wrongip_message) {
			os << autosprintf(_("Replacing wrong IP address specified in your connection request with real one: %s -> %s"), temp.c_str(), conn->mAddrIP.c_str());
			mS->DCPublicHS(os.str(), conn);
		}

		temp = conn->mAddrIP;
	}

	string ctm;
	Create_ConnectToMe(ctm, nick, temp, StringFrom(iport), extra);
	other->mxConn->Send(ctm, true); // send it
	return 0;
}

int cDCProto::DC_RevConnectToMe(cMessageDC *msg, cConnDC *conn)
{
	if (CheckUserLogin(conn, msg))
		return -1;

	if (CheckProtoSyntax(conn, msg))
		return -1;

	const string &mynick = msg->ChunkString(eCH_RC_NICK);

	if (CheckUserNick(conn, mynick))
		return -1;

	if (conn->CheckProtoFlood(msg->mStr, ePF_RCTM)) // protocol flood
		return -1;

	ostringstream os;
	const string &nick = msg->ChunkString(eCH_RC_OTHER); // find other user
	cUser *other = mS->mUserList.GetUserByNick(nick);

	if (!other || !other->mInList) {
		if (!mS->mC.hide_msg_badctm && !conn->mpUser->mHideCtmMsg) {
			os << autosprintf(_("You're trying to connect to an offline user: %s"), nick.c_str());
			mS->DCPublicHS(os.str(), conn);
		}

		return -2;
	}

	if (!other->mxConn) {
		if (!mS->mC.hide_msg_badctm && !conn->mpUser->mHideCtmMsg) {
			os << autosprintf(_("You're trying to connect to a bot: %s"), nick.c_str());
			mS->DCPublicHS(os.str(), conn);
		}

		return -2;
	}

	if (other->mNickHash == conn->mpUser->mNickHash) {
		if (!mS->mC.hide_msg_badctm && !conn->mpUser->mHideCtmMsg)
			mS->DCPublicHS(_("You're trying to connect to yourself."), conn);

		return -2;
	}

	if (other->mPassive && (!conn->mpUser->GetMyFlag(eMF_NAT) || !other->GetMyFlag(eMF_NAT))) { // passive request to passive user, allow if both support nat traversal
		if (!mS->mC.hide_msg_badctm && !conn->mpUser->mHideCtmMsg) {
			os << autosprintf(_("You can't download from this user, because he is also in passive mode: %s"), nick.c_str());
			mS->DCPublicHS(os.str(), conn);
		}

		return -2;
	}

	if (conn->mpUser->mHideShare) { // when my share is hidden other users cant connect to me
		if (!mS->mC.hide_msg_badctm && !conn->mpUser->mHideCtmMsg) {
			os << autosprintf(_("You can't download from this user while your share is hidden, because you are in passive mode: %s"), nick.c_str());
			mS->DCPublicHS(os.str(), conn);
		}

		return -2;
	}

	if (conn->mpUser->mClass < mS->mC.min_class_use_hub_passive) { // check use hub class
		if (!conn->mpUser->mHideCtmMsg && (!mS->mC.use_hub_msg_time || mS->MinDelay(conn->mpUser->mT.msg_ctm, mS->mC.use_hub_msg_time))) { // check message delay
			os << autosprintf(_("You can't download unless you are registered with class %d."), mS->mC.min_class_use_hub_passive);
			mS->DCPublicHS(os.str(), conn);
		}

		return -4;
	}

	unsigned __int64 use_hub_share = 0; // check use hub share

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
		default:
			break;
	}

	use_hub_share = use_hub_share * 1024 * 1024;

	if (conn->mpUser->mShare < use_hub_share) {
		if (!conn->mpUser->mHideCtmMsg && (!mS->mC.use_hub_msg_time || mS->MinDelay(conn->mpUser->mT.msg_ctm, mS->mC.use_hub_msg_time))) { // check message delay
			os << autosprintf(_("You can't download unless you share %s."), convertByte(use_hub_share, false).c_str());
			mS->DCPublicHS(os.str(), conn);
		}

		return -4;
	}

	if (!conn->mpUser->Can(eUR_CTM, mS->mTime.Sec(), 0)) { // check temporary right
		if (!mS->mC.hide_msg_badctm && !conn->mpUser->mHideCtmMsg)
			mS->DCPublicHS(_("You're not allowed to download from anyone."), conn);

		return -4;
	}

	if (((conn->mpUser->mClass + (int)mS->mC.classdif_download) < other->mClass) || ((conn->mpUser->mClass < eUC_OPERATOR) && other->mHideShare)) { // check class difference and hidden share
		if (!mS->mC.hide_msg_badctm && !conn->mpUser->mHideCtmMsg) {
			os << autosprintf(_("You can't download from this user: %s"), nick.c_str());
			mS->DCPublicHS(os.str(), conn);
		}

		return -4;
	}

	if ((other->mxConn->mFeatures & eSF_CHATONLY) && (conn->mpUser->mClass < mS->mC.chatonly_bypass_class)) { // user is here only to chat
		if (!mS->mC.hide_msg_badctm && !conn->mpUser->mHideCtmMsg) {
			os << autosprintf(_("You can't download from this user because he is in chat only mode: %s"), nick.c_str());
			mS->DCPublicHS(os.str(), conn);
		}

		return -1;
	}

	if (mS->CheckProtoFloodAll(conn, ePFA_RCTM, other)) // protocol flood from all
		return -1;

	#ifndef WITHOUT_PLUGINS
		if (!mS->mCallBacks.mOnParsedMsgRevConnectToMe.CallAll(conn, msg))
			return -2;
	#endif

	string _str(msg->mStr);
	other->mxConn->Send(_str, true); // send it
	return 0;
}

int cDCProto::DC_MultiConnectToMe(cMessageDC *msg, cConnDC *conn)
{
	if (CheckUserLogin(conn, msg))
		return -1;

	// todo
	return 0;
}

int cDCProto::DC_Search(cMessageDC *msg, cConnDC *conn)
{
	if (CheckUserLogin(conn, msg))
		return -1;

	if (CheckProtoLen(conn, msg))
		return -1;

	if (CheckProtoSyntax(conn, msg))
		return -1;

	if (conn->CheckProtoFlood(msg->mStr, ePF_SEARCH)) // protocol flood
		return -1;

	ostringstream os;

	if ((conn->mpUser->mClass < eUC_OPERATOR) && (mS->mSysLoad >= eSL_CAPACITY)) { // check hub load first of all
		if (mS->Log(3))
			mS->LogStream() << "Hub load is too high for search request: " << mS->mSysLoad << endl;

		os << _("Sorry, hub load is too high to process your search request. Please try again later.");
		mS->DCPublicHS(os.str(), conn);
		return -2;
	}

	bool passive = true;

	switch (msg->mType) { // detect search mode once
		case eDC_SEARCH:
		case eDC_MSEARCH:
			passive = false;
			break;

		default:
			break;
	}

	string nick, saddr, addr;

	if (passive) { // verify sender
		nick = msg->ChunkString(eCH_PS_NICK);

		if (CheckUserNick(conn, nick))
			return -1;

		saddr.append("Hub:");
		saddr.append(nick);
	} else {
		const string &port = msg->ChunkString(eCH_AS_PORT);

		if (port.empty() || (port.size() > 5))
			return -1;

		unsigned int iport = StringAsLL(port);

		if (!mS->CheckPortNumber(iport))
			return -1;

		addr = msg->ChunkString(eCH_AS_IP);

		if (CheckIP(conn, addr))
			saddr.append(addr);
		else
			saddr.append(conn->mAddrIP);

		saddr.append(1, ':');
		saddr.append(StringFrom(iport));
	}

	unsigned int delay = 0;

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

	if (delay && !mS->MinDelay(conn->mpUser->mT.search, delay)) { // check delay
		if (mS->mC.search_number && (conn->mpUser->mSearchNumber >= mS->mC.search_number)) {
			os << autosprintf(_("Don't search too often.")) << ' ';
			os << autosprintf(ngettext("You can perform %d search in %s.", "You can perform %d searches in %s.", mS->mC.search_number), mS->mC.search_number, cTimePrint(delay, 0).AsPeriod().AsString().c_str());
			mS->DCPublicHS(os.str(), conn);
			return -1;
		}
	} else {
		conn->mpUser->mSearchNumber = 0;
	}

	if (conn->mpUser->mClass < eUC_OPERATOR) { // if not operator
		cUser::tFloodHashType Hash = 0; // check same message flood
		Hash = tHashArray<void*>::HashString(msg->mStr);

		if (Hash && (Hash == conn->mpUser->mFloodHashes[eFH_SEARCH]))
			return -4; // silent filter, user might be using automatic search

		conn->mpUser->mFloodHashes[eFH_SEARCH] = Hash;
	}

	int use_hub_class = ((passive) ? mS->mC.min_class_use_hub_passive : mS->mC.min_class_use_hub); // check use hub class

	if (conn->mpUser->mClass < use_hub_class) {
		if (!conn->mpUser->mHideCtmMsg && (!mS->mC.use_hub_msg_time || mS->MinDelay(conn->mpUser->mT.msg_search, mS->mC.use_hub_msg_time))) { // check message delay
			os << autosprintf(_("You can't search unless you are registered with class %d."), use_hub_class);
			mS->DCPublicHS(os.str(), conn);
		}

		return -4;
	}

	unsigned __int64 use_hub_share = 0; // check use hub share

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

		default:
			break;
	}

	use_hub_share = use_hub_share * 1024 * 1024;

	if (conn->mpUser->mShare < use_hub_share) {
		if (!conn->mpUser->mHideCtmMsg && (!mS->mC.use_hub_msg_time || mS->MinDelay(conn->mpUser->mT.msg_search, mS->mC.use_hub_msg_time))) { // check message delay
			os << autosprintf(_("You can't search unless you share %s."), convertByte(use_hub_share, false).c_str());
			mS->DCPublicHS(os.str(), conn);
		}

		return -4;
	}

	if (!conn->mpUser->Can(eUR_SEARCH, mS->mTime.Sec(), 0)) { // check temporary right
		if (!mS->mC.hide_msg_badctm && !conn->mpUser->mHideCtmMsg)
			mS->DCPublicHS(_("You're not allowed to search for anything."), conn);

		return -4;
	}

	string lims, spat;

	if (passive) {
		lims = msg->ChunkString(eCH_PS_SEARCHLIMITS);
		spat = msg->ChunkString(eCH_PS_SEARCHPATTERN);
	} else {
		lims = msg->ChunkString(eCH_AS_SEARCHLIMITS);
		spat = msg->ChunkString(eCH_AS_SEARCHPATTERN);
	}

	size_t limlen = lims.size();

	if (limlen < 8) // check limits length
		return -1;

	size_t patlen = spat.size();

	if (!patlen) // check base search length
		return -4;

	bool tth = (lims.substr(limlen - 3) == "?9?");

	if (tth) { // check tth searches
		bool tthpref = true;

		if ((patlen < 4) || (StrCompare(spat, 0, 4, "TTH:") != 0))
			tthpref = false;

		if ((patlen != 43) || !tthpref) { // change search type
			lims[limlen - 2] = '1';
			tth = false;

			if (tthpref)
				spat = spat.substr(4);
		}
	}

	if (!tth && (conn->mpUser->mClass < eUC_OPERATOR) && (spat.size() < mS->mC.min_search_chars)) { // check search length if not operator, only if not tth
		os << autosprintf(ngettext("Minimum search length is %d character.", "Minimum search length is %d characters.", mS->mC.min_search_chars), mS->mC.min_search_chars);
		mS->DCPublicHS(os.str(), conn);
		return -1;
	}

	if (passive && mS->CheckProtoFloodAll(conn, ePFA_SEAR)) // protocol flood from all
		return -1;

 	#ifndef WITHOUT_PLUGINS
		if (!mS->mCallBacks.mOnParsedMsgSearch.CallAll(conn, msg))
			return -2;
	#endif

	conn->mpUser->mSearchNumber++; // set counter last of all

	if (passive) {
		conn->mSRCounter = 0;
	} else if (StrCompare(addr, 0, conn->mAddrIP.size(), conn->mAddrIP) != 0) {
		if (conn->Log(3))
			conn->LogStream() << "Fixed wrong IP in $Search: " << addr << endl;

		if (mS->mC.wrongip_message) {
			os << autosprintf(_("Replacing wrong IP address specified in your search request with real one: %s -> %s"), addr.c_str(), conn->mAddrIP.c_str());
			mS->DCPublicHS(os.str(), conn);
		}
	}

	string search, tths;
	Create_Search(search, saddr, lims, spat);

	if (mS->mC.use_search_filter && tth && (StrCompare(lims, 3, 8, "?0?9?") == 0)) { // also create short version to send to modern clients, note: limits length is checked above
		spat = spat.substr(4);

		if (passive)
			Create_SP(tths, spat, nick);
		else
			Create_SA(tths, spat, saddr);
	}

	if (mS->mC.use_search_filter) { // send it using filter
		/*
			conditional and filtered search request
			some users hide their share
			some users dont share anything
			some users are in chat only mode
			pingers dont need to get any searches
			users dont need to get their own searches
			some users are in lan and others are in wan
			some users dont support tth searches
		*/

		mS->SearchToAll(conn, search, tths, passive, tth);
	} else { // send it without filter, old search engine, note: short tth search command can not be used here
		if (passive)
			mS->mActiveUsers.SendToAll(search, mS->mC.delayed_search, true);
		else
			mS->mUserList.SendToAll(search, mS->mC.delayed_search, true);
	}

	return 0;
}

int cDCProto::DC_SA(cMessageDC *msg, cConnDC *conn)
{
	if (CheckUserLogin(conn, msg))
		return -1;

	ostringstream os;

	if (!(conn->mFeatures & eSF_TTHS)) { // check support
		os << _("Invalid login sequence, you didn't specify support for short TTH search command.");

		if (conn->Log(1))
			conn->LogStream() << os.str() << endl;

		mS->ConnCloseMsg(conn, os.str(), 1000, eCR_LOGIN_ERR);
		return -1;
	}

	if (CheckProtoLen(conn, msg))
		return -1;

	if (CheckProtoSyntax(conn, msg))
		return -1;

	if (conn->CheckProtoFlood(msg->mStr, ePF_SEARCH)) // protocol flood
		return -1;

	if ((conn->mpUser->mClass < eUC_OPERATOR) && (mS->mSysLoad >= eSL_CAPACITY)) { // check hub load first of all
		if (mS->Log(3))
			mS->LogStream() << "Hub load is too high for search request: " << mS->mSysLoad << endl;

		os << _("Sorry, hub load is too high to process your search request. Please try again later.");
		mS->DCPublicHS(os.str(), conn);
		return -2;
	}

	const string &tth = msg->ChunkString(eCH_SA_TTH);

	if (tth.size() != 39) // check tth size, todo: be more strict and disconnect user with message
		return -1;

	const string &port = msg->ChunkString(eCH_SA_PORT);

	if (port.empty() || (port.size() > 5))
		return -1;

	unsigned int iport = StringAsLL(port);

	if (!mS->CheckPortNumber(iport))
		return -1;

	string saddr;
	const string &addr = msg->ChunkString(eCH_SA_IP);

	if (CheckIP(conn, addr))
		saddr.append(addr);
	else
		saddr.append(conn->mAddrIP);

	saddr.append(1, ':');
	saddr.append(StringFrom(iport));
	unsigned int delay = 0;

	switch (conn->mpUser->mClass) { // prepare delay
		case eUC_REGUSER:
			delay = mS->mC.int_search_reg;
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
			delay = mS->mC.int_search;
			break;
	}

	if (delay && !mS->MinDelay(conn->mpUser->mT.search, delay)) { // check delay
		if (mS->mC.search_number && (conn->mpUser->mSearchNumber >= mS->mC.search_number)) {
			os << autosprintf(_("Don't search too often.")) << ' ';
			os << autosprintf(ngettext("You can perform %d search in %s.", "You can perform %d searches in %s.", mS->mC.search_number), mS->mC.search_number, cTimePrint(delay, 0).AsPeriod().AsString().c_str());
			mS->DCPublicHS(os.str(), conn);
			return -1;
		}
	} else {
		conn->mpUser->mSearchNumber = 0;
	}

	string search;
	Create_Search(search, saddr, tth, false);

	if (conn->mpUser->mClass < eUC_OPERATOR) { // if not operator
		cUser::tFloodHashType Hash = 0; // check same message flood
		Hash = tHashArray<void*>::HashString(search);

		if (Hash && (Hash == conn->mpUser->mFloodHashes[eFH_SEARCH])) // silent filter, user might be using automatic search
			return -4;

		conn->mpUser->mFloodHashes[eFH_SEARCH] = Hash;
	}

	if (conn->mpUser->mClass < mS->mC.min_class_use_hub) { // check use hub class
		if (!conn->mpUser->mHideCtmMsg && (!mS->mC.use_hub_msg_time || mS->MinDelay(conn->mpUser->mT.msg_search, mS->mC.use_hub_msg_time))) { // check message delay
			os << autosprintf(_("You can't search unless you are registered with class %d."), mS->mC.min_class_use_hub);
			mS->DCPublicHS(os.str(), conn);
		}

		return -4;
	}

	unsigned __int64 use_hub_share = 0; // check use hub share

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

		default:
			break;
	}

	use_hub_share = use_hub_share * 1024 * 1024;

	if (conn->mpUser->mShare < use_hub_share) {
		if (!conn->mpUser->mHideCtmMsg && (!mS->mC.use_hub_msg_time || mS->MinDelay(conn->mpUser->mT.msg_search, mS->mC.use_hub_msg_time))) { // check message delay
			os << autosprintf(_("You can't search unless you share %s."), convertByte(use_hub_share).c_str());
			mS->DCPublicHS(os.str(), conn);
		}

		return -4;
	}

	if (!conn->mpUser->Can(eUR_SEARCH, mS->mTime.Sec(), 0)) { // check temporary right
		if (!mS->mC.hide_msg_badctm && !conn->mpUser->mHideCtmMsg)
			mS->DCPublicHS(_("You're not allowed to search for anything."), conn);

		return -4;
	}

 	#ifndef WITHOUT_PLUGINS
		if (!mS->mCallBacks.mOnParsedMsgSearch.CallAll(conn, msg))
			return -2;
	#endif

	conn->mpUser->mSearchNumber++; // set counter last of all

	if (StrCompare(addr, 0, conn->mAddrIP.size(), conn->mAddrIP) != 0) {
		if (conn->Log(3))
			conn->LogStream() << "Fixed wrong IP in $Search: " << addr << endl;

		if (mS->mC.wrongip_message) {
			os << autosprintf(_("Replacing wrong IP address specified in your search request with real one: %s -> %s"), addr.c_str(), conn->mAddrIP.c_str());
			mS->DCPublicHS(os.str(), conn);
		}
	}

	string tths;
	Create_SA(tths, tth, saddr);
	mS->SearchToAll(conn, search, tths, false); // can send it only using filter
	return 0;
}

int cDCProto::DC_SP(cMessageDC *msg, cConnDC *conn)
{
	if (CheckUserLogin(conn, msg))
		return -1;

	ostringstream os;

	if (!(conn->mFeatures & eSF_TTHS)) { // check support
		os << _("Invalid login sequence, you didn't specify support for short TTH search command.");

		if (conn->Log(1))
			conn->LogStream() << os.str() << endl;

		mS->ConnCloseMsg(conn, os.str(), 1000, eCR_LOGIN_ERR);
		return -1;
	}

	if (CheckProtoLen(conn, msg))
		return -1;

	if (CheckProtoSyntax(conn, msg))
		return -1;

	if (conn->CheckProtoFlood(msg->mStr, ePF_SEARCH)) // protocol flood
		return -1;

	if ((conn->mpUser->mClass < eUC_OPERATOR) && (mS->mSysLoad >= eSL_CAPACITY)) { // check hub load first of all
		if (mS->Log(3))
			mS->LogStream() << "Hub load is too high for search request: " << mS->mSysLoad << endl;

		os << _("Sorry, hub load is too high to process your search request. Please try again later.");
		mS->DCPublicHS(os.str(), conn);
		return -2;
	}

	const string &tth = msg->ChunkString(eCH_SP_TTH);

	if (tth.size() != 39) // check tth size, todo: be more strict and disconnect user with message
		return -1;

	const string &nick = msg->ChunkString(eCH_SP_NICK);

	if (CheckUserNick(conn, nick)) // verify sender
		return -1;

	unsigned int delay = 0;

	switch (conn->mpUser->mClass) { // prepare delay
		case eUC_REGUSER:
			delay = mS->mC.int_search_reg_pas;
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
			delay = mS->mC.int_search_pas;
			break;
	}

	if (delay && !mS->MinDelay(conn->mpUser->mT.search, delay)) { // check delay
		if (mS->mC.search_number && (conn->mpUser->mSearchNumber >= mS->mC.search_number)) {
			os << autosprintf(_("Don't search too often.")) << ' ';
			os << autosprintf(ngettext("You can perform %d search in %s.", "You can perform %d searches in %s.", mS->mC.search_number), mS->mC.search_number, cTimePrint(delay, 0).AsPeriod().AsString().c_str());
			mS->DCPublicHS(os.str(), conn);
			return -1;
		}
	} else {
		conn->mpUser->mSearchNumber = 0;
	}

	string search;
	Create_Search(search, nick, tth, true);

	if (conn->mpUser->mClass < eUC_OPERATOR) { // if not operator
		cUser::tFloodHashType Hash = 0; // check same message flood
		Hash = tHashArray<void*>::HashString(search);

		if (Hash && (Hash == conn->mpUser->mFloodHashes[eFH_SEARCH])) // silent filter, user might be using automatic search
			return -4;

		conn->mpUser->mFloodHashes[eFH_SEARCH] = Hash;
	}

	if (conn->mpUser->mClass < mS->mC.min_class_use_hub_passive) { // check use hub class
		if (!conn->mpUser->mHideCtmMsg && (!mS->mC.use_hub_msg_time || mS->MinDelay(conn->mpUser->mT.msg_search, mS->mC.use_hub_msg_time))) { // check message delay
			os << autosprintf(_("You can't search unless you are registered with class %d."), mS->mC.min_class_use_hub_passive);
			mS->DCPublicHS(os.str(), conn);
		}

		return -4;
	}

	unsigned __int64 use_hub_share = 0; // check use hub share

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

		default:
			break;
	}

	use_hub_share = use_hub_share * 1024 * 1024;

	if (conn->mpUser->mShare < use_hub_share) {
		if (!conn->mpUser->mHideCtmMsg && (!mS->mC.use_hub_msg_time || mS->MinDelay(conn->mpUser->mT.msg_search, mS->mC.use_hub_msg_time))) { // check message delay
			os << autosprintf(_("You can't search unless you share %s."), convertByte(use_hub_share).c_str());
			mS->DCPublicHS(os.str(), conn);
		}

		return -4;
	}

	if (!conn->mpUser->Can(eUR_SEARCH, mS->mTime.Sec(), 0)) { // check temporary right
		if (!mS->mC.hide_msg_badctm && !conn->mpUser->mHideCtmMsg)
			mS->DCPublicHS(_("You're not allowed to search for anything."), conn);

		return -4;
	}

	if (mS->CheckProtoFloodAll(conn, ePFA_SEAR)) // protocol flood from all
		return -1;

 	#ifndef WITHOUT_PLUGINS
		if (!mS->mCallBacks.mOnParsedMsgSearch.CallAll(conn, msg))
			return -2;
	#endif

	conn->mpUser->mSearchNumber++; // set counter last of all
	conn->mSRCounter = 0;
	string tths;
	Create_SP(tths, tth, nick);
	mS->SearchToAll(conn, search, tths, true); // can send it only using filter
	return 0;
}

int cDCProto::DC_SR(cMessageDC *msg, cConnDC *conn)
{
	if (CheckUserLogin(conn, msg))
		return -1;

	if (CheckProtoSyntax(conn, msg))
		return -1;

	const string &from = msg->ChunkString(eCH_SR_FROM);

	if (CheckUserNick(conn, from))
		return -1;

	if (conn->CheckProtoFlood(msg->mStr, ePF_SR)) // protocol flood
		return -1;

	if ((conn->mpUser->mClass < eUC_OPERATOR) && (mS->mSysLoad >= eSL_CAPACITY)) { // check hub load first of all
		if (mS->Log(3))
			mS->LogStream() << "Hub load is too high for search result: " << mS->mSysLoad << endl;

		/*
		os << _("Sorry, hub load is too high to process your search result. Please try again later."); // dont notify user, he is probably sending alot of results
		mS->DCPublicHS(os.str(), conn);
		*/
		return -2;
	}

	cUser *other = mS->mUserList.GetUserByNick(msg->ChunkString(eCH_SR_TO)); // find other user

	if (!other || !other->mxConn || !other->mInList)
		return -1; // silent filter

	#ifndef WITHOUT_PLUGINS
		if (!mS->mCallBacks.mOnParsedMsgSR.CallAll(conn, msg))
			return -2;
	#endif

	if (mS->mC.max_passive_sr && (other->mxConn->mSRCounter++ >= mS->mC.max_passive_sr)) // dont send it
		return 0;

	string sr;
	sr.assign(msg->mStr, 0, msg->mChunks[eCH_SR_TO].first - 1); // cut the end
	other->mxConn->Send(sr, true, !mS->mC.delayed_search); // part of search, must be delayed too
	return 0;
}

/*
	bot commands
*/

int cDCProto::DCB_BotINFO(cMessageDC *msg, cConnDC *conn)
{
	if (CheckUserLogin(conn, msg, false))
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

	#ifndef WITHOUT_PLUGINS
		if (!mS->mCallBacks.mOnParsedMsgBotINFO.CallAll(conn, msg)) {
			conn->CloseNow(eCR_PLUGIN);
			return -1;
		}
	#endif

	os << autosprintf(_("Pinger entered the hub: %s"), msg->ChunkString(eCH_1_PARAM).c_str());

	if (conn->Log(2))
		conn->LogStream() << os.str() << endl;

	if (mS->mC.botinfo_report)
		mS->ReportUserToOpchat(conn, os.str());

	char sep = '$';
	cConnType *ConnType = mS->mConnTypes->FindConnType("default");
	unsigned __int64 minshare = mS->mC.min_share;
	string host, icon, logo, info = mS->mC.hub_host;
	size_t pos;

	if (mS->mC.min_share_use_hub > minshare)
		minshare = mS->mC.min_share_use_hub;

	if (host.size() > 2) { // remove protocol
		pos = host.find("://");

		if (pos != host.npos)
			host.erase(0, pos + 3);
	}

	if (host.size()) {
		pos = host.find(':');

		if ((pos == host.npos) && (mS->mPort != 411)) // add port
			host.append(':' + StringFrom(mS->mPort));
	}

	if (mS->mC.hub_icon_url.size())
		Create_SetIcon(icon, mS->mC.hub_icon_url);

	if (mS->mC.hub_logo_url.size())
		Create_SetLogo(logo, mS->mC.hub_logo_url);

	os.str("");
	os.clear();
	os << mS->mC.hub_name << sep;
	os << host << sep;
	os << mS->mC.hub_desc << sep;
	os << ((mS->mC.max_users[0] < mS->mC.max_users_total) ? mS->mC.max_users[0] : mS->mC.max_users_total) << sep; // prioritize main zone
	os << StringFrom((unsigned __int64)(1024 * 1024) * minshare) << sep;
	os << ((ConnType) ? ConnType->mTagMinSlots : 0) << sep;
	os << mS->mC.tag_max_hubs << sep;
	os << HUB_VERSION_NAME << ' ' << HUB_VERSION_VERS << sep;
	os << mS->mC.hub_owner << sep;
	os << mS->mC.hub_category << sep;
	os << mS->mC.hub_encoding;
	string pars(os.str());
	Create_HubINFO(info, pars);
	pars = icon + logo + info;
	conn->Send(pars, true, false);
	conn->SetLSFlag(eLS_BOTINFO);
	return 0;
}

/*
	operator commands
*/

int cDCProto::DCO_UserIP(cMessageDC *msg, cConnDC *conn)
{
	if (CheckUserLogin(conn, msg))
		return -1;

	if (CheckUserRights(conn, msg, (conn->mpUser->mClass >= mS->mC.user_ip_class)))
		return -1;

	if (CheckProtoSyntax(conn, msg))
		return -1;

	string &data = msg->ChunkString(eCH_1_PARAM);

	if (data.empty())
		return -1;

	cUser *other;
	string back, nick, sep("$$"), omsg;
	data.append(sep);
	size_t pos = data.find(sep);

	while (pos != string::npos) {
		nick = data.substr(0, pos);
		data = data.substr(pos + 2); // size of sep
		pos = data.find(sep);

		if (nick.empty())
			continue;

		other = mS->mUserList.GetUserByNick(nick);

		if (other && other->mInList) {
			if (other->mxConn) { // real user
				back.append(nick);
				back.append(1, ' ');
				back.append(other->mxConn->AddrIP());

			} else { // bots have local ip
				back.append(nick);
				back.append(" 127.0.0.1"); // size() = 1 + 9
			}

			back.append(sep);
		}
	}

	if (back.size()) {
		Create_UserIP(omsg, back); // list is sent, no separator needed
		conn->Send(omsg, true);
	}

	return 0;
}

int cDCProto::DCO_Kick(cMessageDC *msg, cConnDC *conn)
{
	if (CheckUserLogin(conn, msg))
		return -1;

	if (CheckUserRights(conn, msg, conn->mpUser->Can(eUR_KICK, mS->mTime.Sec())))
		return -1;

	if (CheckProtoSyntax(conn, msg))
		return -1;

	mS->DCKickNick(NULL, conn->mpUser, msg->ChunkString(eCH_1_PARAM), mS->mEmpty, eKI_CLOSE); // try to kick user
	return 0;
}

int cDCProto::DCO_OpForceMove(cMessageDC *msg, cConnDC *conn)
{
	if (CheckUserLogin(conn, msg))
		return -1;

	if (CheckUserRights(conn, msg, (conn->mpUser->mClass >= mS->mC.min_class_redir)))
		return -1;

	if (CheckProtoSyntax(conn, msg))
		return -1;

	ostringstream os;
	const string &nick = msg->ChunkString(eCH_FM_NICK); // find other user
	cUser *other = mS->mUserList.GetUserByNick(nick);

	if (!other || !other->mInList) {
		os << autosprintf(_("You're trying to redirect an offline user: %s"), nick.c_str());
		mS->DCPublicHS(os.str(), conn);
		return -2;
	}

	if (!other->mxConn) {
		os << autosprintf(_("You're trying to redirect a bot: %s"), nick.c_str());
		mS->DCPublicHS(os.str(), conn);
		return -2;
	}

	if ((other->mClass >= conn->mpUser->mClass) || (other->mProtectFrom >= conn->mpUser->mClass)) { // check protection
		os << autosprintf(_("You're trying to redirect a protected user: %s"), nick.c_str());
		mS->DCPublicHS(os.str(), conn);
		return -3;
	}

	const string &dest = msg->ChunkString(eCH_FM_DEST);

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
	ofm.append(1, '|');
	Create_ForceMove(ofm, dest, false); // must be last, user might not get reason otherwise
	other->mxConn->Send(ofm, true); // send it
	other->mxConn->CloseNice(5000, eCR_FORCEMOVE); // close after a while, user might not get redirect otherwise
	return 0;
}

int cDCProto::DCO_TempBan(cMessageDC *msg, cConnDC *conn)
{
	if (CheckUserLogin(conn, msg))
		return -1;

	if (CheckUserRights(conn, msg, (conn->mpUser->mClass >= eUC_OPERATOR)))
		return -1;

	if (CheckProtoSyntax(conn, msg))
		return -1;

	ostringstream os;

	// todo: i think ban is not supported here, user will get bad syntax
	long period = 0;
	// calculate time
	if (msg->ChunkString(eCH_NB_TIME).size()) {
		period = mS->Str2Period(msg->ChunkString(eCH_NB_TIME),os);
		if(!period) {
			mS->DCPublicHS(os.str(),conn);
			return -1;
		}
	}

	cUser *other = mS->mUserList.GetUserByNick(msg->ChunkString(eCH_NB_NICK));
	if(!other) {
		os << autosprintf(_("User not found: %s"), msg->ChunkString(eCH_NB_NICK).c_str());
		mS->DCPublicHS(os.str(),conn);
		return -1;
	}

	if(msg->mType == eDCO_TBAN  && !msg->ChunkString(eCH_NB_REASON).size()) {
		os << _("Please provide a valid reason.");
		mS->DCPublicHS(os.str(),conn);
		return -1;
	}

	if(other->mClass >= conn->mpUser->mClass || other->mProtectFrom >= conn->mpUser->mClass) {
		os << _("You can't ban a user that is protected or has higher class.");
		mS->DCPublicHS(os.str(),conn);
		return -1;
	}

	if(!other->mxConn) {
		os << _("You can't ban a bot.");
		mS->DCPublicHS(os.str(),conn);
		return -1;
	}

	if (period)
		os << autosprintf(_("You are temporarily prohibited from entering the hub for %s because: %s"), msg->ChunkString(eCH_NB_TIME).c_str(), msg->ChunkString(eCH_NB_REASON).c_str());
	else
		os << autosprintf(_("You are permanently banned from the hub because: %s"), msg->ChunkString(eCH_NB_REASON).c_str());

	mS->DCPrivateHS(os.str(), other->mxConn, &conn->mpUser->mNick, &conn->mpUser->mNick);
	os.str(mS->mEmpty);

	cBan ban(mS);
	mS->mBanList->NewBan(ban, other->mxConn, conn->mpUser->mNick, msg->ChunkString(eCH_NB_REASON), period, eBF_NICKIP);
	mS->mBanList->AddBan(ban);
	mS->DCKickNick(NULL, conn->mpUser, msg->ChunkString(eCH_NB_NICK), mS->mEmpty, eKI_CLOSE); // drop only
	ban.DisplayKick(os);
	mS->DCPublicHS(os.str(), conn);
	other->mxConn->CloseNice(1000, eCR_KICKED);
	return 0;
}

int cDCProto::DCO_UnBan(cMessageDC *msg, cConnDC *conn)
{
	if (CheckUserLogin(conn, msg))
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
		os << autosprintf(_("No banned user found with IP %s."), msg->ChunkString(eCH_1_PARAM).c_str());
		mS->DCPublicHS(os.str().c_str(),conn);
		return -1;
	}
	os << autosprintf(ngettext("Removed %d ban.", "Removed %d bans.", n), n) << endl;
	mS->DCPublicHS(os.str().c_str(),conn);
	return 0;
}

int cDCProto::DCO_GetBanList(cMessageDC *msg, cConnDC *conn)
{
	if (CheckUserLogin(conn, msg))
		return -1;

	if (CheckUserRights(conn, msg, (conn->mpUser->mClass >= eUC_OPERATOR)))
		return -1;

	// todo: mS->mBanList->GetBanList(conn);
	return 0;
}

int cDCProto::DCO_WhoIP(cMessageDC *msg, cConnDC *conn)
{
	if (CheckUserLogin(conn, msg))
		return -1;

	if (CheckUserRights(conn, msg, (conn->mpUser->mClass >= mS->mC.user_ip_class)))
		return -1;

	if (CheckProtoSyntax(conn, msg))
		return -1;

	const string &ip = msg->ChunkString(eCH_1_PARAM);
	string nicklist, sep("$$");
	nicklist.append("$UsersWithIP ");
	nicklist.append(ip);
	nicklist.append(1, '$');
	unsigned long num = 0;
	cBanList::Ip2Num(ip, num); // not validated, returns 0 in case of error
	mS->WhoIP(num, num, nicklist, sep, true);
	conn->Send(nicklist, true);
	return 0;
}

int cDCProto::DCO_GetTopic(cMessageDC *msg, cConnDC *conn)
{
	if (CheckUserLogin(conn, msg))
		return -1;

	if (CheckUserRights(conn, msg, (conn->mpUser->mClass >= eUC_OPERATOR)))
		return -1;

	if (mS->mC.hub_topic.size()/* && (conn->mFeatures & eSF_HUBTOPIC)*/) {
		string topic;
		Create_HubTopic(topic, mS->mC.hub_topic);
		conn->Send(topic, true);
	}

	return 0;
}

int cDCProto::DCO_SetTopic(cMessageDC *msg, cConnDC *conn)
{
	if (CheckUserLogin(conn, msg))
		return -1;

	if (CheckUserRights(conn, msg, (conn->mpUser->mClass >= mS->mC.topic_mod_class)))
		return -1;

	if (CheckProtoSyntax(conn, msg))
		return -1;

	const string &topic = msg->ChunkString(eCH_1_PARAM); // todo: check length
	mS->mC.hub_topic = topic;
	ostringstream os;
	os << autosprintf(_("Topic is set to: %s"), topic.c_str());
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

int cDCProto::DCC_MyIP(cMessageDC *msg, cConnDC *conn)
{
	if (mS->mTLSProxy.empty())
		return this->DCU_Unknown(msg, conn);

	if (conn->AddrIP() != mS->mTLSProxy)
		return this->DCU_Unknown(msg, conn);

	if ((msg->mLen < 17) || (msg->mLen > 25)) {
		conn->CloseNow(eCR_SYNTAX);
		return -1;
	}

	if (msg->SplitChunks()) {
		conn->CloseNow(eCR_SYNTAX);
		return -1;
	}

	const string &addr = msg->ChunkString(eCH_MYIP_IP);
	string &vers = msg->ChunkString(eCH_MYIP_VERS);

	if (!conn->SetSecConn(addr, vers)) {
		conn->CloseNow(eCR_SYNTAX);
		return -1;
	}

	if (0 > mS->OnNewConn(conn)) {
		mS->delConnection(conn);
		return -1;
	}

	return 0;
}

int cDCProto::DCC_MyNick(cMessageDC *msg, cConnDC *conn)
{
	if (!mS->mC.detect_ctmtohub)
		return this->DCU_Unknown(msg, conn);

	if (conn->mMyNick.size()) {
		conn->CloseNow(eCR_LOGIN_ERR);
		return -1;
	}

	if (msg->mLen > mS->mC.max_len_mynick) {
		conn->CloseNow(eCR_SYNTAX);
		return -1;
	}

	if (msg->SplitChunks()) {
		conn->CloseNow(eCR_SYNTAX);
		return -1;
	}

	const string &nick = msg->ChunkString(eCH_1_PARAM);

	if (nick.empty()) {
		conn->CloseNow(eCR_SYNTAX);
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
		conn->CloseNow(eCR_LOGIN_ERR);
		return -1;
	}

	if (msg->mLen > mS->mC.max_len_lock) {
		conn->CloseNow(eCR_SYNTAX);
		return -1;
	}

	if (msg->SplitChunks()) {
		conn->CloseNow(eCR_SYNTAX);
		return -1;
	}

	const string &lock = msg->ChunkString(eCH_1_PARAM);

	if (lock.empty()) {
		conn->CloseNow(eCR_SYNTAX);
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

bool cDCProto::CheckUserLogin(cConnDC *conn, cMessageDC *msg, bool inlist)
{
	if (!conn)
		return true;

	if (conn->mpUser && (!inlist || conn->mpUser->mInList))
		return false;

	ostringstream rsn;
	string pref;

	if (msg && msg->mStr.size()) {
		pref = msg->mStr.substr(0, msg->mStr.find_first_of(' '));

		if (pref.size())
			EscapeChars(pref, pref);
	}

	if (pref.size())
		rsn << autosprintf(_("Invalid login sequence, your client must validate nick first: %s"), ((msg->mType == eDC_CHAT) ? _("Chat") : pref.c_str()));
	else
		rsn << _("Invalid login sequence, your client must validate nick first.");

	if (conn->Log(1)) {
		if (pref.size())
			conn->LogStream() << "Invalid login sequence: " << msg->mStr << endl;
		else
			conn->LogStream() << "Invalid login sequence" << endl;
	}

	mS->ConnCloseMsg(conn, rsn.str(), 1000, eCR_LOGIN_ERR);
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

		case eDCO_KICK:
			cmd = "Kick";
			break;

		case eDCO_OPFORCEMOVE:
			cmd = "OpForceMove";
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
		case eDC_CONNECTTOME:
		case eDC_MCONNECTTOME:
			cmd = "ConnectToMe";
			break;

		case eDC_RCONNECTTOME:
			cmd = "RevConnectToMe";
			break;

		case eDC_SR:
			cmd = "SR";
			break;

		case eDC_SEARCH_PAS:
		case eDC_SEARCH:
		case eDC_MSEARCH_PAS:
		case eDC_MSEARCH:
			cmd = "Search";
			break;

		case eDC_TTHS:
			cmd = "SA";
			break;

		case eDC_TTHS_PAS:
			cmd = "SP";
			break;

		case eDC_MYINFO:
			cmd = "MyINFO";
			break;

		case eDC_EXTJSON:
			cmd = "ExtJSON";
			break;

		case eDC_KEY:
			cmd = "Key";
			break;

		case eDC_SUPPORTS:
			cmd = "Supports";
			break;

		case eDC_VALIDATENICK:
			cmd = "ValidateNick";
			break;

		case eDC_VERSION:
			cmd = "Version";
			break;

		case eDC_MYHUBURL:
			cmd = "MyHubURL";
			break;

		case eDC_MYPASS:
			cmd = "MyPass";
			break;

		case eDC_TO:
			cmd = "To";
			break;

		case eDC_CHAT:
			cmd = _("Chat");
			break;

		case eDCB_BOTINFO:
			cmd = "BotINFO";
			break;

		case eDC_GETINFO:
			cmd = "GetINFO";
			break;

		case eDCO_USERIP:
			cmd = "UserIP";
			break;

		case eDCO_KICK:
			cmd = "Kick";
			break;

		case eDCO_OPFORCEMOVE:
			cmd = "OpForceMove";
			break;

		case eDC_MCTO:
			cmd = "MCTo";
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

		case eDC_IN:
			cmd = "IN";
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

bool cDCProto::CheckProtoLen(cConnDC *conn, cMessageDC *msg)
{
	unsigned int mlen = 0;

	switch (msg->mType) {
		case eDC_SUPPORTS:
			mlen = mS->mC.max_len_supports;
			break;

		case eDC_VERSION:
			mlen = mS->mC.max_len_version;
			break;

		case eDC_MYINFO:
			mlen = mS->mC.max_len_myinfo;
			break;

		case eDC_IN:
			mlen = mS->mC.max_len_in;
			break;

		case eDC_EXTJSON:
			mlen = mS->mC.max_len_extjson;
			break;

		case eDC_MYHUBURL:
			mlen = mS->mC.max_len_myhuburl;
			break;

		case eDC_TTHS:
		case eDC_TTHS_PAS:
		case eDC_SEARCH_PAS:
		case eDC_SEARCH:
		case eDC_MSEARCH_PAS:
		case eDC_MSEARCH:
			mlen = mS->mC.max_len_search;
			break;

		case eDCC_MYNICK:
			mlen = mS->mC.max_len_mynick;
			break;

		case eDCC_LOCK:
			mlen = mS->mC.max_len_lock;
			break;
	}

	if (!mlen || (msg->mLen <= mlen))
		return false;

	string cmd;

	switch (msg->mType) {
		case eDC_SUPPORTS:
			cmd = "Supports";
			break;

		case eDC_VERSION:
			cmd = "Version";
			break;

		case eDC_MYINFO:
			cmd = "MyINFO";
			break;

		case eDC_IN:
			cmd = "IN";
			break;

		case eDC_EXTJSON:
			cmd = "ExtJSON";
			break;

		case eDC_MYHUBURL:
			cmd = "MyHubURL";
			break;

		case eDC_TTHS:
			cmd = "SA";
			break;

		case eDC_TTHS_PAS:
			cmd = "SP";
			break;

		case eDC_SEARCH_PAS:
		case eDC_SEARCH:
		case eDC_MSEARCH_PAS:
		case eDC_MSEARCH:
			cmd = "Search";
			break;

		case eDCC_MYNICK:
			cmd = "MyNick";
			break;

		case eDCC_LOCK:
			cmd = "Lock";
			break;
	}

	ostringstream os;
	os << autosprintf(_("Your client sent too long protocol command: %s"), cmd.c_str());

	if (conn->Log(1))
		conn->LogStream() << os.str() << " [" << msg->mLen << ':' << mlen << ']' << endl;

	mS->ConnCloseMsg(conn, os.str(), 1000, eCR_SYNTAX);
	return true;
}

bool cDCProto::CheckUserNick(cConnDC *conn, const string &nick)
{
	if (mS->mUserList.Nick2Hash(nick) == conn->mpUser->mNickHash)
		return false;

	ostringstream os;
	os << autosprintf(_("Nick spoofing attempt detected from your client: %s"), nick.c_str());

	if (conn->Log(1))
		conn->LogStream() << os.str() << endl;

	mS->ConnCloseMsg(conn, os.str(), 1000, eCR_SYNTAX);
	return true;
}

bool cDCProto::CheckCompatTLS(cConnDC *one, cConnDC *two)
{
	if (!one || !one->mpUser || !two || !two->mpUser)
		return false;

	ostringstream os;

	if (one->mTLSVer.size() && (one->mTLSVer != "0.0") && two->mTLSVer.size() && (two->mTLSVer != "0.0")) { // we know tls version of both users
		/*
			1.3 works with 1.3 and 1.2
			1.2 works with 1.3 and 1.2 and 1.0
			1.0 works with 1.2 and 1.0
		*/

		if (one->mTLSVer == two->mTLSVer) // both have same version
			return true;

		if (one->mTLSVer == "1.3") { // one has version 1.3
			if (two->mTLSVer == "1.2") { // two has version 1.2
				return true;

			} else if (two->mTLSVer == "1.0") { // two has version 1.0
				if (!mS->mC.hide_msg_badtlsctm && !one->mpUser->mHideCtmMsg) {
					os << autosprintf(_("User you're trying connect to doesn't have compatible TLS version in order to establish secure connection, tell him to upgrade or replace his client: %s"), two->mpUser->mNick.c_str());
					os << "\r\n\r\n " << _("Consider these more or less modern clients") << ":\r\n\r\n";
					os << "\tDC++ @ https://dcplusplus.sourceforge.io\r\n";
					os << "\tFearDC @ https://client.feardc.net\r\n";
					os << "\tAirDC++ @ https://airdcpp.net\r\n";
					os << "\tApexDC++ @ https://apexdc.net\r\n";
					os << "\tEiskaltDC++ @ https://eiskaltdcpp.sourceforge.io\r\n";
					os << "\tFlylinkDC++ @ http://flylinkdc.com\r\n";
					mS->DCPublicHS(os.str(), one);
				}

				return false;
			}

		} else if (one->mTLSVer == "1.2") { // one has version 1.2
			if (two->mTLSVer == "1.3") // two has version 1.3
				return true;
			else if (two->mTLSVer == "1.0") // two has version 1.0
				return true;

		} else if (one->mTLSVer == "1.0") { // one has version 1.0
			if (two->mTLSVer == "1.3") { // two has version 1.3
				if (!mS->mC.hide_msg_badtlsctm && !one->mpUser->mHideCtmMsg) {
					os << autosprintf(_("Your client doesn't have compatible TLS version in order to establish secure connection to following user, please upgrade or replace your client: %s"), two->mpUser->mNick.c_str());
					os << "\r\n\r\n " << _("Consider these more or less modern clients") << ":\r\n\r\n";
					os << "\tDC++ @ https://dcplusplus.sourceforge.io\r\n";
					os << "\tFearDC @ https://client.feardc.net\r\n";
					os << "\tAirDC++ @ https://airdcpp.net\r\n";
					os << "\tApexDC++ @ https://apexdc.net\r\n";
					os << "\tEiskaltDC++ @ https://eiskaltdcpp.sourceforge.io\r\n";
					os << "\tFlylinkDC++ @ http://flylinkdc.com\r\n";
					mS->DCPublicHS(os.str(), one);
				}

				return false;

			} else if (two->mTLSVer == "1.2") { // two has version 1.2
				return true;
			}
		}
	}

	if (!(one->mFeatures & eSF_TLS) || !one->mpUser->GetMyFlag(eMF_TLS)) { // one dont have tls flag in supports or myinfo
		if (!mS->mC.hide_msg_badtlsctm && !one->mpUser->mHideCtmMsg) {
			os << autosprintf(_("Your client doesn't support secure connections to following user, please upgrade or replace your client: %s"), two->mpUser->mNick.c_str());
			os << "\r\n\r\n " << _("Consider these more or less modern clients") << ":\r\n\r\n";
			os << "\tDC++ @ https://dcplusplus.sourceforge.io\r\n";
			os << "\tFearDC @ https://client.feardc.net\r\n";
			os << "\tAirDC++ @ https://airdcpp.net\r\n";
			os << "\tApexDC++ @ https://apexdc.net\r\n";
			os << "\tEiskaltDC++ @ https://eiskaltdcpp.sourceforge.io\r\n";
			os << "\tFlylinkDC++ @ http://flylinkdc.com\r\n";
			mS->DCPublicHS(os.str(), one);
		}

		return false;
	}

	if (!(two->mFeatures & eSF_TLS) || !two->mpUser->GetMyFlag(eMF_TLS)) { // two dont have tls flag in supports or myinfo
		if (!mS->mC.hide_msg_badtlsctm && !one->mpUser->mHideCtmMsg) {
			os << autosprintf(_("User you're trying connect to doesn't support secure connections, tell him to upgrade or replace his client: %s"), two->mpUser->mNick.c_str());
			os << "\r\n\r\n " << _("Consider these more or less modern clients") << ":\r\n\r\n";
			os << "\tDC++ @ https://dcplusplus.sourceforge.io\r\n";
			os << "\tFearDC @ https://client.feardc.net\r\n";
			os << "\tAirDC++ @ https://airdcpp.net\r\n";
			os << "\tApexDC++ @ https://apexdc.net\r\n";
			os << "\tEiskaltDC++ @ https://eiskaltdcpp.sourceforge.io\r\n";
			os << "\tFlylinkDC++ @ http://flylinkdc.com\r\n";
			mS->DCPublicHS(os.str(), one);
		}

		return false;
	}

	return true; // default, note: we can not predict future version compatibility, it is now up to a script to compare tls client versions
}

bool cDCProto::FindInSupports(const string &list, const string &flag)
{
	if (list.empty())
		return false;

	istringstream is(list);
	string item;

	while (1) {
		item.clear();
		is >> item;

		if (item.empty())
			break;

		if (StrCompare(item, 0, flag.size(), flag) == 0)
			return true;
	}

	return false;
}

int cDCProto::NickList(cConnDC *conn)
{
	//try {
		string _str;

		/*
		if (conn->GetLSFlag(eLS_LOGIN_DONE) != eLS_LOGIN_DONE)
			conn->mNickListInProgress = true;
		*/

		if (conn->mFeatures & eSF_NOHELLO) {
			if (conn->Log(3))
				conn->LogStream() << "Sending MyINFO list" << endl;

			if (mS->mC.myinfo_tls_filter && conn->mpUser && !conn->mpUser->GetMyFlag(eMF_TLS)) { // myinfo tls filter
				cUser *user;
				string info;
				_str.clear();

				for (cUserCollection::iterator it = mS->mUserList.begin(); it != mS->mUserList.end(); ++it) {
					user = (cUser*)(*it);

					if (user && user->mInList && user->mFakeMyINFO.size()) {
						if (user->GetMyFlag(eMF_TLS))
							mS->RemoveMyINFOFlag(info, user->mFakeMyINFO, eMF_TLS);
						else
							info = user->mFakeMyINFO;

						_str += info + '|';
					}
				}

			} else {
				mS->mUserList.GetInfoList(_str);
			}

			conn->Send(_str, false); // pipe was already added by list generator

		} else if (conn->mFeatures & eSF_NOGETINFO) {
			if (conn->Log(3))
				conn->LogStream() << "Sending MyINFO list" << endl;

			mS->mUserList.GetNickList(_str);
			conn->Send(_str, true);

			if (mS->mC.myinfo_tls_filter && conn->mpUser && !conn->mpUser->GetMyFlag(eMF_TLS)) { // myinfo tls filter
				cUser *user;
				string info;
				_str.clear();

				for (cUserCollection::iterator it = mS->mUserList.begin(); it != mS->mUserList.end(); ++it) {
					user = (cUser*)(*it);

					if (user && user->mInList && user->mFakeMyINFO.size()) {
						if (user->GetMyFlag(eMF_TLS))
							mS->RemoveMyINFOFlag(info, user->mFakeMyINFO, eMF_TLS);
						else
							info = user->mFakeMyINFO;

						_str += info + '|';
					}
				}

			} else {
				mS->mUserList.GetInfoList(_str);
			}

			conn->Send(_str, false); // pipe was already added by list generator

		} else {
			if (conn->Log(3))
				conn->LogStream() << "Sending Nicklist" << endl;

			mS->mUserList.GetNickList(_str);
			conn->Send(_str, true);
		}

		if (mS->mOpList.Size()) { // send $OpList
			mS->mOpList.GetNickList(_str);
			conn->Send(_str, true);
		}

		if (mS->mRobotList.Size() && (conn->mFeatures & eSF_BOTLIST)) { // send $BotList
			mS->mRobotList.GetNickList(_str);
			conn->Send(_str, true);
		}

		if (mS->mC.send_user_ip && conn->mpUser && (conn->mFeatures & eSF_USERIP2)) { // send $UserIP
			if (conn->mpUser->mClass >= mS->mC.user_ip_class) { // full list
				mS->mUserList.GetIPList(_str);
				conn->Send(_str, true);

			} else { // own ip only
				Create_UserIP(_str, conn->mpUser->mNick, conn->AddrIP());
				conn->Send(_str, true);
			}
		}

		if (!mS->mC.disable_extjson && (conn->mFeatures & eSF_EXTJSON2)) { // extjson
			if (mS->CollectExtJSON(_str, conn))
				conn->Send(_str, false); // no pipe, its already added by collector
		}

	/*
	} catch (...) {
		if (conn->ErrLog(2))
			conn->LogStream() << "Exception in cDCProto::NickList" << endl;

		conn->CloseNow();
		return -1;
	}
	*/

	return 0;
}

int cDCProto::ParseForCommands(string &text, cConnDC *conn, int pm)
{
	if (text.empty() || !conn || !conn->mpUser)
		return 0;

	string cmd(text);

	if (cmd.empty())
		return 0;

	ostringstream omsg;

	// operator commands

	if ((conn->mpUser->mClass >= eUC_OPERATOR) && (mS->mC.cmd_start_op.find_first_of(cmd[0]) != string::npos)) {
		#ifndef WITHOUT_PLUGINS
		if (mS->mCallBacks.mOnHubCommand.CallAll(conn, &cmd, 1, pm) && mS->mCallBacks.mOnOperatorCommand.CallAll(conn, &cmd))
		#endif
		{
			if (!mS->mCo->OpCommand(cmd, conn) && !mS->mCo->UsrCommand(cmd, conn)) {
				if (mS->mC.unknown_cmd_chat)
					return 0;

				omsg << autosprintf(_("Unknown hub command: %s"), cmd.c_str());
				mS->DCPublicHS(omsg.str(), conn);
			}
		}

		return 1;
	}

	// user commands

	if (mS->mC.cmd_start_user.find_first_of(cmd[0]) != string::npos) {
		#ifndef WITHOUT_PLUGINS
		if (mS->mCallBacks.mOnHubCommand.CallAll(conn, &cmd, 0, pm) && mS->mCallBacks.mOnUserCommand.CallAll(conn, &cmd))
		#endif
		{
			if (!mS->mCo->UsrCommand(cmd, conn)) {
				if (mS->mC.unknown_cmd_chat)
					return 0;

				omsg << autosprintf(_("Unknown hub command: %s"), cmd.c_str());
				mS->DCPublicHS(omsg.str(), conn);
			}
		}

		return 1;
	}

	return 0;
}

bool cDCProto::CheckIP(cConnDC *conn, const string &ip)
{
	if (StrCompare(ip, 0, conn->mAddrIP.size(), conn->mAddrIP) == 0)
		return true;

	if (conn->mRegInfo && ((conn->mRegInfo->mAlternateIP.size() && (StrCompare(ip, 0, conn->mRegInfo->mAlternateIP.size(), conn->mRegInfo->mAlternateIP) == 0)) || (conn->mRegInfo->mFakeIP.size() && (StrCompare(ip, 0, conn->mRegInfo->mFakeIP.size(), conn->mRegInfo->mFakeIP) == 0))))
		return true;

	return false;
}

bool cDCProto::isLanIP(const string &ip)
{
	if (ip.substr(0, 4) == "127.")
		return true;

	unsigned long lip = 0;

	if (cBanList::Ip2Num(ip, lip, false) && (((lip >= 167772160UL) && (lip <= 184549375UL)) || ((lip >= 2886729728UL) && (lip <= 2887778303UL)) || ((lip >= 3232235520UL) && (lip <= 3232301055UL))))
		return true;

	/*
		todo
			add list of lan ip ranges defined by hub administrator, external ips that know how to work with lan ips
	*/

	return false;
}

bool cDCProto::CheckChatMsg(const string &text, cConnDC *conn)
{
	if (!conn || !conn->mxServer)
		return true;

	cServerDC *Server = conn->Server();
	ostringstream errmsg;
	unsigned int count = text.size();

	if (!Server->mC.max_chat_msg || (count > Server->mC.max_chat_msg)) {
		errmsg << autosprintf(_("Your chat message contains %d characters but maximum allowed is %d characters."), count, Server->mC.max_chat_msg);
		Server->DCPublicHS(errmsg.str(), conn);
		return false;
	}

	count = CountLines(text);

	if (!Server->mC.max_chat_lines || (count > Server->mC.max_chat_lines)) {
		errmsg << autosprintf(_("Your chat message contains %d lines but maximum allowed is %d lines."), count, Server->mC.max_chat_lines);
		Server->DCPublicHS(errmsg.str(), conn);
		return false;
	}

	return true;
}

void cDCProto::Create_MyINFO(string &dest, const string &nick, const string &desc, const string &speed, const string &mail, const string &share)
{
	if (dest.size())
		dest.clear();

	dest.append("$MyINFO $ALL ");
	dest.append(nick);
	dest.append(1, ' ');
	dest.append(desc);
	dest.append("$ $");
	dest.append(speed);
	dest.append(1, '$');
	dest.append(mail);
	dest.append(1, '$');
	dest.append(share);
	dest.append(1, '$');
}

void cDCProto::Create_Lock(string &dest, const string &lock, const string &name, const string &vers)
{
	if (dest.size())
		dest.clear();

	dest.append("$Lock ");
	dest.append(lock);
	dest.append(" Pk=");
	dest.append(name);
	dest.append(1, ' ');
	dest.append(vers);
}

void cDCProto::Create_Chat(string &dest, const string &nick, const string &text)
{
	if (dest.size())
		dest.clear();

	dest.append(1, '<');
	dest.append(nick);
	dest.append("> ");
	dest.append(text);
}

void cDCProto::Create_Me(string &dest, const string &nick, const string &text)
{
	if (dest.size())
		dest.clear();

	dest.append("** ");
	dest.append(nick);
	dest.append(1, ' ');
	dest.append(text);
}

void cDCProto::Create_PM(string &dest,const string &from, const string &to, const string &sign, const string &text)
{
	if (dest.size())
		dest.clear();

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
	if (start.size())
		start.clear();

	if (end.size())
		end.clear();

	start.append("$To: ");
	end.append(" From: ");
	end.append(from);
	end.append(" $<");
	end.append(sign);
	end.append("> ");
	end.append(text);
}

void cDCProto::Create_HubName(string &dest, const string &name, const string &topic)
{
	if (dest.size())
		dest.clear();

	dest.append("$HubName ");
	dest.append(name);

	if (topic.size()) {
		dest.append(" - ");
		dest.append(topic);
	}
}

void cDCProto::Create_HubTopic(string &dest, const string &topic)
{
	if (dest.size())
		dest.clear();

	dest.append("$HubTopic ");
	dest.append(topic);
}

void cDCProto::Create_Quit(string &dest, const string &nick)
{
	if (dest.size())
		dest.clear();

	dest.append("$Quit ");
	dest.append(nick);
}

void cDCProto::Create_Hello(string &dest, const string &nick)
{
	if (dest.size())
		dest.clear();

	dest.append("$Hello ");
	dest.append(nick);
}

void cDCProto::Create_LogedIn(string &dest, const string &nick)
{
	if (dest.size())
		dest.clear();

	dest.append("$LogedIn ");
	dest.append(nick);
}

void cDCProto::Create_ValidateDenide(string &dest, const string &nick)
{
	if (dest.size())
		dest.clear();

	dest.append("$ValidateDenide ");
	dest.append(nick);
}

void cDCProto::Create_BadNick(string &dest, const string &id, const string &par)
{
	if (dest.size())
		dest.clear();

	dest.append("$BadNick ");
	dest.append(id);

	if (par.size()) {
		dest.append(1, ' ');
		dest.append(par);
	}
}

void cDCProto::Create_NickList(string &dest, const string &nick)
{
	if (dest.size())
		dest.clear();

	dest.append("$NickList ");
	dest.append(nick);
	dest.append("$$");
}

void cDCProto::Create_OpList(string &dest, const string &nick)
{
	if (dest.size())
		dest.clear();

	dest.append("$OpList ");
	dest.append(nick);
	dest.append("$$");
}

void cDCProto::Create_BotList(string &dest, const string &nick)
{
	if (dest.size())
		dest.clear();

	dest.append("$BotList ");
	dest.append(nick);
	dest.append("$$");
}

void cDCProto::Create_Key(string &dest, const string &key)
{
	if (dest.size())
		dest.clear();

	dest.append("$Key ");
	dest.append(key);
}

void cDCProto::Create_FailOver(string &dest, const string &addr)
{
	if (dest.size())
		dest.clear();

	dest.append("$FailOver ");
	dest.append(addr);
}

void cDCProto::Create_ForceMove(string &dest, const string &addr, const bool clear)
{
	if (clear && dest.size())
		dest.clear();

	dest.append("$ForceMove ");
	dest.append(addr);
}

void cDCProto::Create_ConnectToMe(string &dest, const string &nick, const string &addr, const string &port, const string &extra)
{
	if (dest.size())
		dest.clear();

	dest.append("$ConnectToMe ");
	dest.append(nick);
	dest.append(1, ' ');
	dest.append(addr);
	dest.append(1, ':');
	dest.append(port);
	dest.append(extra);
}

void cDCProto::Create_Search(string &dest, const string &addr, const string &lims, const string &spat)
{
	if (dest.size())
		dest.clear();

	dest.append("$Search ");
	dest.append(addr);
	dest.append(1, ' ');
	dest.append(lims);
	dest.append(spat);
}

void cDCProto::Create_Search(string &dest, const string &addr, const string &tth, const bool pas)
{
	if (dest.size())
		dest.clear();

	dest.append("$Search ");

	if (pas)
		dest.append("Hub:");

	dest.append(addr);
	dest.append(" F?T?0?9?TTH:"); // note: taken from dc++ behaviour but nmdc specification says following: is_max_size is F if size_restricted is F
	dest.append(tth);
}

void cDCProto::Create_SA(string &dest, const string &tth, const string &addr)
{
	if (dest.size())
		dest.clear();

	dest.append("$SA ");
	dest.append(tth);
	dest.append(1, ' ');
	dest.append(addr);
}

void cDCProto::Create_SP(string &dest, const string &tth, const string &nick)
{
	if (dest.size())
		dest.clear();

	dest.append("$SP ");
	dest.append(tth);
	dest.append(1, ' ');
	dest.append(nick);
}

void cDCProto::Create_UserIP(string &dest, const string &list)
{
	if (dest.size())
		dest.clear();

	dest.append("$UserIP ");
	dest.append(list);
}

void cDCProto::Create_UserIP(string &dest, const string &nick, const string &addr)
{
	if (dest.size())
		dest.clear();

	dest.append("$UserIP ");
	dest.append(nick);
	dest.append(1, ' ');
	dest.append(addr);
	dest.append("$$");
}

void cDCProto::Create_GetPass(string &dest)
{
	if (dest.size())
		dest.clear();

	dest.append("$GetPass");
}

void cDCProto::Create_BadPass(string &dest)
{
	if (dest.size())
		dest.clear();

	dest.append("$BadPass");
}

void cDCProto::Create_GetHubURL(string &dest)
{
	if (dest.size())
		dest.clear();

	dest.append("$GetHubURL");
}

void cDCProto::Create_HubIsFull(string &dest)
{
	if (dest.size())
		dest.clear();

	dest.append("$HubIsFull");
}

void cDCProto::Create_Supports(string &dest, const string &flags)
{
	if (dest.size())
		dest.clear();

	dest.append("$Supports ");
	dest.append(flags);
}

void cDCProto::Create_NickRule(string &dest, const string &rules)
{
	if (dest.size())
		dest.clear();

	dest.append("$NickRule ");
	dest.append(rules);
}

void cDCProto::Create_SearchRule(string &dest, const string &rules)
{
	if (dest.size())
		dest.clear();

	dest.append("$SearchRule ");
	dest.append(rules);
}

void cDCProto::Create_SetIcon(string &dest, const string &url, bool pipe)
{
	if (dest.size())
		dest.clear();

	dest.append("$SetIcon ");
	dest.append(url);

	if (pipe)
		dest.append(1, '|');
}

void cDCProto::Create_SetLogo(string &dest, const string &url, bool pipe)
{
	if (dest.size())
		dest.clear();

	dest.append("$SetLogo ");
	dest.append(url);

	if (pipe)
		dest.append(1, '|');
}

void cDCProto::Create_HubINFO(string &dest, const string &pars)
{
	if (dest.size())
		dest.clear();

	dest.append("$HubINFO ");
	dest.append(pars);
}

cConnType* cDCProto::ParseSpeed(const string &uspeed)
{
	string speed;

	if (uspeed.size() > 1)
		speed.assign(uspeed, 0, uspeed.size() - 1);

	return mS->mConnTypes->FindConnType(speed);
}

void cDCProto::ParseReferer(const string &lock, string &ref, bool inlock)
{
	size_t pos;

	if (inlock) {
		pos = lock.find("Ref=");

		if (pos == lock.npos)
			return;

		ref = lock.substr(pos + 4);
	} else {
		ref = lock;
	}

	ref = toLower(ref);

	if (ref.size() > 2) { // use this instead, todo: what about nmdcs:// ?
		pos = ref.find("://");

		if (pos != ref.npos)
			ref.erase(0, pos + 3);
	}

	/*
	while (ref.size() > 7) {
		pos = ref.find("dchub://");

		if (pos != ref.npos)
			ref.erase(pos, 8);
		else
			break;
	}

	while (ref.size() > 6) {
		pos = ref.find("nmdc://");

		if (pos != ref.npos)
			ref.erase(pos, 7);
		else
			break;
	}

	while (ref.size() > 7) {
		pos = ref.find("nmdcs://");

		if (pos != ref.npos)
			ref.erase(pos, 8);
		else
			break;
	}

	while (ref.size() > 5) {
		pos = ref.find("adc://");

		if (pos != ref.npos)
			ref.erase(pos, 6);
		else
			break;
	}

	while (ref.size() > 6) {
		pos = ref.find("adcs://");

		if (pos != ref.npos)
			ref.erase(pos, 7);
		else
			break;
	}

	while (ref.size() > 6) {
		pos = ref.find("http://");

		if (pos != ref.npos)
			ref.erase(pos, 7);
		else
			break;
	}

	while (ref.size() > 7) {
		pos = ref.find("https://");

		if (pos != ref.npos)
			ref.erase(pos, 8);
		else
			break;
	}
	*/

	if (ref.size() > 0) { // use this instead
		pos = ref.find('/');

		if (pos != ref.npos)
			ref.erase(pos);
	}

	/*
	while (ref.size() > 0) {
		pos = ref.find('/');

		if (pos != ref.npos)
			ref.erase(pos, 1);
		else
			break;
	}
	*/

	while (ref.size() > 2) {
		pos = ref.find("...");

		if (pos != ref.npos)
			ref.erase(pos, 3);
		else
			break;
	}

	while (ref.size() > 1) {
		pos = ref.find("..");

		if (pos != ref.npos)
			ref.erase(pos, 2);
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

	while (ref.size() > 0) {
		pos = ref.find("\t");

		if (pos != ref.npos)
			ref.erase(pos, 1);
		else
			break;
	}

	while (ref.size() > 0) {
		pos = ref.find("\n");

		if (pos != ref.npos)
			ref.erase(pos, 1);
		else
			break;
	}

	while (ref.size() > 0) {
		pos = ref.find("\r");

		if (pos != ref.npos)
			ref.erase(pos, 1);
		else
			break;
	}

	while (ref.size() > 0) {
		pos = ref.find("]");

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

	pos = ref.size();

	if ((pos > 0) && (ref[pos - 1] == '.'))
		ref.erase(pos - 1, 1);
}

void cDCProto::UnEscapeChars(const string &src, string &dst, bool WithDCN)
{
	dst = src;
	size_t pos = dst.find("&#36;");

	while (pos != dst.npos) {
		dst.replace(pos, 5, 1, '$');
		pos = dst.find("&#36;", pos);
	}

	pos = dst.find("&#124;");

	while (pos != dst.npos) {
		dst.replace(pos, 6, 1, '|');
		pos = dst.find("&#124;", pos);
	}
}

void cDCProto::UnEscapeChars(const string &src, char *dst, unsigned int &len, bool WithDCN)
{
	size_t pos, pos2 = 0;
	string start, end;
	unsigned char c;
	unsigned int i = 0;

	if (!WithDCN) {
		start = "&#";
		end = ";";
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

		if ((pos2 != src.npos) && ((pos2 - pos) <= (start.size() + 3))) {
				c = atoi(src.substr(pos + start.size(), 3).c_str());
				dst[i++] = c;
				pos2 += end.size();
		}

		pos = src.find(start, pos + 1);
	}

	pos = src.size();

	if (pos2 < pos) {
		memcpy(dst + i, src.c_str() + pos2, (pos - pos2) + 1);
		i += pos - pos2;
	}

	len = i;
}

void cDCProto::EscapeChars(const string &src, string &dst, bool WithDCN)
{
	dst = src;
	size_t pos = dst.find_first_of("\x05\x24\x60\x7C\x7E\x00"); // 0, 5, 36, 96, 124, 126
	ostringstream os;

	while (pos != dst.npos) {
		os.str("");

		if (!WithDCN)
			os << "&#" << unsigned(dst[pos]) << ';';
		else
			os << "/%DCN" << unsigned(dst[pos]) << "%/";

		dst.replace(pos, 1, os.str());
		pos = dst.find_first_of("\x05\x24\x60\x7C\x7E\x00", pos);
	}
}

void cDCProto::EscapeChars(const char *buf, int len, string &dest, bool WithDCN)
{
	dest = "";
	unsigned char c;
	unsigned int olen = 0;
	ostringstream os;

	while (len-- > 0) {
		c = *(buf++);
		olen = 0;

		switch (c) {
			case 0:
			case 5:
			case 36:
			case 96:
			case 124:
			case 126:
				os.str("");

				if (!WithDCN) {
					os << "&#" << unsigned(c) << ';';
				} else {
					if (c < 10)
						olen = 7;
					else if (c < 100) // c can not be == 10 here
						olen = 6;

					os.width(olen);
					os.fill('0');
					os << left << "/%DCN" << unsigned(c);
					os.width(0);
					os << "%/";
				}

				dest += os.str();
				break;
			default:
				dest += c;
				break;
		}
	}
}

void cDCProto::Lock2Key(const string &Lock, string &fkey)
{
	unsigned int len = Lock.size();

	if (len < 2) {
		fkey = "";
		return;
	}

	size_t count = 0;
	char *key = 0;
	char *lock = new char[len + 1];
	UnEscapeChars(Lock, lock, len, true);
	key = new char[len + 1];
	key[0] = lock[0] ^ lock[len - 1] ^ lock[len - 2] ^ 5;

	while (++count < len)
		key[count] = lock[count] ^ lock[count - 1];

	key[len] = 0;
	count = 0;

	while (count++ < len)
		key[count - 1] = ((key[count - 1] << 4)) | ((key[count - 1] >> 4));

	cDCProto::EscapeChars(key, len, fkey, true);
	delete [] key;
	delete [] lock;
	key = NULL;
	lock = NULL;
}

	}; // namespace nProtocol
}; // namespace nVerliHub
