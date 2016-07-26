/*
	Copyright (C) 2003-2005 Daniel Muller, dan at verliba dot cz
	Copyright (C) 2006-2016 Verlihub Team, info at verlihub dot net

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
#include "cdcconsole.h"
#include "cuser.h"
#include "cusercollection.h"
#include "ckicklist.h"
#include "ckick.h"
#include "cinterpolexp.h"
#include "cban.h"
#include "cbanlist.h"
#include "cpenaltylist.h"
#include "ctime.h"
#include "creglist.h"
#include <sstream>
#include "ccommand.h"
#include "ctriggers.h"
#include "ccustomredirects.h"
#include "config.h"
#include "cdcclients.h"
#include "i18n.h"
#include <algorithm>
#include <sys/resource.h>

namespace nVerliHub {
	using namespace nUtils;
	using namespace nSocket;
	using namespace nEnums;
	using namespace nTables;

cPCRE cDCConsole::mIPRangeRex("^(\\d+\\.\\d+\\.\\d+\\.\\d+)((\\/(\\d+))|(\\.\\.|-)(\\d+\\.\\d+\\.\\d+\\.\\d+))?$",0);

cDCConsole::cDCConsole(cServerDC *s, cMySQL &mysql):
	cDCConsoleBase(s),
	mServer(s),
	mTriggers(NULL),
	mRedirects(NULL),
	mDCClients(NULL),
	mCmdr(this),
	mUserCmdr(this),
 	mCmdBan(int(eCM_BAN), ".(del|rm|un|info|list|ls|lst)?ban([^_\\s]+)?(_(\\d+\\S))?( this (nick|ip))? ?", "(\\S+)?( (.*)$)?", &mFunBan),
	mCmdGag(int(eCM_GAG), ".(un)?(gag|nochat|nopm|nochats|noctm|nodl|nosearch|kvip|maykick|noshare|mayreg|mayopchat|noinfo|mayinfo|temprights) ?", "(\\S+)?( (\\d+\\w))?", &mFunGag),
	mCmdTrigger(int(eCM_TRIGGER),".(ft|trigger)(\\S+) ", "(\\S+) (.*)", &mFunTrigger),
	mCmdSetVar(int(eCM_SET),".(set|=) ", "(\\[(\\S+)\\] )?(\\S+) (.*)", &mFunSetVar),
	mCmdRegUsr(int(eCM_REG),".r(eg)?(n(ew)?(user)?|del(ete)?|pass(wd)?|(en|dis)able|(set)?class|(protect|hidekick)(class)?|set|=|info|list|lst) ", "(\\S+)( (((\\S+) )?(.*)))?", &mFunRegUsr),
	mCmdRaw(int(eCM_RAW),".proto(\\S+)_(\\S+) ","(.*)", &mFunRaw),
	mCmdCmd(int(eCM_CMD),".cmd(\\S+)","(.*)", &mFunCmd),
	mCmdWho(int(eCM_WHO), ".w(ho)?(\\S+) ", "(.*)", &mFunWho),
	mCmdKick(int(eCM_KICK), ".(kick|drop) ", "(\\S+)( (.*)$)?", &mFunKick, eUR_KICK),
	mCmdInfo(int(eCM_INFO), ".(hub|serv|server|sys|system|os|port|url|huburl|prot|proto|protocol)info ?", "(\\S+)?", &mFunInfo),
	mCmdPlug(int(eCM_PLUG), ".plug(in|out|list|reg|call|calls|callback|callbacks|reload) ?", "(\\S+)?( (.*)$)?", &mFunPlug),
	mCmdReport(int(eCM_REPORT),".report ","(\\S+)( (.*)$)?", &mFunReport),
	mCmdBc(int(eCM_BROADCAST),".(bc|broadcast|oc|ops|regs|guests|vips|cheefs|admins|masters)( |\\r\\n)","(.*)$", &mFunBc), // |ccbc|ccbroadcast
	mCmdGetConfig(int(eCM_GETCONFIG), ".(gc|getconfig|getconf|gv|getvar) ?", "(\\S+)?", &mFunGetConfig),
	mCmdClean(int(eCM_CLEAN),".clean(\\S+) ?", "(\\S+)?", &mFunClean),
	mCmdRedirConnType(int(eCM_CONNTYPE),".(\\S+)conntype ?","(.*)$",&mFunRedirConnType),
	mCmdRedirTrigger(int(eCM_TRIGGERS),".(\\S+)trigger ?","(.*)$",&mFunRedirTrigger),
	mCmdCustomRedir(int(eCM_CUSTOMREDIR),".(\\S+)redirect ?","(.*)$",&mFunCustomRedir),
	mCmdDCClient(int(eCM_DCCLIENT),".(\\S+)client ?","(.*)$",&mFunDCClient),
	mConnTypeConsole(this),
	mTriggerConsole(NULL),
	mRedirectConsole(NULL),
	mDCClientConsole(NULL)
{
	mTriggers = new cTriggers(mServer);
	mTriggers->OnStart();
	mTriggerConsole = new cTriggerConsole(this);

	mDCClients = new cDCClients(mServer);
	mDCClients->OnStart();
	mDCClientConsole = new cDCClientConsole(this);

	mRedirects = new cRedirects(mServer);
	mRedirects->OnStart();
	mRedirectConsole = new cRedirectConsole(this);

	mFunRedirConnType.mConsole = &mConnTypeConsole;
	mFunRedirTrigger.mConsole = mTriggerConsole;
	mFunCustomRedir.mConsole = mRedirectConsole;
	mFunDCClient.mConsole = mDCClientConsole;
	mCmdr.Add(&mCmdBan);
	mCmdr.Add(&mCmdGag);
	mCmdr.Add(&mCmdTrigger);
	mCmdr.Add(&mCmdSetVar);
	mCmdr.Add(&mCmdRegUsr);
	mCmdr.Add(&mCmdInfo);
	mFunInfo.mInfoServer.SetServer(mOwner);
	mCmdr.Add(&mCmdRaw);
	mCmdr.Add(&mCmdWho);
	mCmdr.Add(&mCmdKick);
	mCmdr.Add(&mCmdPlug);
	mCmdr.Add(&mCmdCmd);
	mCmdr.Add(&mCmdBc);
	mCmdr.Add(&mCmdRedirConnType);
	mCmdr.Add(&mCmdRedirTrigger);
	mCmdr.Add(&mCmdCustomRedir);
	mCmdr.Add(&mCmdDCClient);
	mCmdr.Add(&mCmdGetConfig);
	mCmdr.Add(&mCmdClean);
	mCmdr.InitAll(this);
	mUserCmdr.Add(&mCmdReport);
	mUserCmdr.InitAll(this);
}

cDCConsole::~cDCConsole(){
	if (mTriggers) delete mTriggers;
	mTriggers = NULL;
	if (mTriggerConsole) delete mTriggerConsole;
	mTriggerConsole = NULL;
	if (mRedirects) delete mRedirects;
	mRedirects = NULL;
	if (mRedirectConsole) delete mRedirectConsole;
	mRedirectConsole = NULL;
	if (mDCClients) delete mDCClients;
	mDCClients = NULL;
	if (mDCClientConsole) delete mDCClientConsole;
	mDCClientConsole = NULL;
}

int cDCConsole::OpCommand(const string &str, cConnDC *conn)
{
	if (!conn || !conn->mpUser)
		return 0;

	istringstream cmd_line(str);
	string cmd;
	ostringstream os;
	cmd_line >> cmd;
	cmd = toLower(cmd);
	string cmdid = cmd.substr(1);

	switch (conn->mpUser->mClass) {
		case eUC_MASTER:
			if (cmdid == "quit")
				return CmdQuit(cmd_line, conn, 0);

			if (cmdid == "restart")
				return CmdQuit(cmd_line, conn, 1);

			if (cmdid == "dbg_hash") {
				mOwner->mUserList.DumpProfile(cerr);
				return 1;
			}

			if (cmdid == "core_dump")
				return CmdQuit(cmd_line, conn, -1);

			if (cmdid == "hublist") {
				mOwner->RegisterInHublist(mOwner->mC.hublist_host, mOwner->mC.hublist_port, conn);
				return 1;
			}
		case eUC_ADMIN:
			if (cmdid == "userlimit" || cmdid == "ul") return CmdUserLimit(cmd_line, conn);
			if (cmdid == "reload" || cmdid == "re") return CmdReload(cmd_line, conn);
		case eUC_CHEEF:
			if (cmdid == "ccbroadcast" || cmdid == "ccbc") return CmdCCBroadcast(cmd_line, conn, eUC_NORMUSER, eUC_MASTER);
			if (cmdid == "class") return CmdClass(cmd_line, conn);
			if (cmdid == "protect") return CmdProtect(cmd_line, conn);
		case eUC_OPERATOR:
			if (cmdid == "topic" || cmdid == "hubtopic" ) return CmdTopic(cmd_line, conn);
			if (cmdid == "getip" || cmdid == "gi") return CmdGetip(cmd_line, conn);
			if (cmdid == "gethost" || cmdid == "gh") return CmdGethost(cmd_line, conn);
			if (cmdid == "getinfo" || cmdid == "gn") return CmdGetinfo(cmd_line, conn);
			if (cmdid == "help" || cmdid == "?") return CmdHelp(cmd_line, conn);
			if (cmdid == "hideme" || cmdid == "hm") return CmdHideMe(cmd_line, conn);
			if (cmdid == "hidekick" || cmdid == "hk") return CmdHideKick(cmd_line, conn);
			if (cmdid == "unhidekick" || cmdid == "uhk") return CmdUnHideKick(cmd_line, conn);
			if (cmdid == "commands" || cmdid == "cmds") return CmdCmds(cmd_line, conn);

			try {
				if (mCmdr.ParseAll(str, os, conn) >= 0) {
					mOwner->DCPublicHS(os.str().c_str(), conn);
					return 1;
				}
			} catch (const char *ex) {
				if(Log(0)) LogStream() << "Exception in command: " << ex << endl;
			} catch (...) {
				if(Log(0)) LogStream() << "Exception in command." << endl;
			}
			break;
		default:
			return 0;
			break;
	}

	if (mTriggers->DoCommand(conn, cmd, cmd_line, *mOwner))
		return 1;

	return 0;
}

int cDCConsole::UsrCommand(const string &str, cConnDC * conn)
{
	if (!conn || !conn->mpUser) return 0;
	istringstream cmd_line(str);
	ostringstream os;
	string cmd;

	if (mOwner->mC.disable_usr_cmds) {
		mOwner->DCPublicHS(_("User commands are currently disabled."), conn);
		return 1;
	}

	cmd_line >> cmd;
	cmd = toLower(cmd);
	string cmdid = cmd.substr (1);

	switch (conn->mpUser->mClass) {
		case eUC_MASTER:
		case eUC_ADMIN:
		case eUC_CHEEF:
		case eUC_OPERATOR:
		case eUC_VIPUSER:
		case eUC_REGUSER:
			if (cmdid == "kick") return CmdKick(cmd_line, conn);
		case eUC_NORMUSER:
			if (cmdid == "passwd" || cmdid == "password") return CmdRegMyPasswd(cmd_line, conn);
			if (cmdid == "help") return CmdHelp(cmd_line, conn);
			if (cmdid == "myinfo") return CmdMyInfo(cmd_line, conn);
			if (cmdid == "myip") return CmdMyIp(cmd_line, conn);

			if (cmdid == "me")
				return CmdMe(cmd_line, conn);

			if (cmdid == "regme") return CmdRegMe(cmd_line, conn);
			if (cmdid == "chat") return CmdChat(cmd_line, conn, true);
			if (cmdid == "nochat") return CmdChat(cmd_line, conn, false);
			if (cmdid == "info") return CmdUInfo(cmd_line, conn);
			if (cmdid == "release" || cmdid == "verlihub" || cmdid == "vh") return CmdRInfo(cmd_line, conn);

			if (mUserCmdr.ParseAll(str, os, conn) >= 0) {
				mOwner->DCPublicHS(os.str().c_str(), conn);
				return 1;
			}

			break;
		case eUC_PINGER:
			if ((cmdid == "report") && (mUserCmdr.ParseAll(str, os, conn) >= 0)) {
				mOwner->DCPublicHS(os.str().c_str(), conn);
				return 1;
			}

			break;
		default:
			return 0;
			break;
	}

	if (mTriggers->DoCommand(conn, cmd, cmd_line, *mOwner))
		return 1;

	return 0;
}

int cDCConsole::CmdCmds(istringstream &cmd_line, cConnDC *conn)
{
	ostringstream os;
	string omsg;
	os << _("Full list of commands") << ":";
	mCmdr.List(&os);
	mOwner->mP.EscapeChars(os.str(), omsg);
	mOwner->DCPublicHS(omsg.c_str(), conn);
	return 1;
}

int cDCConsole::CmdGetip(istringstream &cmd_line, cConnDC *conn)
{
	ostringstream os, o;
	string s;
	cUser *user;
	int c = 0;

	while (cmd_line.good()) {
		cmd_line >> s;
		if (cmd_line.fail()) break;
		user = mOwner->mUserList.GetUserByNick(s);

		if (user && user->mxConn) {
			os << "\r\n [*] " << _("User") << ": " << user->mNick.c_str();
			os << " ][ " << _("IP") << ": " << user->mxConn->AddrIP().c_str();
		} else
		    os << "\r\n [*] " << autosprintf(_("User not found: %s"), s.c_str());

		c++;
	}

	if (c == 0)
		o << _("Please specify one or more nicks separated by space.");
	else
		o << autosprintf(ngettext("Showing %d result", "Showing %d results", c), c) << ":" << os.str();

	mOwner->DCPublicHS(o.str().c_str(), conn);
	return 1;
}

int cDCConsole::CmdGethost(istringstream &cmd_line, cConnDC *conn)
{
	ostringstream os, o;
	string s;
	cUser *user;
	int c = 0;

	while (cmd_line.good()) {
		cmd_line >> s;
		if (cmd_line.fail()) break;
		user = mOwner->mUserList.GetUserByNick(s);

		if (user && user->mxConn) {
			if (!mOwner->mUseDNS) user->mxConn->DNSLookup();

			if (!user->mxConn->AddrHost().empty()) {
				os << "\r\n [*] " << _("User") << ": " << user->mNick.c_str();
				os << " ][ " << _("Host") << ": " << user->mxConn->AddrHost().c_str();
			} else
				os << "\r\n [*] " << autosprintf(_("Unable to resolve hostname of user: %s"), user->mNick.c_str());
		} else
			os << "\r\n [*] " << autosprintf(_("User not found: %s"), s.c_str());

		c++;
	}

	if (c == 0)
		o << _("Please specify one or more nicks separated by space.");
	else
		o << autosprintf(ngettext("Showing %d result", "Showing %d results", c), c) << ":" << os.str();

	mOwner->DCPublicHS(o.str().c_str(), conn);
	return 1;
}

int cDCConsole::CmdGetinfo(istringstream &cmd_line, cConnDC *conn)
{
	ostringstream os, o;
	string s;
	cUser *user;
	int c = 0;

	while (cmd_line.good()) {
		cmd_line >> s;
		if (cmd_line.fail()) break;
		user = mOwner->mUserList.GetUserByNick(s);

		if (user && user->mxConn) {
			if (!mOwner->mUseDNS) user->mxConn->DNSLookup();
			os << "\r\n [*] " << _("User") << ": " << user->mNick.c_str();
			os << " ][ " << _("IP") << ": " << user->mxConn->AddrIP().c_str();
			os << " ][ " << _("Country") << ": " << user->mxConn->mCC.c_str() << "=" << user->mxConn->mCN.c_str();
			os << " ][ " << _("City") << ": " << user->mxConn->mCity.c_str();
			if (!user->mxConn->AddrHost().empty()) os << " ][ " << _("Host") << ": " << user->mxConn->AddrHost().c_str();
		} else
			os << "\r\n [*] " << autosprintf(_("User not found: %s"), s.c_str());

		c++;
	}

	if (c == 0)
		o << _("Please specify one or more nicks separated by space.");
	else
		o << autosprintf(ngettext("Showing %d result", "Showing %d results", c), c) << ":" << os.str();

	mOwner->DCPublicHS(o.str().c_str(), conn);
	return 1;
}

int cDCConsole::CmdQuit(istringstream &cmd_line, cConnDC *conn, int code)
{
	int delay = 0;

	if (cmd_line.good()) {
		string delay_str;
		getline(cmd_line, delay_str);

		if (delay_str.size()) {
			if (delay_str[0] == ' ') // small fix
				delay_str = delay_str.substr(1);

			if (delay_str.size()) {
				ostringstream conv_err;
				delay = mOwner->Str2Period(delay_str, conv_err);

				if (!delay) {
					if (delay_str == "-1")
						delay = -1;
					else
						mOwner->DCPublicHS(conv_err.str(), conn);
				}
			}
		}
	}

	if (conn->Log(1))
		conn->LogStream() << "Receiving quit command with code " << code << " and delay " << delay << endl;

	ostringstream os;

	if (code == 1) { // restart
		if (delay == -1)
			os << _("Please note, hub is no longer scheduled for restart.");
		else if (delay)
			os << autosprintf (_("Please note, hub has been scheduled to restart in: %s"), cTime((long)delay, 0).AsPeriod().AsString().c_str());
		else
			os << _("Please note, hub will be restarted now.");
	} else { // quit
		if (delay == -1)
			os << _("Please note, hub is no longer scheduled for stop.");
		else if (delay)
			os << autosprintf (_("Please note, hub has been scheduled to stop in: %s"), cTime((long)delay, 0).AsPeriod().AsString().c_str());
		else
			os << _("Please note, hub will be stopped now.");
	}

	mOwner->DCPublicHSToAll(os.str(), (delay ? mOwner->mC.delayed_chat : false));

	if (delay == -1)
		code = 0;

	if (code >= 0) {
		mOwner->stop(code, delay);
	} else {
		cServerDC::mStackTrace = false;
		*(int*)1 = 0;
	}

	return 1;
}

int cDCConsole::CmdHelp(istringstream &, cConnDC * conn)
{
	if(!conn || !conn->mpUser)
		return 1;

	mTriggers->TriggerAll(eTF_HELP, conn);
	return 1;
}

int cDCConsole::CmdCCBroadcast(istringstream &cmd_line, cConnDC *conn, int cl_min, int cl_max)
{
	string start, end, str, cc_zone;
	ostringstream ostr;
	string tmpline;

	// test for existence of parameter
	cmd_line >> cc_zone;
	getline(cmd_line, str);

	while (cmd_line.good()) {
		tmpline = "";
		getline(cmd_line, tmpline);
		str += "\r\n" + tmpline;
	}

	if (!str.size()) {
		ostr << _("Usage example: !ccbc :US:GB: <message>");
		mOwner->DCPublicHS(ostr.str(), conn);
		return 1;
	}

	cc_zone = toUpper(cc_zone);
	mOwner->mP.Create_PMForBroadcast(start, end, mOwner->mC.hub_security, mOwner->mC.hub_security, str); // conn->mpUser->mNick
	cTime TimeBefore, TimeAfter;

	if (mOwner->LastBCNick != "disable")
		mOwner->LastBCNick = conn->mpUser->mNick;

	unsigned int count = mOwner->SendToAllWithNickCCVars(start, end, cl_min, cl_max, cc_zone);
	TimeAfter.Get();
	ostr << autosprintf(ngettext("Message delivered to %d user in zones %s in %s.", "Message delivered to %d users in zones %s in %s.", count), count, cc_zone.c_str(), (TimeAfter - TimeBefore).AsPeriod().AsString().c_str());
	mOwner->DCPublicHS(ostr.str(), conn);
	return 1;
}

int cDCConsole::CmdMyInfo(istringstream & cmd_line, cConnDC * conn)
{
	ostringstream os;
	string omsg;
	os << _("Your information") << ":\r\n";
	conn->mpUser->DisplayInfo(os, eUC_OPERATOR);
	os << "\r\n";
	conn->mpUser->DisplayRightsInfo(os);
	omsg = os.str();
	mOwner->DCPublicHS(omsg, conn);
	return 1;
}

int cDCConsole::CmdMyIp(istringstream &cmd_line, cConnDC *conn)
{
	ostringstream os;
	string omsg;
	os << _("Your IP information") << ":\r\n\r\n";
	os << " [*] " << _("IP") << ": " << conn->AddrIP().c_str() << "\r\n";
	os << " [*] " << _("Country") << ": " << conn->mCC.c_str() << "=" << conn->mCN.c_str() << "\r\n";
	os << " [*] " << _("City") << ": " << conn->mCity.c_str() << "\r\n";
	omsg = os.str();
	mOwner->DCPublicHS(omsg, conn);
	return 1;
}

int cDCConsole::CmdMe(istringstream &cmd_line, cConnDC *conn)
{
	if (!conn || !conn->mpUser)
		return 0;

	if (mOwner->mC.disable_me_cmd) { // check if command is disabled
		mOwner->DCPublicHS(_("This command is currently disabled."), conn);
		return 1;
	}

	if (!conn->mpUser->Can(eUR_CHAT, mOwner->mTime.Sec(), 0)) // check if user is allowed to use main chat
		return 1;

	if (conn->mpUser->mClass < mOwner->mC.mainchat_class) {
		mOwner->DCPublicHS(_("Main chat is currently disabled for users with your class."), conn);
		return 1;
	}

	string text, temp; // prepare text
	getline(cmd_line, text);

	while (cmd_line.good()) {
		temp = "";
		getline(cmd_line, temp);
		text += "\r\n" + temp;
	}

	if (text.size() && (text[0] == ' ')) // small fix for getline
		text = text.substr(1);

	temp.clear(); // check for flood as if it was regular mainchat message
	temp.append("<");
	temp.append(conn->mpUser->mNick);
	temp.append("> ");
	temp.append(text);
	cUser::tFloodHashType Hash = 0;
	Hash = tHashArray<void*>::HashString(temp);

	if (Hash && (conn->mpUser->mClass < eUC_OPERATOR) && (Hash == conn->mpUser->mFloodHashes[eFH_CHAT])) {
		mOwner->DCPublicHS(_("Your message wasn't sent because it equals your previous message."), conn);
		return 1;
	}

	conn->mpUser->mFloodHashes[eFH_CHAT] = Hash;

	if ((conn->mpUser->mClass < eUC_VIPUSER) && !cDCProto::CheckChatMsg(text, conn)) // check message length and other conditions
		return 1;

	temp.clear(); // create and send message
	temp.append("** ");
	temp.append(conn->mpUser->mNick);
	temp.append(" ");
	temp.append(text);
	mOwner->mUserList.SendToAll(temp, mOwner->mC.delayed_chat, true);
	return 1;
}

int cDCConsole::CmdChat(istringstream &cmd_line, cConnDC *conn, bool switchon)
{
	if (!conn) return 0;
	if (!conn->mpUser) return 0;

	if (switchon) { // chat
		if (!mOwner->mChatUsers.ContainsNick(conn->mpUser->mNick)) {
			mOwner->DCPublicHS(_("Public chat messages are now visible. To hide them, write: +nochat"), conn);
			mOwner->mChatUsers.Add(conn->mpUser);
		} else
			mOwner->DCPublicHS(_("Public chat messages are already visible. To hide them, write: +nochat"), conn);
	} else { // nochat
		if (mOwner->mChatUsers.ContainsNick(conn->mpUser->mNick)) {
			mOwner->DCPublicHS(_("Public chat messages are now hidden. To show them, write: +chat"), conn);
			mOwner->mChatUsers.Remove(conn->mpUser);
		} else
			mOwner->DCPublicHS(_("Public chat messages are already hidden. To show them, write: +chat"), conn);
	}

	return 1;
}

int cDCConsole::CmdRInfo(istringstream &cmd_line, cConnDC *conn)
{
	if (!conn || !conn->mpUser)
		return 0;

	ostringstream os;

	os << autosprintf(_("Software: %s %s"), HUB_VERSION_NAME, HUB_VERSION_VERS) << "\r\n\r\n";
	os << " " << _("Authors") << "\r\n\r\n";
	os << "\tVerliba, Daniel Muller, dan at verliba dot cz\r\n";
	os << "\tRoLex, Alexander Zenkov, webmaster at feardc dot net\r\n";
	os << "\tShurik, Alexandr Zeinalov, shurik at sbin dot ru\r\n";
	os << "\tVovochka, Vladimir Perepechin, vovochka13 at gmail dot com\r\n";
	os << "\t" << _("Not forgetting other people who didn't want to be listed here.") << "\r\n\r\n";
	os << " " << _("Translators") << "\r\n\r\n";
	os << "\tCzech (Uhlik), Italian (netcelli, Stefano, DiegoZ), Russian (plugman, MaxFox, RoLex, KCAHDEP),\r\n";
	os << "\tSlovak (uNix), Romanian (WiZaRd, S0RiN), Polish (Zorro, Krzychu), German (Ettore Atalan),\r\n";
	os << "\tSwedish (RoLex), Bulgarian (Boris Stefanov), Hungarian (Oszkar Ocsenas), Turkish (mauron),\r\n";
	os << "\tFrench (@tlantide), Dutch (Modswat)\r\n\r\n";
	os << " " << _("Contributors") << "\r\n\r\n";
	os << "\tFrog, Eco-Logical, Intruder" << "\r\n\r\n";
	os << " " << _("Credits") << "\r\n\r\n";
	os << "\t" << _("We would like to thank everyone in our support hub for their input and valuable support and of course everyone who continues to use this great hub software.") << "\r\n\r\n";
	os << " " << _("More") << "\r\n\r\n";
	os << "\t" << _("Website") << ": https://github.com/verlihub/" << "\r\n";
	os << "\t" << _("Manual") << ": https://github.com/verlihub/verlihub/wiki/" << "\r\n";
	os << "\t" << _("Support hub") << ": dchub://hub.verlihub.net:7777/\r\n";

	mOwner->DCPublicHS(os.str(), conn);
	return 1;
}

int cDCConsole::CmdUInfo(istringstream &cmd_line, cConnDC *conn)
{
	if (!conn || !conn->mpUser)
		return 0;

	ostringstream os;
	int ucl = conn->GetTheoricalClass();
	unsigned int sear = 0;
	os << _("Hub information") << ":\r\n\r\n";

	os << " [*] " << autosprintf(_("Owner: %s"), (mOwner->mC.hub_owner.size() ? mOwner->mC.hub_owner.c_str() : _("Not set"))) << "\r\n";
	os << " [*] " << autosprintf(_("Address: %s"), mOwner->mC.hub_host.c_str()) << "\r\n";
	os << " [*] " << autosprintf(_("Status: %s"), mOwner->SysLoadName()) << "\r\n";
	os << " [*] " << autosprintf(_("Users: %d"), mOwner->mUserCountTot) << "\r\n";
	os << " [*] " << autosprintf(_("Bots: %d"), mOwner->mRobotList.Size()) << "\r\n";
	os << " [*] " << autosprintf(_("Share: %s"), convertByte(mOwner->mTotalShare).c_str()) << "\r\n\r\n";

	os << " " << _("Your information") << ":\r\n\r\n";

	os << " [*] " << autosprintf(_("Class: %d [%s]"), ucl, mOwner->UserClassName(nEnums::tUserCl(ucl))) << "\r\n";

	if (ucl == eUC_NORMUSER) {
		if (conn->mpUser->IsPassive)
			sear = mOwner->mC.int_search_pas;
		else
			sear = mOwner->mC.int_search;
	} else if (ucl == eUC_REGUSER) {
		if (conn->mpUser->IsPassive)
			sear = mOwner->mC.int_search_reg_pas;
		else
			sear = mOwner->mC.int_search_reg;
	} else if (ucl == eUC_VIPUSER) {
		sear = mOwner->mC.int_search_vip;
	} else if (ucl == eUC_OPERATOR) {
		sear = mOwner->mC.int_search_op;
	} else if (ucl == eUC_CHEEF) {
		sear = mOwner->mC.int_search_op;
	} else if (ucl == eUC_ADMIN) {
		sear = mOwner->mC.int_search_op;
	} else if (ucl == eUC_MASTER) {
		sear = mOwner->mC.int_search_op;
	}

	os << " [*] " << autosprintf(ngettext("Search interval: %d second", "Search interval: %d seconds", sear), sear) << "\r\n";
	os << " [*] " << autosprintf(_("Mode: %s"), (conn->mpUser->IsPassive ? _("Passive") : _("Active"))) << "\r\n";
	os << " [*] " << autosprintf(_("Share: %s"), convertByte(conn->mpUser->mShare).c_str()) << "\r\n";
	os << " [*] " << autosprintf(_("Hidden: %s"), (conn->mpUser->mHideShare ? _("Yes") : _("No"))) << "\r\n";
	mOwner->DCPublicHS(os.str(), conn);
	return 1;
}

int cDCConsole::CmdRegMe(istringstream & cmd_line, cConnDC * conn)
{
	ostringstream os;
	string regnick, prefix;
	if (mOwner->mC.disable_regme_cmd) {
		mOwner->DCPublicHS(_("This functionality is currently disabled."), conn);
		return 1;
	}
	if(mOwner->mC.autoreg_class > 3) {
		mOwner->DCPublicHS(_("Registration failed. Please contact an operator for help."), conn);
		return 1;
	}
	__int64 user_share, min_share;

	if(mOwner->mC.autoreg_class >= 0) {
		if(!conn->mpUser) {
			return 0;
		}

		// reg user's online nick
		regnick = conn->mpUser->mNick;
		prefix= mOwner->mC.nick_prefix_autoreg;
		ReplaceVarInString(prefix,"CC",prefix, conn->mCC);

		if( prefix.size() && StrCompare(regnick,0,prefix.size(),prefix) !=0 ) {
			os << autosprintf(_("Your nick must start with: %s"), prefix.c_str());
			mOwner->DCPublicHS(os.str(),conn);
			return 1;
		}

		user_share = conn->mpUser->mShare / (1024*1024);
		min_share = mOwner->mC.min_share_reg;
		if(mOwner->mC.autoreg_class == 2)
			min_share = mOwner->mC.min_share_vip;
		if(mOwner->mC.autoreg_class >= 3)
			min_share = mOwner->mC.min_share_ops;

		if(user_share < min_share) {
			os << autosprintf(_("You need to share at least %s."), convertByte(min_share*1024, false).c_str());
			mOwner->DCPublicHS(os.str(),conn);
			return 1;
		}

		cUser *user = mServer->mUserList.GetUserByNick(regnick);
		cRegUserInfo ui;
		bool RegFound = mOwner->mR->FindRegInfo(ui, regnick);

		if (RegFound) {
			os << _("You are already registered.");
			mOwner->DCPublicHS(os.str(),conn);
			return 1;
		}

		if (user && user->mxConn) {
			string text;
			getline(cmd_line, text);
			if (!text.empty()) text = text.substr(1); // strip space

			if (text.size() < mOwner->mC.password_min_len) {
				os << autosprintf(_("Minimum password length is %d characters. Please retry."), mOwner->mC.password_min_len);
				mOwner->DCPublicHS(os.str(), conn);
				return 1;
			}

			#ifndef WITHOUT_PLUGINS
				/*
					plugin should compare both nicks to see if user is registering himself which equals automatic registration
					plugin should also send message back to user if action is discarded because hub will not send anything
				*/
				if (!mOwner->mCallBacks.mOnNewReg.CallAll(conn->mpUser, regnick, mOwner->mC.autoreg_class))
					return 1;
			#endif

			if (mOwner->mR->AddRegUser(regnick, NULL, mOwner->mC.autoreg_class, text.c_str())) {
				os << autosprintf(_("A new user has been registered with class %d"), mOwner->mC.autoreg_class);
				mOwner->ReportUserToOpchat(conn, os.str(), false);
				os.str(mOwner->mEmpty);
				os << autosprintf(_("You are now registered with nick %s. Please reconnect and login with your new password: %s"), regnick.c_str(), text.c_str());
			} else {
				os << _("An error occured while registering.");
				mOwner->DCPublicHS(os.str(), conn);
				return false;
			}
		}

		mOwner->DCPublicHS(os.str(), conn);
		return 1;
	} else {
		string text;
		getline(cmd_line, text);
		if (!text.empty()) text = text.substr(1); // strip space

		if (!text.empty())
			os << autosprintf(_("Registration request with password: %s"), text.c_str());
		else
			os << _("Registration request without password");

		mOwner->ReportUserToOpchat(conn, os.str(), mOwner->mC.dest_regme_chat);
		mOwner->DCPublicHS(_("Thank you, your request has been sent to operators."), conn);
		return 1;
	}
}

