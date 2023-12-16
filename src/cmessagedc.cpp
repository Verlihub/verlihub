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

#include "cmessagedc.h"
#include "cprotocommand.h"
#include <iostream>

using namespace std;

namespace nVerliHub {
	using namespace nEnums;

	namespace nProtocol {

const static cProtoCommand /*cMessageDC::*/sDC_Commands[] = // this list corresponds to tDCMsg enumeration in .h file
{
	cProtoCommand("$ConnectToMe "),
	cProtoCommand("$RevConnectToMe "),
	cProtoCommand("$SR "),
	cProtoCommand("$Search Hub:"), // note: must be before active $Search
	cProtoCommand("$Search "),
	cProtoCommand("$SA "),
	cProtoCommand("$SP "),
	cProtoCommand("$MyINFO "),
	cProtoCommand("$ExtJSON "),
	cProtoCommand("$Key "),
	cProtoCommand("$Supports "),
	cProtoCommand("$ValidateNick "),
	cProtoCommand("$Version "),
	cProtoCommand("$GetNickList"),
	cProtoCommand("$MyHubURL "),
	cProtoCommand("$MyPass "),
	cProtoCommand("$To: "),
	cProtoCommand("$BotINFO "),
	cProtoCommand("$GetINFO "),
	cProtoCommand("$UserIP "),
	cProtoCommand("$Kick "),
	cProtoCommand("$OpForceMove $Who:"),
	cProtoCommand("$MultiConnectToMe "),
	cProtoCommand("$MultiSearch Hub:"),
	cProtoCommand("$MultiSearch "),
	cProtoCommand("$MCTo: "),
	cProtoCommand("$Quit "),
	cProtoCommand("$Ban "),
	cProtoCommand("$TempBan "),
	cProtoCommand("$UnBan "),
	cProtoCommand("$GetBanList"),
	cProtoCommand("$WhoIP "),
	cProtoCommand("$GetTopic"),
	cProtoCommand("$SetTopic "),
	cProtoCommand("$MyIP "),
	cProtoCommand("$MyNick "),
	cProtoCommand("$Lock "),
	cProtoCommand("$IN "),
	cProtoCommand("<") // note: must always be last
};

cMessageDC::cMessageDC():
	cMessageParser(10)
{
	SetClassName("MessageDC");
}

cMessageDC::~cMessageDC()
{}

/*
	parses the string and sets the state variables
		this function call too many comparisons, its to optimize by a tree
		attention: AreYou returns true if the first part matches, some messages may be confused
*/

int cMessageDC::Parse()
{
	if (mStr.empty()) { // ping
		mType = eDC_UNKNOWN;
		mKWSize = 0;
		mLen = 0;
		return eDC_UNKNOWN;
	}

	if (mStr[0] == '<') { // chat
		mType = eDC_CHAT;
		mKWSize = sDC_Commands[eDC_UNKNOWN - 1].mBaseLength;
		mLen = mStr.size();
		return eDC_CHAT;
	}

	for (unsigned int i = 0; i < eDC_CHAT; i++) { // other commands, note: here we dont check for eDC_UNKNOWN and eDC_CHAT anymore
		if (sDC_Commands[i].AreYou(mStr)) {
			mType = tDCMsg(i);
			mKWSize = sDC_Commands[i].mBaseLength;
			mLen = mStr.size();
			break;
		}
	}

	if (mType == eMSG_UNPARSED)
		mType = eDC_UNKNOWN;

	return mType;
}

/*
	splits message into parts and stores them in the chunk set mChunks
*/

bool cMessageDC::SplitChunks()
{
 	SetChunk(0, 0, mStr.length()); // the zeroth chunk is everywhere the same

	switch (mType) { // now try to find chunks one by one
		case eDC_CONNECTTOME:
		case eDC_MCONNECTTOME:
			if (!SplitOnTwo(mKWSize, ' ', eCH_CM_NICK, eCH_CM_ACTIVE))
				mError = true;

			if (!SplitOnTwo(':', eCH_CM_ACTIVE, eCH_CM_IP, eCH_CM_PORT))
				mError = true;

			break;

		case eDC_RCONNECTTOME:
			if (!SplitOnTwo(mKWSize, ' ', eCH_RC_NICK, eCH_RC_OTHER))
				mError = true;

			break;

		case eDC_SR:
			/*
				$SR <senderNick> <filepath>^E<filesize> <freeslots>/<totalslots>^E<hubname> (<hubhost>[:<hubport>])^E<searchingNick>
				$SR <senderNick> <filepath>^E<filesize> <freeslots>/<totalslots>^ETTH:<TTH-ROOT> (<hub_ip>[:<hubport>])^E<searchingNick>
				$SR <senderNick> <filepath>^E<freeslots>/<totalslots>^ETTH:<TTH-ROOT> (<hub_ip>:[hubport]>)^E<searchingNick>
					1)      ----FROM----|-----------------------------------(PATH)--------------------------------------------------------------
					2)                  |-------PATH-----------|----------------------(SLOTS)---------------------------------------------------
					3)                                         |-----------SLOTS---------|-------------(HUBINFO)--------------------------------
					4)                                                                   |-------------(HUBINFO)---------------|----TO----------
					5)
				eCH_SR_FROM, eCH_SR_PATH, eCH_SR_SIZE, eCH_SR_SLOTS, eCH_SR_SL_FR, eCH_SR_SL_TO, eCH_SR_HUBINFO, eCH_SR_TO
			*/

			if (!SplitOnTwo(mKWSize, ' ', eCH_SR_FROM, eCH_SR_PATH))
				mError = true;

			/*
			if (!SplitOnTwo(' ', eCH_SR_PATH, eCH_SR_PATH, eCH_SR_SLOTS))
				mError = true;

			if (!SplitOnTwo(' ', eCH_SR_SLOTS, eCH_SR_SLOTS, eCH_SR_HUBINFO)
				mError = true;

			if (!SplitOnTwo(' ', eCH_SR_HUBINFO, eCH_SR_HUBINFO, eSR_TO)
				mError = true;
			*/

			if (!SplitOnTwo(0x05, eCH_SR_PATH, eCH_SR_PATH, eCH_SR_SIZE))
				mError = true;

			if (!SplitOnTwo(0x05, eCH_SR_SIZE, eCH_SR_HUBINFO, eCH_SR_TO, false))
				mError = true;

			if (SplitOnTwo(0x05, eCH_SR_HUBINFO, eCH_SR_SIZE, eCH_SR_HUBINFO)) {
				if (!SplitOnTwo(' ', eCH_SR_SIZE, eCH_SR_SIZE, eCH_SR_SLOTS))
					mError = true;

				if (!SplitOnTwo('/', eCH_SR_SLOTS, eCH_SR_SL_FR, eCH_SR_SL_TO))
					mError = true;
			} else {
				SetChunk(eCH_SR_SIZE, 0, 0);
			}

			break;

		case eDC_SEARCH_PAS:
		case eDC_MSEARCH_PAS: // not implemented, but should be same as passive search
			/*
				$Search Hub:<nick> <limits><pattern>
				eCH_PS_ALL, eCH_PS_NICK, eCH_PS_QUERY, eCH_PS_SEARCHLIMITS, eCH_PS_SEARCHPATTERN
			*/

			if (!SplitOnTwo(mKWSize, ' ', eCH_PS_NICK, eCH_PS_QUERY))
				mError = true;

			if (!SplitOnTwo('?', eCH_PS_QUERY, eCH_PS_SEARCHLIMITS, eCH_PS_SEARCHPATTERN, false))
				mError = true;

			if (!ChunkIncLeft(eCH_PS_SEARCHLIMITS, 1)) // get back last question mark
				mError = true;

			break;

		case eDC_SEARCH:
		case eDC_MSEARCH: // not implemented, but should be same as active search
			/*
				$Search <ip>:<port> <limits><pattern>
				eCH_AS_ALL, eCH_AS_ADDR, eCH_AS_IP, eCH_AS_PORT, eCH_AS_QUERY, eCH_AS_SEARCHLIMITS, eCH_AS_SEARCHPATTERN
			*/

			if (!SplitOnTwo(mKWSize, ' ', eCH_AS_ADDR, eCH_AS_QUERY))
				mError = true;

			if (!SplitOnTwo(':', eCH_AS_ADDR, eCH_AS_IP, eCH_AS_PORT))
				mError = true;

			if (!SplitOnTwo(' ', eCH_AS_PORT, eCH_AS_PORT, eCH_AS_QUERY))
				mError = true;

			if (!SplitOnTwo('?', eCH_AS_QUERY, eCH_AS_SEARCHLIMITS, eCH_AS_SEARCHPATTERN, false))
				mError = true;

			if (!ChunkIncLeft(eCH_AS_SEARCHLIMITS, 1)) // get back last question mark
				mError = true;

			break;

		case eDC_TTHS:
			/*
				$SA <tth> <ip>:<port>
				eCH_SA_ALL, eCH_SA_TTH, eCH_SA_ADDR, eCH_SA_IP, eCH_SA_PORT
			*/

			if (!SplitOnTwo(mKWSize, ' ', eCH_SA_TTH, eCH_SA_ADDR))
				mError = true;

			if (!SplitOnTwo(':', eCH_SA_ADDR, eCH_SA_IP, eCH_SA_PORT))
				mError = true;

			break;

		case eDC_TTHS_PAS:
			/*
				$SP <tth> <nick>
				eCH_SP_ALL, eCH_SP_TTH, eCH_SP_NICK
			*/

			if (!SplitOnTwo(mKWSize, ' ', eCH_SP_TTH, eCH_SP_NICK))
				mError = true;

			break;

		case eDC_MYINFO:
			/*
				$MyINFO $ALL <nick> <interest>$ $<speed>$<e-mail>$<sharesize>$
				eCH_MI_ALL, eCH_MI_DEST, eCH_MI_NICK, eCH_MI_INFO, eCH_MI_DESC, eCH_MI_SPEED, eCH_MI_MAIL, eCH_MI_SIZE
			*/

			if (!SplitOnTwo(mKWSize, ' ', eCH_MI_DEST, eCH_MI_NICK))
				mError = true;

			if (!SplitOnTwo(' ', eCH_MI_NICK, eCH_MI_NICK, eCH_MI_INFO))
				mError = true;

			if (!SplitOnTwo('$', eCH_MI_INFO, eCH_MI_DESC, eCH_MI_SPEED))
				mError = true;

			if (!ChunkRedLeft(eCH_MI_SPEED, 2))
				mError = true;

			if (!SplitOnTwo('$', eCH_MI_SPEED, eCH_MI_SPEED, eCH_MI_MAIL))
				mError = true;

			if (!SplitOnTwo('$', eCH_MI_MAIL, eCH_MI_MAIL, eCH_MI_SIZE))
				mError = true;

			if (!ChunkRedRight(eCH_MI_SIZE, 1))
				mError = true;

			break;

		case eDC_EXTJSON:
			/*
				$ExtJSON <nick> {parameters with values}
				eCH_EJ_ALL, eCH_EJ_NICK, eCH_EJ_PARS
			*/

			if (!SplitOnTwo(mKWSize, ' ', eCH_EJ_NICK, eCH_EJ_PARS))
				mError = true;

			break;

		case eDC_KEY: // single parameter
		case eDC_SUPPORTS:
		case eDC_VALIDATENICK:
		case eDC_VERSION:
		case eDC_MYHUBURL:
		case eDC_MYPASS:
		case eDCB_BOTINFO:
		case eDCO_USERIP:
		case eDCO_KICK:
		case eDC_QUIT:
		case eDCO_UNBAN:
		case eDCO_WHOIP:
		case eDCO_SETTOPIC:
		case eDCC_MYNICK:
		case eDCC_LOCK:
			if (mLen < mKWSize)
				mError = true;
			else
				SetChunk(eCH_1_PARAM, mKWSize, mLen - mKWSize);

			break;

		case eDCC_MYIP:
			/*
				$MyIP <ip> <version>
				eCH_MYIP_ALL, eCH_MYIP_IP, eCH_MYIP_VERS
			*/

			if (!SplitOnTwo(mKWSize, ' ', eCH_MYIP_IP, eCH_MYIP_VERS))
				mError = true;

			break;

		case eDC_TO:
			/*
				$To: <othernick> From: <mynick> $<<mynick>> <message>
				eCH_PM_ALL, eCH_PM_TO, eCH_PM_FROM, eCH_PM_CHMSG, eCH_PM_NICK, eCH_PM_MSG
			*/

			if (!SplitOnTwo(mKWSize, " From: ", eCH_PM_TO, eCH_PM_FROM))
				mError = true;

			if (!SplitOnTwo(" $<", eCH_PM_FROM, eCH_PM_FROM, eCH_PM_CHMSG))
				mError = true;

			if (!SplitOnTwo("> ", eCH_PM_CHMSG, eCH_PM_NICK, eCH_PM_MSG))
				mError = true;

			break;

		case eDC_CHAT:
			if (!SplitOnTwo(mKWSize, "> ", eCH_CH_NICK, eCH_CH_MSG))
				mError = true;

			break;

		case eDC_GETINFO:
			if (!SplitOnTwo(mKWSize, ' ', eCH_GI_OTHER , eCH_GI_NICK))
				mError = true;

			break;

		case eDCO_OPFORCEMOVE:
			/*
				$OpForceMove $Who:<victimNick>$Where:<newIp>$Msg:<reasonMsg>
				eCH_FM_NICK eCH_FM_DEST eCH_FM_REASON
			*/

			if (!SplitOnTwo(mKWSize, '$', eCH_FM_NICK, eCH_FM_DEST))
				mError = true;

			if (!ChunkRedLeft(eCH_FM_DEST, 6)) // skip the "Where:" part
				mError = true;

			if (!SplitOnTwo('$', eCH_FM_DEST, eCH_FM_DEST, eCH_FM_REASON))
				mError = true;

			if (!ChunkRedLeft(eCH_FM_REASON, 4)) // skip the "Msg:" part
				mError = true;

			break;

		case eDC_MCTO:
			/*
				https://nmdc.sourceforge.io/NMDC.html#_mcto
				$MCTo: <othernick> $<mynick> <message>
				eCH_MCTO_ALL, eCH_MCTO_TO, eCH_MCTO_CHMSG, eCH_MCTO_NICK, eCH_MCTO_MSG
			*/

			if (!SplitOnTwo(mKWSize, " $", eCH_MCTO_TO, eCH_MCTO_CHMSG))
				mError = true;

			if (!SplitOnTwo(' ', eCH_MCTO_CHMSG, eCH_MCTO_NICK, eCH_MCTO_MSG))
				mError = true;

			break;

		case eDCO_BAN:
			if (!SplitOnTwo(mKWSize, '$', eCH_NB_NICK, eCH_NB_REASON))
				mError = true;

			break;

		case eDCO_TBAN:
			if (!SplitOnTwo(mKWSize, '$', eCH_NB_NICK, eCH_NB_TIME))
				mError = true;

			if (!SplitOnTwo('$', eCH_NB_TIME, eCH_NB_TIME, eCH_NB_REASON))
				mError = true;

			break;

		case eDC_IN:
			/*
				$IN <nick>$<data>[$<data>]
				eCH_IN_ALL, eCH_IN_NICK, eCH_IN_DATA
			*/

			if (!SplitOnTwo(mKWSize, '$', eCH_IN_NICK, eCH_IN_DATA))
				mError = true;

			break;

		default:
			break;
	}

	mModified = false; // reset modified flag
	return mError;
}

	}; //namespace nProtocol
}; // namespace nVerliHub
