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
#include <iomanip>
#include "ccommand.h"
#include "ctriggers.h"
#include "ccustomredirects.h"
#include "curr_date_time.h"
#include "config.h"
#include "cdcclients.h"
#include "i18n.h"
#include <algorithm>
#include <sys/resource.h>

#define PADDING 25

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
 	mCmdBan(int(eCM_BAN),".(del|rm|un|info|list|ls)?ban([^_\\s]+)?(_(\\d+\\S))?( this (nick|ip))? ?", "(\\S+)( (.*)$)?", &mFunBan),
	mCmdGag(int(eCM_GAG),".(un)?(gag|nochat|nopm|nochats|noctm|nodl|nosearch|kvip|maykick|noshare|mayreg|mayopchat|noinfo|mayinfo) ", "(\\S+)( (\\d+\\w))?", &mFunGag),
	mCmdTrigger(int(eCM_TRIGGER),".(ft|trigger)(\\S+) ", "(\\S+) (.*)", &mFunTrigger),
	mCmdSetVar(int(eCM_SET),".(set|=) ", "(\\[(\\S+)\\] )?(\\S+) (.*)", &mFunSetVar),
	mCmdRegUsr(int(eCM_REG),".r(eg)?(n(ew)?(user)?|del(ete)?|pass(wd)?|(en|dis)able|(set)?class|(protect|hidekick)(class)?|set|=|info|list|lst) ", "(\\S+)( (((\\S+) )?(.*)))?", &mFunRegUsr),
	mCmdRaw(int(eCM_RAW),".proto(\\S+)_(\\S+) ","(.*)", &mFunRaw),
	mCmdCmd(int(eCM_CMD),".cmd(\\S+)","(.*)", &mFunCmd),
	mCmdWho(int(eCM_WHO),".w(ho)?(\\S+) ","(.*)", &mFunWho),
	mCmdKick(int(eCM_KICK),".(kick|drop) ","(\\S+)( (.*)$)?", &mFunKick, eUR_KICK ),
	mCmdInfo(int(eCM_INFO),".(hub|server)info ?", "(\\S+)?", &mFunInfo),
	mCmdPlug(int(eCM_PLUG),".plug(in|out|list|reg|reload) ","(\\S+)( (.*)$)?", &mFunPlug),
	mCmdReport(int(eCM_REPORT),".report ","(\\S+)( (.*)$)?", &mFunReport),
	mCmdBc(int(eCM_BROADCAST),".(bc|broadcast|oc|ops|regs|guests|vips|cheefs|admins|masters)( |\\r\\n)","(.*)$", &mFunBc), // |ccbc|ccbroadcast
	mCmdGetConfig(int(eCM_GETCONFIG),".(gc|getconfig) ?","(\\S+)?", &mFunGetConfig),
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

int cDCConsole::OpCommand(const string &str, cConnDC * conn)
{
	if (!conn || !conn->mpUser) return 0;
	istringstream cmd_line(str);
	string cmd;
	ostringstream os;
	cmd_line >> cmd;
	cmd = toLower(cmd);
	string cmdid = cmd.substr (1);

	switch (conn->mpUser->mClass) {
		case eUC_MASTER:
			if (cmdid == "quit") return CmdQuit(cmd_line, conn, 0);
			if (cmdid == "restart") return CmdQuit(cmd_line, conn, 1);
			if (cmdid == "dbg_hash") { mOwner->mUserList.DumpProfile(cerr); return 1; }
			if (cmdid == "core_dump") return CmdQuit(cmd_line, conn, -1);
			if (cmdid == "hublist") { mOwner->RegisterInHublist(mOwner->mC.hublist_host, mOwner->mC.hublist_port, conn); return 1; }
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
				if(Log(0)) LogStream() << "Exception in commands: " << ex << endl;
			} catch (...) {
				if(Log(0)) LogStream() << "Exception in commands." << endl;
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
			if (cmdid == "me") return CmdMe(cmd_line, conn);
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
		o << autosprintf(_("Showing %d results"), c) << ":" << os.str();

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
				os << "\r\n [*] " << autosprintf(_("Unable to resolve users hostname: %s"), user->mNick.c_str());
		} else
			os << "\r\n [*] " << autosprintf(_("User not found: %s"), s.c_str());

		c++;
	}

	if (c == 0)
		o << _("Please specify one or more nicks separated by space.");
	else
		o << autosprintf(_("Showing %d results"), c) << ":" << os.str();

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
		o << autosprintf(_("Showing %d results"), c) << ":" << os.str();

	mOwner->DCPublicHS(o.str().c_str(), conn);
	return 1;
}

int cDCConsole::CmdQuit(istringstream &, cConnDC * conn, int code)
{
	ostringstream os;
	if (conn->Log(1)) conn->LogStream() << "Stopping hub with code: " << code << endl;
	os << _("Stopping hub.");
	mOwner->DCPublicHS(os.str(), conn);

	if (code >= 0)
		mOwner->stop(code);
	else
		*(int*)1 = 0;

	return 1;
}

bool cDCConsole::cfGetConfig::operator()()
{
	if (mConn->mpUser->mClass < eUC_ADMIN) {
		*mOS << _("No rights.");
		return false;
	}

	ostringstream os, lst;
	string file;
	cConfigBaseBase::tIVIt it;
	GetParStr(1, file);

	if (file.empty()) {
		file = mS->mDBConf.config_name;

		for (it = mS->mC.mvItems.begin(); it != mS->mC.mvItems.end(); it++)
			lst << " [*] " << mS->mC.mhItems.GetByHash(*it)->mName << " = " << *(mS->mC.mhItems.GetByHash(*it)) << "\r\n";
	} else
		mS->mSetupList.OutputFile(file.c_str(), lst);

	if (lst.str() == "")
		os << autosprintf(_("Configuration file %s is empty."), file.c_str());
	else
		os << autosprintf(_("Configuration file %s"), file.c_str()) << ":\r\n\r\n" << lst.str();

	mS->DCPrivateHS(os.str(), mConn);
	return true;
}

int cDCConsole::CmdHelp(istringstream &, cConnDC * conn)
{
	if(!conn || !conn->mpUser)
		return 1;
	string file;
	mTriggers->TriggerAll(eTF_HELP, conn);
	return 1;
}

int cDCConsole::CmdCCBroadcast(istringstream & cmd_line, cConnDC * conn, int cl_min, int cl_max)
{
	string start, end, str, cc_zone;
	ostringstream ostr;
	string tmpline;

	// test for existence of parameter
	cmd_line >> cc_zone;

	getline(cmd_line,str);
	while(cmd_line.good()) {
		tmpline="";
		getline(cmd_line,tmpline);
		str += "\r\n" + tmpline;
	}

	if(! str.size()) {
		ostr << _("Usage example: !ccbc :US:GB: <message>");
                mOwner->DCPublicHS(ostr.str(), conn);
		return 1;
	}
	cc_zone = toUpper(cc_zone);
	mOwner->mP.Create_PMForBroadcast(start,end,mOwner->mC.hub_security, conn->mpUser->mNick ,str);

	cTime TimeBefore, TimeAfter;
	if ( mOwner->LastBCNick != "disable")
		mOwner->LastBCNick = conn->mpUser->mNick;
	int count = mOwner->SendToAllWithNickCC(start,end, cl_min, cl_max, cc_zone);
	TimeAfter.Get();
	ostr << autosprintf(_("Message delivered to %d users in zone %s in %s."), count,  cc_zone.c_str(), (TimeAfter-TimeBefore).AsPeriod().AsString().c_str());
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
	// check if command is disabled
	if (mOwner->mC.disable_me_cmd) {
		mOwner->DCPublicHS(_("This functionality is currently disabled."), conn);
		return 1;
	}

	// check if user is allowed to use main chat
	if (!conn->mpUser->Can(eUR_CHAT, mOwner->mTime.Sec(), 0)) return 1;

	if (conn->mpUser->mClass < mOwner->mC.mainchat_class) {
		mOwner->DCPublicHS(_("Main chat is currently disabled for users with your class."), conn);
		return 1;
	}

	// prepare text
	string text;
	getline(cmd_line, text);
	string tmpline;

	while (cmd_line.good()) {
		tmpline = "";
		getline(cmd_line, tmpline);
		text += "\r\n" + tmpline;
	}

	if (text[0] == ' ') text = text.substr(1);

	// check message length
	if (conn->mpUser->mClass < eUC_VIPUSER && !cDCProto::CheckChatMsg(text, conn)) return 1;

	// send message
	ostringstream os;
	os << "** " << conn->mpUser->mNick << " " << text;
	string msg = os.str();
	mOwner->mUserList.SendToAll(msg, true);
	os.str(mOwner->mEmpty);
	return 1;
}