int cDCConsole::CmdTopic(istringstream &cmd_line, cConnDC *conn)
{
	if (!conn || !conn->mpUser)
		return 0;

	if (conn->mpUser->mClass < mOwner->mC.topic_mod_class) {
		mOwner->DCPublicHS(_("You have no rights to change topic."), conn);
		return 1;
	}

	string topic;
	getline(cmd_line, topic);
	ostringstream os;

	if (topic.size() && (topic[0] == ' '))
		topic = topic.substr(1);

	if (topic.size() > 255) {
		os << autosprintf(_("Topic must not exceed 255 characters, but your topic has %d characters."), int(topic.size()));
		mOwner->DCPublicHS(os.str(), conn);
		return 1;
	}

	string file(mOwner->mDBConf.config_name), var("hub_topic"), val_new, val_old;
	int res = mOwner->SetConfig(file.c_str(), var.c_str(), topic.c_str(), val_new, val_old, conn->mpUser);

	if (res == 0) {
		mOwner->DCPublicHS(_("Your action was blocked by a plugin."), conn);
		return 1;
	}

	if (res == 1) {
		string omsg;
		cDCProto::Create_HubName(omsg, mOwner->mC.hub_name, topic);
		mOwner->SendToAll(omsg, eUC_NORMUSER, eUC_MASTER);

		if (topic.size())
			os << autosprintf(_("%s changed the topic to: %s"), conn->mpUser->mNick.c_str(), topic.c_str());
		else
			os << autosprintf(_("%s removed the topic."), conn->mpUser->mNick.c_str());

		mOwner->DCPublicHSToAll(os.str(), mOwner->mC.delayed_chat);
		return 1;
	}

	return 0;
}

