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

#ifndef CPIIPLOG_H
#define CPIIPLOG_H

#include "src/cvhplugin.h"
#include "cconsole.h"
#include "ciplog.h"

namespace nVerliHub {
	namespace nIPLogPlugin {
/**
\brief a statistics plugin for verlihub

users may leave offline messages for registered users or reg users may leave offline messages for anyone

@author Daniel Muller
*/
class cpiIPLog : public nPlugin::cVHPlugin
{
public:
	cpiIPLog();
	virtual ~cpiIPLog();
	virtual bool RegisterAll();
	virtual bool OnOperatorCommand(nSocket::cConnDC *, string *);
	virtual bool OnNewConn(nSocket::cConnDC * conn);
	virtual bool OnCloseConn(nSocket::cConnDC *conn);
	virtual void OnLoad(nSocket::cServerDC *);
	virtual bool OnUserLogin(cUser *);
	virtual bool OnUserLogout(cUser *);
	cConsole mConsole;
	cIPLog * mIPLog;
	int mLogFlags;
};
	}; // namespace using namespace ::nPlugin;
}; // namespace nVerliHub

#endif
