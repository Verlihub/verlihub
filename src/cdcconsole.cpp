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

cDCConsole::cDCConsole(cServerDC *s):
	cDCConsoleBase(s),
	mServer(s),
	mTriggers(NULL),
	mRedirects(NULL),
	mDCClients(NULL),
	mCmdr(this),
	mUserCmdr(this),
 	mCmdBan(int(eCM_BAN), ".(add|new|del|rm|un|info|list|ls|lst)?ban([^_\\s]+)?(_(\\d+\\S))?( this (nick|ip))? ?", "(\\S+)?( (.*)$)?", &mFunBan),
	mCmdTempBan(int(eCM_TEMPBAN), ".(info|del|un)?tempban(nick|ip)? ", "(\\S+)", &mFunTempBan),
	mCmdGag(int(eCM_GAG), ".(un)?(gag|nochats|nochat|nopm|noctm|nodl|nosearch|kvip|maykick|noshare|mayreg|mayopchat|noinfo|mayinfo|temprights) ?", "(\\S+)?( (\\d+\\w))?", &mFunGag),
	mCmdTrigger(int(eCM_TRIGGER),".(ft|trigger)(\\S+) ", "(\\S+)( (.*))?", &mFunTrigger),
	mCmdSetVar(int(eCM_SET),".(set|=) ", "(\\[(\\S+)\\] )?(\\S+) (.*)", &mFunSetVar),
	mCmdRegUsr(int(eCM_REG),".r(eg)?(n(ew)?(user)?|del(ete)?|pass(wd)?|(en|dis)able|(set)?class|(protect|hidekick)(class)?|set|=|info|list|lst) ", "(\\S+)( (((\\S+) )?(.*)))?", &mFunRegUsr),
	mCmdRaw(int(eCM_RAW), ".proto(\\S+)_(\\S+) ", "((\\S+) )?(.+)", &mFunRaw),
	mCmdCmd(int(eCM_CMD),".cmd(\\S+)","(.*)", &mFunCmd),
	mCmdWho(int(eCM_WHO), ".w(ho)?(\\S+) ", "(.*)", &mFunWho),
	mCmdKick(int(eCM_KICK), ".(kick|drop|vipgag|vipungag) ", "(\\S+)( (.*)$)?", &mFunKick), // eUR_KICK
	mCmdInfo(int(eCM_INFO), ".(hub|serv|server|sys|system|os|port|url|huburl|prot|proto|protocol|buf|buffer|mmdb|geoip)info ?", "(\\S+)?", &mFunInfo),
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

	mRedirects->mOldMap[0] = "\150\166\144\061\172\152\170\163\161\161\177\155\072\173\163\203\112\110\111\112\113\000";
	mRedirects->mOldMap[1] = "\167\147\165\160\156\156\174\152\066\172\175\173\167\163\162\204\077\201\205\173\000\000";

	mFunRedirConnType.mConsole = &mConnTypeConsole;
	mFunRedirTrigger.mConsole = mTriggerConsole;
	mFunCustomRedir.mConsole = mRedirectConsole;
	mFunDCClient.mConsole = mDCClientConsole;
	mCmdr.Add(&mCmdBan);
	mCmdr.Add(&mCmdTempBan);
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
	mUserCmdr.Add(&mCmdKick); // vip kicks
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

			if (cmdid == "update") {
				mOwner->CheckForUpdates(mOwner->mC.update_check_git, conn);
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

			//try {
				if (mCmdr.ParseAll(str, os, conn) >= 0) {
					mOwner->DCPublicHS(os.str().c_str(), conn);
					return 1;
				}
			/*
			} catch (const char *ex) {
				if(Log(0)) LogStream() << "Exception in command: " << ex << endl;
			} catch (...) {
				if(Log(0)) LogStream() << "Exception in command." << endl;
			}
			*/
			break;
		default:
			return 0;
			break;
	}

	if (mTriggers->DoCommand(conn, cmd, cmd_line, *mOwner))
		return 1;

	return 0;
}

int cDCConsole::UsrCommand(const string &str, cConnDC *conn)
{
	if (!conn || !conn->mpUser)
		return 0;

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
			/*
			if (cmdid == "kick")
				return CmdKick(cmd_line, conn);
			*/

		case eUC_NORMUSER:
			if ((cmdid == "passwd") || (cmdid == "password"))
				return CmdRegMyPasswd(cmd_line, conn);

			if (cmdid == "help")
				return CmdHelp(cmd_line, conn);

			if (cmdid == "myinfo")
				return CmdMyInfo(cmd_line, conn);

			if (cmdid == "myip")
				return CmdMyIp(cmd_line, conn);

			if (cmdid == "me")
				return CmdMe(cmd_line, conn);

			if ((cmdid == "regme") || (cmdid == "unregme"))
				return CmdRegMe(cmd_line, conn, (cmdid == "unregme"));

			if ((cmdid == "chat") || (cmdid == "nochat"))
				return CmdChat(cmd_line, conn, (cmdid == "chat"));

			if (cmdid == "info")
				return CmdUInfo(cmd_line, conn);

			if ((cmdid == "about") || (cmdid == "verlihub") || (cmdid == "vh") || (cmdid == "release"))
				return CmdRInfo(cmd_line, conn);

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
	}

	if (mTriggers->DoCommand(conn, cmd, cmd_line, *mOwner))
		return 1;

	return 0;
}

int cDCConsole::CmdCmds(istringstream &cmd_line, cConnDC *conn)
{
	ostringstream os;
	string omsg;
	os << _("Full list of commands") << ':';
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
		o << autosprintf(ngettext("Showing %d result", "Showing %d results", c), c) << ':' << os.str();

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
		o << autosprintf(ngettext("Showing %d result", "Showing %d results", c), c) << ':' << os.str();

	mOwner->DCPublicHS(o.str().c_str(), conn);
	return 1;
}

int cDCConsole::CmdGetinfo(istringstream &cmd_line, cConnDC *conn)
{
	ostringstream os, out;
	string nick, cc, cn, ci;
	cUser *user;
	unsigned int tot = 0;

	while (cmd_line.good()) {
		cmd_line >> nick;

		if (cmd_line.fail())
			break;

		user = mOwner->mUserList.GetUserByNick(nick);

		if (user && user->mxConn) {
			cc = user->mxConn->GetGeoCC();
			cn = user->mxConn->GetGeoCN();
			ci = user->mxConn->GetGeoCI();

			if (!mOwner->mUseDNS)
				user->mxConn->DNSLookup();

			os << "\r\n [*] " << _("User") << ": " << user->mNick.c_str();
			os << " ][ " << _("IP") << ": " << user->mxConn->AddrIP().c_str();
			os << " ][ " << _("Country") << ": " << cc.c_str() << '=' << cn.c_str();
			os << " ][ " << _("City") << ": " << ci.c_str();

			if (!user->mxConn->AddrHost().empty())
				os << " ][ " << _("Host") << ": " << user->mxConn->AddrHost().c_str();
		} else {
			os << "\r\n [*] " << autosprintf(_("User not found: %s"), nick.c_str());
		}

		tot++;
	}

	if (tot == 0)
		out << _("Please specify one or more nicks separated by space.");
	else
		out << autosprintf(ngettext("Showing %d result", "Showing %d results", tot), tot) << ':' << os.str();

	mOwner->DCPublicHS(out.str().c_str(), conn);
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
			os << autosprintf (_("Please note, hub has been scheduled to restart in: %s"), cTimePrint((long)delay, 0).AsPeriod().AsString().c_str());
		else
			os << _("Please note, hub will be restarted now.");
	} else { // quit
		if (delay == -1)
			os << _("Please note, hub is no longer scheduled for stop.");
		else if (delay)
			os << autosprintf (_("Please note, hub has been scheduled to stop in: %s"), cTimePrint((long)delay, 0).AsPeriod().AsString().c_str());
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
	cTimePrint TimeBefore, TimeAfter;

	if (mOwner->LastBCNick != "disable")
		mOwner->LastBCNick = conn->mpUser->mNick;

	unsigned int count = mOwner->SendToAllWithNickCCVars(start, end, cl_min, cl_max, cc_zone);
	TimeAfter = mOwner->mTime;
	ostr << autosprintf(ngettext("Message delivered to %d user in zones %s in %s.", "Message delivered to %d users in zones %s in %s.", count),
	     count, cc_zone.c_str(), cTimePrint(TimeAfter - TimeBefore).AsPeriod().AsString().c_str());
	mOwner->DCPublicHS(ostr.str(), conn);
	return 1;
}

int cDCConsole::CmdMyInfo(istringstream &cmd_line, cConnDC *conn)
{
	ostringstream os;
	os << _("Your information") << ":\r\n";
	conn->mpUser->DisplayInfo(os);
	os << "\r\n";
	conn->mpUser->DisplayRightsInfo(os);
	mOwner->DCPublicHS(os.str(), conn);
	return 1;
}

int cDCConsole::CmdMyIp(istringstream &cmd_line, cConnDC *conn)
{
	if (!conn || !conn->mpUser)
		return 0;

	ostringstream os;
	string cc = conn->GetGeoCC(), cn = conn->GetGeoCN(), ci = conn->GetGeoCI();
	os << _("Your IP information") << ":\r\n\r\n";
	os << " [*] " << _("IP") << ": " << conn->AddrIP().c_str() << "\r\n";

	if (conn->AddrHost().size())
		os << " [*] " << autosprintf(_("Host: %s"), conn->AddrHost().c_str()) << "\r\n";

	os << " [*] " << _("Country") << ": " << cc.c_str() << '=' << cn.c_str() << "\r\n";
	os << " [*] " << _("City") << ": " << ci.c_str() << "\r\n";

	if (mOwner->mTLSProxy.size()) // client to hub tls
		os << " [*] " << autosprintf(_("Hub TLS: %s"), ((conn->mTLSVer.size() && (conn->mTLSVer != "0.0")) ? conn->mTLSVer.c_str() : _("No"))) << "\r\n";

	os << " [*] " << autosprintf(_("Client TLS: %s"), (conn->mpUser->GetMyFlag(eMF_TLS) ? _("Yes") : _("No"))) << "\r\n"; // client to client tls
	os << " [*] " << autosprintf(_("Client NAT: %s"), (conn->mpUser->GetMyFlag(eMF_NAT) ? _("Yes") : _("No"))) << "\r\n"; // client to client nat
	mOwner->DCPublicHS(os.str(), conn);
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
		mOwner->DCPublicHS(_("Main chat is currently disabled for users with your class, please consider registering on the hub or contact an operator."), conn);
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

	mOwner->mP.Create_Chat(temp, conn->mpUser->mNick, text);
	cUser::tFloodHashType Hash = 0;
	Hash = tHashArray<void*>::HashString(temp);

	if (Hash && (conn->mpUser->mClass < eUC_OPERATOR) && (Hash == conn->mpUser->mFloodHashes[eFH_CHAT])) { // check for flood as if it was regular mainchat message
		mOwner->DCPublicHS(_("Your message wasn't sent because it equals your previous message."), conn);
		return 1;
	}

	conn->mpUser->mFloodHashes[eFH_CHAT] = Hash;

	if ((conn->mpUser->mClass < eUC_VIPUSER) && !cDCProto::CheckChatMsg(text, conn)) // check message length and other conditions
		return 1;

	mOwner->mP.Create_Me(temp, conn->mpUser->mNick, text);
	mOwner->mUserList.SendToAll(temp, mOwner->mC.delayed_chat, true);
	return 1;
}