int cDCConsole::CmdKick(istringstream &cmd_line, cConnDC *conn)
{
	if (!conn || !conn->mpUser)
		return 0;

	if (!conn->mpUser->Can(eUR_KICK, mOwner->mTime.Sec())) // todo: notify user about missing kick right
		return 1;

	ostringstream os;
	string temp, nick, why;
	cmd_line >> nick;
	getline(cmd_line, why);

	while (cmd_line.good()) {
		temp.clear();
		getline(cmd_line, temp);
		why += "\r\n" + temp;
	}

	if (why.size() && (why[0] == ' '))
		why = why.substr(1);

	mOwner->DCKickNick(&os, conn->mpUser, nick, why, (eKI_CLOSE | eKI_WHY | eKI_PM | eKI_BAN));

	if (os.str().size()) {
		temp = os.str();
		mOwner->DCPublicHS(temp, conn);
	}

	return 1;
}

int cDCConsole::CmdRegMyPasswd(istringstream & cmd_line, cConnDC * conn)
{
	string str;
	int crypt = 0;
	ostringstream ostr;
	cRegUserInfo ui;

	if(!mOwner->mR->FindRegInfo(ui,conn->mpUser->mNick))
		return 0;

	if(!ui.mPwdChange) {
		ostr << _("You are not allowed to change your password now. Ask an operator for help.");
		mOwner->DCPrivateHS(ostr.str(),conn);
		mOwner->DCPublicHS(ostr.str(),conn);
		return 1;
	}

	cmd_line >> str >> crypt;
	if(str.size() < mOwner->mC.password_min_len) {
		ostr << autosprintf(_("Minimum password length is %d characters. Please retry."), mOwner->mC.password_min_len);
		mOwner->DCPrivateHS(ostr.str(),conn);
		mOwner->DCPublicHS(ostr.str(),conn);
		return 1;
	}
	if(!mOwner->mR->ChangePwd(conn->mpUser->mNick, str,crypt)) {
		ostr << _("Error updating password.");
		mOwner->DCPrivateHS(ostr.str(),conn);
		mOwner->DCPublicHS(ostr.str(),conn);
		return 1;
	}

	ostr << _("Password updated successfully.");
	mOwner->DCPrivateHS(ostr.str(),conn);
	mOwner->DCPublicHS(ostr.str(),conn);
	conn->ClearTimeOut(eTO_SETPASS);
	return 1;
}

int cDCConsole::CmdHideMe(istringstream & cmd_line, cConnDC * conn)
{
	int cls = -1;
	cmd_line >> cls;
	ostringstream omsg;
	if(cls < 0) {
		omsg << _("Please use: !hideme <class>, where <class> is the maximum class of users that may not see your kicks.") << endl;
		mOwner->DCPublicHS(omsg.str(),conn);
		return 1;
	}
	if(cls > conn->mpUser->mClass) cls = conn->mpUser->mClass;
	conn->mpUser->mHideKicksForClass = cls;
	omsg << autosprintf(_("Your kicks are now hidden from users with class %d and below."),  cls);
	mOwner->DCPublicHS(omsg.str(),conn);
	return 1;
}

int cDCConsole::CmdUserLimit(istringstream &cmd_line, cConnDC *conn)
{
	ostringstream ostr;
	int minutes = 60, maximum = -1;
	cmd_line >> maximum >> minutes;

	if (maximum < 0) {
		ostr << _("Command usage: !userlimit <max_users> [<minutes>=60]");
		mOwner->DCPublicHS(ostr.str(), conn);
		return 1;
	}

	cInterpolExp *fn = new cInterpolExp(mOwner->mC.max_users_total, maximum, (60 * minutes) / mOwner->timer_serv_period, (6 * minutes) / mOwner->timer_serv_period); // 60 steps at most

	if (fn) {
		mOwner->mTmpFunc.push_back((cTempFunctionBase*)fn);
		ostr << autosprintf(ngettext("Updating max_users variable to %d for the duration of %d minute.", "Updating max_users variable to %d for the duration of %d minutes.", minutes), maximum, minutes);
	} else {
		ostr << autosprintf(ngettext("Failed to update max_users variable to %d for the duration of %d minute.", "Failed to update max_users variable to %d for the duration of %d minutes.", minutes), maximum, minutes);
	}

	mOwner->DCPublicHS(ostr.str(), conn);
	return 1;
}

/*
	!class <nick> <new_class_0_to_5>
*/

