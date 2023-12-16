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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <cconndc.h>
#include "cconsole.h"
#include "cpiplug.h"
#include "cplugs.h"
#include "src/i18n.h"

namespace nVerliHub {
	namespace nPlugMan {

cPlugs *cPlugConsole::GetTheList()
{
	return mOwner->mList;
}

void cPlugConsole::ListHead(ostream *os)
{
	(*os) << "\r\n\r\n";
	(*os) << " [*] " << autosprintf(_("%s version: %s"), mOwner->Name().c_str(), mOwner->Version().c_str()) << "\r\n";
	(*os) << " [*] " << autosprintf(_("%s executable: %s"), HUB_VERSION_NAME, mOwner->mServer->mExecPath.c_str()) << "\r\n";
	(*os) << " [*] " << autosprintf(_("%s build time: %s"), HUB_VERSION_NAME, cTimePrint(mOwner->mList->mVHTime, 0).AsDate().AsString().c_str()) << "\r\n";
}

const char *cPlugConsole::CmdSuffix()
{
	return "plug";
}

const char *cPlugConsole::CmdPrefix()
{
	return "!";
}

bool cPlugConsole::IsConnAllowed(cConnDC *conn, int cmd)
{
	if (!conn || !conn->mpUser)
		return false;

	switch (cmd) {
		case eLC_ADD:
		case eLC_DEL:
		case eLC_MOD:
		case eLC_ON:
		case eLC_OFF:
		case eLC_RE:
			return conn->mpUser->mClass >= eUC_ADMIN;
		case eLC_LST:
		case eLC_HELP:
			return conn->mpUser->mClass >= eUC_OPERATOR;
		default:
			return false;
	}
}

void cPlugConsole::GetHelp(ostream &os)
{
	os << "https://github.com/verlihub/verlihub/wiki/command-list#plugins"; // todo
}

void cPlugConsole::GetHelpForCommand(int cmd, ostream &os)
{
	string help_str;

	switch (cmd) {
		case eLC_LST:
			help_str = "!lstplug\r\nGive a list of registered plugins";
			break;
		case eLC_ADD:
		case eLC_MOD:
			help_str = "!(add|mod)plug <nick>"
				"[ -p <\"path\">]"
				"[ -d <\"desc\">]"
				"[ -a <autoload>]"
				"\r\n""      register or update a plugin\r\n"
				"     * name - short plugin name\r\n"
				"     * path - a relative or absolute filename of the plugin's binary (it's better to provide absoulute path)\r\n"
				"     * desc - a breif description of what the plugin does\r\n"
				"     * autoload - 1/0 to autoload plugin at startup";

			break;
		case eLC_DEL:
			help_str = "!delplug <ipmin_or_iprange>";
			break;
		default:
			break;
	}

	cDCProto::EscapeChars(help_str,help_str);
	os << help_str;
}

const char* cPlugConsole::GetParamsRegex(int cmd)
{
	switch (cmd) {
		case eLC_ADD:
		case eLC_MOD:
			return "^(\\S+)(" // <nick>
				"( -p ?(\")?((?(4)[^\"]+?|\\S+))(?(4)\"))|" // <"path">
				"( -d ?(\")?((?(7)[^\"]+?|\\S+))(?(7)\"))|" // [ <desc>]
				"( -a ?([01]))|"
				")*\\s*$"; // the end of message
		case eLC_DEL:
		case eLC_ON:
		case eLC_OFF:
		case eLC_RE:
			return "(\\S+)";
		default:
			return "";
	}
}

const char* cPlugConsole::CmdWord(int cmd)
{
	switch (cmd) {
		case eLC_ON:
			return "on";
		case eLC_OFF:
			return "off";
		case eLC_RE:
			return "re";
		default:
			return tPlugConsoleBase::CmdWord(cmd);
	}
}

bool cPlugConsole::ReadDataFromCmd(cfBase *cmd, int id, cPlug &data)
{
	/// regex parts for add command
	enum {aADD_ALL, eADD_NICK, eADD_CHOICE,
		eADD_PATHp, eADD_QUOTE , eADD_PATH,
		eADD_DESCP, eADD_QUOTE2, eADD_DESC,
		eADD_AUTOp, eADD_AUTO};

	cmd->GetParStr(eADD_NICK, data.mNick);
	if ((data.mNick.size() > 10) && (id == eLC_ADD)) {
		*cmd->mOS << _("Plugin name must be max 10 characters long, please provide another one.");
		return false;
	}
	cmd->GetParUnEscapeStr(eADD_PATH, data.mPath);
	if(data.mPath.size() < 1 && (id == eLC_ADD)) {
		*cmd->mOS << _("Please provide a valid path for the plugin.");
		return false;
	}
	cmd->GetParStr(eADD_DESC, data.mDesc);
	cmd->GetParBool(eADD_AUTO, data.mLoadOnStartup);
	return true;
}

void cPlugConsole::AddCommands()
{
	tPlugConsoleBase::AddCommands();
	mCmdOn.Init(eLC_ON, CmdId(eLC_ON), GetParamsRegex(eLC_ON), &mcfOn);
	mCmdOff.Init(eLC_OFF, CmdId(eLC_OFF), GetParamsRegex(eLC_OFF), &mcfOff);
	mCmdRe.Init(eLC_RE, CmdId(eLC_RE), GetParamsRegex(eLC_RE), &mcfRe);
	mCmdr.Add(&mCmdOn);
	mCmdr.Add(&mCmdOff);
	mCmdr.Add(&mCmdRe);
}

bool cPlugConsole::cfOn::operator()()
{
	cPlug Data;

	if (GetConsole() && GetConsole()->ReadDataFromCmd(this, eLC_ON, Data)) {
		cPlug *Plug = GetTheList()->FindData(Data);

		if (Plug) {
			bool res = Plug->Plugin();

			if (!res) // show an error if it fails
				(*mOS) << Plug->mLastError;
			else
				(*mOS) << _("Plugin loaded.");

			return res;
		}

		(*mOS) << autosprintf(_("Plugin not found: %s"), Data.mNick.c_str());
	}

	return false;
}

bool cPlugConsole::cfOff::operator()()
{
	cPlug Data;

	if (GetConsole() && GetConsole()->ReadDataFromCmd(this, eLC_ON, Data)) {
		cPlug *Plug = GetTheList()->FindData(Data);

		if (Plug) {
			if (Plug->Plugout()) {
				(*mOS) << _("Plugin unloaded.");
				return true;
			} else {
				(*mOS) << _("Plugin not unloaded, probably because it's not loaded.");
				return false;
			}
		}

		(*mOS) << autosprintf(_("Plugin not found: %s"), Data.mNick.c_str());
	}

	return false;
}

bool cPlugConsole::cfRe::operator()()
{
	cPlug Data;

	if (GetConsole() && GetConsole()->ReadDataFromCmd(this, eLC_ON, Data)) {
		cPlug *Plug = GetTheList()->FindData(Data);

		if (Plug) {
			if (Plug->Replug()) {
				(*mOS) << _("Plugin reloaded.");
				return true;
			} else {
				(*mOS) << _("Plugin not reloaded, probably because it's not loaded.");
				return false;
			}
		}

		(*mOS) << autosprintf(_("Plugin not found: %s"), Data.mNick.c_str());
	}

	return false;
}

cPlugConsole::~cPlugConsole(){}

	}; // namespace nPlugMan
}; // namespace nPlugMan
