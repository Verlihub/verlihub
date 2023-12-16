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

#ifndef CMESSAGEDC_H
#define CMESSAGEDC_H

/**a dc Message incomming or outgoing
  *@author Daniel Muller
  */

#include "cobj.h"
#include "cprotocol.h"
namespace nVerliHub {
	namespace nEnums {
		typedef enum // these constants correspond to sDC_Commands in .cpp file, note: they are ordered by frequency of usage for best performance
		{
			eDC_CONNECTTOME,
			eDC_RCONNECTTOME,
			eDC_SR,
			eDC_SEARCH_PAS,
			eDC_SEARCH,
			eDC_TTHS,
			eDC_TTHS_PAS,
			eDC_MYINFO,
			eDC_EXTJSON,
			eDC_KEY,
			eDC_SUPPORTS,
			eDC_VALIDATENICK,
			eDC_VERSION,
			eDC_GETNICKLIST,
			eDC_MYHUBURL,
			eDC_MYPASS,
			eDC_TO,
			eDCB_BOTINFO,
			eDC_GETINFO,
			eDCO_USERIP,
			eDCO_KICK,
			eDCO_OPFORCEMOVE,
			eDC_MCONNECTTOME,
			eDC_MSEARCH_PAS,
			eDC_MSEARCH,
			eDC_MCTO,
			eDC_QUIT,
			eDCO_BAN,
			eDCO_TBAN,
			eDCO_UNBAN,
			eDCO_GETBANLIST,
			eDCO_WHOIP,
			eDCO_GETTOPIC,
			eDCO_SETTOPIC,
			eDCC_MYIP,
			eDCC_MYNICK,
			eDCC_LOCK,
			eDC_IN,
			eDC_CHAT, // note: must always be before unknown
			eDC_UNKNOWN
		} tDCMsg;

		/** Following constants design the placements of corresponding attribute of every
		 D *C protocol message comming from the client... in the mChunks,
		 these are used in the cDCServer::DC_* functions
		 and also in the cMessageDC::SplitChunks ... they must be corresponding
		 */
		// chunk numbers for simple commands
		enum { eCH_0_ALL }; // eDC_GETNICKLIST
		// chunk numbers for cmd with one param only
		enum { eCH_1_ALL, eCH_1_PARAM }; //KEY VALIDATENICK VERSION MYPASS QUIT KICK UNBAN
		// -----*** chunk numbers for all other ***-----
		// chat
		enum { eCH_CH_ALL, eCH_CH_NICK, eCH_CH_MSG };
		// GetINFO
		enum { eCH_GI_ALL, eCH_GI_OTHER, eCH_GI_NICK };
		// RevConnectToMe   $RevConnectToMe <nick> <remoteNick>
		enum { eCH_RC_ALL, eCH_RC_NICK, eCH_RC_OTHER };
		// private chat : CHMSH is a "<nick> msg" together
		enum { eCH_PM_ALL, eCH_PM_TO, eCH_PM_FROM, eCH_PM_CHMSG, eCH_PM_NICK, eCH_PM_MSG } ;
		// private mainchat message
		enum {eCH_MCTO_ALL, eCH_MCTO_TO, eCH_MCTO_CHMSG, eCH_MCTO_NICK, eCH_MCTO_MSG};
		// MyINFO : INFO is all the rest together
		enum { eCH_MI_ALL, eCH_MI_DEST, eCH_MI_NICK, eCH_MI_INFO, eCH_MI_DESC, eCH_MI_SPEED, eCH_MI_MAIL, eCH_MI_SIZE };
		// IN
		enum { eCH_IN_ALL, eCH_IN_NICK, eCH_IN_DATA };
		// MyIP
		enum { eCH_MYIP_ALL, eCH_MYIP_IP, eCH_MYIP_VERS };
		// ExtJSON
		enum { eCH_EJ_ALL, eCH_EJ_NICK, eCH_EJ_PARS };
		/// connecttome   $ConnectToMe <remoteNick> <senderIp>:<senderPort>
		enum {eCH_CM_ALL, eCH_CM_NICK, eCH_CM_ACTIVE, eCH_CM_IP, eCH_CM_PORT};
		//OpForce move
		enum {eCH_FM_ALL, eCH_FM_NICK, eCH_FM_DEST, eCH_FM_REASON };
		// active search	@dReiska:(query format:  <sizerestricted>?<isminimumsize>?<size>?<datatype>?<searchpattern> --> searchpattern is added here)
		enum {eCH_AS_ALL, eCH_AS_ADDR, eCH_AS_IP, eCH_AS_PORT, eCH_AS_QUERY, eCH_AS_SEARCHLIMITS, eCH_AS_SEARCHPATTERN};
		// passive search	 @dReiska:(query format:  <sizerestricted>?<isminimumsize>?<size>?<datatype>?<searchpattern> --> searchpattern is added here)
		enum {eCH_PS_ALL, eCH_PS_NICK, eCH_PS_QUERY, eCH_PS_SEARCHLIMITS, eCH_PS_SEARCHPATTERN};
		// short active tth search
		enum { eCH_SA_ALL, eCH_SA_TTH, eCH_SA_ADDR, eCH_SA_IP, eCH_SA_PORT };
		// short passive tth search
		enum { eCH_SP_ALL, eCH_SP_TTH, eCH_SP_NICK };
		// search result $SR <resultNick> <filepath>^E<filesize> <freeslots>/<totalslots>^E<hubname> (<hubhost>[:<hubport>])^E<searchingNick>
		enum {eCH_SR_ALL, eCH_SR_FROM, eCH_SR_PATH, eCH_SR_SIZE, eCH_SR_SLOTS, eCH_SR_SL_FR, eCH_SR_SL_TO, eCH_SR_HUBINFO, eCH_SR_TO};
		// $Ban nick$reason
		// $TempBan nick$time$reason
		enum {eCH_NB_ALL, eCH_NB_NICK, eCH_NB_REASON, eCH_NB_TIME};

	};
	namespace nProtocol {
/**
* The Direct Connect Protocol Message parser and container
* provides access to all important parts of the message as std::string
*/

class cMessageDC: public cMessageParser
{
	public:
		cMessageDC();
		virtual ~cMessageDC();
		// parses the string and sets the state variables
		virtual int Parse(); // override
		// splits message to its important parts and stores their info in the chunkset mChunks
		virtual bool SplitChunks(); // override
};

	}; // namespace nProtocol
}; // namespace nVerliHub

#endif