int cDCConsole::CmdClass(istringstream &cmd_line, cConnDC *conn)
{
	ostringstream os;
	string nick;
	int new_class = 3, old_class = 0, op_class = conn->mpUser->mClass;
	int max_allowed_class = (op_class > 5 ? 5 : op_class - 1);
	cmd_line >> nick >> new_class;

	if (nick.empty() || (new_class < 0) || (new_class > max_allowed_class)) {
		os << _("Command usage: !class <nick> [class=3]") << " (" << autosprintf(_("maximum allowed class is %d"), max_allowed_class) << ")";
		mOwner->DCPublicHS(os.str().c_str(), conn);
		return 1;
	}

	cUser *user = mOwner->mUserList.GetUserByNick(nick);

	if (user && user->mxConn) {
		old_class = user->mClass;

		if (old_class < op_class) {
			os << autosprintf(_("Temporarily changing class from %d to %d for user: %s"), old_class, new_class, nick.c_str());
			user->mClass = (tUserCl)new_class;

			if ((old_class < mOwner->mC.opchat_class) && (new_class >= mOwner->mC.opchat_class)) { // opchat list
				if (!mOwner->mOpchatList.ContainsNick(user->mNick))
					mOwner->mOpchatList.Add(user);
			} else if ((old_class >= mOwner->mC.opchat_class) && (new_class < mOwner->mC.opchat_class)) {
				if (mOwner->mOpchatList.ContainsNick(user->mNick))
					mOwner->mOpchatList.Remove(user);
			}

			string msg;

			if ((old_class < eUC_OPERATOR) && (new_class >= eUC_OPERATOR)) { // oplist
				if (!(user->mxConn && user->mxConn->mRegInfo && user->mxConn->mRegInfo->mHideKeys)) {
					mOwner->mP.Create_OpList(msg, user->mNick); // send short oplist
					mOwner->mUserList.SendToAll(msg, false, true); // todo: no cache, why?
					mOwner->mInProgresUsers.SendToAll(msg, false, true);

					if (!mOwner->mOpList.ContainsNick(user->mNick))
						mOwner->mOpList.Add(user);
				}
			} else if ((old_class >= eUC_OPERATOR) && (new_class < eUC_OPERATOR)) {
				if (!(user->mxConn && user->mxConn->mRegInfo && user->mxConn->mRegInfo->mHideKeys)) {
					if (mOwner->mOpList.ContainsNick(user->mNick))
						mOwner->mOpList.Remove(user);

					mOwner->mP.Create_Quit(msg, user->mNick); // send quit to all
					mOwner->mUserList.SendToAll(msg, false, true); // todo: no cache, why?
					mOwner->mInProgresUsers.SendToAll(msg, false, true);
					mOwner->mP.Create_Hello(msg, user->mNick); // send hello to hello users
					mOwner->mHelloUsers.SendToAll(msg, false, true);
					mOwner->mUserList.SendToAll(user->mMyINFO, false, true); // send myinfo to all
					mOwner->mInProgresUsers.SendToAll(user->mMyINFO, false, true); // todo: no cache, why?
				}
			}
		} else {
			os << autosprintf(_("You have no rights to change class for this user: %s"), nick.c_str());
		}
	} else {
		os << autosprintf(_("User not found: %s"), nick.c_str());
	}

	mOwner->DCPublicHS(os.str().c_str(), conn);
	return 1;
}

int cDCConsole::CmdHideKick(istringstream &cmd_line, cConnDC *conn)
{
	ostringstream os;
	string s;
	cUser * user;

	while(cmd_line.good()) {
		cmd_line >> s;
		if(cmd_line.fail()) break;
		user = mOwner->mUserList.GetUserByNick(s);
		if(user) {
			if(user-> mxConn && user->mClass < conn->mpUser->mClass) {
				os << autosprintf(_("Kicks of this user are now hidden: %s"), s.c_str());
				user->mHideKick = true;
			} else {
				os << autosprintf(_("You have no rights to do this."));
			}
		} else {
			os << autosprintf(_("User not found: %s"), s.c_str());
		}
	}
	mOwner->DCPublicHS(os.str().c_str(),conn);
	return 1;
}

int cDCConsole::CmdUnHideKick(istringstream &cmd_line, cConnDC *conn)
{
	ostringstream os;
	string s;
	cUser * user;

	while(cmd_line.good())
	{
		cmd_line >> s;
		if(cmd_line.fail()) break;
		user = mOwner->mUserList.GetUserByNick(s);
		if(user) {
			if(user-> mxConn && user->mClass < conn->mpUser->mClass) {
				os << autosprintf(_("Kicks of this user are now visible: %s"), s.c_str());
				user->mHideKick = false;
			} else {
				os << _("You have no rights to do this.");
			}
		} else
			os << autosprintf(_("User not found: %s"), s.c_str());
	}
	mOwner->DCPublicHS(os.str().c_str(),conn);
	return 1;
}

int cDCConsole::CmdProtect(istringstream &cmd_line, cConnDC *conn)
{
	ostringstream os;
	string s;
	cUser * user;
	int oclass, mclass = conn->mpUser->mClass, nclass = mclass -1;
	if(nclass > 5)
		nclass = 5;

	cmd_line >> s >> nclass;

	if(!s.size() || nclass < 0 || nclass > 5 || nclass >= mclass) {
		os << _("Use !protect <nickname> <class>.") << " "
			<< autosprintf(_("Max class is %d"), mclass-1) << endl;
		mOwner->DCPublicHS(os.str().c_str(),conn);
		return 1;
	}


	user = mOwner->mUserList.GetUserByNick(s);

	if(user && user->mxConn) {
		oclass = user->mClass;
		if(oclass < mclass) {
			os << autosprintf(_("User %s is temporarily protected from class %d"), s.c_str(), nclass) << endl;
			user->mProtectFrom = nclass;
		} else
			os << _("You have no rights to do this.") << endl;
	} else {
		os << autosprintf(_("User not found: %s"), s.c_str()) << endl;
	}
	mOwner->DCPublicHS(os.str().c_str(),conn);
	return 1;
}

int cDCConsole::CmdReload(istringstream &cmd_line, cConnDC *conn)
{
	mOwner->Reload();
	mOwner->DCPublicHS(_("Done reloading all lists."), conn);
	return 1;
}

bool cDCConsole::cfReport::operator()()
{
	if (mS->mC.disable_report_cmd) {
		*mOS << _("Report command is currently disabled.");
		return false;
	}

	ostringstream os;
	string nick, reason;
	cUser *user;
	enum {eREP_ALL, eREP_NICK, eREP_RASONP, eREP_REASON};
	GetParOnlineUser(eREP_NICK, user, nick);
	GetParStr(eREP_REASON, reason);

	// to opchat
	if (user && user->mxConn) {
		os << autosprintf(_("Reported user %s"), user->mNick.c_str());
		os << " ][ " << _("IP") << ": " << user->mxConn->AddrIP().c_str();
		if (!user->mxConn->AddrHost().empty()) os << " ][ " << _("Host") << ": " << user->mxConn->AddrHost().c_str();
	} else
		os << autosprintf(_("Reported offline user %s"), nick.c_str());

	if (reason.empty())
		os << " ][ " << _("Without reason");
	else
		os << " ][ " << _("Reason") << ": " << reason.c_str();

	mS->ReportUserToOpchat(mConn, os.str(), mS->mC.dest_report_chat);
	// to sender
	*mOS << _("Thank you, your report has been accepted.");
	return true;
}

bool cDCConsole::cfRaw::operator()()
{
	if (!mConn || !mConn->mpUser) return false;

	if (mConn->mpUser->mClass < eUC_ADMIN) {
		(*mOS) << _("You have no rights to do this.");
		return false;
	}

	enum {eRW_ALL, eRW_USR, eRW_HELLO, eRW_PASSIVE, eRW_ACTIVE};
	static const char *actionnames[] = {"all", "user", "usr", "hello", "hel", "passive", "pas", "active", "act"};
	static const int actionids[] = {eRW_ALL, eRW_USR, eRW_USR, eRW_HELLO, eRW_HELLO, eRW_PASSIVE, eRW_PASSIVE, eRW_ACTIVE, eRW_ACTIVE};
	enum {eRC_HUBNAME, eRC_HELLO, eRC_QUIT, eRC_REDIR, eRC_PM, eRC_CHAT};
	static const char *cmdnames[] = {"hubname", "name", "hello", "quit", "redir", "move", "pm", "chat", "mc"};
	static const int cmdids[] = {eRC_HUBNAME, eRC_HUBNAME, eRC_HELLO, eRC_QUIT, eRC_REDIR, eRC_REDIR, eRC_PM, eRC_CHAT, eRC_CHAT};

	int CmdID = -1;
	string tmp;
	mIdRex->Extract(1, mIdStr, tmp);
	const int Action = this->StringToIntFromList(tmp, actionnames, actionids, sizeof(actionnames) / sizeof(char*));
	if (Action < 0) return false;
	mIdRex->Extract(2, mIdStr, tmp);
	CmdID = this->StringToIntFromList(tmp, cmdnames, cmdids, sizeof(cmdnames) / sizeof(char*));
	if (CmdID < 0) return false;
	string theCommand, endOfCommand;
	string param, nick;
	GetParStr(1, param);
	bool WithNick = false;

	switch (CmdID) {
		case eRC_HUBNAME:
			cDCProto::Create_HubName(theCommand, "", "");
			break;
		case eRC_HELLO:
			cDCProto::Create_Hello(theCommand, "");
			break;
		case eRC_QUIT:
			cDCProto::Create_Quit(theCommand, "");
			break;
		case eRC_REDIR:
			cDCProto::Create_ForceMove(theCommand, "");
			break;
		case eRC_PM:
			mS->mP.Create_PMForBroadcast(theCommand, endOfCommand, mS->mC.hub_security, mConn->mpUser->mNick, param);
			WithNick = true;
			break;
		case eRC_CHAT:
			cDCProto::Create_Chat(theCommand, mConn->mpUser->mNick, "");
			break;
		default:
			return false;
	}

	if (!WithNick) {
		theCommand += param;
		theCommand += "|";
	}

	cUser *target_usr = NULL;

	switch (Action) {
		case eRW_ALL:
			if (!WithNick)
				mS->mUserList.SendToAll(theCommand);
			else
				mS->mUserList.SendToAllWithNick(theCommand, endOfCommand);

			break;

		case eRW_HELLO:
			if (!WithNick)
				mS->mHelloUsers.SendToAll(theCommand);
			else
				mS->mHelloUsers.SendToAllWithNick(theCommand, endOfCommand);

			break;

		case eRW_PASSIVE:
			if (!WithNick)
				mS->mPassiveUsers.SendToAll(theCommand);
			else
				mS->mPassiveUsers.SendToAllWithNick(theCommand, endOfCommand);

			break;

		case eRW_ACTIVE:
			if (!WithNick)
				mS->mActiveUsers.SendToAll(theCommand);
			else
				mS->mActiveUsers.SendToAllWithNick(theCommand, endOfCommand);

			break;

		case eRW_USR:
			target_usr = mS->mUserList.GetUserByNick(nick);

			if (target_usr && target_usr->mxConn) {
				if (WithNick) {
					theCommand += nick;
					theCommand += endOfCommand;
				}

				target_usr->mxConn->Send(theCommand);
			}

			break;

		default:
			return false;
			break;
	}

	(*mOS) << _("Protocol command successfully sent.");
	return true;
}

bool cDCConsole::cfClean::operator()()
{
	if (!mConn || !mConn->mpUser)
		return false;

	if (mConn->mpUser->mClass < eUC_OPERATOR) {
		(*mOS) << _("You have no rights to do this.");
		return false;
	}

	static const char *cleanames[] = {
		"banlist",
		"tempbans",
		"unbanlist",
		"kicklist",
		"temprights"
	};

	enum {
		CLEAN_BAN,
		CLEAN_TBAN,
		CLEAN_UNBAN,
		CLEAN_KICK,
		CLEAN_TRIGHTS
	};

	static const int cleanids[] = {
		CLEAN_BAN,
		CLEAN_TBAN,
		CLEAN_UNBAN,
		CLEAN_KICK,
		CLEAN_TRIGHTS
	};

	string tmp;
	int CleanType = eBF_NICKIP;

	if (mIdRex->PartFound(1)) {
		mIdRex->Extract(1, mIdStr, tmp);
		CleanType = this->StringToIntFromList(tmp, cleanames, cleanids, sizeof(cleanames) / sizeof(char*));

		if (CleanType < 0)
			return false;
	}

	switch (CleanType) {
		case CLEAN_BAN:
			mS->mBanList->TruncateTable();
			(*mOS) << _("Ban list has been cleaned.");
			break;
		case CLEAN_TBAN:
			mS->mBanList->RemoveOldShortTempBans(0);
			(*mOS) << _("Temporary ban list has been cleaned.");
			break;
		case CLEAN_UNBAN:
			mS->mUnBanList->TruncateTable();
			(*mOS) << _("Unban list has been cleaned.");
			break;
		case CLEAN_KICK:
		  	mS->mKickList->TruncateTable();
			(*mOS) << _("Kick list has been cleaned.");
			break;
		case CLEAN_TRIGHTS:
			mS->mPenList->TruncateTable();
			mS->mPenList->ReloadCache();

			(*mOS) << _("Temporary rights list has been cleaned.");
			break;
		default:
			(*mOS) << _("This command is not implemented.");
			return false;
	}

	return true;
}