int cDCConsole::CmdChat(istringstream &cmd_line, cConnDC *conn, bool switchon)
{
	if (!conn) return 0;
	if (!conn->mpUser) return 0;

	if (switchon) { // chat
		if (!mOwner->mChatUsers.ContainsNick(conn->mpUser->mNick)) {
			mOwner->DCPublicHS(_("Now you will see public chat messages, to disable use +nochat command."), conn);
			mOwner->mChatUsers.Add(conn->mpUser);
		} else
			mOwner->DCPublicHS(_("You already see public chat messages, to disable use +nochat command."), conn);
	} else { // nochat
		if (mOwner->mChatUsers.ContainsNick(conn->mpUser->mNick)) {
			mOwner->DCPublicHS(_("Now you won't see public chat messages, to restore use +chat command."), conn);
			mOwner->mChatUsers.Remove(conn->mpUser);
		} else
			mOwner->DCPublicHS(_("You already don't see public chat messages, to restore use +chat command."), conn);
	}

	return 1;
}

int cDCConsole::CmdRInfo(istringstream &cmd_line, cConnDC *conn)
{
	if (!conn->mpUser)
		return 0;

	ostringstream os;
	string omsg;

	os << "Verlihub " << VERSION << " build " << __CURR_DATE_TIME__ << "\r\n";
	os << " == " << toUpper(_("Authors")) << " ==\r\n";
	os << "\tDaniel Muller (dan@verliba.cz)" << "\r\n";
	os << "\tDavide Simoncelli (netcelli@verlihub-project.org)" << "\r\n";
	os << "\tchaosuk (chaos@dchublist.com)" << "\r\n";
	os << "\tRoLex (webmaster@feardc.net)" << "\r\n";
	os << " == " << toUpper(_("Translators")) << " ==\r\n";
	os << "\tCzech (Uhlik), Italian (netcelli), Russian (plugman)" << "\r\n";
	os << " == " << toUpper(_("Contributors")) << " ==\r\n";
	os << "\tStefano, Intruder, RoLex, Frog" << "\r\n";
	os << " == " << toUpper(_("Web part")) << " ==\r\n";
	os << "\tStefano Simoncelli (netcelli@verlihub-project.org)" << "\r\n";
	os << " == " << toUpper(_("Credits")) << " ==\r\n";
	os << "\tWe would like to thank everyone in VAZ for their input and valuable support and of course everyone who continues to use this great hubsoft." << "\r\n";
	os << " == " << toUpper(_("More")) << " ==\r\n";
	os << "\tWebsite: http://verlihub-project.org/" << "\r\n";
	os << "\tManual: http://verlihub-project.org/doc/" << "\r\n";
	os << "\tSupport hub: dchub://hub.verlihub.net:7777/";

	omsg = os.str();
	mOwner->DCPublicHS(omsg, conn);
	return 1;
}

int cDCConsole::CmdUInfo(istringstream & cmd_line, cConnDC * conn)
{
	if (!conn->mpUser) return 0;
	string uType, cType, hubOwner;
	int sInt = 0;

	if (conn->GetTheoricalClass() == eUC_NORMUSER) {
		uType = _("Unregistered");
		sInt = mOwner->mC.int_search;
	} else if (conn->GetTheoricalClass() == eUC_REGUSER) {
		uType = _("Registered");
		sInt = mOwner->mC.int_search_reg;
	} else if (conn->GetTheoricalClass() == eUC_VIPUSER) {
		uType = _("VIP");
		sInt = mOwner->mC.int_search_vip;
	} else if (conn->GetTheoricalClass() == eUC_OPERATOR) {
		uType = _("Operator");
		sInt = mOwner->mC.int_search_op;
	} else if (conn->GetTheoricalClass() == eUC_CHEEF) {
		uType = _("Super OP");
		sInt = mOwner->mC.int_search_op;
	} else if (conn->GetTheoricalClass() == eUC_ADMIN) {
		uType = _("Admin");
		sInt = mOwner->mC.int_search_op;
	} else if (conn->GetTheoricalClass() == eUC_MASTER) {
		uType = _("Master");
		sInt = mOwner->mC.int_search_op;
	}

	if (!conn->mpUser->IsPassive == true) {
		cType = _("Active");
	} else {
		cType = _("Passive");
		sInt = mOwner->mC.int_search_pas;
	}

	ostringstream os;
	string omsg;
	os << "\r\n";

	if(!mOwner->mC.hub_owner.empty())
		hubOwner = mOwner->mC.hub_owner;
	else
		hubOwner = "--";

	os << " [*] " << setw(PADDING) << setiosflags(ios::left) << _("Hub owner") << hubOwner << endl;
	os << " [*] " << setw(PADDING) << setiosflags(ios::left) << _("Address") << mOwner->mC.hub_host.c_str() << endl;
	os << " [*] " << setw(PADDING) << setiosflags(ios::left) << _("Total users") << mServer->mUserCountTot << endl;
	os << " [*] " << setw(PADDING) << setiosflags(ios::left) << _("Total bots") <<  mServer->mRobotList.Size() << endl;
	os << " [*] " << setw(PADDING) << setiosflags(ios::left) << _("Total share") << convertByte(mServer->mTotalShare, false).c_str() << endl;

	// hub health
	mServer->mStatus = _("Not available");

	if (mServer->mFrequency.mNumFill > 0) {
		if (mServer->mSysLoad == eSL_RECOVERY) mServer->mStatus = _("Recovery mode");
		else if (mServer->mSysLoad == eSL_CAPACITY) mServer->mStatus = _("Near capacity");
		else if (mServer->mSysLoad == eSL_PROGRESSIVE) mServer->mStatus = _("Progressive mode");
		else if (mServer->mSysLoad == eSL_NORMAL) mServer->mStatus = _("Normal mode");
	}

	os << " [*] " << setw(PADDING) << setiosflags(ios::left) << _("Hub health") << mServer->mStatus.c_str() << endl;
	os << " [*] " << setw(PADDING) << setiosflags(ios::left) << _("Your status") << uType.c_str() << endl;
	os << " [*] " << setw(PADDING) << setiosflags(ios::left) << _("Your can search every") << autosprintf(ngettext("%d second", "%d seconds", sInt), sInt) << endl;
	os << " [*] " << setw(PADDING) << setiosflags(ios::left) << _("Connection type") << cType.c_str() << endl;
	os << " [*] " << setw(PADDING) << setiosflags(ios::left) << _("You are sharing") << convertByte(conn->mpUser->mShare, false).c_str();
	omsg = os.str();
	mOwner->DCPublicHS(omsg, conn);
	return 1;
}

