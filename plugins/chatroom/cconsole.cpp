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

#include "src/cconndc.h"
#include "src/i18n.h"
#include "cconsole.h"
#include "cpichatroom.h"
#include "crooms.h"

namespace nVerliHub {
	using namespace nEnums;
	using namespace nSocket;
	namespace nChatRoom {

cRooms *cRoomConsole::GetTheList()
{
	return mOwner->mList;
}

const char *cRoomConsole::CmdSuffix()
{
	return "room";
}

const char *cRoomConsole::CmdPrefix()
{
	return "!";
}

void cRoomConsole::GetHelpForCommand(int cmd, ostream &os)
{
	string help_str;
	switch(cmd)
	{
		case eLC_LST:
		help_str = "!lstroom\r\n" + string(_("Give a list of chatrooms"));
		break;
		case eLC_ADD:
		case eLC_MOD:
		help_str = "!(add|mod)room <nickname>"
			" [-CC <country_codes>]"
			" [-ac <min_auto_class>]"
			" [-AC <max_auto_class>]"
			" [-c <min_class>]"
			" [-t <\"topic\">]";
		break;
		case eLC_DEL:
		help_str = "!delroom <nickname>"; break;
		default: break;
	}
	cDCProto::EscapeChars(help_str,help_str);
	os << help_str;
}

const char * cRoomConsole::GetParamsRegex(int cmd)
{
	switch(cmd)
	{
		case eLC_ADD:
		case eLC_MOD:
			return "^(\\S+)(" // <nick>
			      "( -t(\")?((?(4)[^\"]+?|\\S+))(?(4)\"))?|" // <"topic">
			      "( -CC ?(\\S+))?|"   //[ -CC<country_codes>]
			      "( -c ?(\\d+))?|"
			      "( -ac ?(\\d+))?|"
			      "( -AC ?(\\d+))?|"
			      ")*\\s*$" // the end of message
			      ; break;
		case eLC_DEL: return "(\\S+)"; break;
		default : return ""; break;
	};
}

bool cRoomConsole::ReadDataFromCmd(cfBase *cmd, int id, cRoom &data)
{
	/// regex parts for add command
	enum {aADD_ALL, eADD_NICK, eADD_CHOICE,
		eADD_TOPICp, eADD_QUOTE, eADD_TOPIC,
		eADD_CCp, eADD_CC,
		eADD_MINCp, eADD_MINC,
		eADD_MINACp, eADD_MINAC,
		eADD_MAXACp, eADD_MAXAC
	};

	cmd->GetParStr(eADD_NICK, data.mNick);
	cmd->GetParStr(eADD_TOPIC, data.mTopic);
	cmd->GetParStr(eADD_CC, data.mAutoCC);
	cmd->GetParInt(eADD_MINC, data.mMinClass);
	cmd->GetParInt(eADD_MINAC,data.mAutoClassMin);
	cmd->GetParInt(eADD_MAXAC,data.mAutoClassMax);
	return true;
}

bool cRoomConsole::IsConnAllowed(cConnDC *conn,int cmd)
{
	bool result = true;
	if(!conn || !conn->mpUser)
		return false;
	int UserClass = conn->mpUser->mClass;

	switch(cmd) {
		case eLC_ADD: result = UserClass >= mOwner->mCfg->min_class_add; break;
		case eLC_MOD: result = UserClass >= mOwner->mCfg->min_class_mod; break;
		case eLC_DEL: result = UserClass >= mOwner->mCfg->min_class_del; break;
		case eLC_LST: result = UserClass >= mOwner->mCfg->min_class_lst; break;
	}
	return result;
}

cRoomConsole::~cRoomConsole()
{}

	}; // namespace nChatRoom
}; // namespace nVerliHub
