/*
	Copyright (C) 2003-2005 Daniel Muller, dan at verliba dot cz
	Copyright (C) 2006-2018 Verlihub Team, info at verlihub dot net

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

#include "cserverdc.h"
#include "cban.h"
#include "cbanlist.h"
#include "creglist.h"
#include "cusercollection.h"
#include "script_api.h"
#include "cconfigitembase.h"
#include "i18n.h"

using namespace std;

namespace nVerliHub {
	using namespace nSocket;
	using namespace nEnums;
	using namespace nMySQL;

cServerDC* GetCurrentVerlihub()
{
	return (cServerDC*)cServerDC::sCurrentServer;
}

cUser* GetUser(const char *nick)
{
	cServerDC *serv = GetCurrentVerlihub();

	if (!serv) {
		cerr << "Server not found" << endl;
		return NULL;
	}

	cUser *user = serv->mUserList.GetUserByNick(nick);
	return user; // user without connection, a bot, must be returned as well, its up to the call to check for connection
}

bool SendDataToUser(const char *data, const char *nick, bool delay)
{
	if (!data || !nick)
		return false;

	cUser *user = GetUser(nick);

	if (!user || !user->mxConn)
		return false;

	string omsg(data);
	const bool pipe = CheckDataPipe(omsg);
	user->mxConn->Send(omsg, pipe, !delay);
	return true;
}

bool SendToClass(const char *data, int min_class, int max_class, bool delay)
{
	if (!data || (min_class > max_class))
		return false;

	cServerDC *serv = GetCurrentVerlihub();

	if (!serv) {
		cerr << "Server not found" << endl;
		return false;
	}

	string omsg(data);
	const bool pipe = CheckDataPipe(omsg);
	serv->mUserList.SendToAllWithClass(omsg, min_class, max_class, delay, pipe);
	return true;
}

bool SendToAll(const char *data, bool delay)
{
	if (!data)
		return false;

	cServerDC *serv = GetCurrentVerlihub();

	if (!serv) {
		cerr << "Server not found" << endl;
		return false;
	}

	string omsg(data);
	const bool pipe = CheckDataPipe(omsg);
	serv->mUserList.SendToAll(omsg, delay, pipe);
	return true;
}

bool SendToActive(const char *data, bool delay)
{
	if (!data)
		return false;

	cServerDC *serv = GetCurrentVerlihub();

	if (!serv) {
		cerr << "Server not found" << endl;
		return false;
	}

	string omsg(data);
	const bool pipe = CheckDataPipe(omsg);
	serv->mActiveUsers.SendToAll(omsg, delay, pipe);
	return true;
}

bool SendToActiveClass(const char *data, int min_class, int max_class, bool delay)
{
	if (!data || (min_class > max_class))
		return false;

	cServerDC *serv = GetCurrentVerlihub();

	if (!serv) {
		cerr << "Server not found" << endl;
		return false;
	}

	string omsg(data);
	const bool pipe = CheckDataPipe(omsg);
	serv->mActiveUsers.SendToAllWithClass(omsg, min_class, max_class, delay, pipe);
	return true;
}

bool SendToPassive(const char *data, bool delay)
{
	if (!data)
		return false;

	cServerDC *serv = GetCurrentVerlihub();

	if (!serv) {
		cerr << "Server not found" << endl;
		return false;
	}

	string omsg(data);
	const bool pipe = CheckDataPipe(omsg);
	serv->mPassiveUsers.SendToAll(omsg, delay, pipe);
	return true;
}

bool SendToPassiveClass(const char *data, int min_class, int max_class, bool delay)
{
	if (!data || (min_class > max_class))
		return false;

	cServerDC *serv = GetCurrentVerlihub();

	if (!serv) {
		cerr << "Server not found" << endl;
		return false;
	}

	string omsg(data);
	const bool pipe = CheckDataPipe(omsg);
	serv->mPassiveUsers.SendToAllWithClass(omsg, min_class, max_class, delay, pipe);
	return true;
}

bool SendPMToAll(const char *data, const char *from, int min_class, int max_class)
{
	if (!data || !from || (min_class > max_class))
		return false;

	cServerDC *serv = GetCurrentVerlihub();

	if (!serv) {
		cerr << "Server not found" << endl;
		return false;
	}

	string start, end;
	serv->mP.Create_PMForBroadcast(start, end, from, from, data, false); // dont reserve for pipe, buffer is copied before sending
	serv->SendToAllWithNick(start, end, min_class, max_class);
	return true;
}

bool SendToChat(const char *nick, const char *text, int min_class, int max_class)
{
	if (!nick || !text || (min_class > max_class))
		return false;

	cServerDC *serv = GetCurrentVerlihub();

	if (!serv) {
		cerr << "Server not found" << endl;
		return false;
	}

	string omsg;
	serv->mP.Create_Chat(omsg, nick, text, true); // reserve for pipe
	serv->mChatUsers.SendToAllWithClass(omsg, min_class, max_class, serv->mC.delayed_chat, true);
	return true;
}

bool SendToOpChat(const char *data, const char *nick)
{
	if (!data)
		return false;

	cServerDC *serv = GetCurrentVerlihub();

	if (!serv || !serv->mOpChat) {
		cerr << "Server not found" << endl;
		return false;
	}

	cUser *user = NULL;

	if (nick && nick[0] != '\0')
		user = GetUser(nick);

	serv->mOpChat->SendPMToAll(data, (user ? user->mxConn : NULL), true, false);
	return true;
}

bool KickUser(const char *oper, const char *nick, const char *why, const char *note_op, const char *note_usr)
{
	if (!oper || !nick)
		return false;

	cServerDC *serv = GetCurrentVerlihub();

	if (!serv) {
		cerr << "Server not found" << endl;
		return false;
	}

	cUser *opuser = GetUser(oper);

	if (!opuser)
		return false;

	cUser *user = GetUser(nick);

	if (!user || !user->mxConn)
		return false;

	serv->DCKickNick(NULL, opuser, nick, why, (eKI_CLOSE | eKI_WHY | eKI_PM | eKI_BAN), (note_op ? note_op : ""), (note_usr ? note_usr : ""));
	return true;
}

bool CloseConnection(const char *nick, long delay)
{
	if (!nick)
		return false;

	cUser *user = GetUser(nick);

	if (!user || !user->mxConn)
		return false;

	if (delay)
		user->mxConn->CloseNice(delay, eCR_KICKED);
	else
		user->mxConn->CloseNow();

	return true;
}

bool StopHub(int code, int delay)
{
	cServerDC *serv = GetCurrentVerlihub();

	if (!serv) {
		cerr << "Server not found" << endl;
		return false;
	}

	serv->cAsyncSocketServer::stop(code, delay);
	return true;
}

string GetIPCC(const char *ip)
{
	if (!ip)
		return "";

	cServerDC *serv = GetCurrentVerlihub();

	if (!serv) {
		cerr << "Server not found" << endl;
		return "";
	}

	const unsigned long ipnum = cBanList::Ip2Num(ip);
	cConnDC *conn = serv->GetConnByIP(ipnum); // look in connection list first, script ususally call this for connected but not yet logged in users
	string cc;

	if (conn && conn->ok) {
		cc = conn->GetGeoCC(); // this also sets location data on user, will be useful if he passes all checks and finally logs in
		return cc;
	}

	if (serv->mMaxMindDB->GetCC(ip, cc)) // else perform mmdb lookup
		return cc;

	return "";
}

string GetIPCN(const char *ip)
{
	if (!ip)
		return "";

	cServerDC *serv = GetCurrentVerlihub();

	if (!serv) {
		cerr << "Server not found" << endl;
		return "";
	}

	const unsigned long ipnum = cBanList::Ip2Num(ip);
	cConnDC *conn = serv->GetConnByIP(ipnum); // look in connection list first, script ususally call this for connected but not yet logged in users
	string cn;

	if (conn && conn->ok) {
		cn = conn->GetGeoCN(); // this also sets location data on user, will be useful if he passes all checks and finally logs in
		return cn;
	}

	if (serv->mMaxMindDB->GetCN(ip, cn)) // else perform mmdb lookup
		return cn;

	return "";
}

string GetIPCity(const char *ip, const char *db)
{
	if (!ip)
		return "";

	cServerDC *serv = GetCurrentVerlihub();

	if (!serv) {
		cerr << "Server not found" << endl;
		return "";
	}

	const unsigned long ipnum = cBanList::Ip2Num(ip);
	cConnDC *conn = serv->GetConnByIP(ipnum); // look in connection list first, script ususally call this for connected but not yet logged in users
	string ci;

	if (conn && conn->ok) {
		ci = conn->GetGeoCI(); // this also sets location data on user, will be useful if he passes all checks and finally logs in
		return ci;
	}

	if (serv->mMaxMindDB->GetCity(ci, ip, db)) // else perform mmdb lookup
		return ci;

	return "";
}

const char *GetMyINFO(const char *nick)
{
	cUser *usr = GetUser(nick);

	if (usr)
		return usr->mMyINFO.c_str();
	else
		return "";
}

int GetUserClass(const char *nick)
{
	cUser *usr = GetUser(nick);

	if (usr)
		return usr->mClass;
	else
		return -2;
}

const char *GetUserHost(const char *nick)
{
	cUser *usr = GetUser(nick);

	if (!usr || !usr->mxConn) {
		return "";
	} else {
		cServerDC *server = GetCurrentVerlihub();

		if (!server) {
			cerr << "Server verlihub is unfortunately not running or not found." << endl;
			return "";
		}

		if (!server->mUseDNS)
			usr->mxConn->DNSLookup();

		return usr->mxConn->AddrHost().c_str();
	}
}

const char *GetUserIP(const char *nick)
{
	cUser *usr = GetUser(nick);

	if (!usr || !usr->mxConn) {
		return "";
	} else {
		return usr->mxConn->AddrIP().c_str();
	}
}

bool Ban(const char *nick, const string &op, const string &reason, unsigned howlong, unsigned bantype)
{
	cServerDC *server = GetCurrentVerlihub();

	if (!server) {
		cerr << "Server verlihub is unfortunately not running or not found." << endl;
		return false;
	}

	cUser *usr = GetUser(nick);

	if (!usr || !usr->mxConn)
		return false;

	cBan ban(server);
	server->mBanList->NewBan(ban, usr->mxConn, op, reason, howlong, bantype);
	server->mBanList->AddBan(ban);
	usr->mxConn->CloseNice(1000, eCR_KICKED);
	return true;
}

bool DeleteNickTempBan(const char *nick)
{
	cServerDC *serv = GetCurrentVerlihub();

	if (!serv) {
		cerr << "Server not found" << endl;
		return false;
	}

	if (!serv->mBanList->IsNickTempBanned(nick))
		return false;

	serv->mBanList->DelNickTempBan(nick);
	return true;
}

bool DeleteIPTempBan(const char *ip)
{
	cServerDC *serv = GetCurrentVerlihub();

	if (!serv) {
		cerr << "Server not found" << endl;
		return false;
	}

	if (!serv->mBanList->IsIPTempBanned(ip))
		return false;

	serv->mBanList->DelIPTempBan(ip);
	return true;
}

bool ParseCommand(const char *nick, const char *cmd, int pm)
{
	cServerDC *serv = GetCurrentVerlihub();

	if (!serv) {
		cerr << "Server verlihub is unfortunately not running or not found." << endl;
		return false;
	}

	cUser *usr = GetUser(nick);

	if (!usr || !usr->mxConn)
		return false;

	string command(cmd);
	serv->mP.ParseForCommands(command, usr->mxConn, pm);
	return true;
}

bool SetConfig(const char *conf, const char *var, const char *val)
{
	if (!conf || !var)
		return false;

	cServerDC *serv = GetCurrentVerlihub();

	if (!serv) {
		cerr << "Server not found" << endl;
		return false;
	}

	string val_new, val_old;
	return (serv->SetConfig(conf, var, val, val_new, val_old) == 1);
}

char* GetConfig(const char *conf, const char *var, const char *def)
{
	if (!conf || !var)
		return (def ? strdup(def) : NULL);

	cServerDC *serv = GetCurrentVerlihub();

	if (!serv) {
		cerr << "Server not found" << endl;
		return (def ? strdup(def) : NULL);
	}

	return serv->GetConfig(conf, var, def);
}

int __GetUsersCount()
{
	cServerDC *server = GetCurrentVerlihub();

	if (!server) {
		cerr << "Server verlihub is unfortunately not running or not found." << endl;
		return 0;
	}

	return server->mUserCountTot;
}

unsigned __int64 GetTotalShareSize()
{
	cServerDC *server = GetCurrentVerlihub();

	if (!server) {
		cerr << "Server verlihub is unfortunately not running or not found." << endl;
		return 0;
	}

	return server->mTotalShare;
}

const char* __GetNickList()
{
	cServerDC *server = GetCurrentVerlihub();

	if (server) {
		string list;
		server->mUserList.GetNickList(list, false);
		return strdup(list.c_str());
	}

	return "";
}

const char * GetVHCfgDir()
{
	cServerDC *server = GetCurrentVerlihub();
	if (!server) {
		cerr << "Server verlihub is unfortunately not running or not found." << endl;
		return 0;
	}
	return server->mConfigBaseDir.c_str();
}

#define count_of(arg) (sizeof(arg) / sizeof(arg[0]))
bool GetTempRights(const char *nick,  map<string,int> &rights)
{
	cUser *user = GetUser(nick);
	if(user == NULL) return false;
	cTime time = cTime().Sec();

	static const int ids[] = { eUR_CHAT, eUR_PM, eUR_SEARCH, eUR_CTM, eUR_KICK, eUR_REG, eUR_OPCHAT, eUR_DROP, eUR_TBAN, eUR_PBAN, eUR_NOSHARE };
	for(unsigned int i = 0; i < count_of(ids); i++) {
		string key;
		switch(ids[i]) {
			case eUR_CHAT:
				key = "mainchat";
			break;
			case eUR_PM:
				key = "pm";
			break;
			case eUR_SEARCH:
				key = "search";
			break;
			case eUR_CTM:
				key = "ctm";
			break;
			case eUR_KICK:
				key = "kick";
			break;
			case eUR_REG:
				key = "reg";
			break;
			case eUR_OPCHAT:
				key = "opchat";
			break;
			case eUR_DROP:
				key = "drop";
			break;
			case eUR_TBAN:
				key = "tempban";
			break;
			case eUR_PBAN:
				key = "perban";
			break;
			case eUR_NOSHARE:
				key = "noshare";
			break;
		}
		if(!key.empty()) rights[key] = (user->Can(ids[i],time) ? 1 : 0);
	}
	return true;
}

bool AddRegUser(const char *nick, int clas, const char *pass, const char* op)
{
	if (!nick || (clas == eUC_MASTER))
		return false;

	cServerDC *serv = GetCurrentVerlihub();

	if (!serv) {
		cerr << "Server not found" << endl;
		return false;
	}

	cConnDC *conn = NULL;
	cUser *user = NULL;

	if (op[0] != '\0') {
		user = GetUser(op);

		if (user && user->mxConn)
			conn = user->mxConn;
	}

	bool res = serv->mR->AddRegUser(nick, conn, clas, (pass[0] != '\0') ? pass : NULL);
	user = GetUser(nick);

	if (res && user && user->mxConn) { // no need to reconnect for class to take effect
		ostringstream ostr;
		string data;
		ostr << autosprintf(_("You have been registered with class %d. Please set your password by using following command: +passwd <password>"), clas); // todo: use password request here aswell, send_pass_request
		serv->DCPrivateHS(ostr.str(), user->mxConn);
		serv->DCPublicHS(ostr.str(), user->mxConn);

		if ((clas >= serv->mC.opchat_class) && !serv->mOpchatList.ContainsHash(user->mNickHash)) // opchat list
			serv->mOpchatList.AddWithHash(user, user->mNickHash);

		if ((clas >= serv->mC.oplist_class) && !serv->mOpList.ContainsHash(user->mNickHash)) { // oplist
			serv->mOpList.AddWithHash(user, user->mNickHash);

			serv->mP.Create_OpList(data, user->mNick, true); // send short oplist, reserve for pipe
			serv->mUserList.SendToAll(data, serv->mC.delayed_myinfo, true);
		}

		user->mClass = tUserCl(clas);
		serv->SetUserRegInfo(user->mxConn, user->mNick); // update registration information in real time aswell
	}

	return res;
}

bool DelRegUser(const char *nick)
{
	if (!nick)
		return false;

	cServerDC *serv = GetCurrentVerlihub();

	if (!serv) {
		cerr << "Server not found" << endl;
		return false;
	}

	cRegUserInfo ui;

	if (!serv->mR->FindRegInfo(ui, nick))
		return false;

	if (ui.mClass == eUC_MASTER)
		return false;

	bool res = serv->mR->DelReg(nick);
	cUser *user = GetUser(nick);

	if (res && user && user->mxConn) { // no need to reconnect for class to take effect
		string data;
		serv->DCPrivateHS(_("You have been unregistered."), user->mxConn);
		serv->DCPublicHS(_("You have been unregistered."), user->mxConn);

		if (serv->mOpchatList.ContainsHash(user->mNickHash)) // opchat list
			serv->mOpchatList.RemoveByHash(user->mNickHash);

		if (serv->mOpList.ContainsHash(user->mNickHash)) { // oplist, only if user is there
			serv->mOpList.RemoveByHash(user->mNickHash);

			serv->mP.Create_Quit(data, user->mNick, true); // send quit to all, reserve for pipe
			serv->mUserList.SendToAll(data, serv->mC.delayed_myinfo, true);

			if (data.capacity() < (user->mMyINFO.size() + 1)) // send myinfo to all, reserve for pipe
				data.reserve(user->mMyINFO.size() + 1);

			data = user->mMyINFO;
			serv->mUserList.SendToAll(data, serv->mC.delayed_myinfo, true);

			if (serv->mC.send_user_ip) { // send userip to operators
				serv->mP.Create_UserIP(data, user->mNick, user->mxConn->AddrIP(), true); // reserve for pipe
				serv->mUserList.SendToAllWithClassFeature(data, serv->mC.user_ip_class, eUC_MASTER, eSF_USERIP2, serv->mC.delayed_myinfo, true); // must be delayed too
			}
		}

		user->mClass = eUC_NORMUSER;

		if (user->mHideShare) { // recalculate total share
			user->mHideShare = false;
			serv->mTotalShare += user->mShare;
		}

		serv->SetUserRegInfo(user->mxConn, user->mNick); // update registration information in real time aswell
	}

	return res;
}

bool SetRegClass(const char *nick, int clas)
{
	if (!nick || (clas < eUC_REGUSER) || ((clas > eUC_ADMIN) && (clas < eUC_MASTER)) || (clas > eUC_MASTER))
		return false;

	cServerDC *serv = GetCurrentVerlihub();

	if (!serv) {
		cerr << "Server not found" << endl;
		return false;
	}

	cRegUserInfo ui;

	if (!serv->mR->FindRegInfo(ui, nick))
		return false;

	if (ui.mClass == clas) // no need to set same class
		return false;

	cUser *user = GetUser(nick);

	if (user && user->mxConn) { // no need to reconnect for class to take effect
		ostringstream ostr;
		string data;
		ostr << autosprintf(_("Your class has been changed to %d."), clas);
		serv->DCPrivateHS(ostr.str(), user->mxConn);
		serv->DCPublicHS(ostr.str(), user->mxConn);

		if ((user->mClass < serv->mC.opchat_class) && (clas >= serv->mC.opchat_class)) { // opchat list
			if (!serv->mOpchatList.ContainsHash(user->mNickHash))
				serv->mOpchatList.AddWithHash(user, user->mNickHash);

		} else if ((user->mClass >= serv->mC.opchat_class) && (clas < serv->mC.opchat_class)) {
			if (serv->mOpchatList.ContainsHash(user->mNickHash))
				serv->mOpchatList.RemoveByHash(user->mNickHash);
		}

		if ((user->mClass < serv->mC.oplist_class) && (clas >= serv->mC.oplist_class)) { // oplist
			if (!ui.mHideKeys && !serv->mOpList.ContainsHash(user->mNickHash)) {
				serv->mOpList.AddWithHash(user, user->mNickHash);

				serv->mP.Create_OpList(data, user->mNick, true); // send short oplist, reserve for pipe
				serv->mUserList.SendToAll(data, serv->mC.delayed_myinfo, true);
			}

		} else if ((user->mClass >= serv->mC.oplist_class) && (clas < serv->mC.oplist_class)) {
			if (!ui.mHideKeys && serv->mOpList.ContainsHash(user->mNickHash)) {
				serv->mOpList.RemoveByHash(user->mNickHash);

				serv->mP.Create_Quit(data, user->mNick, true); // send quit to all, reserve for pipe
				serv->mUserList.SendToAll(data, serv->mC.delayed_myinfo, true);

				if (data.capacity() < (user->mMyINFO.size() + 1)) // send myinfo to all, reserve for pipe
					data.reserve(user->mMyINFO.size() + 1);

				data = user->mMyINFO;
				serv->mUserList.SendToAll(data, serv->mC.delayed_myinfo, true);

				if (serv->mC.send_user_ip) { // send userip to operators
					serv->mP.Create_UserIP(data, user->mNick, user->mxConn->AddrIP(), true); // reserve for pipe
					serv->mUserList.SendToAllWithClassFeature(data, serv->mC.user_ip_class, eUC_MASTER, eSF_USERIP2, serv->mC.delayed_myinfo, true); // must be delayed too
				}
			}
		}

		user->mClass = tUserCl(clas);
		serv->SetUserRegInfo(user->mxConn, user->mNick); // update registration information in real time aswell
	}

	string svar("class"), sval(StringFrom(clas));
	return serv->mR->SetVar(nick, svar, sval);
}

bool ScriptCommand(string *cmd, string *data, string *plug, string *script, bool inst)
{
	cServerDC *serv = GetCurrentVerlihub();

	if (!serv) {
		cerr << "Server not found" << endl;
		return false;
	}

	/*
		do the restriction stuff, for example check cmd
		plug = "python" for python, "lua" for lua, "perl" for perl
	*/

	return serv->AddScriptCommand(cmd, data, plug, script, inst);
}