int cDCConsole::CmdRegMe(istringstream & cmd_line, cConnDC * conn)
{
	ostringstream os;
	string regnick, prefix;
	if (mOwner->mC.disable_regme_cmd) {
		mOwner->DCPublicHS(_("This functionality is currently disabled."),conn);
		return 1;
	}
	if(mOwner->mC.autoreg_class > 3) {
		mOwner->DCPublicHS(_("Registration failed, please contact an operator for more help."),conn);
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

			if (text.size() < (unsigned int)mOwner->mC.password_min_len) {
				os << autosprintf(_("Minimum password length is %d characters, please retry."), mOwner->mC.password_min_len);
				mOwner->DCPublicHS(os.str(), conn);
				return 1;
			}

			if (mOwner->mR->AddRegUser(regnick, NULL, mOwner->mC.autoreg_class, text.c_str())) {
				os << autosprintf(_("A new user has been registered with class %d"), mOwner->mC.autoreg_class);
				mOwner->ReportUserToOpchat(conn, os.str(), false);
				os.str(mOwner->mEmpty);
				os << autosprintf(_("You are now registered with nick %s, please reconnect and login with your new password: %s"), regnick.c_str(), text.c_str());
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
			os << autosprintf(_("Registration request with password %s"), text.c_str());
		else
			os << _("Registration request without password");

		mOwner->ReportUserToOpchat(conn, os.str(), mOwner->mC.dest_regme_chat);
		mOwner->DCPublicHS(_("Thank you, your request has been sent to operators."), conn);
		return 1;
	}
}

int cDCConsole::CmdTopic(istringstream &cmd_line, cConnDC *conn)
{
	ostringstream os;
	string omsg,topic;
	getline(cmd_line,topic);
	if(conn->mpUser->mClass < mOwner->mC.topic_mod_class) {
		mOwner->DCPublicHS(_("You do not have permissions to change the topic."),conn);
		return 1;
	}
	if(topic[0] == ' ')
		topic = topic.substr(1);
	if(topic.length() > 255) {
		os << autosprintf(_("Topic must be max 255 characters long. Your topic was %d characters long."), (int) topic.length());
		mOwner->DCPublicHS(os.str().data(),conn);
		return 1;
	}

	mOwner->mC.hub_topic = topic;
	// Save topic
	cConfigItemBase *ci = mOwner->mC["hub_topic"];
	if(ci) {
		ci->ConvertFrom(topic);
		mOwner->mSetupList.SaveItem(mOwner->mDBConf.config_name.c_str(), ci);
	}

	cDCProto::Create_HubName(omsg, mOwner->mC.hub_name, topic);
	mOwner->SendToAll(omsg, eUC_NORMUSER, eUC_MASTER);
	if (topic.length())
		os << autosprintf(_("%s changed topic to: %s"), conn->mpUser->mNick.c_str(), topic.c_str());
	else
		os << autosprintf(_("%s removed topic."), conn->mpUser->mNick.c_str());
	mOwner->DCPublicHSToAll(os.str());
	return 1;
}

int cDCConsole::CmdKick(istringstream & cmd_line, cConnDC * conn)
{
	ostringstream os;
	string omsg, OtherNick, Reason;
	string tmpline;

	if(conn && conn->mpUser && conn->mpUser->Can(eUR_KICK, mOwner->mTime.Sec())) {
		cmd_line >> OtherNick;
		getline(cmd_line,Reason);
		while(cmd_line.good()) {
			tmpline="";
			getline(cmd_line,tmpline);
			Reason += "\r\n" + tmpline;
		}
		if (Reason[0] == ' ') Reason = Reason.substr(1);
		if (Reason.size() > 3) {
			mOwner->DCKickNick(&os, conn->mpUser, OtherNick, Reason, eKCK_Drop | eKCK_Reason | eKCK_PM| eKCK_TBAN);
		}
	}
	omsg = os.str();
	mOwner->DCPublicHS(omsg,conn);
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
		ostr << _("You are not allowed to change your password now, ask an operator.");
		mOwner->DCPrivateHS(ostr.str(),conn);
		mOwner->DCPublicHS(ostr.str(),conn);
		return 1;
	}

	cmd_line >> str >> crypt;
	if(str.size() < (unsigned int) mOwner->mC.password_min_len) {
		ostr << autosprintf(_("Minimum password length is %d characters, please retry."), mOwner->mC.password_min_len);
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
		omsg << _("Please use: !hideme <class> where <class> is the maximum class of users that may not see your actions.") << endl;
		mOwner->DCPublicHS(omsg.str(),conn);
		return 1;
	}
	if(cls > conn->mpUser->mClass) cls = conn->mpUser->mClass;
	conn->mpUser->mHideKicksForClass = cls;
	omsg << autosprintf(_("Your command actions are now hidden from users with class below %d."),  cls);
	mOwner->DCPublicHS(omsg.str(),conn);
	return 1;
}

int cDCConsole::CmdUserLimit(istringstream & cmd_line, cConnDC * conn)
{
	string str;
	ostringstream ostr;
	int minutes = 60, maximum = -1;
	cmd_line >> maximum >> minutes;

	if( maximum < 0 )
	{
		ostr << _("Type !help for more information (usage !userlimit <max_users> [<minutes>=60])");
		mOwner->DCPublicHS(ostr.str(), conn);
		return 1;
	}

	// 60 steps at most
	cInterpolExp *fn = new
		cInterpolExp(mOwner->mC.max_users_total, maximum, (60*minutes) / mOwner->timer_serv_period ,(6*minutes) / mOwner->timer_serv_period);
	mOwner->mTmpFunc.push_back((cTempFunctionBase *)fn);

	ostr << "Starting to update max_users variable to: " << maximum
		<< " (Duration: " << minutes << " minutes)";
	mOwner->DCPublicHS(ostr.str(), conn);

	return 1;
}

/** class user (change temporarily his class)
	!class <nick> <new_class_0_to_5>
*/
int cDCConsole::CmdClass(istringstream &cmd_line, cConnDC *conn)
{
	ostringstream os;
	string s;
	cUser * user;
	int nclass = 3, oclass, mclass = conn->mpUser->mClass;

	cmd_line >> s >> nclass;

	if(!s.size() || nclass < 0 || nclass > 5 || nclass >= mclass)
	{
		os << _("Usage: !class <nick> [<class>=3].") << " " << autosprintf(_("Maximum class is %d."), mclass);
		mOwner->DCPublicHS(os.str().c_str(),conn);
		return 1;
	}


	user = mOwner->mUserList.GetUserByNick(s);

	if(user && user->mxConn) {
		oclass = user->mClass;
		if(oclass < mclass) {
			os << autosprintf(_("Temporarily changed class to %d for user %s"), nclass, s.c_str()) << endl;
			user->mClass = (tUserCl) nclass;
			if ((oclass < eUC_OPERATOR) && (nclass >= eUC_OPERATOR)) {
				mOwner->mOpchatList.Add(user);
				if (!(user->mxConn && user->mxConn->mRegInfo && user->mxConn->mRegInfo->mHideKeys)) {
					mOwner->mOpList.Add(user);
					mOwner->mUserList.SendToAll(mOwner->mOpList.GetNickList(), false);
				}
			} else if ((oclass >= eUC_OPERATOR) && (nclass < eUC_OPERATOR)) {
				mOwner->mOpchatList.Remove(user);
				mOwner->mOpList.Remove(user);
				mOwner->mUserList.SendToAll(mOwner->mOpList.GetNickList(), false);
			}
		} else {
			os << autosprintf(_("You have no rights to change class of %s."), s.c_str()) << endl;
		}
	} else {
		os << autosprintf(_("User %s not found."), s.c_str()) << endl;
	}
	mOwner->DCPublicHS(os.str().c_str(),conn);
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
				os << autosprintf(_("Kicks of user %s are now hidden."), s.c_str()) << endl;
				user->mHideKick = true;
			} else {
				os << autosprintf(_("You have no rights to do this.")) << endl;
			}
		} else {
			os << autosprintf(_("User %s not found."), s.c_str()) << endl;
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
				os << autosprintf(_("Kicks of user %s are now visible."), s.c_str()) << endl;
				user->mHideKick = false;
			} else {
				os << _("You have no rights to do this.") << endl;
			}
		} else
			os << autosprintf(_("User %s not found."), s.c_str()) << endl;
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
		os << _("Use !protect <nickname> <class>.") << " " << _("Please type !help for more info.") << endl
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
		os << autosprintf(_("User %s not found."), s.c_str()) << endl;
	}
	mOwner->DCPublicHS(os.str().c_str(),conn);
	return 1;
}

