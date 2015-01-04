/**************************************************************************
*   Copyright (C) 2003 by Dan Muller                                      *
*   dan at verliba.cz                                                     *
*                                                                         *
*   Copyright (C) 2011-2013 by Shurik                                     *
*   shurik at sbin.ru                                                     *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
*   This program is distributed in the hope that it will be useful,       *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
*   GNU General Public License for more details.                          *
*                                                                         *
*   You should have received a copy of the GNU General Public License     *
*   along with this program; if not, write to the                         *
*   Free Software Foundation, Inc.,                                       *
*   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
***************************************************************************/
#ifndef CPIPERL_H
#define CPIPERL_H

#include "src/cvhplugin.h"
#include "src/cconndc.h"
#include "cconsole.h"
#include "cperlmulti.h"

#define PERLSCRIPT_PI_IDENTIFIER "PerlScript"

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
	virtual bool OnParsedMsgSupport(cConnDC *, cMessageDC *);
	virtual bool OnParsedMsgBotINFO(cConnDC *, cMessageDC *);
	virtual bool OnParsedMsgVersion(cConnDC *, cMessageDC *);
	virtual bool OnParsedMsgMyPass(cConnDC *, cMessageDC *);
	virtual bool OnOperatorCommand(cConnDC *, std::string *);
	virtual bool OnOperatorKicks(cUser *, cUser *, std::string *);
	virtual bool OnOperatorDrops(cUser *, cUser *);
	virtual bool OnValidateTag(cConnDC *, cDCTag *);
	virtual bool OnUserCommand(cConnDC *, std::string *);
	virtual bool OnHubCommand(cConnDC *, std::string *, int, int);
	virtual bool OnUserLogin(cUser *);
	virtual bool OnUserLogout(cUser *);
	virtual bool OnTimer(long);
	virtual bool OnNewReg(cRegUserInfo *);
	virtual bool OnNewBan(cBan *);
	virtual bool OnUnBan(std::string, std::string, std::string);
	virtual bool OnParsedMsgConnectToMe(cConnDC *, cMessageDC *);
	virtual bool OnParsedMsgRevConnectToMe(cConnDC *, cMessageDC *);
	virtual bool OnDelReg(std::string, int);
	virtual bool OnUpdateClass(std::string, int, int);
	virtual bool OnHubName(std::string, std::string);
	virtual bool OnScriptCommand(std::string, std::string, std::string, std::string);

	int Size() { return mPerl.Size(); }

	nMySQL::cQuery *mQuery;

private:
	cConsole mConsole;
	virtual bool AutoLoad();
	string mScriptDir;
};

	}; // namespace nPerlPlugin
}; // namespace nVerliHub

#endif
