/*
	Copyright (C) 2003-2005 Daniel Muller, dan at verliba dot cz
	Copyright (C) 2006-2014 Verlihub Project, devs at verlihub-project dot org

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

#define DEFAULT_PERL_SCRIPT "/scripts/test.pl"

#include <verlihub/cvhplugin.h>
#include <verlihub/cconndc.h>
#include "cperlinterpreter.h"

using namespace nScripts;
using namespace nDirectConnect;
using namespace nDirectConnect::nPlugin;

/**
a Plugin that allows to load a Perl script

@author Daniel Muller
*/
class cpiPerl : public cVHPlugin
{
public:
	cpiPerl();
	virtual ~cpiPerl();
	virtual void OnLoad(cServerDC* server);
	virtual bool RegisterAll();
	virtual bool OnNewConn(cConnDC *);
	virtual bool OnCloseConn(cConnDC *);
	virtual bool OnParsedMsgChat(cConnDC *, cMessageDC *);
	virtual bool OnParsedMsgPM(cConnDC *, cMessageDC *);
	virtual bool OnParsedMsgSearch(cConnDC *, cMessageDC *);
	virtual bool OnParsedMsgSR(cConnDC *, cMessageDC *);
	virtual bool OnParsedMsgMyINFO(cConnDC *, cMessageDC *);
	virtual bool OnParsedMsgValidateNick(cConnDC *, cMessageDC *);
	virtual bool OnParsedMsgAny(cConnDC *, cMessageDC *);
	virtual bool OnParsedMsgSupport(cConnDC *, cMessageDC *);
	virtual bool OnParsedMsgMyPass(cConnDC *, cMessageDC *);
	//virtual bool OnUnknownMsg(cConnDC *, cMessageDC *);
	virtual bool OnOperatorCommand(cConnDC *, std::string *);
	virtual bool OnOperatorKicks(cUser *, cUser *, std::string *);
	virtual bool OnOperatorDrops(cUser *, cUser *);
	virtual bool OnValidateTag(cConnDC *, cDCTag *);
	virtual bool OnUserCommand(cConnDC *, std::string *);
	virtual bool OnUserLogin(cUser *);
	virtual bool OnUserLogout(cUser *);
	virtual bool OnTimer();
	virtual bool OnNewReg(cRegUserInfo *);
	virtual bool OnNewBan(cBan *);

private:
	cPerlInterpreter mPerl;
};

#endif