int cDCConsole::CmdReload(istringstream &cmd_line, cConnDC *conn)
{
	ostringstream os;

	os << _("Reloading triggers, custom redirects, configuration and registration list cache.");
	mTriggers->ReloadAll();
	mRedirects->ReloadAll();
	mOwner->mC.Load();
	mOwner->DCPublicHS(os.str().c_str(),conn);
	if (mOwner->mC.use_reglist_cache)
		mOwner->mR->UpdateCache();
	return 1;
}

bool cDCConsole::cfReport::operator()()
{
	if (mS->mC.disable_report_cmd) {
		*mOS << _("Report command is currently disabled.");
		return false;
	}

	ostringstream os;
	string omsg, nick, reason;
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
	if (mConn->mpUser->mClass < eUC_ADMIN) return false;

	enum {eRW_ALL, eRW_USR, eRW_HELLO, eRW_PASSIVE, eRW_ACTIVE};
	static const char *actionnames[] = {"all", "user", "usr", "hello", "hel", "passive", "pas", "active", "act"};
	static const int actionids[] = {eRW_ALL, eRW_USR, eRW_USR, eRW_HELLO, eRW_HELLO, eRW_PASSIVE, eRW_PASSIVE, eRW_ACTIVE, eRW_ACTIVE};
	enum {eRC_HUBNAME, eRC_HELLO, eRC_QUIT, eRC_REDIR, eRC_PM, eRC_CHAT};
	static const char *cmdnames[] = {"hubname", "name", "hello", "quit", "redir", "move", "pm", "chat", "mc"};
	static const int cmdids[] = {eRC_HUBNAME, eRC_HUBNAME, eRC_HELLO, eRC_QUIT, eRC_REDIR, eRC_REDIR, eRC_PM, eRC_CHAT, eRC_CHAT};

	int Action = -1;
	int CmdID = -1;
	string tmp;
	mIdRex->Extract(1, mIdStr, tmp);
	Action = this->StringToIntFromList(tmp, actionnames, actionids, sizeof(actionnames) / sizeof(char*));
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
			theCommand = "$HubName ";
			break;
		case eRC_HELLO:
			theCommand = "$Hello ";
			break;
		case eRC_QUIT:
			cDCProto::Create_Quit(theCommand, "");
			break;
		case eRC_REDIR:
			theCommand = "$ForceMove ";
			break;
		case eRC_PM:
			mS->mP.Create_PMForBroadcast(theCommand, endOfCommand, mS->mC.hub_security, mConn->mpUser->mNick, param);
			WithNick = true;
			break;
		case eRC_CHAT:
			theCommand = "<" + mConn->mpUser->mNick +"> ";
			break;
		default:
			return false;
			break;
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

	static const char *cleanames[] = { "banlist", "unbanlist", "kicklist", "temprights" };
	enum { CLEAN_BAN, CLEAN_UNBAN, CLEAN_KICK, CLEAN_TRIGHTS };
	static const int cleanids[]= { CLEAN_BAN, CLEAN_UNBAN, CLEAN_KICK, CLEAN_TRIGHTS };

	if(!mConn->mpUser) return false;
	if(mConn->mpUser->mClass < eUC_OPERATOR) return false;

	string tmp;
	int CleanType = eBF_NICKIP;
	if(mIdRex->PartFound(1)) {
		mIdRex->Extract( 1, mIdStr, tmp);
		CleanType = this->StringToIntFromList(tmp, cleanames, cleanids, sizeof(cleanames)/sizeof(char*));
		if (CleanType < 0)
		    return false;
	}
	switch (CleanType) {
		case CLEAN_BAN:
			mS->mBanList->TruncateTable();
			(*mOS) << endl << _("All bans have been deleted.");
		break;
		case CLEAN_UNBAN:
			mS->mUnBanList->TruncateTable();
			(*mOS) << endl << _("All unbans have been deleted.");
		break;
		case CLEAN_KICK:
		  	mS->mKickList->TruncateTable();
			(*mOS) << endl << _("All kicks have been deleted.");
		break;
		case CLEAN_TRIGHTS:
			mS->mPenList->TruncateTable();
			(*mOS) << endl << _("All temporary rights have been deleted.");
		break;
		default:
			(*mOS) << _("This command is not implemented.") << "\r\n" << _("Available command are: ") << endl;
			return false;
		break;
	}
	return true;
}