int cDCConsole::CmdChat(istringstream &cmd_line, cConnDC *conn, bool switchon)
{
	// note: here we dont set user->mHideChat because user might want to temporarily enable chat

	if (!conn || !conn->mpUser)
		return 0;

	if (switchon) { // chat
		if (!mOwner->mChatUsers.ContainsHash(conn->mpUser->mNickHash)) {
			mOwner->DCPublicHS(_("Public chat messages are now visible. To hide them, write: +nochat"), conn);
			mOwner->mChatUsers.AddWithHash(conn->mpUser, conn->mpUser->mNickHash);
		} else {
			mOwner->DCPublicHS(_("Public chat messages are already visible. To hide them, write: +nochat"), conn);
		}

	} else { // nochat
		if (mOwner->mChatUsers.ContainsHash(conn->mpUser->mNickHash)) {
			mOwner->DCPublicHS(_("Public chat messages are now hidden. To show them, write: +chat"), conn);
			mOwner->mChatUsers.RemoveByHash(conn->mpUser->mNickHash);
		} else {
			mOwner->DCPublicHS(_("Public chat messages are already hidden. To show them, write: +chat"), conn);
		}
	}

	return 1;
}

int cDCConsole::CmdRInfo(istringstream &cmd_line, cConnDC *conn)
{
	if (!conn || !conn->mpUser)
		return 0;

	ostringstream os;

	os << autosprintf(_("Software: %s %s"), HUB_VERSION_NAME, HUB_VERSION_VERS) << "\r\n\r\n";
	os << " [*] " << _("Authors") << ":\r\n\r\n";
	os << "\tVerliba, Daniel Muller, dan at verliba dot cz\r\n";
	os << "\tRoLex, Aleksandr Zenkov, webmaster at feardc dot net\r\n";
	os << "\tFlylinkDC-dev, Pavel Pimenov, pavel dot pimenov at gmail dot com\r\n";
	os << "\tDexo, Denys Smirnov, dexo at verlihub dot net\r\n";
	os << "\tShurik, Aleksandr Zeinalov, shurik at sbin dot ru\r\n";
	os << "\tVovochka, Vladimir Perepechin, vovochka13 at gmail dot com\r\n";
	os << "\t" << _("Not forgetting other 4 people who didn't want to be listed here.") << "\r\n\r\n";
	os << " [*] " << _("Translators") << ":\r\n\r\n";
	os << "\tCzech (Uhlik), Italian (netcelli, Stefano, DiegoZ), Russian (plugman, MaxFox, RoLex, KCAHDEP),\r\n";
	os << "\tSlovak (uNix), Romanian (WiZaRd, S0RiN, DANNY05), Polish (Zorro, Krzychu, Frog, PWiAM), German (Ettore Atalan),\r\n";
	os << "\tSwedish (RoLex), Bulgarian (Boris Stefanov), Hungarian (Oszkar Ocsenas), Turkish (mauron),\r\n";
	os << "\tFrench (@tlantide), Dutch (Modswat), Lithuanian (Trumpy, Deivis), Spanish (Raytrax), Chinese (kinosang),\r\n";
	os << "\tUkrainian (Master)\r\n\r\n";
	os << "\t" << _("Translate") << ": https://transifex.com/feardc/verlihub/\r\n\r\n";
	os << " [*] " << _("Contributors") << ":\r\n\r\n";
	os << "\tFrog, Eco-Logical, Intruder, PWiAM" << "\r\n\r\n";
	os << " [*] " << _("Credits") << ":\r\n\r\n";
	os << "\t" << _("We would like to thank everyone in our support hub for their input and valuable support and of course everyone who continues to use this great hub software.") << "\r\n\r\n";
	os << " [*] " << _("Links") << ":\r\n\r\n";
	os << "\t" << _("Website") << ": https://github.com/verlihub/\r\n";
	os << "\t" << "Git" << ": https://github.com/verlihub/verlihub/\r\n";
	os << "\t" << _("Wiki") << ": https://github.com/verlihub/verlihub/wiki/\r\n";
	os << "\t" << _("Crash") << ": https://crash.verlihub.net/\r\n";
	os << "\t" << _("Translate") << ": https://transifex.com/feardc/verlihub/\r\n";
	os << "\t" << "Ledokol" << ": https://ledo.feardc.net/\r\n";
	os << "\t" << "MMDB" << ": https://ledo.feardc.net/mmdb/\r\n";
	os << "\t" << _("Proxy") << ": https://github.com/verlihub/tls-proxy/\r\n";
	os << "\t" << _("Hublist") << ": https://te-home.net/?do=hublist\r\n";
	os << "\t" << _("Support") << ": nmdcs://hub.verlihub.net:7777\r\n\r\n";
	os << " [*] " << _("Donation") << ":\r\n\r\n";
	os << "\tPayPal: https://www.paypal.com/paypalme/feardc/\r\n";
	os << "\t" << _("Donators") << ": Deivis (100 EUR), Captain (30 EUR)\r\n";

	mOwner->DCPublicHS(os.str(), conn);
	return 1;
}

