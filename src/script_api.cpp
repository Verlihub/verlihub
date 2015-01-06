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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#if HAVE_LIBGEOIP
#include "cgeoip.h"
#endif
#include <iostream>
#include <cserverdc.h>
#include <cban.h>
#include <cbanlist.h>
#include <creglist.h>
#include "script_api.h"
#include "cconfigitembase.h"

using namespace std;
namespace nVerliHub {
	using namespace nSocket;
	using namespace nEnums;
	using namespace nMySQL;

cServerDC *GetCurrentVerlihub()
{
	return (cServerDC *)cServerDC::sCurrentServer;
}

cUser *GetUser(char *nick)
{
	cServerDC *serv = GetCurrentVerlihub();

	if (!serv) {
		cerr << "Server not found" << endl;
		return NULL;
	}

	cUser *usr = serv->mUserList.GetUserByNick(string(nick));
	return usr; // user without connection, a bot, must be accepted as well
}

bool SendDataToUser(char *data, char *nick)
{
	cUser *usr = GetUser(nick);

	if (!usr || !usr->mxConn)
		return false;

	string omsg(data);

	if ((omsg.find("$ConnectToMe") != string::npos) || (omsg.find("$RevConnectToMe") != string::npos))
		return false;

	usr->mxConn->Send(omsg, CheckDataPipe(omsg), true);
	return true;
}

bool KickUser(char *op, char *nick, char *reason)
{
	cServerDC *serv = GetCurrentVerlihub();

	if (!serv) {
		cerr << "Server not found" << endl;
		return false;
	}

	cUser *usr = GetUser(op);

	if (!usr || !usr->mxConn)
		return false;

	serv->DCKickNick(NULL, usr, nick, reason, eKCK_Drop | eKCK_Reason | eKCK_PM | eKCK_TBAN);
	return true;
}

bool SendToClass(char *data, int min_class, int max_class)
{
	cServerDC *serv = GetCurrentVerlihub();

	if (!serv) {
		cerr << "Server not found" << endl;
		return false;
	}

	if (min_class > max_class)
		return false;

	string omsg(data);

	if ((omsg.find("$ConnectToMe") != string::npos) || (omsg.find("$RevConnectToMe") != string::npos))
		return false;

	serv->mUserList.SendToAllWithClass(omsg, min_class, max_class, false, CheckDataPipe(omsg));
	return true;
}

bool SendToAll(char *data)
{
	cServerDC *serv = GetCurrentVerlihub();

	if (!serv) {
		cerr << "Server not found" << endl;
		return false;
	}

	string omsg(data);

	if ((omsg.find("$ConnectToMe") != string::npos) || (omsg.find("$RevConnectToMe") != string::npos))
		return false;

	serv->mUserList.SendToAll(omsg, false, CheckDataPipe(omsg));
	return true;
}

bool SendToActive(char *data)
{
	cServerDC *serv = GetCurrentVerlihub();

	if (!serv) {
		cerr << "Server not found" << endl;
		return false;
	}

	string omsg(data);

	if ((omsg.find("$ConnectToMe") != string::npos) || (omsg.find("$RevConnectToMe") != string::npos))
		return false;

	serv->mActiveUsers.SendToAll(omsg, false, CheckDataPipe(omsg));
	return true;
}

bool SendToActiveClass(char *data, int min_class, int max_class)
{
	cServerDC *serv = GetCurrentVerlihub();

	if (!serv) {
		cerr << "Server not found" << endl;
		return false;
	}

	if (min_class > max_class)
		return false;

	string omsg(data);

	if ((omsg.find("$ConnectToMe") != string::npos) || (omsg.find("$RevConnectToMe") != string::npos))
		return false;

	serv->mActiveUsers.SendToAllWithClass(omsg, min_class, max_class, false, CheckDataPipe(omsg));
	return true;
}

bool SendToPassive(char *data)
{
	cServerDC *serv = GetCurrentVerlihub();

	if (!serv) {
		cerr << "Server not found" << endl;
		return false;
	}

	string omsg(data);

	if ((omsg.find("$ConnectToMe") != string::npos) || (omsg.find("$RevConnectToMe") != string::npos))
		return false;

	serv->mPassiveUsers.SendToAll(omsg, false, CheckDataPipe(omsg));
	return true;
}

bool SendToPassiveClass(char *data, int min_class, int max_class)
{
	cServerDC *serv = GetCurrentVerlihub();

	if (!serv) {
		cerr << "Server not found" << endl;
		return false;
	}

	if (min_class > max_class)
		return false;

	string omsg(data);

	if ((omsg.find("$ConnectToMe") != string::npos) || (omsg.find("$RevConnectToMe") != string::npos))
		return false;

	serv->mPassiveUsers.SendToAllWithClass(omsg, min_class, max_class, false, CheckDataPipe(omsg));
	return true;
}

bool SendPMToAll(char *data, char *from, int min_class, int max_class)
{
	cServerDC *serv = GetCurrentVerlihub();

	if (!serv) {
		cerr << "Server not found" << endl;
		return false;
	}

	string start, end;
	serv->mP.Create_PMForBroadcast(start, end, from, from, data);
	serv->SendToAllWithNick(start, end, min_class, max_class);
	return true;
}

bool SendToOpChat(char *data)
{
	if (!data)
		return false;

	cServerDC *serv = GetCurrentVerlihub();

	if (!serv || !serv->mOpChat) {
		cerr << "Server not found" << endl;
		return false;
	}

	serv->mOpChat->SendPMToAll(data, NULL);
	return true;
}

bool CloseConnection(char *nick)
{
	cUser *usr = GetUser(nick);
	if((!usr) || (usr && !usr->mxConn))
		return false;
	else
	{
		usr->mxConn->CloseNow();
		return true;
	}
}

bool CloseConnectionNice(char *nick)
{
	cUser *usr = GetUser(nick);

	if ((!usr) || (usr && !usr->mxConn)) {
		return false;
	} else {
		usr->mxConn->CloseNice(1000, eCR_KICKED);
		return true;
	}
}

bool StopHub(int code)
{
	cServerDC *server = GetCurrentVerlihub();

	if (!server) {
		cerr << "Server verlihub is unfortunately not running or not found." << endl;
		return false;
	}

	server->cAsyncSocketServer::stop(code);
	return true;
}

char* GetUserCC(char *nick)
{
	cUser *user = GetUser(nick);

	if (user && user->mxConn)
		return (char*)user->mxConn->mCC.c_str();

	return NULL;
}

#if HAVE_LIBGEOIP

char* GetIPCC(char *ip)
{
	if (!ip)
		return NULL;

	cServerDC *serv = GetCurrentVerlihub();

	if (!serv) {
		cerr << "Server not found" << endl;
		return NULL;
	}

	string cc;

	if (serv->sGeoIP.GetCC(ip, cc))
		return (char*)cc.c_str();

	return NULL;
}

char* GetIPCN(char *ip)
{
	if (!ip)
		return NULL;

	cServerDC *serv = GetCurrentVerlihub();

	if (!serv) {
		cerr << "Server not found" << endl;
		return NULL;
	}

	string cn;

	if (serv->sGeoIP.GetCN(ip, cn))
		return (char*)cn.c_str();

	return NULL;
}

#endif

char *GetMyINFO(char *nick)
{
	cUser *usr = GetUser(nick);
	if(usr) return (char*) usr->mMyINFO.c_str();
	else return (char *)"";
}

int GetUserClass(char *nick)
{
	cUser *usr = GetUser(nick);

	if (usr)
		return usr->mClass;
	else
		return -2;
}

char *GetUserHost(char *nick)
{
	cUser *usr = GetUser(nick);
	if ((!usr) || (usr && !usr->mxConn))
		return (char *)"";
	else
	{
		cServerDC *server = GetCurrentVerlihub();
		if ( server == NULL) {
			cerr << "Server verlihub is unfortunately not running or not found." << endl;
			return (char *)"";
		}
		if(!server->mUseDNS)
			usr->mxConn->DNSLookup();
		return (char*)usr->mxConn->AddrHost().c_str();
	}
}

char *GetUserIP(char *nick)
{
	cUser *usr = GetUser(nick);
	if ((!usr) || (usr && !usr->mxConn))
		return (char *)"";
	else
	{
		return (char*)usr->mxConn->AddrIP().c_str();
	}
}

bool Ban(char *nick, const string op, const string reason, unsigned howlong, unsigned bantype)
{
	cServerDC *server = GetCurrentVerlihub();
	if(!server) {
		cerr << "Server verlihub is unfortunately not running or not found." << endl;
		return false;
	}
	cUser *usr = GetUser(nick);
	if ((!usr) || (usr && !usr->mxConn)) return false;

	cBan ban(server);
	server->mBanList->NewBan(ban, usr->mxConn, op, reason, howlong, bantype);
	server->mBanList->AddBan(ban);
	usr->mxConn->CloseNice(1000, eCR_KICKED);
	return true;
}

bool ParseCommand(char *nick, char *cmd, int pm)
{
	cServerDC *serv = GetCurrentVerlihub();

	if (!serv) {
		cerr << "Server verlihub is unfortunately not running or not found." << endl;
		return false;
	}

	cUser *usr = GetUser(nick);
	if (!usr || !usr->mxConn) return false;
	serv->mP.ParseForCommands(cmd, usr->mxConn, pm);
	return true;
}

bool SetConfig(char *config_name, char *var, char *val)
{
	cServerDC *server = GetCurrentVerlihub();
	if(!server)
	{
		cerr << "Server verlihub is unfortunately not running or not found." << endl;
		return false;
	}

	string file(server->mDBConf.config_name);

	cConfigItemBase *ci = NULL;
	if(file == server->mDBConf.config_name)
	{
		ci = server->mC[var];
		if( !ci )
		{
		cerr << "Undefined variable: " << var << endl;
		return false;
		}
	}
	if(ci)
	{
		ci->ConvertFrom(val);
		server->mSetupList.SaveItem(file.c_str(), ci);
	}
	return true;
}

int GetConfig(char *config_name, char *var, char *buf, int size)
{
	cServerDC *server = GetCurrentVerlihub();
	if(!server)
	{
		cerr << "Server verlihub is unfortunately not running or not found." << endl;
		return -1;
	}

	if (size < 1) return -1;
	buf[0] = 0;

	string val;
	string file(server->mDBConf.config_name);

	cConfigItemBase *ci = NULL;
	if(file == server->mDBConf.config_name)
	{
		ci = server->mC[var];
		if(!ci)
		{
			cerr << "Undefined variable: " << var << endl;
			return -1;
		}
	}

	if(ci)
	{
		ci->ConvertTo(val);
		if(!val.size()) return 0;
		if(int(val.size()) < size)
		{
			memcpy(buf, val.data(), val.size());
			buf[val.size()] = 0;
		}
		return val.size();
	}

	return -1;
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

__int64 GetTotalShareSize()
{
	cServerDC *server = GetCurrentVerlihub();

	if (!server) {
		cerr << "Server verlihub is unfortunately not running or not found." << endl;
		return 0;
	}

	return server->mTotalShare;
}

char *__GetNickList()
{
	cServerDC *server = GetCurrentVerlihub();
	if(server)
	{
		return (char*)server->mUserList.GetNickList().c_str();
	} else return (char *) "";
}

char * GetVHCfgDir()
{
	cServerDC *server = GetCurrentVerlihub();
	if (!server) {
		cerr << "Server verlihub is unfortunately not running or not found." << endl;
		return 0;
	}
	return (char *) server->mConfigBaseDir.c_str();
}

#define count_of(arg) (sizeof(arg) / sizeof(arg[0]))
bool GetTempRights(char *nick,  map<string,int> &rights)
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

bool AddRegUser(char *nick, int uclass, char *pass, char* op)
{
	if (!nick || (uclass == eUC_MASTER))
		return false;

	cServerDC *serv = GetCurrentVerlihub();

	if (!serv) {
		cerr << "Server not found" << endl;
		return false;
	}

	cConnDC *conn = NULL;

	if (strlen(op)) {
		cUser *user = GetUser(op);

		if (user && user->mxConn)
			conn = user->mxConn;
	}

	return serv->mR->AddRegUser(nick, conn, uclass, (strlen(pass) ? pass : NULL));
}

bool DelRegUser(char *nick)
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

	return serv->mR->DelReg(nick);
}

bool ScriptCommand(string *cmd, string *data, string *plug, string *script)
{
	cServerDC *serv = GetCurrentVerlihub();

	if (!serv) {
		cerr << "Server not found" << endl;
		return false;
	}

	/*
		do the restriction stuff, for example check cmd
		plug = "py" for python, "lua" for lua
	*/

	serv->OnScriptCommand(cmd, data, plug, script);
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

	string badchars("\0$|<> ");

	if (nick.npos != nick.find_first_of(badchars))
		return 3;

	if ((nick == serv->mC.hub_security) || (nick == serv->mC.opchat_name))
		return 4;

	return 1;
}

bool CheckDataPipe(const string &data)
{
	if (data.size() && (data[data.size() - 1] == '|'))
		return false;
	else
		return true;
}

extern "C" {
	int GetUsersCount()
	{
		return __GetUsersCount();
	}
	char *GetNickList()
	{
		return __GetNickList();
	}
}

}; // namespace nVerliHub