bool cDCConsole::cfBan::operator()()
{
	if (!mConn || !mConn->mpUser)
		return false;

	int MyClass = mConn->mpUser->mClass;

	if (MyClass < eUC_OPERATOR) {
		(*mOS) << _("You have no rights to do this.");
		return false;
	}

	static const char *bannames[] = {
		"nick",
		"ip",
		"nickip", "",
		"range",
		"host1",
		"host2",
		"host3",
		"hostr1",
		"share",
		"prefix"
	};

	static const int banids[] = {
		eBF_NICK,
		eBF_IP,
		eBF_NICKIP, eBF_NICKIP,
		eBF_RANGE,
		eBF_HOST1,
		eBF_HOST2,
		eBF_HOST3,
		eBF_HOSTR1,
		eBF_SHARE,
		eBF_PREFIX
	};

	enum {
		BAN_BAN,
		BAN_UNBAN,
		BAN_INFO,
		BAN_LIST
	};

	static const char *prefixnames[] = {
		"add", "new",
		"rm", "del", "un",
		"info", "check",
		"list", "ls", "lst"
	};

	static const int prefixids[] = {
		BAN_BAN, BAN_BAN,
		BAN_UNBAN, BAN_UNBAN, BAN_UNBAN,
		BAN_INFO, BAN_INFO,
		BAN_LIST, BAN_LIST, BAN_LIST
	};

	enum {
		BAN_PREFIX = 1, // mIdStr
		BAN_TYPE = 2,
		BAN_LENGTH = 4,
		BAN_THIS = 6,
		BAN_WHO = 1, // mParStr
		BAN_REASON = 3
	};

	string tmp;
	int BanAction = BAN_BAN;

	if (mIdRex->PartFound(BAN_PREFIX)) {
		mIdRex->Extract(BAN_PREFIX, mIdStr, tmp);
		BanAction = this->StringToIntFromList(tmp, prefixnames, prefixids, sizeof(prefixnames) / sizeof(char*));

		if (BanAction < 0)
			return false;
	}

	int Count = 0;

	if (BanAction == BAN_LIST) { // ban list
		Count = 100;

		if (mParRex->PartFound(BAN_WHO))
			GetParInt(BAN_WHO, Count);

		ostringstream os;
		mS->mBanList->List(os, Count);
		mS->DCPrivateHS(os.str(), mConn);
		return true;
	}

	if (!mParRex->PartFound(BAN_WHO)) {
		(*mOS) << _("Missing command parameters.");
		return false;
	}

	int BanType = eBF_NICKIP;

	if (mIdRex->PartFound(BAN_TYPE)) {
		mIdRex->Extract(BAN_TYPE, mIdStr, tmp);
		BanType = this->StringToIntFromList(tmp, bannames, banids, sizeof(bannames) / sizeof(char*));

		if (BanType < 0)
			return false;
	}

	bool IsNick = false;

	if (BanType == eBF_NICK)
		IsNick = true;

	if (mIdRex->PartFound(BAN_THIS))
		IsNick = (0 == mIdRex->Compare(BAN_TYPE, mIdStr, "nick"));

	bool IsPerm = !mIdRex->PartFound(BAN_LENGTH);
	time_t BanTime = 0;

	if (!IsPerm) {
		mIdRex->Extract(BAN_LENGTH, mIdStr, tmp);

		if (tmp != "perm") {
			BanTime = mS->Str2Period(tmp, *mOS);

			if (BanTime < 0) {
				(*mOS) << _("Please provide a valid ban time.");
				return false;
			}
		} else {
			IsPerm = true;
		}
	}

	cBan Ban(mS);
	cKick Kick;
	string Who;
	GetParStr(BAN_WHO, Who);
	bool unban = (BanAction == BAN_UNBAN);
	tmp.clear();

	switch (BanAction) {
		case BAN_UNBAN:
		case BAN_INFO:
			if (unban) {
				if (!GetParStr(BAN_REASON, tmp)) // default reason
					tmp = _("No reason specified");

				#ifndef WITHOUT_PLUGINS
					if (!mS->mCallBacks.mOnUnBan.CallAll(mConn->mpUser, Who, mConn->mpUser->mNick, tmp)) {
						(*mOS) << _("Your action was blocked by a plugin.");
						return false;
					}
				#endif

				(*mOS) << _("Removed ban") << ":\r\n";
			} else {
				(*mOS) << _("Ban information") << ":\r\n";
			}

			if (BanType == eBF_NICKIP) {
				Count += mS->mBanList->Unban(*mOS, Who, tmp, mConn->mpUser->mNick, eBF_NICK, unban);
				Count += mS->mBanList->Unban(*mOS, Who, tmp, mConn->mpUser->mNick, eBF_IP, unban);

				if (!unban) {
					Count += mS->mBanList->Unban(*mOS, Who, tmp, mConn->mpUser->mNick, eBF_RANGE, false);
					string Host;

					if (mConn->DNSResolveReverse(Who, Host)) {
						Count += mS->mBanList->Unban(*mOS, Host, tmp, mConn->mpUser->mNick, eBF_HOSTR1, false);
						Count += mS->mBanList->Unban(*mOS, Host, tmp, mConn->mpUser->mNick, eBF_HOST3, false);
						Count += mS->mBanList->Unban(*mOS, Host, tmp, mConn->mpUser->mNick, eBF_HOST2, false);
						Count += mS->mBanList->Unban(*mOS, Host, tmp, mConn->mpUser->mNick, eBF_HOST1, false);
					}
				}
			} else if (BanType == eBF_NICK) {
				Count += mS->mBanList->Unban(*mOS, Who, tmp, mConn->mpUser->mNick, eBF_NICK, unban);
				Count += mS->mBanList->Unban(*mOS, Who, tmp, mConn->mpUser->mNick, eBF_NICKIP, unban);
			} else {
				Count += mS->mBanList->Unban(*mOS, Who, tmp, mConn->mpUser->mNick, BanType, unban);
			}

			(*mOS) << "\r\n" << autosprintf(ngettext("%d ban found.", "%d bans found.", Count), Count);
			break;

		case BAN_BAN:
			Ban.mNickOp = mConn->mpUser->mNick;
			mParRex->Extract(BAN_REASON, mParStr, tmp);

			if (tmp.size())
				Ban.mReason = tmp;
			else
				Ban.mReason = _("No reason specified"); // default reason, doesnt work when no reason is specified, bad regexp

			Ban.mDateStart = cTime().Sec();

			if (BanTime)
				Ban.mDateEnd = Ban.mDateStart + BanTime;
			else
				Ban.mDateEnd = 0;

			Ban.mLastHit = 0;
			Ban.SetType(BanType);

			switch (BanType) {
				case eBF_NICKIP:
				case eBF_NICK:
				case eBF_IP:
					if (mS->mKickList->FindKick(Kick, Who, mConn->mpUser->mNick, 3000, true, true, IsNick)) {
						mS->mBanList->NewBan(Ban, Kick, BanTime, BanType);
					} else {
						if (BanType == eBF_NICKIP)
							BanType = eBF_IP;

						mParRex->Extract(BAN_REASON, mParStr, Kick.mReason);
						Kick.mOp = mConn->mpUser->mNick;
						Kick.mTime = cTime().Sec();

						if (BanType == eBF_NICK)
							Kick.mNick = Who;
						else
							Kick.mIP = Who;

						mS->mBanList->NewBan(Ban, Kick, BanTime, BanType);
					}

					break;

				case eBF_HOST1:
				case eBF_HOST2:
				case eBF_HOST3:
				case eBF_HOSTR1:
					if (MyClass < (eUC_ADMIN - (BanType - eBF_HOST1))) {
						(*mOS) << _("You have no rights to do this.");
						return false;
					}

					Ban.mHost = Who;
					Ban.mIP = Who;
					break;

				case eBF_RANGE:
					if (!cDCConsole::GetIPRange(Who, Ban.mRangeMin, Ban.mRangeMax)) {
						(*mOS) << autosprintf(_("Unknown range format: %s"), Who.c_str());
						return false;
					}

					Ban.mIP = Who;
					break;

				case eBF_PREFIX:
					Ban.mNick = Who;
					break;

				case eBF_SHARE:
					{
						istringstream is(Who);
						__int64 share;
						is >> share;
						Ban.mShare = share;
					}

					break;

				default:
					break;
			}

			#ifndef WITHOUT_PLUGINS
				if (!mS->mCallBacks.mOnNewBan.CallAll(mConn->mpUser, &Ban)) {
					(*mOS) << _("Your action was blocked by a plugin.");
					return false;
				}
			#endif

			if ((BanType == eBF_IP) || (BanType == eBF_RANGE)) { // drop users if ban type is ip or range
				cUserCollection::iterator i;
				cConnDC *conn;

				for (i = mS->mUserList.begin(); i != mS->mUserList.end(); ++i) {
					conn = ((cUser*)(*i))->mxConn;

					if (conn) {
						unsigned long ipnum = cBanList::Ip2Num(conn->AddrIP());

						if (((BanType == eBF_IP) && (Ban.mIP == conn->AddrIP())) || ((BanType == eBF_RANGE) && (Ban.mRangeMin <= ipnum) && (Ban.mRangeMax >= ipnum)))
							conn->CloseNow();
					}
				}
			} else if (Ban.mNick.size()) { // drop user if nick is specified
				cUser *user = mS->mUserList.GetUserByNick(Ban.mNick);

				if (user && user->mxConn)
					user->mxConn->CloseNow();
			}

			mS->mBanList->AddBan(Ban);
			(*mOS) << _("Added new ban") << ":\r\n";
			Ban.DisplayComplete(*mOS);
			break;

		default:
			(*mOS) << _("This command is not implemented.");
			return false;
	}

	return true;
}

bool cDCConsole::cfInfo::operator()()
{
	if (!mConn || !mConn->mpUser)
		return false;

	if (mConn->mpUser->mClass < eUC_OPERATOR) {
		(*mOS) << _("You have no rights to do this.");
		return false;
	}

	enum {
		eINFO_HUB,
		eINFO_SERVER,
		eINFO_PORT,
		eINFO_HUBURL,
		eINFO_PROTOCOL
	};

	static const char *infonames[] = {
		"hub",
		"serv", "server", "sys", "system", "os",
		"port",
		"url", "huburl",
		"prot", "proto", "protocol"
	};

	static const int infoids[] = {
		eINFO_HUB,
		eINFO_SERVER, eINFO_SERVER, eINFO_SERVER, eINFO_SERVER, eINFO_SERVER,
		eINFO_PORT,
		eINFO_HUBURL, eINFO_HUBURL,
		eINFO_PROTOCOL, eINFO_PROTOCOL, eINFO_PROTOCOL
	};

	string tmp;
	mIdRex->Extract(1, mIdStr, tmp);
	int InfoType = this->StringToIntFromList(tmp, infonames, infoids, sizeof(infonames) / sizeof(char*));

	if (InfoType < 0)
		return false;

	switch (InfoType) {
		case eINFO_PORT:
			mInfoServer.PortInfo(*mOS);
			break;
		case eINFO_HUBURL:
			mInfoServer.HubURLInfo(*mOS);
			break;
		case eINFO_PROTOCOL:
			mInfoServer.ProtocolInfo(*mOS);
			break;
		case eINFO_SERVER:
			mInfoServer.SystemInfo(*mOS);
			break;
		case eINFO_HUB:
			mInfoServer.Output(*mOS, mConn->mpUser->mClass);
			break;
		default:
			(*mOS) << _("This command is not implemented.");
			return false;
	}

	return true;
}

bool cDCConsole::cfTrigger::operator()()
{
	string ntrigger;
	string text, cmd;

	if (mConn->mpUser->mClass < eUC_MASTER) {
		(*mOS) << _("You have no rights to do this.");
		return false;
	}

   	mIdRex->Extract(2,mIdStr,cmd);
	enum {eAC_ADD, eAC_DEL, eAC_EDIT, eAC_DEF, eAC_FLAGS};
	static const char *actionnames[]={"new","add","del","edit","def", "setflags"};
	static const int actionids[]={eAC_ADD, eAC_ADD, eAC_DEL, eAC_EDIT, eAC_DEF, eAC_FLAGS};
	int Action = this->StringToIntFromList(cmd, actionnames, actionids, sizeof(actionnames)/sizeof(char*));
	if (Action < 0) return false;

	mParRex->Extract(1,mParStr,ntrigger);
	mParRex->Extract(2,mParStr,text);
	int i;
	int flags = 0;
	istringstream is(text);
	bool result = false;
	cTrigger *tr;

	switch(Action)
	{
	case eAC_ADD:
		tr = new cTrigger;
		tr->mCommand = ntrigger;
		tr->mDefinition = text;
		break;

		case eAC_EDIT:
		for( i = 0; i < ((cDCConsole*)mCo)->mTriggers->Size(); ++i )
		{
			if( ntrigger == (*((cDCConsole*)mCo)->mTriggers)[i]->mCommand )
			{
				mS->SaveFile(((*((cDCConsole*)mCo)->mTriggers)[i])->mDefinition,text);
				result = true;
				break;
			}
		}
		break;
	case eAC_FLAGS:
		flags = -1;
		is >> flags;
		if (flags >= 0) {
			for( i = 0; i < ((cDCConsole*)mCo)->mTriggers->Size(); ++i )
			{
				if( ntrigger == (*((cDCConsole*)mCo)->mTriggers)[i]->mCommand )
				{
				(*((cDCConsole*)mCo)->mTriggers)[i]->mFlags = flags;
				((cDCConsole*)mCo)->mTriggers->SaveData(i);
				result = true;
				break;
				}
			}
		}

		break;
	default: (*mOS) << _("This command is not implemented.");
		break;
	};
		return result;
}