int cDCConsole::CmdUInfo(istringstream &cmd_line, cConnDC *conn)
{
	if (!conn || !conn->mpUser)
		return 0;

	ostringstream os;
	string temp, host = mOwner->mC.hub_host, port = StringFrom(mOwner->mPort);
	size_t pos = string::npos;
	int ucl = conn->GetTheoricalClass();
	unsigned int sear = 0, contot = 0, consec = 0;
	cAsyncConn *usco;

	for (cUserCollection::iterator it = mOwner->mUserList.begin(); it != mOwner->mUserList.end(); ++it) {
		usco = ((cUser*)(*it))->mxConn;

		if (usco && usco->ok) {
			contot++;

			if (usco->mTLSVer.size() && (usco->mTLSVer != "0.0"))
				consec++;
		}
	}

	os << _("Hub information") << ":\r\n\r\n";

	os << " [*] " << autosprintf(_("Owner: %s"), (mOwner->mC.hub_owner.size() ? mOwner->mC.hub_owner.c_str() : _("Not set"))) << "\r\n";

	if (host.size() > 2) { // remove protocol
		pos = host.find("://");

		if (pos != host.npos)
			host.erase(0, pos + 3);
	}

	if (host.size()) {
		pos = host.find(':');

		if ((pos == host.npos) && (mOwner->mPort != 411)) { // add port
			host.append(':' + port);
			pos = 1; // note below
		}

		temp = "dchub://";
		temp.append(host);
	}

	os << " [*] " << autosprintf(_("Address: %s"), (temp.size() ? temp.c_str() : _("Not set"))) << "\r\n";

	if (mOwner->mTLSProxy.size() && host.size()) {
		temp = "nmdcs://";
		temp.append(host);

		if (pos == host.npos) // always add port for nmdcs
			temp.append(':' + port);

		os << " [*] " << autosprintf(_("Secure: %s"), temp.c_str()) << "\r\n";
	}

	os << " [*] " << autosprintf(_("Status: %s"), mOwner->SysLoadName()) << "\r\n";
	os << " [*] " << autosprintf(_("Users: %d of %d"), mOwner->mUserCountTot, mOwner->mC.max_users_total) << "\r\n";
	os << " [*] " << autosprintf(_("Secure: %d of %d"), consec, contot) << "\r\n";
	os << " [*] " << autosprintf(_("Active: %d"), mOwner->mActiveUsers.Size()) << "\r\n";
	os << " [*] " << autosprintf(_("Passive: %d"), mOwner->mPassiveUsers.Size()) << "\r\n";
	os << " [*] " << autosprintf(_("Bots: %d"), mOwner->mRobotList.Size()) << "\r\n";
	os << " [*] " << autosprintf(_("Share: %s"), convertByte(mOwner->mTotalShare).c_str()) << "\r\n\r\n";

	os << ' ' << _("Your information") << ":\r\n\r\n";

	os << " [*] " << autosprintf(_("Class: %d [%s]"), ucl, mOwner->UserClassName(nEnums::tUserCl(ucl))) << "\r\n";

	if (ucl == eUC_NORMUSER) {
		if (conn->mpUser->mPassive)
			sear = mOwner->mC.int_search_pas;
		else
			sear = mOwner->mC.int_search;
	} else if (ucl == eUC_REGUSER) {
		if (conn->mpUser->mPassive)
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
	os << " [*] " << autosprintf(_("Mode: %s"), (conn->mpUser->mPassive ? _("Passive") : _("Active"))) << "\r\n";
	os << " [*] " << autosprintf(_("Hub TLS: %s"), ((conn->mTLSVer.size() && (conn->mTLSVer != "0.0")) ? conn->mTLSVer.c_str() : _("No"))) << "\r\n";
	os << " [*] " << autosprintf(_("Client TLS: %s"), (conn->mpUser->GetMyFlag(eMF_TLS) ? _("Yes") : _("No"))) << "\r\n";
	os << " [*] " << autosprintf(_("Client NAT: %s"), (conn->mpUser->GetMyFlag(eMF_NAT) ? _("Yes") : _("No"))) << "\r\n";
	os << " [*] " << autosprintf(_("Share: %s"), convertByte(conn->mpUser->mShare).c_str()) << "\r\n";
	os << " [*] " << autosprintf(_("Hidden: %s"), (conn->mpUser->mHideShare ? _("Yes") : _("No"))) << "\r\n";
	os << " [*] " << autosprintf(_("Chat: %s"), (conn->mpUser->mHideChat ? _("Yes") : _("No"))) << "\r\n";
	mOwner->DCPublicHS(os.str(), conn);
	return 1;
}

int cDCConsole::CmdRegMe(istringstream &cmd_line, cConnDC *conn, bool unreg)
{
	if (!conn || !conn->mpUser)
		return 0;

	if (mOwner->mC.disable_regme_cmd) {
		mOwner->DCPublicHS(_("This functionality is currently disabled."), conn);
		return 1;
	}

	if (mOwner->mC.autoreg_class > eUC_OPERATOR) { // low class operators can also be autoregistered
		mOwner->DCPublicHS(_("Registration failed. Please contact an operator for help."), conn);
		return 1;
	}

	cRegUserInfo ui;
	bool found = mOwner->mR->FindRegInfo(ui, conn->mpUser->mNick);

	if (found && !unreg) {
		mOwner->DCPublicHS(_("You are already registered."), conn);
		return 1;
	} else if (!found && unreg) {
		mOwner->DCPublicHS(_("You are not registered yet."), conn);
		return 1;
	}

	string data;

	if (unreg) { // user wants to unregister
		if ((mOwner->mC.autounreg_class > 0) && (ui.mClass <= mOwner->mC.autounreg_class)) { // automatic unregistration is enabled
			/*
				plugin should compare both nicks to see if user is unregistering himself which equals automatic unregistration
				plugin should also send message back to user if action is discarded because hub will not send anything
			*/

			#ifndef WITHOUT_PLUGINS
				if (!mOwner->mCallBacks.mOnDelReg.CallAll(conn->mpUser, conn->mpUser->mNick, ui.mClass))
					return 1;
			#endif

			if (mOwner->mR->DelReg(conn->mpUser->mNick)) { // no need to reconnect for class to take effect
				mOwner->DCPrivateHS(_("You have been unregistered."), conn);
				mOwner->DCPublicHS(_("You have been unregistered."), conn);

				if (mOwner->mOpchatList.ContainsHash(conn->mpUser->mNickHash)) // opchat list
					mOwner->mOpchatList.RemoveByHash(conn->mpUser->mNickHash);

				if (mOwner->mOpList.ContainsHash(conn->mpUser->mNickHash)) { // oplist, only if user is there
					mOwner->mOpList.RemoveByHash(conn->mpUser->mNickHash);

					mOwner->mP.Create_Quit(data, conn->mpUser->mNick); // send quit to all
					mOwner->mUserList.SendToAll(data, mOwner->mC.delayed_myinfo, true);

					if (mOwner->mC.myinfo_tls_filter && conn->mpUser->GetMyFlag(eMF_TLS)) { // myinfo tls filter
						data = conn->mpUser->mFakeMyINFO;
						mOwner->mUserList.SendToAllWithMyFlag(data, eMF_TLS, mOwner->mC.delayed_myinfo, true);
						mOwner->RemoveMyINFOFlag(data, conn->mpUser->mFakeMyINFO, eMF_TLS);
						mOwner->mUserList.SendToAllWithoutMyFlag(data, eMF_TLS, mOwner->mC.delayed_myinfo, true);

					} else {
						data = conn->mpUser->mFakeMyINFO;
						mOwner->mUserList.SendToAll(data, mOwner->mC.delayed_myinfo, true); // send myinfo to all
					}

					mOwner->ShowUserIP(conn); // send userip to operators
				}

				conn->mpUser->mClass = eUC_NORMUSER;

				if (conn->mpUser->mHideShare) { // recalculate total share
					conn->mpUser->mHideShare = false;
					mOwner->mTotalShare += conn->mpUser->mShare;
				}

				mOwner->SetUserRegInfo(conn, conn->mpUser->mNick); // update registration information in real time aswell
				mOwner->ReportUserToOpchat(conn, _("User has been unregistered"), false);
				return 1;
			} else {
				mOwner->DCPublicHS(_("An error occured while unregistering."), conn);
				return 0;
			}
		} else { // disabled or user dont apply, send message to operators instead
			//mOwner->DCPublicHS(_("You are not allowed to unregister yourself."), conn);
			mOwner->ReportUserToOpchat(conn, _("Unregistration request"), mOwner->mC.dest_regme_chat);
			mOwner->DCPublicHS(_("Thank you, your unregistration request has been sent to operators. Please await an answer."), conn);
			return 1;
		}
	}

	string pass;
	ostringstream os;

	if (mOwner->mC.autoreg_class > 0) { // automatic registration is enabled
		string pref = mOwner->mC.nick_prefix_autoreg;

		if (pref.size()) {
			if (pref.find("%[CC]") != pref.npos) { // only if found
				data = conn->GetGeoCC();
				ReplaceVarInString(pref, "CC", pref, data);
			}

			if (StrCompare(conn->mpUser->mNick, 0, pref.size(), pref) != 0) {
				os << autosprintf(_("Your nick must start with: %s"), pref.c_str());
				mOwner->DCPublicHS(os.str(), conn);
				return 1;
			}
		}

		__int64 mish, ussh = conn->mpUser->mShare / (1024 * 1024);

		switch (mOwner->mC.autoreg_class) {
			case eUC_OPERATOR:
				mish = mOwner->mC.min_share_ops;
				break;

			case eUC_VIPUSER:
				mish = mOwner->mC.min_share_vip;
				break;

			default:
				mish = mOwner->mC.min_share_reg;
				break;
		}

		if (ussh < mish) {
			os << autosprintf(_("You need to share at least: %s"), convertByte(mish * 1024).c_str());
			mOwner->DCPublicHS(os.str(), conn);
			return 1;
		}

		getline(cmd_line, pass);

		if (pass.size())
			pass = pass.substr(1); // strip space

		if (pass.size() < mOwner->mC.password_min_len) {
			os << autosprintf(_("Minimum password length is %d characters. Please retry."), mOwner->mC.password_min_len);
			mOwner->DCPublicHS(os.str(), conn);
			return 1;
		}

		#ifndef WITHOUT_PLUGINS
			/*
				plugin should compare both nicks to see if user is registering himself which equals automatic registration
				plugin should also send message back to user if action is discarded because hub will not send anything
			*/

			if (!mOwner->mCallBacks.mOnNewReg.CallAll(conn->mpUser, conn->mpUser->mNick, mOwner->mC.autoreg_class))
				return 1;
		#endif

		if (mOwner->mR->AddRegUser(conn->mpUser->mNick, NULL, mOwner->mC.autoreg_class, pass.c_str())) { // no need to reconnect for class to take effect
			os << autosprintf(_("You have been registered with class %d and following password: %s"), mOwner->mC.autoreg_class, pass.c_str());
			mOwner->DCPrivateHS(os.str(), conn);
			mOwner->DCPublicHS(os.str(), conn);

			if ((mOwner->mC.autoreg_class >= mOwner->mC.opchat_class) && !mOwner->mOpchatList.ContainsHash(conn->mpUser->mNickHash)) // opchat list
				mOwner->mOpchatList.AddWithHash(conn->mpUser, conn->mpUser->mNickHash);

			if ((mOwner->mC.autoreg_class >= mOwner->mC.oplist_class) && !mOwner->mOpList.ContainsHash(conn->mpUser->mNickHash)) { // oplist
				mOwner->mOpList.AddWithHash(conn->mpUser, conn->mpUser->mNickHash);

				mOwner->mP.Create_OpList(data, conn->mpUser->mNick); // send short oplist
				mOwner->mUserList.SendToAll(data, mOwner->mC.delayed_myinfo, true);
			}

			conn->mpUser->mClass = tUserCl(mOwner->mC.autoreg_class);
			mOwner->SetUserRegInfo(conn, conn->mpUser->mNick); // update registration information in real time aswell
			os.str("");
			os << autosprintf(_("New user has been registered with class %d"), mOwner->mC.autoreg_class);
			mOwner->ReportUserToOpchat(conn, os.str(), false);
			return 1;
		} else {
			mOwner->DCPublicHS(_("An error occured while registering."), conn);
			return 0;
		}
	}

	getline(cmd_line, pass);

	if (pass.size())
		pass = pass.substr(1); // strip space

	if (pass.size()) // todo: make password_min_len usable here?
		os << autosprintf(_("Registration request with password: %s"), pass.c_str());
	else
		os << _("Registration request without password");

	mOwner->ReportUserToOpchat(conn, os.str(), mOwner->mC.dest_regme_chat);
	mOwner->DCPublicHS(_("Thank you, your registration request has been sent to operators. Please await an answer."), conn);
	return 1;
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
		mOwner->mP.Create_HubName(omsg, mOwner->mC.hub_name, topic);
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

/*
int cDCConsole::CmdKick(istringstream &cmd_line, cConnDC *conn)
{
	if (!conn || !conn->mpUser)
		return 0;

	if (!conn->mpUser->Can(eUR_KICK, mOwner->mTime.Sec())) {
		mOwner->DCPublicHS(_("You have no rights to kick anyone."), conn);
		return 1;
	}

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
*/

int cDCConsole::CmdRegMyPasswd(istringstream &cmd_line, cConnDC *conn)
{
	if (!conn || !conn->mpUser)
		return 0;

	cRegUserInfo ui;

	if (!mOwner->mR->FindRegInfo(ui, conn->mpUser->mNick))
		return 0;

	ostringstream ostr;
	string str;

	if (mOwner->mC.max_class_self_repass && (conn->mpUser->mClass >= int(mOwner->mC.max_class_self_repass)))
		ui.mPwdChange = true;

	if (!ui.mPwdChange) {
		ostr << _("You are not allowed to change your password now. Ask an operator for help.");
		mOwner->DCPrivateHS(ostr.str(), conn);
		mOwner->DCPublicHS(ostr.str(), conn);
		return 1;
	}

	cmd_line >> str;

	if (str.size() < mOwner->mC.password_min_len) {
		ostr << autosprintf(_("Minimum password length is %d characters. Please retry."), mOwner->mC.password_min_len);
		mOwner->DCPrivateHS(ostr.str(), conn);
		mOwner->DCPublicHS(ostr.str(), conn);
		return 1;
	}

	if (!mOwner->mR->ChangePwd(conn->mpUser->mNick, str, conn)) {
		ostr << _("Error updating password.");
		mOwner->DCPrivateHS(ostr.str(), conn);
		mOwner->DCPublicHS(ostr.str(), conn);
		return 1;
	}

	conn->mpUser->mClass = tUserCl(ui.getClass());

	if ((conn->mpUser->mClass >= mOwner->mC.opchat_class) && !mOwner->mOpchatList.ContainsHash(conn->mpUser->mNickHash)) // opchat list
		mOwner->mOpchatList.AddWithHash(conn->mpUser, conn->mpUser->mNickHash);

	if ((conn->mpUser->mClass >= mOwner->mC.oplist_class) && !mOwner->mOpList.ContainsHash(conn->mpUser->mNickHash)) { // oplist
		mOwner->mOpList.AddWithHash(conn->mpUser, conn->mpUser->mNickHash);
		mOwner->mP.Create_OpList(str, conn->mpUser->mNick); // send short oplist
		mOwner->mUserList.SendToAll(str, mOwner->mC.delayed_myinfo, true);
	}

	mOwner->SetUserRegInfo(conn, conn->mpUser->mNick); // update registration information in real time aswell
	ostr << _("Password updated successfully.");
	mOwner->DCPrivateHS(ostr.str(), conn);
	mOwner->DCPublicHS(ostr.str(), conn);
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

	cInterpolExp *fn = new cInterpolExp(mOwner->mC.max_users_total, maximum, (60 * minutes) / mOwner->timer_serv_period, (60 * minutes) / mOwner->timer_serv_period); // 60 steps at most

	/* todo: use try instead
	if (fn) {
	*/
		mOwner->mTmpFunc.push_back((cTempFunctionBase*)fn);
		ostr << autosprintf(ngettext("Updating max_users variable to %d for the duration of %d minute.", "Updating max_users variable to %d for the duration of %d minutes.", minutes), maximum, minutes);
	/*
	} else {
		ostr << autosprintf(ngettext("Failed to update max_users variable to %d for the duration of %d minute.", "Failed to update max_users variable to %d for the duration of %d minutes.", minutes), maximum, minutes);
	}
	*/

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
	int new_class = eUC_OPERATOR, old_class = eUC_NORMUSER, op_class = conn->mpUser->mClass;
	int max_allowed_class = (op_class > eUC_ADMIN ? eUC_ADMIN : op_class - eUC_REGUSER);
	cmd_line >> nick >> new_class;

	if (nick.empty() || (new_class < eUC_NORMUSER) || (new_class > max_allowed_class)) {
		os << autosprintf(_("Command usage: !class <nick> [class=%d]"), eUC_OPERATOR) << " (" << autosprintf(_("maximum allowed class is %d"), max_allowed_class) << ')';
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
				if (!mOwner->mOpchatList.ContainsHash(user->mNickHash))
					mOwner->mOpchatList.AddWithHash(user, user->mNickHash);

			} else if ((old_class >= mOwner->mC.opchat_class) && (new_class < mOwner->mC.opchat_class)) {
				if (mOwner->mOpchatList.ContainsHash(user->mNickHash))
					mOwner->mOpchatList.RemoveByHash(user->mNickHash);
			}

			string msg;

			if (user->mxConn->mRegInfo) {
				if ((old_class < mOwner->mC.oplist_class) && (new_class >= mOwner->mC.oplist_class)) { // oplist
					if (!user->mxConn->mRegInfo->mHideKeys && !mOwner->mOpList.ContainsHash(user->mNickHash)) {
						mOwner->mOpList.AddWithHash(user, user->mNickHash);

						mOwner->mP.Create_OpList(msg, user->mNick); // send short oplist
						mOwner->mUserList.SendToAll(msg, mOwner->mC.delayed_myinfo, true);
					}

				} else if ((old_class >= mOwner->mC.oplist_class) && (new_class < mOwner->mC.oplist_class)) {
					if (!user->mxConn->mRegInfo->mHideKeys && mOwner->mOpList.ContainsHash(user->mNickHash)) {
						mOwner->mOpList.RemoveByHash(user->mNickHash);

						mOwner->mP.Create_Quit(msg, user->mNick); // send quit to all
						mOwner->mUserList.SendToAll(msg, mOwner->mC.delayed_myinfo, true);

						if (mOwner->mC.myinfo_tls_filter && user->GetMyFlag(eMF_TLS)) { // myinfo tls filter
							msg = user->mFakeMyINFO;
							mOwner->mUserList.SendToAllWithMyFlag(msg, eMF_TLS, mOwner->mC.delayed_myinfo, true);
							mOwner->RemoveMyINFOFlag(msg, user->mFakeMyINFO, eMF_TLS);
							mOwner->mUserList.SendToAllWithoutMyFlag(msg, eMF_TLS, mOwner->mC.delayed_myinfo, true);

						} else {
							msg = user->mFakeMyINFO;
							mOwner->mUserList.SendToAll(msg, mOwner->mC.delayed_myinfo, true); // send myinfo to all
						}

						mOwner->ShowUserIP(user->mxConn); // send userip to operators
					}
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
		os << _("Use !protect <nickname> <class>.") << ' ' << autosprintf(_("Max class is %d"), mclass-1) << endl;
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
	mOwner->ReloadNow();
	mOwner->DCPublicHS(_("Done reloading all lists and databases."), conn);
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
	if (!mConn || !mConn->mpUser)
		return false;

	if (mConn->mpUser->mClass < eUC_ADMIN) {
		(*mOS) << _("You have no rights to do this.");
		return false;
	}

	enum {
		eRW_ALL,
		eRW_USR,
		eRW_PASSIVE,
		eRW_ACTIVE
	};

	static const char *actionnames[] = {
		"all",
		"user", "usr",
		"passive", "pas",
		"active", "act"
	};

	static const int actionids[] = {
		eRW_ALL,
		eRW_USR, eRW_USR,
		eRW_PASSIVE, eRW_PASSIVE,
		eRW_ACTIVE, eRW_ACTIVE
	};

	enum {
		eRC_HUBNAME,
		eRC_QUIT,
		eRC_REDIR,
		eRC_PM,
		eRC_CHAT
	};

	static const char *cmdnames[] = {
		"hubname", "name",
		"quit",
		"redir", "move",
		"pm",
		"chat", "mc"
	};

	static const int cmdids[] = {
		eRC_HUBNAME, eRC_HUBNAME,
		eRC_QUIT,
		eRC_REDIR, eRC_REDIR,
		eRC_PM,
		eRC_CHAT, eRC_CHAT
	};

	string par;
	mIdRex->Extract(1, mIdStr, par);
	int act = this->StringToIntFromList(par, actionnames, actionids, sizeof(actionnames) / sizeof(char*));

	if (act < 0)
		return false;

	mIdRex->Extract(2, mIdStr, par);
	int id = this->StringToIntFromList(par, cmdnames, cmdids, sizeof(cmdnames) / sizeof(char*));

	if (id < 0)
		return false;

	string nick;

	if (act == eRW_USR) { // get nick
		if (mParRex->PartFound(2)) {
			GetParStr(2, nick);
		} else {
			(*mOS) << _("Missing command parameters.");
			return true;
		}
	}

	string cmd, end;
	GetParStr(3, par);

	switch (id) {
		case eRC_HUBNAME:
			mS->mP.Create_HubName(cmd, par, ""); // topic is included in param
			break;

		case eRC_QUIT:
			mS->mP.Create_Quit(cmd, par);
			break;

		case eRC_REDIR:
			mS->mP.Create_ForceMove(cmd, par, true);
			break;

		case eRC_PM:
			mS->mP.Create_PMForBroadcast(cmd, end, mS->mC.hub_security, mConn->mpUser->mNick, par);
			break;

		case eRC_CHAT:
			mS->mP.Create_Chat(cmd, mConn->mpUser->mNick, par);
			break;

		default:
			return false;
	}

	cUser *user = NULL;

	switch (act) {
		case eRW_ALL:
		case eRW_PASSIVE:
		case eRW_ACTIVE:
			if (id == eRC_PM)
				mS->mUserList.SendToAllWithNick(cmd, end);
			else
				mS->mUserList.SendToAll(cmd, false, true);

			break;

		case eRW_USR:
			user = mS->mUserList.GetUserByNick(nick);

			if (user && user->mxConn) {
				if (id == eRC_PM) {
					cmd.append(nick);
					cmd.append(end);
				}

				user->mxConn->Send(cmd, true);
			} else {
				(*mOS) << autosprintf(_("User not found: %s"), nick.c_str());
				return true;
			}

			break;

		default:
			return false;
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

	static const char *clna[] = {
		"banlist",
		"tempbans",
		"unbanlist",
		"kicklist",
		"temprights",
		"allban"
	};

	enum {
		CLEAN_BAN,
		CLEAN_TBAN,
		CLEAN_UNBAN,
		CLEAN_KICK,
		CLEAN_TRIGHTS,
		CLEAN_ALLBAN
	};

	static const int clid[] = {
		CLEAN_BAN,
		CLEAN_TBAN,
		CLEAN_UNBAN,
		CLEAN_KICK,
		CLEAN_TRIGHTS,
		CLEAN_ALLBAN
	};

	string temp;
	int act = eBF_NICKIP;

	if (mIdRex->PartFound(1)) {
		mIdRex->Extract(1, mIdStr, temp);
		act = this->StringToIntFromList(temp, clna, clid, sizeof(clna) / sizeof(char*));

		if (act < 0)
			return false;
	}

	switch (act) {
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

		case CLEAN_ALLBAN:
			mS->mBanList->TruncateTable();
			mS->mBanList->RemoveOldShortTempBans(0);
			mS->mUnBanList->TruncateTable();
			mS->mKickList->TruncateTable();

			if (mS->mC.clean_gags_cleanbans) {
				mS->mPenList->CleanType("st_chat");
				mS->mPenList->CleanType("st_pm");
				(*mOS) << _("Ban, unban, kick, temporary ban and chat gag lists has been cleaned.");

			} else {
				(*mOS) << _("Ban, unban, kick and temporary ban lists has been cleaned.");
			}

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
		"info",
		"list", "ls", "lst"
	};

	static const int prefixids[] = {
		BAN_BAN, BAN_BAN,
		BAN_UNBAN, BAN_UNBAN, BAN_UNBAN,
		BAN_INFO,
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

			if (BanTime == 0) {
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

			Ban.mDateStart = mS->mTime.Sec();

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
						Kick.mTime = mS->mTime.Sec();

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
				ostringstream toall;
				unsigned long ipnum;

				for (i = mS->mUserList.begin(); i != mS->mUserList.end(); ++i) {
					conn = ((cUser*)(*i))->mxConn;

					if (conn && conn->mpUser) {
						ipnum = conn->AddrToNumber();

						if (((BanType == eBF_IP) && (Ban.mIP == conn->AddrIP())) || ((BanType == eBF_RANGE) && (Ban.mRangeMin <= ipnum) && (Ban.mRangeMax >= ipnum))) {
							if (mS->mC.notify_kicks_to_all > -1) { // message to all
								toall.str("");

								if (BanTime) {
									if (tmp.size())
										toall << autosprintf(_("%s was banned for %s by %s with reason: %s"), conn->mpUser->mNick.c_str(), cTimePrint(BanTime, 0).AsPeriod().AsString().c_str(), Ban.mNickOp.c_str(), tmp.c_str());
									else
										toall << autosprintf(_("%s was banned for %s by %s without reason."), conn->mpUser->mNick.c_str(), cTimePrint(BanTime, 0).AsPeriod().AsString().c_str(), Ban.mNickOp.c_str());
								} else {
									if (tmp.size())
										toall << autosprintf(_("%s was banned permanently by %s with reason: %s"), conn->mpUser->mNick.c_str(), Ban.mNickOp.c_str(), tmp.c_str());
									else
										toall << autosprintf(_("%s was banned permanently by %s without reason."), conn->mpUser->mNick.c_str(), Ban.mNickOp.c_str());
								}

								mS->DCPublicToAll(mS->mC.hub_security, toall.str(), mS->mC.notify_kicks_to_all, int(eUC_MASTER), mS->mC.delayed_chat);
							}

							conn->CloseNow(eCR_KICKED); // todo: user dont get any message, only if notify_kicks_to_all is in his range
						}
					}
				}
			} else if (Ban.mNick.size()) { // drop user if nick is specified
				cUser *user = mS->mUserList.GetUserByNick(Ban.mNick);

				if (user && user->mxConn) {
					if (mS->mC.notify_kicks_to_all > -1) { // message to all
						ostringstream toall;

						if (BanTime) {
							if (tmp.size())
								toall << autosprintf(_("%s was banned for %s by %s with reason: %s"), user->mNick.c_str(), cTimePrint(BanTime, 0).AsPeriod().AsString().c_str(), Ban.mNickOp.c_str(), tmp.c_str());
							else
								toall << autosprintf(_("%s was banned for %s by %s without reason."), user->mNick.c_str(), cTimePrint(BanTime, 0).AsPeriod().AsString().c_str(), Ban.mNickOp.c_str());
						} else {
							if (tmp.size())
								toall << autosprintf(_("%s was banned permanently by %s with reason: %s"), user->mNick.c_str(), Ban.mNickOp.c_str(), tmp.c_str());
							else
								toall << autosprintf(_("%s was banned permanently by %s without reason."), user->mNick.c_str(), Ban.mNickOp.c_str());
						}

						mS->DCPublicToAll(mS->mC.hub_security, toall.str(), mS->mC.notify_kicks_to_all, int(eUC_MASTER), mS->mC.delayed_chat);
					}

					user->mxConn->CloseNow(eCR_KICKED); // todo: user dont get any message, only if notify_kicks_to_all is in his range
				}
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

bool cDCConsole::cfTempBan::operator()()
{
	if (!mConn || !mConn->mpUser)
		return false;

	if (mConn->mpUser->mClass < eUC_OPERATOR) {
		(*mOS) << _("You have no rights to do this.");
		return false;
	}

	enum {
		BAN_PREFIX = 1, // mIdStr
		BAN_TYPE = 2,
		BAN_WHO = 1 // mParStr
	};

	string who;

	if (mParRex->PartFound(BAN_WHO))
		GetParStr(BAN_WHO, who);

	if (who.empty()) {
		(*mOS) << _("Missing command parameters.");
		return false;
	}

	enum {
		BAN_INFO,
		BAN_UNBAN
	};

	static const char *prefixnames[] = {
		"info",
		"del", "un"
	};

	static const int prefixids[] = {
		BAN_INFO,
		BAN_UNBAN, BAN_UNBAN
	};

	string temp;
	int act = BAN_INFO;

	if (mIdRex->PartFound(BAN_PREFIX)) {
		mIdRex->Extract(BAN_PREFIX, mIdStr, temp);
		act = this->StringToIntFromList(temp, prefixnames, prefixids, sizeof(prefixnames) / sizeof(char*));

		if (act < 0)
			return false;
	}

	static const char *bannames[] = {
		"nick",
		"ip"
	};

	static const int banids[] = {
		eBF_NICK,
		eBF_IP
	};

	int type = eBF_NICKIP;

	if (mIdRex->PartFound(BAN_TYPE)) {
		mIdRex->Extract(BAN_TYPE, mIdStr, temp);
		type = this->StringToIntFromList(temp, bannames, banids, sizeof(bannames) / sizeof(char*));

		if (type < 0)
			return false;
	}

	switch (act) {
		case BAN_UNBAN: // unban
			if (type == eBF_NICKIP) { // any
				if (mS->mBanList->IsNickTempBanned(who)) { // test nick
					(*mOS) << _("Removed temporary nick ban") << ':';
					mS->mBanList->ShowNickTempBan(*mOS, who);
					mS->mBanList->DelNickTempBan(who);

				} else if (mS->mBanList->IsIPTempBanned(who)) { // test ip
					(*mOS) << _("Removed temporary IP ban") << ':';
					mS->mBanList->ShowIPTempBan(*mOS, who);
					mS->mBanList->DelIPTempBan(who);

				} else { // not found
					(*mOS) << _("Temporary ban not found") << ": " << who;
				}

			} else if (type == eBF_NICK) { // nick
				if (mS->mBanList->IsNickTempBanned(who)) { // test nick
					(*mOS) << _("Removed temporary nick ban") << ':';
					mS->mBanList->ShowNickTempBan(*mOS, who);
					mS->mBanList->DelNickTempBan(who);

				} else { // not found
					(*mOS) << _("Temporary nick ban not found") << ": " << who;
				}

			} else if (type == eBF_IP) { // ip
				if (mS->mBanList->IsIPTempBanned(who)) { // test ip
					(*mOS) << _("Removed temporary IP ban") << ':';
					mS->mBanList->ShowIPTempBan(*mOS, who);
					mS->mBanList->DelIPTempBan(who);

				} else { // not found
					(*mOS) << _("Temporary IP ban not found") << ": " << who;
				}
			}

			break;

		case BAN_INFO: // info
			if (type == eBF_NICKIP) { // any
				if (mS->mBanList->IsNickTempBanned(who)) { // test nick
					(*mOS) << _("Temporary nick ban information") << ':';
					mS->mBanList->ShowNickTempBan(*mOS, who);

				} else if (mS->mBanList->IsIPTempBanned(who)) { // test ip
					(*mOS) << _("Temporary IP ban information") << ':';
					mS->mBanList->ShowIPTempBan(*mOS, who);

				} else { // not found
					(*mOS) << _("Temporary ban not found") << ": " << who;
				}

			} else if (type == eBF_NICK) { // nick
				if (mS->mBanList->IsNickTempBanned(who)) { // test nick
					(*mOS) << _("Temporary nick ban information") << ':';
					mS->mBanList->ShowNickTempBan(*mOS, who);

				} else { // not found
					(*mOS) << _("Temporary nick ban not found") << ": " << who;
				}

			} else if (type == eBF_IP) { // ip
				if (mS->mBanList->IsIPTempBanned(who)) { // test ip
					(*mOS) << _("Temporary IP ban information") << ':';
					mS->mBanList->ShowIPTempBan(*mOS, who);

				} else { // not found
					(*mOS) << _("Temporary IP ban not found") << ": " << who;
				}
			}

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

	if (mConn->mpUser->mClass < eUC_ADMIN) {
		(*mOS) << _("You have no rights to do this.");
		return false;
	}

	enum {
		eINFO_HUB,
		eINFO_SERVER,
		eINFO_PORT,
		eINFO_HUBURL,
		eINFO_PROTOCOL,
		eINFO_BUFFER,
		eINFO_MMDB
	};

	static const char *infonames[] = {
		"hub",
		"serv", "server", "sys", "system", "os",
		"port",
		"url", "huburl",
		"prot", "proto", "protocol",
		"buf", "buffer",
		"mmdb", "geoip"
	};

	static const int infoids[] = {
		eINFO_HUB,
		eINFO_SERVER, eINFO_SERVER, eINFO_SERVER, eINFO_SERVER, eINFO_SERVER,
		eINFO_PORT,
		eINFO_HUBURL, eINFO_HUBURL,
		eINFO_PROTOCOL, eINFO_PROTOCOL, eINFO_PROTOCOL,
		eINFO_BUFFER, eINFO_BUFFER,
		eINFO_MMDB, eINFO_MMDB
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
		case eINFO_MMDB:
			mS->mMaxMindDB->ShowInfo(*mOS);
			break;
		case eINFO_PROTOCOL:
			mInfoServer.ProtocolInfo(*mOS);
			break;
		case eINFO_BUFFER:
			mInfoServer.BufferInfo(*mOS);
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
	if (mConn->mpUser->mClass < eUC_MASTER) {
		(*mOS) << _("You have no rights to do this.");
		return false;
	}

	string cmd;
   	mIdRex->Extract(2, mIdStr, cmd);

	enum {
		eAC_ADD,
		eAC_DEL,
		eAC_EDIT,
		eAC_GET,
		eAC_FLAGS
	};

	static const char *acna[] = {
		"new", "add",
		"del",
		"edit",
		"get",
		"setflags"
	};

	static const int acid[] = {
		eAC_ADD, eAC_ADD,
		eAC_DEL,
		eAC_EDIT,
		eAC_GET,
		eAC_FLAGS
	};

	int act = this->StringToIntFromList(cmd, acna, acid, sizeof(acna) / sizeof(char*));

	if (act < 0)
		return false;

	string trig/*, text*/;
	mParRex->Extract(1, mParStr, trig);
	//mParRex->Extract(3, mParStr, text);
	int i/*, flags = 0*/;
	//istringstream is(text);
	bool res = false;
	//cTrigger *tr;

	switch (act) {
		/*
		case eAC_ADD:
			tr = new cTrigger;
			tr->mCommand = trig;
			tr->mDefinition = text;
			break;

		case eAC_EDIT:
			for (i = 0; i < ((cDCConsole*)mCo)->mTriggers->Size(); ++i) {
				if (trig == (*((cDCConsole*)mCo)->mTriggers)[i]->mCommand) {
					mS->SaveFile(((*((cDCConsole*)mCo)->mTriggers)[i])->mDefinition, text);
					res = true;
					break;
				}
			}

			break;

		case eAC_FLAGS:
			flags = -1;
			is >> flags;

			if (flags >= 0) {
				for (i = 0; i < ((cDCConsole*)mCo)->mTriggers->Size(); ++i) {
					if (trig == (*((cDCConsole*)mCo)->mTriggers)[i]->mCommand) {
						(*((cDCConsole*)mCo)->mTriggers)[i]->mFlags = flags;
						((cDCConsole*)mCo)->mTriggers->SaveData(i);
						res = true;
						break;
					}
				}
			}

			break;
		*/

		case eAC_GET:
			for (i = 0; i < ((cDCConsole*)mCo)->mTriggers->Size(); ++i) {
				if (trig == (*((cDCConsole*)mCo)->mTriggers)[i]->mCommand) {
					(*mOS) << autosprintf(_("Trigger definition: %s"), trig.c_str()) << "\r\n\r\n" << ((*((cDCConsole*)mCo)->mTriggers)[i])->mDefinition << "\r\n";
					return true;
				}
			}

			(*mOS) << autosprintf(_("Trigger not found: %s"), trig.c_str());
			break;

		default:
			(*mOS) << _("This command is not implemented.");
			break;
	}

	return res;
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

	static const char *actionnames[] = { // todo: there are no such rights as drop and tban, flags exist though
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
	int act = this->StringToIntFromList(temp, actionnames, actionids, sizeof(actionnames) / sizeof(char*));

	if (act < 0)
		return false;

	if (mConn->mpUser->mClass < eUC_OPERATOR) {
		(*mOS) << _("You have no rights to do this.");
		return false;
	}

	if (act == eAC_LIST) {
		ostringstream os;
		mS->mPenList->ListAll(os);
		mS->DCPrivateHS(os.str(), mConn);
		return true;
	}

	if (!mParRex->PartFound(1)) {
		if (temp == "nochat") { // nochat command
			istringstream cmd_line("nochat");
			return ((cDCConsole*)mCo)->CmdChat(cmd_line, mConn, false);
		} else {
			(*mOS) << _("Missing command parameters.");
			return false;
		}
	}

	string nick;
	mParRex->Extract(1, mParStr, nick);
	cUser *user = NULL;

	if (act == eAC_NOINFO) {
		user = mS->mUserList.GetUserByNick(nick);

		if (user && user->mxConn) {
			user->DisplayRightsInfo((*mOS), true);
			return true;
		} else {
			(*mOS) << autosprintf(_("User not found: %s"), nick.c_str());
			return false;
		}
	}

	unsigned long per = 24 * 3600 * 7;
	unsigned long now = 1;
	bool isun = mIdRex->PartFound(1);

	if (mParRex->PartFound(3)) {
		mParRex->Extract(3, mParStr, temp);
		per = mS->Str2Period(temp, (*mOS));

		if (!per)
			return false;
	}

	cPenaltyList::sPenalty pen;

	if (!mS->mPenList->LoadTo(pen, nick))
		pen.mNick = nick;

	pen.mOpNick = mConn->mpUser->mNick;

	if (!isun)
		now = mS->mTime.Sec() + per;

	switch (act) {
		case eAC_GAG:
			pen.mStartChat = now;
			break;

		case eAC_NOPM:
			pen.mStartPM = now;
			break;

		case eAC_NOCHATS:
			pen.mStartChat = now;
			pen.mStartPM = now;
			break;

		case eAC_NODL:
			pen.mStartCTM = now;
			break;

		case eAC_NOSEARCH:
			pen.mStartSearch = now;
			break;

		case eAC_KVIP:
			pen.mStopKick = now;
			break;

		case eAC_OPCHAT:
			pen.mStopOpchat = now;
			break;

		case eAC_NOSHARE:
			pen.mStopShare0 = now;
			break;

		case eAC_CANREG:
			pen.mStopReg = now;
			break;

		default:
			return false;
	}

	if (!(isun ? mS->mPenList->RemPenalty(pen) : mS->mPenList->AddPenalty(pen))) {
		(*mOS) << autosprintf(_("Error setting rights or restrictions for user: %s"), nick.c_str());
		return false;
	}

	user = mS->mUserList.GetUserByNick(nick);

	switch (act) {
		case eAC_GAG:
			if ((mS->mC.notify_kicks_to_all == -1) || !user) {
				if (isun)
					(*mOS) << autosprintf(_("Allowing user to use main chat again: %s"), nick.c_str());
				else
					(*mOS) << autosprintf(_("Restricting user from using main chat: %s for %s"), nick.c_str(), cTimePrint(per, 0).AsPeriod().AsString().c_str());
			}

			if (user) {
				if (mS->mC.notify_kicks_to_all > -1) { // message to all
					ostringstream toall;

					if (isun)
						toall << autosprintf(_("%s was allowed to use main chat again by %s without reason."), nick.c_str(), pen.mOpNick.c_str());
					else
						toall << autosprintf(_("%s was restricted from using main chat for %s by %s without reason."), nick.c_str(), cTimePrint(per, 0).AsPeriod().AsString().c_str(), pen.mOpNick.c_str());

					mS->DCPublicToAll(mS->mC.hub_security, toall.str(), mS->mC.notify_kicks_to_all, int(eUC_MASTER), mS->mC.delayed_chat);
				}

				user->SetRight(eUR_CHAT, pen.mStartChat, isun, true);
			}

			break;

		case eAC_NOPM:
			if ((mS->mC.notify_kicks_to_all == -1) || !user) {
				if (isun)
					(*mOS) << autosprintf(_("Allowing user to use private chat again: %s"), nick.c_str());
				else
					(*mOS) << autosprintf(_("Restricting user from using private chat: %s for %s"), nick.c_str(), cTimePrint(per, 0).AsPeriod().AsString().c_str());
			}

			if (user) {
				if (mS->mC.notify_kicks_to_all > -1) { // message to all
					ostringstream toall;

					if (isun)
						toall << autosprintf(_("%s was allowed to use private chat again by %s without reason."), nick.c_str(), pen.mOpNick.c_str());
					else
						toall << autosprintf(_("%s was restricted from using private chat for %s by %s without reason."), nick.c_str(), cTimePrint(per, 0).AsPeriod().AsString().c_str(), pen.mOpNick.c_str());

					mS->DCPublicToAll(mS->mC.hub_security, toall.str(), mS->mC.notify_kicks_to_all, int(eUC_MASTER), mS->mC.delayed_chat);
				}

				user->SetRight(eUR_PM, pen.mStartPM, isun, true);
			}

			break;

		case eAC_NOCHATS:
			if ((mS->mC.notify_kicks_to_all == -1) || !user) {
				if (isun)
					(*mOS) << autosprintf(_("Allowing user to use main and private chat again: %s"), nick.c_str());
				else
					(*mOS) << autosprintf(_("Restricting user from using main and private chat: %s for %s"), nick.c_str(), cTimePrint(per, 0).AsPeriod().AsString().c_str());
			}

			if (user) {
				if (mS->mC.notify_kicks_to_all > -1) { // message to all
					ostringstream toall;

					if (isun)
						toall << autosprintf(_("%s was allowed to use main and private chat again by %s without reason."), nick.c_str(), pen.mOpNick.c_str());
					else
						toall << autosprintf(_("%s was restricted from using main and private chat for %s by %s without reason."), nick.c_str(), cTimePrint(per, 0).AsPeriod().AsString().c_str(), pen.mOpNick.c_str());

					mS->DCPublicToAll(mS->mC.hub_security, toall.str(), mS->mC.notify_kicks_to_all, int(eUC_MASTER), mS->mC.delayed_chat);
				}

				user->SetRight(eUR_CHAT, pen.mStartChat, isun, true);
				user->SetRight(eUR_PM, pen.mStartPM, isun, true);
			}

			break;

		case eAC_NODL:
			if (isun)
				(*mOS) << autosprintf(_("Allowing user to download again: %s"), nick.c_str());
			else
				(*mOS) << autosprintf(_("Restricting user from downloading: %s for %s"), nick.c_str(), cTimePrint(per, 0).AsPeriod().AsString().c_str());

			if (user)
				user->SetRight(eUR_CTM, pen.mStartCTM, isun, true);

			break;

		case eAC_NOSEARCH:
			if (isun)
				(*mOS) << autosprintf(_("Allowing user to use search again: %s"), nick.c_str());
			else
				(*mOS) << autosprintf(_("Restricting user from using search: %s for %s"), nick.c_str(), cTimePrint(per, 0).AsPeriod().AsString().c_str());

			if (user)
				user->SetRight(eUR_SEARCH, pen.mStartSearch, isun, true);

			break;

		case eAC_NOSHARE:
			if (isun)
				(*mOS) << autosprintf(_("Taking away the right to hide share: %s"), nick.c_str());
			else
				(*mOS) << autosprintf(_("Giving user the right to hide share: %s for %s"), nick.c_str(), cTimePrint(per, 0).AsPeriod().AsString().c_str());

			if (user)
				user->SetRight(eUR_NOSHARE, pen.mStopShare0, isun, true);

			break;

		case eAC_CANREG:
			if (isun)
				(*mOS) << autosprintf(_("Taking away the right to register others: %s"), nick.c_str());
			else
				(*mOS) << autosprintf(_("Giving user the right to register others: %s for %s"), nick.c_str(), cTimePrint(per, 0).AsPeriod().AsString().c_str());

			if (user)
				user->SetRight(eUR_REG, pen.mStopReg, isun, true);

			break;

		case eAC_KVIP:
			if (isun)
				(*mOS) << autosprintf(_("Taking away the right to kick others: %s"), nick.c_str());
			else
				(*mOS) << autosprintf(_("Giving user the right to kick others: %s for %s"), nick.c_str(), cTimePrint(per, 0).AsPeriod().AsString().c_str());

			if (user)
				user->SetRight(eUR_KICK, pen.mStopKick, isun, true);

			break;

		case eAC_OPCHAT:
			if (isun)
				(*mOS) << autosprintf(_("Taking away the right to use operator chat: %s"), nick.c_str());
			else
				(*mOS) << autosprintf(_("Giving user the right to use operator chat: %s for %s"), nick.c_str(), cTimePrint(per, 0).AsPeriod().AsString().c_str());

			if (user) {
				user->SetRight(eUR_OPCHAT, pen.mStopOpchat, isun, true);
				cUserCollection::tHashType hash = mS->mUserList.Nick2Hash(nick); // add or remove user in operator chat

				if (isun && mS->mOpchatList.ContainsHash(hash))
					mS->mOpchatList.RemoveByHash(hash);
				else if (!isun && !mS->mOpchatList.ContainsHash(hash))
					mS->mOpchatList.AddWithHash(user, hash);
			}

			break;

		default:
			return false;
	}

	if (user) // apply directly
		user->ApplyRights(pen);

	return true;
}

bool cDCConsole::cfCmd::operator()() // todo: remove this, we have complete help
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
		eAC_HUBURL,
		eAC_TLS,
		eAC_SUPS,
		eAC_VER,
		eAC_INFO
	};

	static const char *actionnames[] = {
		"ip", "addr",
		"range", "subnet", "sub",
		"cc", "country",
		"city",
		"hubport", "port",
		"huburl", "url",
		"tls",
		"supports", "sups",
		"nmdc", "ver",
		"myinfo", "info"
	};

	static const int actionids[] = {
		eAC_IP, eAC_IP,
		eAC_RANGE, eAC_RANGE, eAC_RANGE,
		eAC_CC, eAC_CC,
		eAC_CITY,
		eAC_HUBPORT, eAC_HUBPORT,
		eAC_HUBURL, eAC_HUBURL,
		eAC_TLS,
		eAC_SUPS, eAC_SUPS,
		eAC_VER, eAC_VER,
		eAC_INFO, eAC_INFO
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
	string who;
	unsigned long ip_min = 0, ip_max = 0;
	unsigned int cnt, port = 0;

	switch (Action) {
		case eAC_IP:
			if (!cBanList::Ip2Num(tmp, ip_min, false)) {
				(*mOS) << autosprintf(_("Specified value is expected to be a valid IP address within range %s to %s."), "0.0.0.1", "255.255.255.255");
				return false;
			}

			ip_max = ip_min;
			cnt = mS->WhoIP(ip_min, ip_max, who, sep, true);

			if (cnt)
				(*mOS) << autosprintf(ngettext("Found %d user with IP %s", "Found %d users with IP %s", cnt), cnt, tmp.c_str());

			break;

		case eAC_RANGE:
			if (!cDCConsole::GetIPRange(tmp, ip_min, ip_max)) {
				(*mOS) << autosprintf(_("Unknown range format: %s"), tmp.c_str());
				return false;
			}

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
			cnt = mS->WhoCity(tmp, who, sep);

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
			cnt = mS->WhoHubURL(tmp, who, sep);

			if (cnt)
				(*mOS) << autosprintf(ngettext("Found %d user with hub URL %s", "Found %d users with hub URL %s", cnt), cnt, tmp.c_str());

			break;

		case eAC_TLS:
			cnt = mS->WhoTLSVer(tmp, who, sep);

			if (cnt)
				(*mOS) << autosprintf(ngettext("Found %d user with TLS version %s", "Found %d users with TLS version %s", cnt), cnt, tmp.c_str());

			break;

		case eAC_SUPS:
			cnt = mS->WhoSupports(tmp, who, sep);

			if (cnt)
				(*mOS) << autosprintf(ngettext("Found %d user with supports %s", "Found %d users with supports %s", cnt), cnt, tmp.c_str());

			break;

		case eAC_VER:
			cnt = mS->WhoNMDCVer(tmp, who, sep);

			if (cnt)
				(*mOS) << autosprintf(ngettext("Found %d user with NMDC version %s", "Found %d users with NMDC version %s", cnt), cnt, tmp.c_str());

			break;

		case eAC_INFO:
			cnt = mS->WhoMyINFO(tmp, who, sep);

			if (cnt)
				(*mOS) << autosprintf(ngettext("Found %d user with MyINFO %s", "Found %d users with MyINFO %s", cnt), cnt, tmp.c_str());

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
		eAC_DROP,
		eAC_GAG,
		eAC_UNGAG
	};

	static const char *actionnames[] = {
		"kick",
		"drop",
		"vipgag",
		"vipungag"
	};

	static const int actionids[] = {
		eAC_KICK,
		eAC_DROP,
		eAC_GAG,
		eAC_UNGAG
	};

	if ((mConn->mpUser->mClass < eUC_VIPUSER) || !mConn->mpUser->Can(eUR_KICK, mS->mTime.Sec())) {
		(*mOS) << _("You have no rights to gag, drop or kick anyone.");
		return false;
	}

	string temp;
	mIdRex->Extract(1, mIdStr, temp);
	int act = this->StringToIntFromList(temp, actionnames, actionids, sizeof(actionnames) / sizeof(char*));

	if (act < 0)
		return false;

	string nick;
	mParRex->Extract(1, mParStr, nick);
	cUser *user = mS->mUserList.GetUserByNick(nick);
	cRegUserInfo ui;

	switch (act) {
		case eAC_KICK: // kick and drop
		case eAC_DROP:
			if (!user || !user->mxConn) {
				(*mOS) << autosprintf(_("User not found: %s"), nick.c_str());
				return false;
			} else if ((user->mClass + int(mS->mC.classdif_kick)) > mConn->mpUser->mClass) {
				(*mOS) << autosprintf(_("You have no rights to drop or kick user: %s"), nick.c_str());
				return false;
			}

			break;

		case eAC_GAG: // gag
		case eAC_UNGAG:
			if (user && !user->mxConn) { // is bot
				(*mOS) << autosprintf(_("User not found: %s"), nick.c_str());
				return false;
			} else if (user && user->mxConn) { // user online
				if ((user->mClass + int(mS->mC.classdif_kick)) > mConn->mpUser->mClass) {
					(*mOS) << autosprintf(_("You have no rights to gag user: %s"), nick.c_str());
					return false;
				}
			} else if (mS->mR->FindRegInfo(ui, nick)) { // user offline, reg found
				if ((ui.mClass + int(mS->mC.classdif_kick)) > mConn->mpUser->mClass) {
					(*mOS) << autosprintf(_("You have no rights to gag user: %s"), nick.c_str());
					return false;
				}
			} else if (int(eUC_NORMUSER + mS->mC.classdif_kick) > mConn->mpUser->mClass) { // offline nick
				(*mOS) << autosprintf(_("You have no rights to gag nick: %s"), nick.c_str());
				return false;
			}

			break;
	}

	temp.clear();

	if (mParRex->PartFound(2)) {
		mParRex->Extract(2, mParStr, temp);

		if (temp.size() && (temp[0] == ' ')) // small fix
			temp = temp.substr(1);
	}

	cPenaltyList::sPenalty pen;

	switch (act) {
		case eAC_KICK: // kick
			mS->DCKickNick(mOS, mConn->mpUser, nick, temp, (eKI_CLOSE | eKI_WHY | eKI_PM | eKI_BAN));
			break;

		case eAC_DROP: // drop
			mS->DCKickNick(mOS, mConn->mpUser, nick, temp, (eKI_CLOSE | eKI_PM | eKI_DROP));
			break;

		case eAC_GAG: // gag
		case eAC_UNGAG:
			if (!mS->mPenList->LoadTo(pen, nick))
				pen.mNick = nick;

			pen.mOpNick = mConn->mpUser->mNick;
			pen.mStartChat = (unsigned long)((act == eAC_GAG) ? mS->mTime.Sec() + mS->mC.tban_kick : 1);

			if ((act == eAC_GAG) ? !mS->mPenList->AddPenalty(pen) : !mS->mPenList->RemPenalty(pen)) {
				(*mOS) << autosprintf(_("Error setting rights or restrictions for user: %s"), pen.mNick.c_str());
				return false;
			}

			if ((mS->mC.notify_kicks_to_all == -1) || !user || !user->mxConn) {
				if (act == eAC_GAG)
					(*mOS) << autosprintf(_("Restricting user from using main chat: %s for %s"), pen.mNick.c_str(), cTimePrint(mS->mC.tban_kick, 0).AsPeriod().AsString().c_str());
				else
					(*mOS) << autosprintf(_("Allowing user to use main chat again: %s"), pen.mNick.c_str());
			}

			if (user && user->mxConn) {
				if (mS->mC.notify_kicks_to_all > -1) { // message to all
					ostringstream toall;

					if (act == eAC_GAG) {
						if (temp.size()) // with reason
							toall << autosprintf(_("%s was restricted from using main chat for %s by %s with reason: %s"), pen.mNick.c_str(), cTimePrint(mS->mC.tban_kick, 0).AsPeriod().AsString().c_str(), pen.mOpNick.c_str(), temp.c_str());
						else
							toall << autosprintf(_("%s was restricted from using main chat for %s by %s without reason."), pen.mNick.c_str(), cTimePrint(mS->mC.tban_kick, 0).AsPeriod().AsString().c_str(), pen.mOpNick.c_str());
					} else {
						if (temp.size()) // with reason
							toall << autosprintf(_("%s was allowed to use main chat again by %s with reason: %s"), pen.mNick.c_str(), pen.mOpNick.c_str(), temp.c_str());
						else
							toall << autosprintf(_("%s was allowed to use main chat again by %s without reason."), pen.mNick.c_str(), pen.mOpNick.c_str());
					}

					mS->DCPublicToAll(mS->mC.hub_security, toall.str(), mS->mC.notify_kicks_to_all, int(eUC_MASTER), mS->mC.delayed_chat);
				}

				user->SetRight(eUR_CHAT, pen.mStartChat, (act == eAC_UNGAG), true);
				user->ApplyRights(pen);
			}

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

				if (tmp == PLUGMAN_NAME) {
					(*mOS) << _("You don't want to unload Plugman because all other plugins are loaded via it.");
					return true;
				}

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
				if (tmp == PLUGMAN_NAME) {
					(*mOS) << _("You don't want to reload Plugman because all other plugins are loaded via it.");
					return true;
				}

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

		if ((MyClass < eUC_MASTER) && (MyClass < cls)) {
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
			authorized = RegFound;
			break;
	}

	if (MyClass == eUC_MASTER)
		authorized = (RegFound || (Action == eAC_NEW));

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

			if (mS->mR->AddRegUser(nick, mConn, ParClass, (WithPass ? pass.c_str() : NULL))) { // no need to reconnect for class to take effect
				if (user && user->mxConn) {
					ostr.str("");

					if (WithPass)
						ostr << autosprintf(_("You have been registered with class %d and following password: %s"), ParClass, pass.c_str());
					else
						ostr << autosprintf(_("You have been registered with class %d. Please set your password by using following command: +passwd <password>"), ParClass); // todo: use password request here aswell, send_pass_request

					mS->DCPrivateHS(ostr.str(), user->mxConn);
					mS->DCPublicHS(ostr.str(), user->mxConn);

					if ((ParClass >= mS->mC.opchat_class) && !mS->mOpchatList.ContainsHash(user->mNickHash)) // opchat list
						mS->mOpchatList.AddWithHash(user, user->mNickHash);

					if ((ParClass >= mS->mC.oplist_class) && !mS->mOpList.ContainsHash(user->mNickHash)) { // oplist
						mS->mOpList.AddWithHash(user, user->mNickHash);

						mS->mP.Create_OpList(msg, user->mNick); // send short oplist
						mS->mUserList.SendToAll(msg, mS->mC.delayed_myinfo, true);
					}

					user->mClass = tUserCl(ParClass);
					mS->SetUserRegInfo(user->mxConn, user->mNick); // update registration information in real time aswell
				}

				if (WithPass)
					(*mOS) << autosprintf(_("%s has been registered with class %d and password."), nick.c_str(), ParClass);
				else
					(*mOS) << autosprintf(_("%s has been registered with class %d. Please tell him to set his password."), nick.c_str(), ParClass);
			} else {
				(*mOS) << _("Error adding new user.");
				return false;
			}

			break;

		case eAC_DEL:
			#ifndef WITHOUT_PLUGINS
				if (RegFound && !mS->mCallBacks.mOnDelReg.CallAll(this->mConn->mpUser, nick, ui.mClass)) {
					(*mOS) << _("Your action was blocked by a plugin.");
					return false;
				}
			#endif

			ostr << ui;

			if (RegFound && mS->mR->DelReg(nick)) {
				(*mOS) << autosprintf(_("%s has been unregistered, user information"), nick.c_str()) << ":\r\n" << ostr.str();

				if (user && user->mxConn) { // no need to reconnect for class to take effect
					mS->DCPrivateHS(_("You have been unregistered."), user->mxConn);
					mS->DCPublicHS(_("You have been unregistered."), user->mxConn);

					if (mS->mOpchatList.ContainsHash(user->mNickHash)) // opchat list
						mS->mOpchatList.RemoveByHash(user->mNickHash);

					if (mS->mOpList.ContainsHash(user->mNickHash)) { // oplist, only if user is there
						mS->mOpList.RemoveByHash(user->mNickHash);

						mS->mP.Create_Quit(msg, user->mNick); // send quit to all
						mS->mUserList.SendToAll(msg, mS->mC.delayed_myinfo, true);

						if (mS->mC.myinfo_tls_filter && user->GetMyFlag(eMF_TLS)) { // myinfo tls filter
							msg = user->mFakeMyINFO;
							mS->mUserList.SendToAllWithMyFlag(msg, eMF_TLS, mS->mC.delayed_myinfo, true);
							mS->RemoveMyINFOFlag(msg, user->mFakeMyINFO, eMF_TLS);
							mS->mUserList.SendToAllWithoutMyFlag(msg, eMF_TLS, mS->mC.delayed_myinfo, true);

						} else {
							msg = user->mFakeMyINFO;
							mS->mUserList.SendToAll(msg, mS->mC.delayed_myinfo, true); // send myinfo to all
						}

						mS->ShowUserIP(user->mxConn); // send userip to operators
					}

					user->mClass = eUC_NORMUSER;

					if (user->mHideShare) { // recalculate total share
						user->mHideShare = false;
						mS->mTotalShare += user->mShare;
					}

					mS->SetUserRegInfo(user->mxConn, user->mNick); // update registration information in real time aswell
				}

				return true;
			} else {
				(*mOS) << autosprintf(_("Error unregistering %s, user information"), nick.c_str()) << ":\r\n" << ostr.str();
				return false;
			}

			break;

		case eAC_PASS:
			if (WithPar) {
				if (mS->mR->ChangePwd(nick, par)) {
					(*mOS) << _("Password updated.");
				} else {
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
				if (RegFound && !mS->mCallBacks.mOnUpdateClass.CallAll(this->mConn->mpUser, nick, ui.mClass, ParClass)) {
					(*mOS) << _("Your action was blocked by a plugin.");
					return false;
				}
			#endif

			if (user && user->mxConn) { // no need to reconnect for class to take effect
				if ((user->mClass < mS->mC.opchat_class) && (ParClass >= mS->mC.opchat_class)) { // opchat list
					if (!mS->mOpchatList.ContainsHash(user->mNickHash))
						mS->mOpchatList.AddWithHash(user, user->mNickHash);

				} else if ((user->mClass >= mS->mC.opchat_class) && (ParClass < mS->mC.opchat_class)) {
					if (mS->mOpchatList.ContainsHash(user->mNickHash))
						mS->mOpchatList.RemoveByHash(user->mNickHash);
				}

				if (RegFound) {
					if ((user->mClass < mS->mC.oplist_class) && (ParClass >= mS->mC.oplist_class)) { // oplist
						if (!ui.mHideKeys && !mS->mOpList.ContainsHash(user->mNickHash)) {
							mS->mOpList.AddWithHash(user, user->mNickHash);

							mS->mP.Create_OpList(msg, user->mNick); // send short oplist
							mS->mUserList.SendToAll(msg, mS->mC.delayed_myinfo, true);
						}

					} else if ((user->mClass >= mS->mC.oplist_class) && (ParClass < mS->mC.oplist_class)) {
						if (!ui.mHideKeys && mS->mOpList.ContainsHash(user->mNickHash)) {
							mS->mOpList.RemoveByHash(user->mNickHash);

							mS->mP.Create_Quit(msg, user->mNick); // send quit to all
							mS->mUserList.SendToAll(msg, mS->mC.delayed_myinfo, true);

							if (mS->mC.myinfo_tls_filter && user->GetMyFlag(eMF_TLS)) { // myinfo tls filter
								msg = user->mFakeMyINFO;
								mS->mUserList.SendToAllWithMyFlag(msg, eMF_TLS, mS->mC.delayed_myinfo, true);
								mS->RemoveMyINFOFlag(msg, user->mFakeMyINFO, eMF_TLS);
								mS->mUserList.SendToAllWithoutMyFlag(msg, eMF_TLS, mS->mC.delayed_myinfo, true);

							} else {
								msg = user->mFakeMyINFO;
								mS->mUserList.SendToAll(msg, mS->mC.delayed_myinfo, true); // send myinfo to all
							}

							mS->ShowUserIP(user->mxConn); // send userip to operators
						}
					}
				}

				user->mClass = tUserCl(ParClass);
			}

			field = "class";
			ostr << autosprintf(_("Your class has been changed to %d."), ParClass);
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
			if (RegFound)
				(*mOS) << autosprintf(_("%s registration information"), ui.GetNick().c_str()) << ":\r\n" << ui;

			break;

		default:
			mIdRex->Extract(1, mIdStr, par);
			(*mOS) << _("This command is not implemented.");
			return false;
	}

	if ((WithPar && (Action >= eAC_ENABLE) && (Action <= eAC_SET)) || ((Action == eAC_PASS) && !WithPar)) {
		if (mS->mR->SetVar(nick, field, par)) {
			if (user && user->mxConn && RegFound) { // we are working with online user
				if (field == "hide_share") { // change hidden share flag on the fly, also update total share
					if (user->mHideShare && (par == "0")) { // setting to 0
						user->mHideShare = false;
						mS->mTotalShare += user->mShare;

						if (ostr.str().empty())
							ostr << _("Your share is now visible.");

					} else if (!user->mHideShare && (par != "0")) { // setting to 0, it appears that only 0 means false, anything else means true
						user->mHideShare = true;
						mS->mTotalShare -= user->mShare;

						if (ostr.str().empty())
							ostr << _("Your share is now hidden.");
					}

				} else if (field == "hide_chat") { // change hidden chat flag on the fly
					if (user->mHideChat && (par == "0")) { // setting to 0
						user->mHideChat = false;

						if (!mS->mChatUsers.ContainsHash(user->mNickHash)) {
							mS->mChatUsers.AddWithHash(user, user->mNickHash);

							if (ostr.str().empty())
								ostr << _("Public chat messages are now visible. To hide them, write: +nochat");
						}

					} else if (!user->mHideChat && (par != "0")) { // setting to 0, it appears that only 0 means false, anything else means true
						user->mHideChat = true;

						if (mS->mChatUsers.ContainsHash(user->mNickHash)) {
							mS->mChatUsers.RemoveByHash(user->mNickHash);

							if (ostr.str().empty())
								ostr << _("Public chat messages are now hidden. To show them, write: +chat");
						}
					}

				} else if (field == "hide_keys") { // hide operator key, mHideKeys always overrides mShowKeys and oplist_class has last position
					if (ui.mHideKeys && (par == "0")) { // setting to 0
						if ((ui.mShowKeys || (user->mClass >= mS->mC.oplist_class)) && !mS->mOpList.ContainsHash(user->mNickHash)) {
							mS->mOpList.AddWithHash(user, user->mNickHash);

							mS->mP.Create_OpList(msg, user->mNick); // send short oplist
							mS->mUserList.SendToAll(msg, mS->mC.delayed_myinfo, true);

							if (ostr.str().empty())
								ostr << _("Your operator key is now visible.");
						}

					} else if (!ui.mHideKeys && (par != "0")) { // setting to 1
						if ((ui.mShowKeys || (user->mClass >= mS->mC.oplist_class)) && mS->mOpList.ContainsHash(user->mNickHash)) {
							mS->mOpList.RemoveByHash(user->mNickHash);

							mS->mP.Create_Quit(msg, user->mNick); // send quit to all
							mS->mUserList.SendToAll(msg, mS->mC.delayed_myinfo, true);

							if (mS->mC.myinfo_tls_filter && user->GetMyFlag(eMF_TLS)) { // myinfo tls filter
								msg = user->mFakeMyINFO;
								mS->mUserList.SendToAllWithMyFlag(msg, eMF_TLS, mS->mC.delayed_myinfo, true);
								mS->RemoveMyINFOFlag(msg, user->mFakeMyINFO, eMF_TLS);
								mS->mUserList.SendToAllWithoutMyFlag(msg, eMF_TLS, mS->mC.delayed_myinfo, true);

							} else {
								msg = user->mFakeMyINFO;
								mS->mUserList.SendToAll(msg, mS->mC.delayed_myinfo, true); // send myinfo to all
							}

							mS->ShowUserIP(user->mxConn); // send userip to operators

							if (ostr.str().empty())
								ostr << _("Your operator key is now hidden.");
						}
					}

				} else if (field == "show_keys") { // show operator key, mHideKeys always overrides mShowKeys and oplist_class has last position
					if (!ui.mShowKeys && (par != "0")) { // setting to 1
						if (!ui.mHideKeys && !mS->mOpList.ContainsHash(user->mNickHash)) {
							mS->mOpList.AddWithHash(user, user->mNickHash);

							mS->mP.Create_OpList(msg, user->mNick); // send short oplist
							mS->mUserList.SendToAll(msg, mS->mC.delayed_myinfo, true);

							if (ostr.str().empty())
								ostr << _("Your operator key is now visible.");
						}

					} else if (ui.mShowKeys && (par == "0")) { // setting to 0
						if (!ui.mHideKeys && (user->mClass < mS->mC.oplist_class) && mS->mOpList.ContainsHash(user->mNickHash)) {
							mS->mOpList.RemoveByHash(user->mNickHash);

							mS->mP.Create_Quit(msg, user->mNick); // send quit to all
							mS->mUserList.SendToAll(msg, mS->mC.delayed_myinfo, true);

							if (mS->mC.myinfo_tls_filter && user->GetMyFlag(eMF_TLS)) { // myinfo tls filter
								msg = user->mFakeMyINFO;
								mS->mUserList.SendToAllWithMyFlag(msg, eMF_TLS, mS->mC.delayed_myinfo, true);
								mS->RemoveMyINFOFlag(msg, user->mFakeMyINFO, eMF_TLS);
								mS->mUserList.SendToAllWithoutMyFlag(msg, eMF_TLS, mS->mC.delayed_myinfo, true);

							} else {
								msg = user->mFakeMyINFO;
								mS->mUserList.SendToAll(msg, mS->mC.delayed_myinfo, true); // send myinfo to all
							}

							mS->ShowUserIP(user->mxConn); // send userip to operators

							if (ostr.str().empty())
								ostr << _("Your operator key is now hidden.");
						}
					}

				} else if (field == "fake_ip") { // set user fake ip in real time, if empty, we dont know real ip, so need to reconnect
					if (par.size()) {
						if (user->mxConn->SetUserIP(par)) {
							user->mxConn->ResetGeo();

							if (ostr.str().empty())
								ostr << autosprintf(_("Your fake IP is now set: %s"), par.c_str());

						} else {
							(*mOS) << autosprintf(_("Specified fake IP is not valid: %s"), par.c_str());
							par = "";
							mS->mR->SetVar(nick, field, par);
							return false;
						}

					} else if (ostr.str().empty()) {
						ostr << _("Your fake IP is now unset, please reconnect to get back your real IP.");
					}
				}
			}

			(*mOS) << autosprintf(_("Updated variable %s to value %s for user: %s"), field.c_str(), par.c_str(), nick.c_str());

			if ((Action == eAC_PASS) && !WithPar && mS->mC.report_pass_reset)
				mS->ReportUserToOpchat(mConn, autosprintf(_("Password reset for user: %s"), nick.c_str()), false);

			if (user && user->mxConn) { // send both in pm and mc, so user see the message for sure
				mS->SetUserRegInfo(user->mxConn, user->mNick); // update registration information in real time aswell

				if (ostr.str().size()) {
					mS->DCPrivateHS(ostr.str(), user->mxConn);
					mS->DCPublicHS(ostr.str(), user->mxConn);
				}
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
	cTimePrint TimeBefore, TimeAfter;

	if (mS->LastBCNick != "disable")
		mS->LastBCNick = mConn->mpUser->mNick;

	int count = mS->SendToAllWithNickVars(start, end, MinClass, MaxClass);
	TimeAfter = mS->mTime;
	*mOS << autosprintf(ngettext("Message delivered to %d user in %s.", "Message delivered to %d users in %s.", count),
	     count, cTimePrint(TimeAfter - TimeBefore).AsPeriod().AsString().c_str());
	return true;
}

bool cDCConsole::GetIPRange(const string &rang, unsigned long &frdr, unsigned long &todr)
{
	// "^(\\d+\\.\\d+\\.\\d+\\.\\d+)((\\/(\\d+))|(\\.\\.|-)(\\d+\\.\\d+\\.\\d+\\.\\d+))?$"

	enum {
		R_IP1 = 1,
		R_RANGE = 2,
		R_BITS = 4,
		R_DOTS = 5,
		R_IP2 = 6
	};

	if (!mIPRangeRex.Exec(rang))
		return false;

	string temp;

	if (mIPRangeRex.PartFound(R_RANGE)) { // <from>-<to>
		if (mIPRangeRex.PartFound(R_DOTS)) {
			mIPRangeRex.Extract(R_IP1, rang, temp);
			cBanList::Ip2Num(temp, frdr);
			/*
			if (!cBanList::Ip2Num(temp, frdr))
				return false;
			*/
			mIPRangeRex.Extract(R_IP2, rang, temp);
			cBanList::Ip2Num(temp, todr);
			/*
			if (!cBanList::Ip2Num(temp, todr))
				return false;
			*/

		} else { // <addr>/<mask>
			mIPRangeRex.Extract(0, rang, temp);
			cBanList::Ip2Num(temp, frdr);
			/*
			if (!cBanList::Ip2Num(temp, frdr))
				return false;
			*/
			int pos = temp.find_first_of("/\\");
			istringstream is(temp.substr(pos + 1));
			is >> pos;
			frdr = frdr & (0xFFFFFFFF << (32 - pos));
			todr = frdr + (0xFFFFFFFF >> pos);
		}

	} else { // <addr>
		mIPRangeRex.Extract(R_IP1, rang, temp);
		cBanList::Ip2Num(temp, frdr);
		/*
		if (!cBanList::Ip2Num(temp, frdr))
			return false;
		*/
		todr = frdr;
	}

	return true;
}

}; // namespace nVerliHub