bool cDCConsole::cfBan::operator()()
{
	if (!mConn->mpUser) return false;
	int MyClass = mConn->mpUser->mClass;

	if (MyClass < eUC_OPERATOR) {
		(*mOS) << _("You have no rights to do this.");
		return false;
	}

	static const char *bannames[] = {"nick", "ip", "nickip", "", "range", "host1", "host2", "host3", "hostr1", "share", "prefix"};
	static const int banids[] = {eBF_NICK, eBF_IP, eBF_NICKIP, eBF_NICKIP, eBF_RANGE, eBF_HOST1, eBF_HOST2, eBF_HOST3, eBF_HOSTR1, eBF_SHARE, eBF_PREFIX};
	enum {BAN_BAN, BAN_UNBAN, BAN_INFO, BAN_LIST};
	static const char *prefixnames[] = {"add", "new", "rm", "del", "un", "info", "check", "list", "ls"};
	static const int prefixids[] = {BAN_BAN, BAN_BAN, BAN_UNBAN, BAN_UNBAN, BAN_UNBAN, BAN_INFO, BAN_INFO, BAN_LIST, BAN_LIST};
	//".(del|rm|un|info|list|ls)?ban([^_\\s]+)?(_(\\d+\\S))?( this (nick|ip))? ?", "(\\S+)( (.*)$)?"
	enum {BAN_PREFIX = 1, BAN_TYPE = 2, BAN_LENGTH = 4, BAN_THIS = 6, BAN_WHO = 1, BAN_REASON = 3};
	string tmp;
	int BanAction = BAN_BAN;

	if (mIdRex->PartFound(BAN_PREFIX)) {
		mIdRex->Extract(BAN_PREFIX, mIdStr, tmp);
		BanAction = this->StringToIntFromList(tmp, prefixnames, prefixids, sizeof(prefixnames) / sizeof(char*));
		if (BanAction < 0) return false;
	}

	int BanType = eBF_NICKIP;

	if (mIdRex->PartFound(BAN_TYPE)) {
		mIdRex->Extract(BAN_TYPE, mIdStr, tmp);
		BanType = this->StringToIntFromList(tmp, bannames, banids, sizeof(bannames) / sizeof(char*));
		if (BanType < 0) return false;
	}

	bool IsNick = false;
	if (BanType == eBF_NICK) IsNick = true;
	if (mIdRex->PartFound(BAN_THIS)) IsNick = ((0 == mIdRex->Compare(BAN_TYPE, mIdStr, "nick")) || (BanType == eBF_NICK));
	time_t BanTime = 0;
	bool IsPerm = !mIdRex->PartFound(BAN_LENGTH);

	if (!IsPerm) {
		mIdRex->Extract(BAN_LENGTH, mIdStr, tmp);

		if (tmp != "perm") {
			BanTime = mS->Str2Period(tmp, *mOS);

			if (BanTime < 0) {
				(*mOS) << _("Please provide a valid ban time.");
				return false;
			}
		} else
			IsPerm = true;
	}

	cBan Ban(mS);
	cKick Kick;
	string Who;
	GetParStr(BAN_WHO, Who);
	bool unban = (BanAction == BAN_UNBAN);
	int BanCount = 100;
	int Count = 0;

	switch (BanAction) {
		case BAN_UNBAN:
		case BAN_INFO:
			if (unban) {
				if (!GetParStr(BAN_REASON, tmp)) tmp = _("No reason specified"); // default reason

				#ifndef WITHOUT_PLUGINS
				if (!mS->mCallBacks.mOnUnBan.CallAll(mConn->mpUser, Who, mConn->mpUser->mNick, tmp)) {
					(*mOS) << _("Action has been discarded by plugin.");
					return false;
				}
				#endif

				(*mOS) << _("Removed ban") << ":\r\n";
			} else
				(*mOS) << _("Ban information") << ":\r\n";

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
			} else
				Count += mS->mBanList->Unban(*mOS, Who, tmp, mConn->mpUser->mNick, BanType, unban);

			(*mOS) << "\r\n" << autosprintf(ngettext("%d ban found.", "%d bans found.", Count), Count);
		break;
	case BAN_BAN:
		Ban.mNickOp = mConn->mpUser->mNick;
		mParRex->Extract(BAN_REASON, mParStr, tmp);

		if (tmp.length() > 0)
			Ban.mReason = tmp;
		else
			Ban.mReason = _("No reason specified"); // default reason, doesnt work when no reason is specified, bad regexp

		Ban.mDateStart = cTime().Sec();

		if (BanTime)
			Ban.mDateEnd = Ban.mDateStart + BanTime;
		else
			Ban.mDateEnd = 0;

		Ban.SetType(BanType);

		switch (BanType) {
			case eBF_NICKIP:
			case eBF_NICK:
			case eBF_IP:
				if (mS->mKickList->FindKick(Kick, Who, mConn->mpUser->mNick, 3000, true, true, IsNick))
					mS->mBanList->NewBan(Ban, Kick, BanTime, BanType);
				else {
					if (BanType == eBF_NICKIP) BanType = eBF_IP;
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
		default: break;
		}

		#ifndef WITHOUT_PLUGINS
		if (!mS->mCallBacks.mOnNewBan.CallAll(mConn->mpUser, &Ban)) {
			(*mOS) << _("Action has been discarded by plugin.");
			return false;
		}
		#endif

		if ((BanType == eBF_IP) || (BanType == eBF_RANGE)) { // try to kick users if ban type is ip or range
			cUserCollection::iterator i;
			cConnDC *conn;

			for (i = mS->mUserList.begin(); i != mS->mUserList.end(); ++i) {
				conn = ((cUser*)(*i))->mxConn;

				if (conn) {
					unsigned long ipnum = cBanList::Ip2Num(conn->AddrIP());

					if (((BanType == eBF_IP) && (Ban.mIP == conn->AddrIP())) || ((BanType == eBF_RANGE) && (Ban.mRangeMin <= ipnum) && (Ban.mRangeMax >= ipnum))) {
						mS->DCKickNick(mOS, mConn->mpUser, (*i)->mNick, Ban.mReason, eKCK_Reason | eKCK_Drop);
						(*mOS) << "\r\n";
					}
				}
			}
		} else { // try to kick user if ban type is other
			cUser *user = mS->mUserList.GetUserByNick(Ban.mNick);

			if (user != NULL)
				mS->DCKickNick(mOS, mConn->mpUser, Ban.mNick, Ban.mReason, eKCK_Reason | eKCK_Drop);
		}

		mS->mBanList->AddBan(Ban);
		(*mOS) << _("Added new ban") << ":\r\n";
		Ban.DisplayComplete(*mOS);
		break;
	case BAN_LIST:
		{
			GetParInt(BAN_WHO, BanCount);
			ostringstream os;
			mS->mBanList->List(os, BanCount);
			mS->DCPrivateHS(os.str(), mConn);
		}
		break;
	default:
		(*mOS) << _("This command is not implemented, available commands are") << ":";
		return false;
		break;
	}

	return true;
}

bool cDCConsole::cfInfo::operator()()
{
	enum {eINFO_HUB, eINFO_SERVER };
	static const char * infonames [] = { "hub","server" };
	static const int infoids [] = { eINFO_HUB, eINFO_SERVER };

	string tmp;
	mIdRex->Extract(1,mIdStr,tmp);

	int InfoType = this->StringToIntFromList(tmp, infonames, infoids, sizeof(infonames)/sizeof(char*));
	if (InfoType < 0) return false;

	int MyClass = mConn->mpUser->mClass;
	if( MyClass < eUC_VIPUSER ) return false;

	switch(InfoType)
	{
		case eINFO_SERVER: mInfoServer.SystemInfo(*mOS); break;
		case eINFO_HUB: mInfoServer.Output(*mOS, MyClass); break;
		default : (*mOS) << _("This command is not implemented, available commands are: !hubinfo !serverinfo");
			return false;
	}

	return true;
}