bool cDCConsole::cfGetConfig::operator()()
{
	if (!mConn || !mConn->mpUser)
		return false;

	if (mConn->mpUser->mClass < eUC_ADMIN) {
		(*mOS) << _("You have no rights to do this.");
		return false;
	}

	enum { eAC_GETCONF, eAC_GETVAR };

	static const char *actionnames[] = {
		"gc", "getconf", "getconfig",
		"gv", "getvar"
	};

	static const int actionids[] = {
		eAC_GETCONF, eAC_GETCONF, eAC_GETCONF,
		eAC_GETVAR, eAC_GETVAR
	};

	string temp;
	mIdRex->Extract(1, mIdStr, temp);
	int act = this->StringToIntFromList(temp, actionnames, actionids, sizeof(actionnames) / sizeof(char*));

	if (act < 0)
		return false;

	ostringstream os, lst;
	temp.clear();
	GetParStr(1, temp);

	if (act == eAC_GETCONF) {
		if (temp.empty()) {
			temp = mS->mDBConf.config_name;
			cConfigBaseBase::tIVIt it;

			for (it = mS->mC.mvItems.begin(); it != mS->mC.mvItems.end(); ++it)
				lst << " [*] " << mS->mC.mhItems.GetByHash(*it)->mName << " = " << (*(mS->mC.mhItems.GetByHash(*it))) << "\r\n";
		} else {
			mS->mSetupList.OutputFile(temp.c_str(), lst);
		}

		if (lst.str().size()) {
			os << autosprintf(_("Configuration file: %s"), temp.c_str()) << "\r\n\r\n" << lst.str();
			mS->DCPrivateHS(os.str(), mConn);
		} else {
			os << autosprintf(_("Configuration file is empty: %s"), temp.c_str());
			mS->DCPublicHS(os.str(), mConn);
		}
	} else if (act == eAC_GETVAR) {
		if (temp.empty()) {
			(*mOS) << _("Missing command parameters.");
			return false;
		}

		string file/*(mS->mDBConf.config_name)*/, var(temp);
		size_t pos = var.find('.'); // file.variable style

		if (pos != string::npos) {
			file = var.substr(0, pos);
			var = var.substr(pos + 1);
		}

		mS->mSetupList.FilterFiles(var.c_str(), lst, file.c_str());

		if (lst.str().size())
			os << autosprintf(_("Configuration filter results: %s"), temp.c_str()) << "\r\n\r\n" << lst.str();
		else
			os << autosprintf(_("Configuration filter didn't return any results: %s"), temp.c_str());

		mS->DCPublicHS(os.str(), mConn);
	}

	return true;
}

bool cDCConsole::cfSetVar::operator()()
{
	if (!mConn || !mConn->mpUser)
		return false;

	if (mConn->mpUser->mClass < eUC_ADMIN) {
		(*mOS) << _("You have no rights to do this.");
		return false;
	}

	string file(mS->mDBConf.config_name), var, val, val_new, val_old;

	if (mParRex->PartFound(2)) // [file] variable value style
		mParRex->Extract(2, mParStr, file);

	mParRex->Extract(3, mParStr, var);
	size_t pos = var.find('.'); // file.variable value style

	if (pos != string::npos) {
		file = var.substr(0, pos);
		var = var.substr(pos + 1);
	}

	mParRex->Extract(4, mParStr, val);
	int res = mS->SetConfig(file.c_str(), var.c_str(), val.c_str(), val_new, val_old, mConn->mpUser);

	switch (res) {
		case -1:
			(*mOS) << autosprintf(_("Undefined configuration variable: %s.%s"), file.c_str(), var.c_str());
			return true;
		case 0:
			(*mOS) << _("Your action was blocked by a plugin.");
			return true;
		case 1:
			(*mOS) << autosprintf(_("Updated configuration %s.%s: %s -> %s"), file.c_str(), var.c_str(), val_old.c_str(), val_new.c_str());
			break;
		default:
			return false;
	}

	struct rlimit userLimit;

	if (!getrlimit(RLIMIT_NOFILE, &userLimit) && userLimit.rlim_cur < mS->mC.max_users_total) // get maximum file descriptor number
		(*mOS) << "\r\n" << autosprintf(_("Warning: %s allows maximum %d users, but current resource limit is %d, so consider running ulimit -n <max_users> and restarting the hub."), HUB_VERSION_NAME, mS->mC.max_users_total, int(userLimit.rlim_cur));

	return true;
}

bool cDCConsole::cfGag::operator()()
{
	if (!mConn || !mConn->mpUser)
		return false;

	enum {
		eAC_GAG,
		eAC_NOPM,
		eAC_NOCHATS,
		eAC_NODL,
		eAC_NOSEARCH,
		eAC_KVIP,
		eAC_NOSHARE,
		eAC_CANREG,
		eAC_OPCHAT,
		eAC_NOINFO,
		eAC_LIST
	};

	static const char *actionnames[] = {
		"gag", "nochat",
		"nopm",
		"nochats",
		"noctm", "nodl",
		"nosearch",
		"kvip", "maykick",
		"noshare",
		"mayreg",
		"mayopchat",
		"noinfo", "mayinfo",
		"temprights"
	};

	static const int actionids[] = {
		eAC_GAG, eAC_GAG,
		eAC_NOPM,
		eAC_NOCHATS,
		eAC_NODL, eAC_NODL,
		eAC_NOSEARCH,
		eAC_KVIP, eAC_KVIP,
		eAC_NOSHARE,
		eAC_CANREG,
		eAC_OPCHAT,
		eAC_NOINFO, eAC_NOINFO,
		eAC_LIST
	};

	string temp;
	mIdRex->Extract(2, mIdStr, temp);
	int Action = this->StringToIntFromList(temp, actionnames, actionids, sizeof(actionnames) / sizeof(char*));

	if (Action < 0)
		return false;

	if (mConn->mpUser->mClass < eUC_OPERATOR) {
		(*mOS) << _("You have no rights to do this.");
		return false;
	}

	if (Action == eAC_LIST) {
		ostringstream os;
		mS->mPenList->ListAll(os);
		mS->DCPrivateHS(os.str(), mConn);
		return true;
	}

	if (!mParRex->PartFound(1)) {
		(*mOS) << _("Missing command parameters.");
		return false;
	}

	string nick;
	mParRex->Extract(1, mParStr, nick);

	if (Action == eAC_NOINFO) {
		cUser *user = mS->mUserList.GetUserByNick(nick);

		if (user && user->mxConn) {
			user->DisplayRightsInfo((*mOS), true);
			return true;
		} else {
			(*mOS) << autosprintf(_("User not found: %s"), nick.c_str());
			return false;
		}
	}

	unsigned long period = 24 * 3600 * 7;
	unsigned long Now = 1;
	bool isUn = mIdRex->PartFound(1);

	if (mParRex->PartFound(3)) {
		mParRex->Extract(3, mParStr, temp);
		period = mS->Str2Period(temp, (*mOS));

		if (!period)
			return false;
	}

	cPenaltyList::sPenalty penalty;

	if (!mS->mPenList->LoadTo(penalty, nick))
		penalty.mNick = nick;

	penalty.mOpNick = mConn->mpUser->mNick;

	if (!isUn)
		Now = cTime().Sec() + period;

	switch (Action) {
		case eAC_GAG:
			penalty.mStartChat = Now;
			break;
		case eAC_NOPM:
			penalty.mStartPM = Now;
			break;
		case eAC_NOCHATS:
			penalty.mStartChat = Now;
			penalty.mStartPM = Now;
			break;
		case eAC_NODL:
			penalty.mStartCTM = Now;
			break;
		case eAC_NOSEARCH:
			penalty.mStartSearch = Now;
			break;
		case eAC_KVIP:
			penalty.mStopKick = Now;
			break;
		case eAC_OPCHAT:
			penalty.mStopOpchat = Now;
			break;
		case eAC_NOSHARE:
			penalty.mStopShare0 = Now;
			break;
		case eAC_CANREG:
			penalty.mStopReg = Now;
			break;
		default:
			return false;
	}

	bool ret = false;

	if (isUn)
		ret = mS->mPenList->RemPenalty(penalty);
	else
		ret = mS->mPenList->AddPenalty(penalty);

	if (!ret) {
		(*mOS) << autosprintf(_("Error setting rights or restrictions for user: %s"), penalty.mNick.c_str());
		return false;
	}

	cUser *usr = mS->mUserList.GetUserByNick(nick);

	switch (Action) {
		case eAC_GAG:
			if (isUn)
				(*mOS) << autosprintf(_("Allowing user to use main chat again: %s"), penalty.mNick.c_str());
			else
				(*mOS) << autosprintf(_("Restricting user from using main chat: %s for %s"), penalty.mNick.c_str(), cTime(period, 0).AsPeriod().AsString().c_str());

			if (usr)
				usr->SetRight(eUR_CHAT, penalty.mStartChat, isUn, true);

			break;

		case eAC_NOPM:
			if (isUn)
				(*mOS) << autosprintf(_("Allowing user to use private chat again: %s"), penalty.mNick.c_str());
			else
				(*mOS) << autosprintf(_("Restricting user from using private chat: %s for %s"), penalty.mNick.c_str(), cTime(period, 0).AsPeriod().AsString().c_str());

			if (usr)
				usr->SetRight(eUR_PM, penalty.mStartPM, isUn, true);

			break;

		case eAC_NOCHATS:
			if (isUn)
				(*mOS) << autosprintf(_("Allowing user to use main and private chat again: %s"), penalty.mNick.c_str());
			else
				(*mOS) << autosprintf(_("Restricting user from using main and private chat: %s for %s"), penalty.mNick.c_str(), cTime(period, 0).AsPeriod().AsString().c_str());

			if (usr) {
				usr->SetRight(eUR_CHAT, penalty.mStartChat, isUn, true);
				usr->SetRight(eUR_PM, penalty.mStartPM, isUn, true);
			}

			break;

		case eAC_NODL:
			if (isUn)
				(*mOS) << autosprintf(_("Allowing user to download again: %s"), penalty.mNick.c_str());
			else
				(*mOS) << autosprintf(_("Restricting user from downloading: %s for %s"), penalty.mNick.c_str(), cTime(period, 0).AsPeriod().AsString().c_str());

			if (usr)
				usr->SetRight(eUR_CTM, penalty.mStartCTM, isUn, true);

			break;

		case eAC_NOSEARCH:
			if (isUn)
				(*mOS) << autosprintf(_("Allowing user to use search again: %s"), penalty.mNick.c_str());
			else
				(*mOS) << autosprintf(_("Restricting user from using search: %s for %s"), penalty.mNick.c_str(), cTime(period, 0).AsPeriod().AsString().c_str());

			if (usr)
				usr->SetRight(eUR_SEARCH, penalty.mStartSearch, isUn, true);

			break;

		case eAC_NOSHARE:
			if (isUn)
				(*mOS) << autosprintf(_("Taking away the right to hide share: %s"), penalty.mNick.c_str());
			else
				(*mOS) << autosprintf(_("Giving user the right to hide share: %s for %s"), penalty.mNick.c_str(), cTime(period, 0).AsPeriod().AsString().c_str());

			if (usr)
				usr->SetRight(eUR_NOSHARE, penalty.mStopShare0, isUn, true);

			break;

		case eAC_CANREG:
			if (isUn)
				(*mOS) << autosprintf(_("Taking away the right to register others: %s"), penalty.mNick.c_str());
			else
				(*mOS) << autosprintf(_("Giving user the right to register others: %s for %s"), penalty.mNick.c_str(), cTime(period, 0).AsPeriod().AsString().c_str());

			if (usr)
				usr->SetRight(eUR_REG, penalty.mStopReg, isUn, true);

			break;

		case eAC_KVIP:
			if (isUn)
				(*mOS) << autosprintf(_("Taking away the right to kick others: %s"), penalty.mNick.c_str());
			else
				(*mOS) << autosprintf(_("Giving user the right to kick others: %s for %s"), penalty.mNick.c_str(), cTime(period, 0).AsPeriod().AsString().c_str());

			if (usr)
				usr->SetRight(eUR_KICK, penalty.mStopKick, isUn, true);

			break;

		case eAC_OPCHAT:
			if (isUn)
				(*mOS) << autosprintf(_("Taking away the right to use operator chat: %s"), penalty.mNick.c_str());
			else
				(*mOS) << autosprintf(_("Giving user the right to use operator chat: %s for %s"), penalty.mNick.c_str(), cTime(period, 0).AsPeriod().AsString().c_str());

			if (usr) {
				usr->SetRight(eUR_OPCHAT, penalty.mStopOpchat, isUn, true);
				cUserCollection::tHashType Hash = mS->mUserList.Nick2Hash(usr->mNick); // add or remove user in operator chat

				if (isUn && mS->mOpchatList.ContainsHash(Hash))
					mS->mOpchatList.RemoveByHash(Hash);
				else if (!isUn && !mS->mOpchatList.ContainsHash(Hash))
					mS->mOpchatList.AddWithHash(usr, Hash);
			}

			break;

		default:
			return false;
	}

	return true;
}

bool cDCConsole::cfCmd::operator()()
{
	enum { eAC_LIST };
	static const char * actionnames [] = { "list", "lst" };
	static const int actionids [] = { eAC_LIST, eAC_LIST };

	string tmp;
	mIdRex->Extract(1,mIdStr,tmp);
	int Action = this->StringToIntFromList(tmp, actionnames, actionids, sizeof(actionnames)/sizeof(char*));
	if(Action < 0)
		return false;


	switch(Action) {
//		case eAC_LIST: this->mS->mCo.mCmdr.List(mOS); break;
		default: return false;
	}
	return true;
}

