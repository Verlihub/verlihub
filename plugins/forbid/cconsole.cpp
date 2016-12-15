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

//#include "src/cconndc.h"
#include "cconsole.h"
#include "cpiforbid.h"
#include "cforbidden.h"
#include "src/i18n.h"

namespace nVerliHub {
	namespace nForbidPlugin {

cForbidden *cForbidConsole::GetTheList()
{
	return mOwner->mList;
}

const char *cForbidConsole::CmdSuffix()
{
	return "forbid";
}

const char *cForbidConsole::CmdPrefix()
{
	return "!";
}

void cForbidConsole::GetHelpForCommand(int cmd, ostream &os)
{
	string help_str;
	switch(cmd) {
		case eLC_LST:
		help_str = "!lstforbid\r\n" + string(_("Give a list of forbidden expressions"));
		break;
		case eLC_ADD:
		case eLC_MOD:
		help_str = "!(add|mod)forbid <\"expression\">"
			" [-f <flags>]"
			" [-C <max_class>]"
			" [-r <\"kick_reason\">]"
			"\r\n";
		break;
		case eLC_DEL:
		help_str = "!delforbid <\"expression\">"; break;
		default: break;
	}
	cDCProto::EscapeChars(help_str,help_str);
	os << help_str;
}


const char * cForbidConsole::GetParamsRegex(int cmd)
{
	switch(cmd)
	{
		case eLC_ADD:
		case eLC_MOD: return "^(\")?((?(1)[^\"]+?|\\S+))(?(1)\")("
      "( -f ?(\\d+))?|" //[ -f <flags>]
			"( -C ?(\\d+))?|" // [ -C <max_class>]
			"( -r ?(\")?((?(9)[^\"]+?|\\S+))(?(9)\"))?|" //[ -r <"kick_reason">]
			")*\\s*$";
			//return "^(\\S+)( (\\d)( (\\d+)( ([^\\r\\n]*))?)?)?$"; break;
		case eLC_DEL: return "(.*)"; break;
		default : return ""; break;
	};
}

bool cForbidConsole::ReadDataFromCmd(cfBase *cmd, int id, cForbiddenWorker &data)
{
	enum {eADD_ALL, eADD_PATTERNq, eADD_PATTERN, eADD_PARAMS,
		eADD_MASKp, eADD_MASK,
		eADD_CLASSp, eADD_CLASS,
		eADD_REASONp, eADD_REASONq, eADD_REASON};

	switch(id) {
		case eLC_DEL:
			cmd->GetParUnEscapeStr(eADD_ALL,data.mWord);
			break;
		default:
			if(! cmd->GetParRegex(eADD_PATTERN,data.mWord)) {
				(*cmd->mOS) << _("Sorry the regular expression you provided is not valid.") << "\r\n";
				return false;
			}
			cmd->GetParInt(eADD_MASK, data.mCheckMask);
			cmd->GetParInt(eADD_CLASS, data.mAfClass);
			cmd->GetParStr(eADD_REASON, data.mReason);
	}
	return true;
}

cForbidConsole::~cForbidConsole(){}
	}; // namespace nForbidPlugin
}; // namespace nVerliHub