bool cDCConsole::cfTrigger::operator()()
{
	string ntrigger;
	string text, cmd;
	if(mConn->mpUser->mClass < eUC_MASTER) return false;
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

bool cDCConsole::cfSetVar::operator()()
{
	string file(mS->mDBConf.config_name),var,val, fake_val;
	bool DeleteItem = false;

	if(mConn->mpUser->mClass < eUC_ADMIN)
		return false;

	// [file] variable value style
	if (mParRex->PartFound(2))
		mParRex->Extract(2,mParStr,file);

	mParRex->Extract(3,mParStr,var);
	// file.variable value style
	size_t pos  = var.find('.');
	if(pos != string::npos) {
		file = var.substr(0, pos);
		var = var.substr(pos+1);
	}
	mParRex->Extract(4,mParStr,val);

	cConfigItemBase *ci = NULL;
	if (file == mS->mDBConf.config_name) {
		ci = mS->mC[var];
		if(!ci) {
			(*mOS) << autosprintf(_("Undefined configuration variable: %s"), var.c_str());
			return false;
		}
	} else {
		DeleteItem = true;
		ci = new cConfigItemBaseString(fake_val,var);
		mS->mSetupList.LoadItem(file.c_str(), ci);
	}

	if(ci) {
		ostringstream oldValue;
		oldValue << *ci;
		ci->ConvertFrom(val);
		ostringstream newValue;
		newValue << *ci;
		(*mOS) << autosprintf(_("Updated configuration %s.%s from '%s' to '%s'."), file.c_str(), var.c_str(), oldValue.str().c_str(), newValue.str().c_str());
		mS->mSetupList.SaveItem(file.c_str(), ci);
		if(DeleteItem)
			delete ci;
		ci = NULL;
	}

	struct rlimit userLimit;
	// Get maximum file descriptor number
	if(!getrlimit(RLIMIT_NOFILE, &userLimit) && userLimit.rlim_cur < mS->mC.max_users_total)
		(*mOS) << "\n" << autosprintf(_("Warning: Verlihub allows maximum %d users, but current resource limit is %d. Consider to run ulimit -n <max_users> and restart the hub."), mS->mC.max_users_total, (int) userLimit.rlim_cur);
	return true;
}

bool cDCConsole::cfGag::operator()()
{
	enum {eAC_GAG, eAC_NOPM, eAC_NOCHATS, eAC_NODL, eAC_NOSEARCH, eAC_KVIP, eAC_NOSHARE, eAC_CANREG, eAC_OPCHAT, eAC_NOINFO};
	static const char *actionnames[] = {"gag", "nochat", "nopm", "nochats", "noctm", "nodl", "nosearch", "kvip", "maykick", "noshare", "mayreg", "mayopchat", "noinfo", "mayinfo"};
	static const int actionids[] = {eAC_GAG, eAC_GAG, eAC_NOPM, eAC_NOCHATS, eAC_NODL, eAC_NODL, eAC_NOSEARCH, eAC_KVIP, eAC_KVIP, eAC_NOSHARE, eAC_CANREG, eAC_OPCHAT, eAC_NOINFO, eAC_NOINFO};
	string cmd;
	mIdRex->Extract(2, mIdStr, cmd);
	int Action = this->StringToIntFromList(cmd, actionnames, actionids, sizeof(actionnames) / sizeof(char*));
	if (Action < 0) return false;

	if (mConn->mpUser->mClass < eUC_OPERATOR) {
		(*mOS) << _("You have no rights to do this.");
		return false;
	}

	if (Action == eAC_NOINFO) {
		string nick;
		mParRex->Extract(1, mParStr, nick);
		cUser *user = mS->mUserList.GetUserByNick(nick);

		if (user && user->mxConn) {
			user->DisplayRightsInfo(*mOS, true);
			return true;
		} else {
			(*mOS) << autosprintf(_("%s is not in list."), nick.c_str());
			return false;
		}
	}

	string nick, howlong;
	time_t period = 24 * 3600 * 7;
	time_t Now = 1;
	bool isUn = false;
	isUn = mIdRex->PartFound(1);
	mParRex->Extract(1, mParStr, nick);

	if (mParRex->PartFound(3)) {
		mParRex->Extract(3, mParStr, howlong);
		period = mS->Str2Period(howlong, *mOS);
		if (!period) return false;
	}

	cPenaltyList::sPenalty penalty;
	penalty.mNick = nick;
	penalty.mOpNick = mConn->mpUser->mNick;
	if (!isUn) Now = cTime().Sec() + period;

	switch (Action) {
		case eAC_GAG: penalty.mStartChat = Now; break;
		case eAC_NOPM: penalty.mStartPM = Now; break;
		case eAC_NOCHATS: penalty.mStartChat = Now; penalty.mStartPM = Now; break;
		case eAC_NODL: penalty.mStartCTM = Now; break;
		case eAC_NOSEARCH: penalty.mStartSearch = Now; break;
		case eAC_KVIP: penalty.mStopKick = Now; break;
		case eAC_OPCHAT: penalty.mStopOpchat = Now; break;
		case eAC_NOSHARE: penalty.mStopShare0 = Now; break;
		case eAC_CANREG: penalty.mStopReg = Now; break;
		default: return false;
	};

	bool ret = false;

	if (!isUn)
		ret = mS->mPenList->AddPenalty(penalty);
	else
		ret = mS->mPenList->RemPenalty(penalty);

	cUser *usr = mS->mUserList.GetUserByNick(nick);

	if (usr != NULL) {
		switch (Action) {
			case eAC_GAG: usr->SetRight(eUR_CHAT, penalty.mStartChat, isUn, true); break;
			case eAC_NOPM: usr->SetRight(eUR_PM, penalty.mStartPM, isUn, true); break;
			case eAC_NOCHATS: usr->SetRight(eUR_CHAT, penalty.mStartChat, isUn, true); usr->SetRight(eUR_PM, penalty.mStartPM, isUn, true); break;
			case eAC_NODL: usr->SetRight(eUR_CTM, penalty.mStartCTM, isUn, true); break;
			case eAC_NOSEARCH: usr->SetRight(eUR_SEARCH, penalty.mStartSearch, isUn, true); break;
			case eAC_NOSHARE: usr->SetRight(eUR_NOSHARE, penalty.mStopShare0, isUn, true); break;
			case eAC_CANREG: usr->SetRight(eUR_REG, penalty.mStopReg, isUn, true); break;
			case eAC_KVIP: usr->SetRight(eUR_KICK, penalty.mStopKick, isUn, true); break;
			case eAC_OPCHAT: usr->SetRight(eUR_OPCHAT, penalty.mStopOpchat, isUn, true); break;
			default: return false;
		};

		// apply rights to user object
		usr->ApplyRights(penalty);
		cUserCollection::tHashType Hash = mS->mUserList.Nick2Hash(usr->mNick);

		// apply operator chat right
		if (usr->Can(eUR_OPCHAT, mS->mTime.Sec()))
			mS->mOpchatList.AddWithHash(usr, Hash);
		else {
			if (mS->mOpchatList.ContainsHash(Hash)) mS->mOpchatList.RemoveByHash(Hash);
		}
	}

	ostringstream description;
	description << penalty;

	if (ret) {
		if (description.str() == "") {
			switch (Action) {
				case eAC_GAG: (*mOS) << autosprintf(_("Resetting main chat right for %s."), penalty.mNick.c_str()); break;
				case eAC_NOPM: (*mOS) << autosprintf(_("Resetting private chat right for %s."), penalty.mNick.c_str()); break;
				case eAC_NOCHATS: (*mOS) << autosprintf(_("Resetting main and private chat rights for %s."), penalty.mNick.c_str()); break;
				case eAC_NODL: (*mOS) << autosprintf(_("Resetting download right for %s."), penalty.mNick.c_str()); break;
				case eAC_NOSEARCH: (*mOS) << autosprintf(_("Resetting search right for %s."), penalty.mNick.c_str()); break;
				case eAC_NOSHARE: (*mOS) << autosprintf(_("Resetting hidden share right for %s."), penalty.mNick.c_str()); break;
				case eAC_CANREG: (*mOS) << autosprintf(_("Resetting registering right for %s."), penalty.mNick.c_str()); break;
				case eAC_KVIP: (*mOS) << autosprintf(_("Resetting kick right for %s."), penalty.mNick.c_str()); break;
				case eAC_OPCHAT: (*mOS) << autosprintf(_("Resetting operator chat right for %s."), penalty.mNick.c_str()); break;
				default: return false;
			};
		} else
			(*mOS) << description.str().c_str();
	} else
		(*mOS) << autosprintf(_("Error setting right for %s."), penalty.mNick.c_str());

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
	enum { eAC_IP, eAC_RANGE, eAC_CC};
	static const char * actionnames [] = { "ip" , "range", "subnet", "cc" };
	static const int actionids [] = { eAC_IP, eAC_RANGE, eAC_RANGE, eAC_CC};

	if(this->mConn->mpUser->mClass < eUC_OPERATOR)
		return false;

	string tmp;
	mIdRex->Extract(2,mIdStr,tmp);
	int Action = this->StringToIntFromList(tmp, actionnames, actionids, sizeof(actionnames)/sizeof(char*));
	if(Action < 0)
		return false;

	string separator("\r\n\t");
	string userlist, actionName;

	mParRex->Extract(0, mParStr,tmp);
	unsigned long ip_min, ip_max;
	int cnt = 0;

	switch(Action) {
		case eAC_IP:
			ip_min = cBanList::Ip2Num(tmp);
			ip_max = ip_min;
			cnt = mS->WhoIP(ip_min, ip_max, userlist, separator, true);
			if(cnt)
				(*mOS) << autosprintf(ngettext("Found %d user with IP %s:", "Found %d users with IP %s:", cnt), cnt, tmp.c_str());
		break;
		case eAC_RANGE:
			if(! cDCConsole::GetIPRange(tmp, ip_min, ip_max) ) return false;
			cnt = mS->WhoIP(ip_min, ip_max, userlist, separator, false);
			if(cnt)
				(*mOS) << autosprintf(ngettext("Found %d user with range %s:", "Found %d users with range %s:", cnt), cnt, tmp.c_str());
		break;
		case eAC_CC:
			if(tmp.size() != 2) {
				(*mOS) << _("Country code must be 2 characters long, for example US.");
				return false;
			}

			tmp = toUpper(tmp);
			cnt = mS->WhoCC(tmp, userlist, separator);
			if(cnt)
				(*mOS) << autosprintf(ngettext("Found %d user with country code %s:", "Found %d users with country code %s:", cnt), cnt, tmp.c_str());
		break;
		default: return false;
	}

	if(cnt)
		(*mOS) << userlist;
	else
		(*mOS) << _("No users found.");
	return true;
}

bool cDCConsole::cfKick::operator()()
{
	enum { eAC_KICK, eAC_DROP };
	static const char * actionnames [] = { "kick", "drop" };
	static const int actionids [] = { eAC_KICK, eAC_DROP };

	if (this->mConn->mpUser->mClass < eUC_VIPUSER) return false;
	string tmp;
	mIdRex->Extract(1,mIdStr,tmp);
	int Action = this->StringToIntFromList(tmp, actionnames, actionids, sizeof(actionnames)/sizeof(char*));
	if (Action < 0) return false;

	string nick, text;

	mParRex->Extract(1,mParStr,nick);

	ostringstream os;
	string CoolNick, ostr;

	switch(Action) {
		case eAC_KICK:
			if(!mParRex->PartFound(2)) {
				(*mOS) << _("Please provide a valid reason.") << endl;
				return false;
			}
			mParRex->Extract(2,mParStr,text);
		case eAC_DROP:
			mS->DCKickNick(mOS, this->mConn->mpUser, nick, text,
				(Action == eAC_KICK) ?
				(eKCK_Drop | eKCK_Reason | eKCK_PM| eKCK_TBAN) :
				(eKCK_Drop | eKCK_Reason));
		break;
		default: (*mOS) << _("This command is not implemented.") << endl;
		return false;
	};
	return true;
}

bool cDCConsole::cfPlug::operator()()
{
	enum { eAC_IN, eAC_OUT, eAC_LIST, eAC_REG, eAC_RELAOD };
	static const char * actionnames [] = { "in","out","list","reg","reload", NULL};
	static const int actionids [] = { eAC_IN, eAC_OUT, eAC_LIST, eAC_REG, eAC_RELAOD };

	if (this->mConn->mpUser->mClass < mS->mC.plugin_mod_class)
	{
		(*mOS) << _("You have no rights to do this.");
		return false;
	}

	string tmp;
	mIdRex->Extract(1,mIdStr,tmp);
	int Action = this->StringToIntFromList(tmp, actionnames, actionids, sizeof(actionnames)/sizeof(char*));
	if (Action < 0) return false;

	switch (Action)
	{
		case eAC_LIST:
			(*mOS) << _("Loaded plugins") << ":\r\n";
			mS->mPluginManager.List(*mOS);
			break;
		case eAC_REG:
			(*mOS) << _("Available callbacks") << ":\r\n";
			mS->mPluginManager.ListAll(*mOS);
			break;
		case eAC_OUT:
			if(mParRex->PartFound(1)) {
				mParRex->Extract(1, mParStr, tmp);
				if(!mS->mPluginManager.UnloadPlugin(tmp))
					return false;
			}
			break;
		case eAC_IN:
			if(mParRex->PartFound(1)) {
				mParRex->Extract(1, mParStr, tmp);
				if(!mS->mPluginManager.LoadPlugin(tmp)) {
					(*mOS) << mS->mPluginManager.GetError();
					return false;
				}
			}
			break;
		case eAC_RELAOD:
			if(GetParStr(1, tmp)) {
				if(!mS->mPluginManager.ReloadPlugin(tmp)) {
					(*mOS) << mS->mPluginManager.GetError();
					return false;
				}
			}
			break;
		default: break;
	}
	return true;
}

bool cDCConsole::cfRegUsr::operator()()
{
	if ((this->mConn == NULL) || (this->mConn->mpUser == NULL)) return false;
	enum {eAC_NEW, eAC_DEL, eAC_PASS, eAC_ENABLE, eAC_DISABLE, eAC_CLASS, eAC_PROTECT, eAC_HIDEKICK, eAC_SET, eAC_INFO, eAC_LIST};
	static const char *actionnames[] = {"n", "new", "newuser", "del", "delete", "pass", "passwd", "enable", "disable", "class", "setclass", "protect", "protectclass", "hidekick", "hidekickclass", "set", "=", "info", "list", "lst"};
	static const int actionids[] = {eAC_NEW, eAC_NEW, eAC_NEW, eAC_DEL, eAC_DEL, eAC_PASS, eAC_PASS, eAC_ENABLE, eAC_DISABLE, eAC_CLASS, eAC_CLASS, eAC_PROTECT, eAC_PROTECT, eAC_HIDEKICK, eAC_HIDEKICK, eAC_SET, eAC_SET, eAC_INFO, eAC_LIST, eAC_LIST};
	string tmp;
	mIdRex->Extract(2, mIdStr, tmp);
	int Action = this->StringToIntFromList(tmp, actionnames, actionids, sizeof(actionnames) / sizeof(char*));
	if (Action < 0) return false;
	int MyClass = this->mConn->mpUser->mClass;

	if ((MyClass < eUC_OPERATOR) || ((Action != eAC_INFO) && (!this->mConn->mpUser->Can(eUR_REG, mS->mTime.Sec())))) {
		(*mOS) << _("You have no rights to do this.");
		return false;
	}

	if (mS->mC.classdif_reg > 10) {
		(*mOS) << _("Valid classdif_reg value must be between 1 and 5, please correct this first.");
		return false;
	}

	if (Action == eAC_LIST) {
		int cls = 0;
		this->GetParInt(1, cls);

		/*if (!this->GetParInt(1, cls)) {
			(*mOS) << _("Missing class parameter.");
			return false;
		}*/

		if ((MyClass < eUC_MASTER) && (cls >= 0) && (cls > (MyClass - mS->mC.classdif_reg))) {
			(*mOS) << _("You have no rights to do this.");
			return false;
		}

		ostringstream lst;
		int res = mS->mR->ShowUsers(this->mConn, lst, cls);

		if (res != 0)
			(*mOS) << autosprintf(_("Found %d registered users with class %d"), res, cls) << ":\r\n\r\n" << lst.str().c_str();
		else
			(*mOS) << autosprintf(_("No registered users with class %d found."), cls);

		return true;
	}

	string nick, par, field, pass;
	int ParClass = 1;
	mParRex->Extract(1, mParStr, nick);
	bool WithPar = false;
	WithPar = mParRex->PartFound(3);
	if (Action != eAC_SET && WithPar) mParRex->Extract(3, mParStr, par);
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
		if (WithPar) mParRex->Extract(6, mParStr, par);
	}

	cUser *user = mS->mUserList.GetUserByNick(nick);
	cRegUserInfo ui;
	ostringstream ostr;
	bool RegFound = mS->mR->FindRegInfo(ui, nick);
	bool authorized = false;

	// check rights
	if (RegFound && ((MyClass < eUC_MASTER) && !((MyClass >= (int)(ui.mClass + mS->mC.classdif_reg) && MyClass >= (ui.mClassProtect)) || ((Action == eAC_INFO) && (MyClass >= (ui.mClass - 1)))))) {
		(*mOS) << _("You have no rights to do this.");
		return false;
	}

	switch (Action) {
		case eAC_CLASS:
		case eAC_PROTECT:
		case eAC_HIDEKICK:
		case eAC_NEW:
			std::istringstream lStringIS(par);
			lStringIS >> ParClass;
			break;
	};

	switch (Action) {
		case eAC_SET:
			authorized = RegFound && ((MyClass >= eUC_ADMIN) && (MyClass > ui.mClass) && (field != "class"));
			break;
		case eAC_NEW:
			authorized = !RegFound && (MyClass >= (int)(ParClass + mS->mC.classdif_reg));
			break;
		case eAC_PASS:
		case eAC_HIDEKICK:
		case eAC_ENABLE:
		case eAC_DISABLE:
		case eAC_DEL:
			authorized = RegFound && (MyClass >= (int)(ui.mClass + mS->mC.classdif_reg));
			break;
		case eAC_CLASS:
			authorized = RegFound && (MyClass >= (int)(ui.mClass + mS->mC.classdif_reg)) && (MyClass >= (int)(ParClass + mS->mC.classdif_reg));
			break;
		case eAC_PROTECT:
			authorized = RegFound && (MyClass >= (int)(ui.mClass + mS->mC.classdif_reg)) && (MyClass >= (ParClass + 1));
			break;
		case eAC_INFO:
			authorized = RegFound && (MyClass >= eUC_OPERATOR);
			break;
	};

	if (MyClass == eUC_MASTER) authorized = RegFound || (!RegFound && (Action == eAC_NEW));

	if (!authorized) {
		if (!RegFound)
			(*mOS) << autosprintf(_("Registered user not found: %s"), nick.c_str());
		else if (Action == eAC_NEW)
			(*mOS) << autosprintf(_("%s is already registered."), nick.c_str());
		else
			(*mOS) << _("You have no rights to do this.");

		return false;
	}

 	if ((Action >= eAC_CLASS) && (Action <= eAC_SET) && !WithPar) {
 		(*mOS) << _("Missing command parameters.");
 		return false;
	}

	switch (Action) {
		case eAC_NEW: // new
			if (RegFound) {
				*mOS << autosprintf(_("%s is already registered."), nick.c_str());
				return false;
			}

			#ifndef WITHOUT_PLUGINS
			if (!mS->mCallBacks.mOnNewReg.CallAll(this->mConn->mpUser, nick, ParClass)) {
				(*mOS) << _("Action has been discarded by plugin.");
				return false;
			}
			#endif

			if (mS->mR->AddRegUser(nick, mConn, ParClass, (WithPass ? pass.c_str() : NULL))) {
				if (user && user->mxConn) {
					ostr.str(mS->mEmpty);

					if (!WithPass)
						ostr << autosprintf(_("You have been registered with class %d, please set up your password with +passwd <password> command."), ParClass);
					else
						ostr << autosprintf(_("You have been registered with class %d and following password: %s"), ParClass, pass.c_str());

					mS->DCPrivateHS(ostr.str(), user->mxConn);
					// todo: use password request here alwell, send_pass_request
					// todo: no reconnect required
				}

				if (!WithPass)
					(*mOS) << autosprintf(_("%s has been registered with class %d, please tell him to set his password."), nick.c_str(), ParClass);
				else
					(*mOS) << autosprintf(_("%s has been registered with class %d and password."), nick.c_str(), ParClass);
			} else {
				(*mOS) << _("Error adding new user.");
				return false;
			}

			break;

		case eAC_DEL: // delete
			#ifndef WITHOUT_PLUGINS
			if (!mS->mCallBacks.mOnDelReg.CallAll(this->mConn->mpUser, nick, ui.mClass)) {
				(*mOS) << _("Action has been discarded by plugin.");
				return false;
			}
			#endif

			if (mS->mR->DelReg(nick)) {
				ostringstream userInfo;
				userInfo << ui;
				(*mOS) << autosprintf(_("%s has been unregistered, user information"), nick.c_str()) << ":\r\n" << userInfo.str();
				return true;
			} else {
				ostringstream userInfo;
				userInfo << ui;
				(*mOS) << autosprintf(_("Error unregistering %s, user information"), nick.c_str()) << ":\r\n" << userInfo.str();
				return false;
			}

			break;
		case eAC_PASS: // pass
			if (WithPar) {
				#if ! defined _WIN32
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
				ostr << _("You can change your password now, use +passwd command followed by your new password.");
			}

			break;
		case eAC_CLASS: // class
			#ifndef WITHOUT_PLUGINS
			if (!mS->mCallBacks.mOnUpdateClass.CallAll(this->mConn->mpUser, nick, ui.mClass, ParClass)) {
				(*mOS) << _("Action has been discarded by plugin.");
				return false;
			}
			#endif

			field = "class";
			ostr << autosprintf(_("Your class has been changed to %s, changes will take effect on next login."), par.c_str());
		break;
		case eAC_ENABLE: //enable
			field = "enabled";
			par = "1";
			WithPar = true;
			ostr << _("Your registration has been enabled.");
			break;
		case eAC_DISABLE: // disable
			field = "enabled";
			par = "0";
			WithPar = true;
			ostr << _("Your registration has been disabled.");
			break;
		case eAC_PROTECT: // protect
			field = "class_protect";
			ostr << autosprintf(_("You are protected by classes greater than %s."), par.c_str());
			if (user) user->mProtectFrom = ParClass;
			break;
		case eAC_HIDEKICK: // hidekick
			field = "class_hidekick";
			ostr << autosprintf(_("You kicks are hidden by classes greater than %s."), par.c_str());
			break;
		case eAC_SET: // set
			break;
		case eAC_INFO:
			(*mOS) << autosprintf(_("%s registration information"), ui.GetNick().c_str()) << ":\r\n" << ui;
			break;
		default:
			mIdRex->Extract(1, mIdStr, par);
			(*mOS) << _("This command is not implemented.");
			return false;
			break;
	}

	if ((WithPar && (Action >= eAC_ENABLE) && (Action <= eAC_SET)) || ((Action == eAC_PASS) && !WithPar)) {
		if (mS->mR->SetVar(nick, field, par)) {
			(*mOS) << autosprintf(_("Updated variable %s to value %s for user %s."), field.c_str(), par.c_str(), nick.c_str());
			if (user && user->mxConn && ostr.str().size()) mS->DCPrivateHS(ostr.str(), user->mxConn);
			return true;
		} else {
			(*mOS) << autosprintf(_("Error setting variable %s to value %s for user %s."), field.c_str(), par.c_str(), nick.c_str());
			return false;
		}
	}

	return true;
}