bool cDCConsole::cfWho::operator()()
{
	if (!mConn || !mConn->mpUser)
		return false;

	if (mConn->mpUser->mClass < eUC_OPERATOR) {
		(*mOS) << _("You have no rights to do this.");
		return false;
	}

	enum {
		eAC_IP,
		eAC_RANGE,
		eAC_CC,
		eAC_CITY,
		eAC_HUBPORT,
		eAC_HUBURL
	};

	static const char *actionnames[] = {
		"ip", "addr",
		"range", "subnet", "sub",
		"cc", "country",
		"city",
		"hubport", "port",
		"huburl", "url"
	};

	static const int actionids[] = {
		eAC_IP, eAC_IP,
		eAC_RANGE, eAC_RANGE, eAC_RANGE,
		eAC_CC, eAC_CC,
		eAC_CITY,
		eAC_HUBPORT, eAC_HUBPORT,
		eAC_HUBURL, eAC_HUBURL
	};

	string tmp;
	mIdRex->Extract(2, mIdStr, tmp);
	int Action = this->StringToIntFromList(tmp, actionnames, actionids, sizeof(actionnames) / sizeof(char*));

	if (Action < 0)
		return false;

	mParRex->Extract(0, mParStr, tmp);

	if (tmp.empty()) {
		(*mOS) << _("Missing command parameters.");
		return false;
	}

	string sep("\r\n\t");
	string who, low;
	unsigned long ip_min, ip_max;
	unsigned int cnt, port = 0;

	switch (Action) {
		case eAC_IP:
			ip_min = cBanList::Ip2Num(tmp);
			ip_max = ip_min;
			cnt = mS->WhoIP(ip_min, ip_max, who, sep, true);

			if (cnt)
				(*mOS) << autosprintf(ngettext("Found %d user with IP %s", "Found %d users with IP %s", cnt), cnt, tmp.c_str());

			break;

		case eAC_RANGE:
			if (!cDCConsole::GetIPRange(tmp, ip_min, ip_max))
				return false;

			cnt = mS->WhoIP(ip_min, ip_max, who, sep, false);

			if (cnt)
				(*mOS) << autosprintf(ngettext("Found %d user with range %s", "Found %d users with range %s", cnt), cnt, tmp.c_str());

			break;

		case eAC_CC:
			if (tmp.size() != 2) {
				(*mOS) << _("Country code must be 2 characters long, for example RU.");
				return false;
			}

			tmp = toUpper(tmp);
			cnt = mS->WhoCC(tmp, who, sep);

			if (cnt)
				(*mOS) << autosprintf(ngettext("Found %d user with country code %s", "Found %d users with country code %s", cnt), cnt, tmp.c_str());

			break;

		case eAC_CITY:
			low = toLower(tmp);
			cnt = mS->WhoCity(low, who, sep);

			if (cnt)
				(*mOS) << autosprintf(ngettext("Found %d user with city %s", "Found %d users with city %s", cnt), cnt, tmp.c_str());

			break;

		case eAC_HUBPORT:
			port = StringAsLL(tmp);
			cnt = mS->WhoHubPort(port, who, sep);

			if (cnt)
				(*mOS) << autosprintf(ngettext("Found %d user on hub port %d", "Found %d users on hub port %d", cnt), cnt, port);

			break;

		case eAC_HUBURL:
			low = toLower(tmp);
			cnt = mS->WhoHubURL(low, who, sep);

			if (cnt)
				(*mOS) << autosprintf(ngettext("Found %d user with hub URL %s", "Found %d users with hub URL %s", cnt), cnt, tmp.c_str());

			break;

		default:
			return false;
	}

	if (cnt)
		(*mOS) << ':' << who;
	else
		(*mOS) << _("No users found.");

	return true;
}

bool cDCConsole::cfKick::operator()()
{
	if (!mConn || !mConn->mpUser)
		return false;

	enum {
		eAC_KICK,
		eAC_DROP
	};

	static const char *actionnames[] = {
		"kick",
		"drop"
	};

	static const int actionids[] = {
		eAC_KICK,
		eAC_DROP
	};

	if (mConn->mpUser->mClass < eUC_VIPUSER) {
		(*mOS) << _("You have no rights to do this.");
		return false;
	}

	string temp;
	mIdRex->Extract(1, mIdStr, temp);
	int Action = this->StringToIntFromList(temp, actionnames, actionids, sizeof(actionnames) / sizeof(char*));

	if (Action < 0)
		return false;

	string nick;
	temp.clear();
	mParRex->Extract(1, mParStr, nick);

	if (mParRex->PartFound(2)) {
		mParRex->Extract(2, mParStr, temp);

		if (temp.size() && (temp[0] == ' ')) // small fix
			temp = temp.substr(1);
	}

	switch (Action) {
		case eAC_KICK:
			mS->DCKickNick(mOS, mConn->mpUser, nick, temp, (eKI_CLOSE | eKI_WHY | eKI_PM | eKI_BAN));
			break;
		case eAC_DROP:
			mS->DCKickNick(mOS, mConn->mpUser, nick, temp, (eKI_CLOSE | eKI_PM | eKI_DROP));
			break;
		default:
			(*mOS) << _("This command is not implemented.");
			return false;
	}

	return true;
}

bool cDCConsole::cfPlug::operator()()
{
	enum {
		eAC_IN,
		eAC_OUT,
		eAC_LIST,
		eAC_REG,
		eAC_RELOAD
	};

	static const char *actionnames[] = {
		"in",
		"out",
		"list",
		"reg", "call", "calls", "callback", "callbacks",
		"reload"
	};

	static const int actionids[] = {
		eAC_IN,
		eAC_OUT,
		eAC_LIST,
		eAC_REG, eAC_REG, eAC_REG, eAC_REG, eAC_REG,
		eAC_RELOAD
	};

	if (this->mConn->mpUser->mClass < mS->mC.plugin_mod_class) {
		(*mOS) << _("You have no rights to do this.");
		return false;
	}

	string tmp;
	mIdRex->Extract(1, mIdStr, tmp);
	int Action = this->StringToIntFromList(tmp, actionnames, actionids, sizeof(actionnames) / sizeof(char*));

	if (Action < 0)
		return false;

	switch (Action) {
		case eAC_LIST:
			(*mOS) << _("Loaded plugins") << ":\r\n\r\n";
			mS->mPluginManager.List(*mOS);
			break;
		case eAC_REG:
			(*mOS) << _("Plugin callbacks") << ":\r\n\r\n";
			mS->mPluginManager.ListAll(*mOS);
			break;
		case eAC_OUT:
			if (mParRex->PartFound(1)) {
				mParRex->Extract(1, mParStr, tmp);

				if (!mS->mPluginManager.UnloadPlugin(tmp))
					return false;
			}

			break;
		case eAC_IN:
			if (mParRex->PartFound(1)) {
				mParRex->Extract(1, mParStr, tmp);

				if (!mS->mPluginManager.LoadPlugin(tmp)) {
					(*mOS) << mS->mPluginManager.GetError();
					return false;
				}
			}

			break;
		case eAC_RELOAD:
			if (GetParStr(1, tmp)) {
				if (!mS->mPluginManager.ReloadPlugin(tmp)) {
					(*mOS) << mS->mPluginManager.GetError();
					return false;
				}
			}

			break;
		default:
			break;
	}

	return true;
}