bool ScriptQuery(string *cmd, string *data, string *recipient, string *sender, ScriptResponses *responses)
{
	if (!cmd || !data || !recipient || !sender || !responses)
		return false;

	cServerDC *serv = GetCurrentVerlihub();
	if (!serv) {
		cerr << "Server not found" << endl;
		return false;
	}

	serv->OnScriptQuery(cmd, data, recipient, sender, responses);
	return true;
}

int CheckBotNick(const string &nick)
{
	/*
		0 = error
		1 = ok
		2 = empty
		3 = bad character
		4 = reserved nick
	*/

	cServerDC *serv = GetCurrentVerlihub();

	if (!serv) {
		cerr << "CheckBotNick failed because server is not found" << endl;
		return 0;
	}

	if (nick.empty())
		return 2;

	string badchars(string(BAD_NICK_CHARS_NMDC) + string(BAD_NICK_CHARS_OWN));

	if (nick.npos != nick.find_first_of(badchars))
		return 3;

	if ((nick == serv->mC.hub_security) || (nick == serv->mC.opchat_name))
		return 4;

	return 1;
}

bool CheckDataPipe(string &data)
{
	if (data.size() && (data[data.size() - 1] == '|'))
		return false;

	if (data.capacity() < (data.size() + 1)) // reserve for pipe
		data.reserve(data.size() + 1);

	return true;
}

extern "C" {
	int GetUsersCount()
	{
		return __GetUsersCount();
	}

	const char* GetNickList()
	{
		return __GetNickList();
	}
}

}; // namespace nVerliHub