bool cDCConsole::cfRedirToConsole::operator()()
{
	if (this->mConn->mpUser->mClass < eUC_OPERATOR)
		return false;
	if(this->mConsole != NULL)
		return mConsole->OpCommand(mIdStr + mParStr, mConn);
	else return false;
}

bool cDCConsole::cfBc::operator()()
{
	enum { eBC_BC, eBC_OC, eBC_GUEST, eBC_REG, eBC_VIP, eBC_CHEEF, eBC_ADMIN, eBC_MASTER, eBC_CC };
	enum { eBC_ALL, eBC_MSG };
	const char *cmds[] = { "bc","broadcast","oc","ops","guests","regs","vips","cheefs","admins",",masters","ccbc","ccbroadcast", NULL};
	const int nums[] = {eBC_BC,eBC_BC, eBC_OC, eBC_OC, eBC_GUEST, eBC_REG, eBC_VIP, eBC_CHEEF, eBC_ADMIN, eBC_MASTER, eBC_CC, eBC_CC };
	string message;
	int cmdid;

	if (!GetIDEnum(1,cmdid, cmds, nums)) return false;

	GetParStr(eBC_MSG, message);
	//rights to broadcast
	int MinClass = mS->mC.min_class_bc;
	int MaxClass = eUC_MASTER;
	int MyClass = this->mConn->mpUser->mClass;
	int AllowedClass = eUC_MASTER;
	switch(cmdid)
	{
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
		default: break;
	}

	if (MyClass < AllowedClass)
	{
		*mOS << _("You have no rights to do this.");
		return false;
	}

	string start, end;
	mS->mP.Create_PMForBroadcast(start,end,mS->mC.hub_security, this->mConn->mpUser->mNick ,message);
	cTime TimeBefore, TimeAfter;
	if(mS->LastBCNick != "disable")
		mS->LastBCNick = mConn->mpUser->mNick;
	int count = mS->SendToAllWithNick(start,end, MinClass, MaxClass);
	TimeAfter.Get();
	*mOS << autosprintf(_("Message delivered to %d users in %s") , count, (TimeAfter-TimeBefore).AsPeriod().AsString().c_str());
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
			unsigned long mask = 0;
			unsigned long addr1 = 0;
			unsigned long addr2 = 0xFFFFFFFF;
			mIPRangeRex.Extract(0, range, tmp);
			from = cBanList::Ip2Num(tmp);
			int i;
			i = tmp.find_first_of("/\\");
			istringstream is(tmp.substr(i+1));
			mask = from;
			is >> i;
			addr1 = mask & (0xFFFFFFFF << (32-i));
			addr2 = addr1 + (0xFFFFFFFF >> i);
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