bool cDCConsole::cfRegUsr::operator()()
{
	if (!this->mConn || !this->mConn->mpUser)
		return false;

	enum {
		eAC_NEW,
		eAC_DEL,
		eAC_PASS,
		eAC_ENABLE,
		eAC_DISABLE,
		eAC_CLASS,
		eAC_PROTECT,
		eAC_HIDEKICK,
		eAC_SET,
		eAC_INFO,
		eAC_LIST
	};

	static const char *actionnames[] = {
		"n", "new", "newuser",
		"del", "delete",
		"pass", "passwd",
		"enable", "disable",
		"class", "setclass",
		"protect", "protectclass",
		"hidekick", "hidekickclass",
		"set", "=",
		"info",
		"list", "lst"
	};

	static const int actionids[] = {
		eAC_NEW, eAC_NEW, eAC_NEW,
		eAC_DEL, eAC_DEL,
		eAC_PASS, eAC_PASS,
		eAC_ENABLE, eAC_DISABLE,
		eAC_CLASS, eAC_CLASS,
		eAC_PROTECT, eAC_PROTECT,
		eAC_HIDEKICK, eAC_HIDEKICK,
		eAC_SET, eAC_SET,
		eAC_INFO,
		eAC_LIST, eAC_LIST
	};

	string tmp;
	mIdRex->Extract(2, mIdStr, tmp);
	int Action = this->StringToIntFromList(tmp, actionnames, actionids, sizeof(actionnames) / sizeof(char*));

	if (Action < 0)
		return false;

	int MyClass = this->mConn->mpUser->mClass;

	if ((MyClass < eUC_OPERATOR) || ((Action != eAC_INFO) && (!this->mConn->mpUser->Can(eUR_REG, mS->mTime.Sec())))) {
		(*mOS) << _("You have no rights to do this.");
		return false;
	}

	if (mS->mC.classdif_reg > eUC_ADMIN) {
		(*mOS) << autosprintf(_("classdif_reg value must be between %d and %d. Please correct this first."), eUC_NORMUSER, eUC_ADMIN);
		return false;
	}

	if (Action == eAC_LIST) {
		int cls = eUC_REGUSER;
		this->GetParInt(1, cls);

		/*
		if (!this->GetParInt(1, cls)) {
			(*mOS) << _("Missing class parameter.");
			return false;
		}
		*/

		if ((MyClass < eUC_MASTER) && (cls >= eUC_NORMUSER) && (cls > int(MyClass - mS->mC.classdif_reg))) {
			(*mOS) << _("You have no rights to do this.");
			return false;
		}

		ostringstream lst;
		int res = mS->mR->ShowUsers(this->mConn, lst, cls);

		if (res)
			(*mOS) << autosprintf(ngettext("Found %d registered user with class %d", "Found %d registered users with class %d", res), res, cls) << ":\r\n\r\n" << lst.str().c_str();
		else
			(*mOS) << autosprintf(_("No registered users with class %d found."), cls);

		return true;
	}

	string nick, par, field, pass;	
	mParRex->Extract(1, mParStr, nick);
	bool WithPar = mParRex->PartFound(3);

	if ((Action != eAC_SET) && WithPar)
		mParRex->Extract(3, mParStr, par);

	bool WithPass = false;

	if (Action == eAC_NEW) {
		WithPass = mParRex->PartFound(5);

		if (WithPass) {
			mParRex->Extract(5, mParStr, par); // class
			mParRex->Extract(6, mParStr, pass); // password
		}
	}

	if (Action == eAC_SET) {
		WithPar = WithPar && mParRex->PartFound(5);

		if (!WithPar) {
			(*mOS) << _("Missing command parameters.");
			return false;
		}

		mParRex->Extract(5, mParStr, field);
		mParRex->Extract(6, mParStr, par);
	}

	cRegUserInfo ui;
	bool RegFound = mS->mR->FindRegInfo(ui, nick);

	if (RegFound && ((MyClass < eUC_MASTER) && !(((MyClass >= int(ui.mClass + mS->mC.classdif_reg)) && (MyClass >= ui.mClassProtect)) || ((Action == eAC_INFO) && (MyClass >= int(ui.mClass - 1)))))) { // check rights
		(*mOS) << _("You have no rights to do this.");
		return false;
	}

	int ParClass = eUC_REGUSER;

	if ((Action == eAC_CLASS) || (Action == eAC_PROTECT) || (Action == eAC_HIDEKICK) || (Action == eAC_NEW) || ((Action == eAC_SET) && (field == "class"))) { // convert par to class number
		std::istringstream lStringIS(par);
		lStringIS >> ParClass;

		if ((ParClass < eUC_PINGER) || (ParClass == eUC_NORMUSER) || ((ParClass > eUC_ADMIN) && (ParClass < eUC_MASTER)) || (ParClass > eUC_MASTER)) { // validate class number
			(*mOS) << _("You specified invalid class number.");
			return false;
		}
	}

	bool authorized = false;

	switch (Action) {
		case eAC_SET:
			authorized = (RegFound && (MyClass >= eUC_ADMIN) && (MyClass >= int(ui.mClass + mS->mC.classdif_reg)) && (field != "class"));
			break;

		case eAC_NEW:
			authorized = (!RegFound && (MyClass >= int(ParClass + mS->mC.classdif_reg)));
			break;

		case eAC_PASS:
		case eAC_HIDEKICK:
		case eAC_ENABLE:
		case eAC_DISABLE:
		case eAC_DEL:
			authorized = (RegFound && (MyClass >= int(ui.mClass + mS->mC.classdif_reg)));
			break;

		case eAC_CLASS:
			authorized = (RegFound && (MyClass >= int(ui.mClass + mS->mC.classdif_reg)) && (MyClass >= int(ParClass + mS->mC.classdif_reg)));
			break;

		case eAC_PROTECT:
			authorized = (RegFound && (MyClass >= int(ui.mClass + mS->mC.classdif_reg)) && (MyClass >= (ParClass + 1)));
			break;

		case eAC_INFO:
			authorized = (RegFound && (MyClass >= eUC_OPERATOR));
			break;
	}

	if (MyClass == eUC_MASTER)
		authorized = (RegFound || (!RegFound && (Action == eAC_NEW)));

	if (!authorized) {
		if (!RegFound && (Action != eAC_NEW))
			(*mOS) << autosprintf(_("Registered user not found: %s"), nick.c_str());
		else if (RegFound && (Action == eAC_NEW))
			(*mOS) << autosprintf(_("User is already registered: %s"), nick.c_str());
		else
			(*mOS) << _("You have no rights to do this.");

		return false;
	}

 	if ((Action >= eAC_CLASS) && (Action <= eAC_SET) && !WithPar) {
 		(*mOS) << _("Missing command parameters.");
 		return false;
	}

	string msg;
	ostringstream ostr;
	cUser *user = mS->mUserList.GetUserByNick(nick);

	switch (Action) {
		case eAC_NEW:
			if (RegFound) {
				(*mOS) << autosprintf(_("User is already registered: %s"), nick.c_str());
				return false;
			}

			#ifndef WITHOUT_PLUGINS
				if (!mS->mCallBacks.mOnNewReg.CallAll(this->mConn->mpUser, nick, ParClass)) {
					(*mOS) << _("Your action was blocked by a plugin.");
					return false;
				}
			#endif

			if (mS->mR->AddRegUser(nick, mConn, ParClass, (WithPass ? pass.c_str() : NULL))) {
				if (user && user->mxConn) {
					ostr.str("");

					if (!WithPass)
						ostr << autosprintf(_("You have been registered with class %d. Please set your password by using following command: +passwd <password>"), ParClass);
					else
						ostr << autosprintf(_("You have been registered with class %d and following password: %s"), ParClass, pass.c_str());

					mS->DCPrivateHS(ostr.str(), user->mxConn);
					/*
						todo
							use password request here aswell, send_pass_request
							no reconnect required
					*/
				}

				if (!WithPass)
					(*mOS) << autosprintf(_("%s has been registered with class %d. Please tell him to set his password."), nick.c_str(), ParClass);
				else
					(*mOS) << autosprintf(_("%s has been registered with class %d and password."), nick.c_str(), ParClass);
			} else {
				(*mOS) << _("Error adding new user.");
				return false;
			}

			break;

		case eAC_DEL:
			#ifndef WITHOUT_PLUGINS
				if (!mS->mCallBacks.mOnDelReg.CallAll(this->mConn->mpUser, nick, ui.mClass)) {
					(*mOS) << _("Your action was blocked by a plugin.");
					return false;
				}
			#endif

			if (mS->mR->DelReg(nick)) {
				ostr << ui;
				(*mOS) << autosprintf(_("%s has been unregistered, user information"), nick.c_str()) << ":\r\n" << ostr.str();
				return true;
			} else {
				ostr << ui;
				(*mOS) << autosprintf(_("Error unregistering %s, user information"), nick.c_str()) << ":\r\n" << ostr.str();
				return false;
			}

			break;

		case eAC_PASS:
			if (WithPar) {
				#ifndef _WIN32
				if (mS->mR->ChangePwd(nick, par, 1))
				#else
				if (mS->mR->ChangePwd(nick, par, 0))
				#endif
					(*mOS) << _("Password updated.");
				else {
					(*mOS) << _("Error updating password.");
					return false;
				}
			} else {
				field = "pwd_change";
				par = "1";
				ostr << _("You can change your password now. Use the +passwd command followed by your new password.");
			}

			break;

		case eAC_CLASS:
			#ifndef WITHOUT_PLUGINS
				if (!mS->mCallBacks.mOnUpdateClass.CallAll(this->mConn->mpUser, nick, ui.mClass, ParClass)) {
					(*mOS) << _("Your action was blocked by a plugin.");
					return false;
				}
			#endif

			if (user && user->mxConn) { // no need to reconnect for class to take effect
				if ((user->mClass < mS->mC.opchat_class) && (ParClass >= mS->mC.opchat_class)) { // opchat list
					if (!mS->mOpchatList.ContainsNick(user->mNick))
						mS->mOpchatList.Add(user);
				} else if ((user->mClass >= mS->mC.opchat_class) && (ParClass < mS->mC.opchat_class)) {
					if (mS->mOpchatList.ContainsNick(user->mNick))
						mS->mOpchatList.Remove(user);
				}

				if ((user->mClass < eUC_OPERATOR) && (ParClass >= eUC_OPERATOR)) { // oplist
					if (RegFound && !ui.mHideKeys) {
						mS->mP.Create_OpList(msg, user->mNick); // send short oplist
						mS->mUserList.SendToAll(msg, false, true); // todo: no cache, why?
						mS->mInProgresUsers.SendToAll(msg, false, true);

						if (!mS->mOpList.ContainsNick(user->mNick))
							mS->mOpList.Add(user);
					}
				} else if ((user->mClass >= eUC_OPERATOR) && (ParClass < eUC_OPERATOR)) {
					if (RegFound && !ui.mHideKeys) {
						if (mS->mOpList.ContainsNick(user->mNick))
							mS->mOpList.Remove(user);

						mS->mP.Create_Quit(msg, user->mNick); // send quit to all
						mS->mUserList.SendToAll(msg, false, true); // todo: no cache, why?
						mS->mInProgresUsers.SendToAll(msg, false, true);
						mS->mP.Create_Hello(msg, user->mNick); // send hello to hello users
						mS->mHelloUsers.SendToAll(msg, false, true);
						mS->mUserList.SendToAll(user->mMyINFO, false, true); // send myinfo to all
						mS->mInProgresUsers.SendToAll(user->mMyINFO, false, true); // todo: no cache, why?
					}
				}

				user->mClass = (tUserCl)ParClass;
			}

			field = "class";
			ostr << autosprintf(_("Your class has been changed to %s."), par.c_str());
			break;

		case eAC_ENABLE:
			field = "enabled";
			par = "1";
			WithPar = true;
			ostr << _("Your registration has been enabled.");
			break;

		case eAC_DISABLE:
			field = "enabled";
			par = "0";
			WithPar = true;
			ostr << _("Your registration has been disabled.");
			break;

		case eAC_PROTECT:
			field = "class_protect";
			ostr << autosprintf(_("You are protected from being kicked by class %s and lower."), par.c_str());

			if (user)
				user->mProtectFrom = ParClass;

			break;

		case eAC_HIDEKICK:
			field = "class_hidekick";
			ostr << autosprintf(_("You kicks are hidden from class %s and lower."), par.c_str());
			break;

		case eAC_SET:
			break;

		case eAC_INFO:
			(*mOS) << autosprintf(_("%s registration information"), ui.GetNick().c_str()) << ":\r\n" << ui;
			break;

		default:
			mIdRex->Extract(1, mIdStr, par);
			(*mOS) << _("This command is not implemented.");
			return false;
	}

	if ((WithPar && (Action >= eAC_ENABLE) && (Action <= eAC_SET)) || ((Action == eAC_PASS) && !WithPar)) {
		if (mS->mR->SetVar(nick, field, par)) {
			if ((field == "hide_share") && user && user->mxConn) { // change hidden share flag on the fly, also update total share
				if (user->mHideShare && (par == "0")) {
					user->mHideShare = false;
					mS->mTotalShare += user->mShare;

					if (ostr.str().empty())
						ostr << _("Your share is now visible.");
				} else if (!user->mHideShare && (par != "0")) { // it appears that only 0 means false, anything else means true
					user->mHideShare = true;
					mS->mTotalShare -= user->mShare;

					if (ostr.str().empty())
						ostr << _("Your share is now hidden.");
				}
			}

			(*mOS) << autosprintf(_("Updated variable %s to value %s for user: %s"), field.c_str(), par.c_str(), nick.c_str());

			if ((Action == eAC_PASS) && !WithPar && mS->mC.report_pass_reset)
				mS->ReportUserToOpchat(mConn, autosprintf(_("Password reset for user: %s"), nick.c_str()), false);

			if (user && user->mxConn && ostr.str().size()) { // send both in pm and mc, so user see the message for sure
				mS->DCPrivateHS(ostr.str(), user->mxConn);
				mS->DCPublicHS(ostr.str(), user->mxConn);
			}

			return true;
		} else {
			(*mOS) << autosprintf(_("Error setting variable %s to value %s for user: %s"), field.c_str(), par.c_str(), nick.c_str());
			return false;
		}
	}

	return true;
}

bool cDCConsole::cfRedirToConsole::operator()()
{
	if (!this->mConn || !this->mConn->mpUser)
		return false;

	if (this->mConn->mpUser->mClass < eUC_OPERATOR) {
		(*mOS) << _("You have no rights to do this.");
		return false;
	}

	if (this->mConsole) {
		if (mConsole->OpCommand(mIdStr + mParStr, mConn)) {
			return true;
		} else {
			(*mOS) << _("This command is not implemented.");
			return false;
		}
	} else {
		(*mOS) << _("Command console is not ready.");
		return false;
	}
}

bool cDCConsole::cfBc::operator()()
{
	enum { eBC_BC, eBC_OC, eBC_GUEST, eBC_REG, eBC_VIP, eBC_CHEEF, eBC_ADMIN, eBC_MASTER, eBC_CC };
	enum { eBC_ALL, eBC_MSG };
	const char *cmds[] = { "bc", "broadcast", "oc", "ops", "guests", "regs", "vips", "cheefs", "admins", "masters", "ccbc", "ccbroadcast" };
	const int nums[] = { eBC_BC, eBC_BC, eBC_OC, eBC_OC, eBC_GUEST, eBC_REG, eBC_VIP, eBC_CHEEF, eBC_ADMIN, eBC_MASTER, eBC_CC, eBC_CC };
	int cmdid;

	if (!GetIDEnum(1, cmdid, cmds, nums))
		return false;

	string message;
	GetParStr(eBC_MSG, message);

	// rights to broadcast
	int MinClass = mS->mC.min_class_bc;
	int MaxClass = eUC_MASTER;
	int MyClass = this->mConn->mpUser->mClass;
	int AllowedClass = eUC_MASTER;

	switch (cmdid) {
		case eBC_BC:
			MinClass = eUC_NORMUSER;
			MaxClass = eUC_MASTER;
			AllowedClass = mS->mC.min_class_bc;
			break;
		case eBC_GUEST:
			MinClass = eUC_NORMUSER;
			MaxClass = eUC_NORMUSER;
			AllowedClass = mS->mC.min_class_bc_guests;
			break;
		case eBC_REG:
			MinClass = eUC_REGUSER;
			MaxClass = eUC_REGUSER;
			AllowedClass = mS->mC.min_class_bc_regs;
			break;
		case eBC_VIP:
			MinClass = eUC_VIPUSER;
			MaxClass = eUC_VIPUSER;
			AllowedClass = mS->mC.min_class_bc_vips;
			break;
		case eBC_OC:
			MinClass = eUC_OPERATOR;
			MaxClass = eUC_MASTER;
			AllowedClass = eUC_OPERATOR;
			break;
		case eBC_CHEEF:
			MinClass = eUC_CHEEF;
			MaxClass = eUC_ADMIN;
			AllowedClass = eUC_OPERATOR;
			break;
		case eBC_ADMIN:
			MinClass = eUC_ADMIN;
			MaxClass = eUC_MASTER;
			AllowedClass = eUC_ADMIN;
			break;
		case eBC_MASTER:
			MinClass = eUC_MASTER;
			MaxClass = eUC_MASTER;
			AllowedClass = eUC_ADMIN;
			break;
		case eBC_CC:
			break;
		default:
			break;
	}

	if (MyClass < AllowedClass) {
		*mOS << _("You have no rights to do this.");
		return false;
	}

	string start, end;
	mS->mP.Create_PMForBroadcast(start, end, mS->mC.hub_security, mS->mC.hub_security, message); // this->mConn->mpUser->mNick
	cTime TimeBefore, TimeAfter;

	if (mS->LastBCNick != "disable")
		mS->LastBCNick = mConn->mpUser->mNick;

	int count = mS->SendToAllWithNickVars(start, end, MinClass, MaxClass);
	TimeAfter.Get();
	*mOS << autosprintf(ngettext("Message delivered to %d user in %s.", "Message delivered to %d users in %s.", count), count, (TimeAfter - TimeBefore).AsPeriod().AsString().c_str());
	return true;
}

bool cDCConsole::GetIPRange(const string &range, unsigned long &from, unsigned long &to)
{
	//"^(\\d+\\.\\d+\\.\\d+\\.\\d+)((\\/(\\d+))|(\\.\\.|-)(\\d+\\.\\d+\\.\\d+\\.\\d+))?$"
	enum {R_IP1 = 1, R_RANGE = 2, R_BITS=4, R_DOTS = 5, R_IP2 = 6};
	if(!mIPRangeRex.Exec(range))
		return false;
	string tmp;
	// easy : from..to
	if(mIPRangeRex.PartFound(R_RANGE)) {
		if(mIPRangeRex.PartFound(R_DOTS)) {
			mIPRangeRex.Extract(R_IP1, range, tmp);
			from = cBanList::Ip2Num(tmp);
			mIPRangeRex.Extract(R_IP2, range, tmp);
			to = cBanList::Ip2Num(tmp);
			return true;
		// the more complicated 1.2.3.4/16 style mask
		} else {
			mIPRangeRex.Extract(0, range, tmp);
			from = cBanList::Ip2Num(tmp);
			int i = tmp.find_first_of("/\\");
			istringstream is(tmp.substr(i+1));
			unsigned long mask = from;
			is >> i;
			unsigned long addr1 = mask & (0xFFFFFFFF << (32-i));
			unsigned long addr2 = addr1 + (0xFFFFFFFF >> i);
			from = addr1;
			to   = addr2;
			return true;
		}
	} else {
		mIPRangeRex.Extract(R_IP1, range, tmp);
		from = cBanList::Ip2Num(tmp);
		to = from;
		return true;
	}
	return false;
}

}; // namespace nVerliHub
