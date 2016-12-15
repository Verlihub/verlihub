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

#ifndef CPIPERL_H
#define CPIPERL_H

#include "src/cvhplugin.h"
#include "src/cconndc.h"
#include "cconsole.h"
#include "cperlmulti.h"

#ifndef _WIN32
#define __int64 long long
#endif

#define PERLSCRIPT_PI_IDENTIFIER "Perl"

namespace nVerliHub {
	namespace nPerlPlugin {

class cpiPerl : public nPlugin::cVHPlugin
{
public:
	cPerlMulti mPerl;

	cpiPerl();
	virtual ~cpiPerl();
	virtual void OnLoad(cServerDC* server);
	virtual bool RegisterAll();
	virtual bool OnNewConn(cConnDC *);
	virtual bool OnCloseConn(cConnDC *);
	virtual bool OnCloseConnEx(cConnDC *);
	virtual bool OnParsedMsgChat(cConnDC *, cMessageDC *);
	virtual bool OnParsedMsgPM(cConnDC *, cMessageDC *);
	virtual bool OnParsedMsgMCTo(cConnDC *, cMessageDC *);
	virtual bool OnParsedMsgSearch(cConnDC *, cMessageDC *);
	virtual bool OnParsedMsgSR(cConnDC *, cMessageDC *);
	virtual bool OnParsedMsgMyINFO(cConnDC *, cMessageDC *);
	virtual bool OnFirstMyINFO(cConnDC *, cMessageDC *);
	virtual bool OnParsedMsgValidateNick(cConnDC *, cMessageDC *);
	virtual bool OnParsedMsgAny(cConnDC *, cMessageDC *);
	virtual bool OnParsedMsgAnyEx(cConnDC *, cMessageDC *);
	virtual bool OnUnknownMsg(cConnDC *, cMessageDC *);
	virtual bool OnParsedMsgSupports(cConnDC *, cMessageDC *, string *);
	virtual bool OnParsedMsgBotINFO(cConnDC *, cMessageDC *);
	virtual bool OnParsedMsgVersion(cConnDC *, cMessageDC *);
	virtual bool OnParsedMsgMyPass(cConnDC *, cMessageDC *);
	virtual bool OnOperatorCommand(cConnDC *, std::string *);
	virtual bool OnOperatorKicks(cUser *, cUser *, std::string *);
	virtual bool OnOperatorDrops(cUser *, cUser *, std::string *);
	virtual bool OnValidateTag(cConnDC *, cDCTag *);
	virtual bool OnUserCommand(cConnDC *, std::string *);
	virtual bool OnHubCommand(cConnDC *, std::string *, int, int);
	virtual bool OnUserLogin(cUser *);
	virtual bool OnUserLogout(cUser *);
	virtual bool OnTimer(__int64);
	virtual bool OnNewReg(cUser *, string, int);
	virtual bool OnNewBan(cUser *, nTables::cBan *);
	virtual bool OnUnBan(cUser *, std::string, std::string, std::string);
	virtual bool OnParsedMsgConnectToMe(cConnDC *, cMessageDC *);
	virtual bool OnParsedMsgRevConnectToMe(cConnDC *, cMessageDC *);
	virtual bool OnDelReg(cUser *, std::string, int);
	virtual bool OnUpdateClass(cUser *, std::string, int, int);
	virtual bool OnHubName(std::string, std::string);
	virtual bool OnScriptCommand(std::string *, std::string *, std::string *, std::string *);

	unsigned int Size() { return mPerl.Size(); }

	nMySQL::cQuery *mQuery;

private:
	cConsole mConsole;
	virtual bool AutoLoad();
	string mScriptDir;
};

	}; // namespace nPerlPlugin
}; // namespace nVerliHub

#endif
