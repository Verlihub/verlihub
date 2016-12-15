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

#include "cpiiplog.h"
#include "src/cserverdc.h"

namespace nVerliHub {
	using namespace nSocket;
	using namespace nEnums;
	namespace nIPLogPlugin {

cpiIPLog::cpiIPLog() :  mConsole(this),	mIPLog(NULL)
{
	mName = "IPLog";
	mVersion = IPLOG_VERSION;
	mLogFlags = eLT_ALL;
}

void cpiIPLog::OnLoad(cServerDC *server)
{
	cVHPlugin::OnLoad(server);
	mServer = server;
	mIPLog = new cIPLog(server);
	mIPLog->CreateTable();
}

bool cpiIPLog::RegisterAll()
{
	RegisterCallBack("VH_OnOperatorCommand");
        RegisterCallBack("VH_OnNewConn");
	RegisterCallBack("VH_OnCloseConn");
	RegisterCallBack("VH_OnUserLogin");
	RegisterCallBack("VH_OnUserLogout");
	return true;
}

bool cpiIPLog::OnCloseConn(cConnDC *conn)
{
	if ((1 << eLT_DISCONNECT) & mLogFlags)
		mIPLog->Log(conn, eLT_DISCONNECT, conn->mCloseReason);
	return true;
}

bool cpiIPLog::OnNewConn(cConnDC * conn)
{
	if ((1 << eLT_CONNECT) & mLogFlags)
		mIPLog->Log(conn, eLT_CONNECT, 0);
	return true;
}

bool cpiIPLog::OnUserLogout(cUser *user)
{
	if (user->mxConn && ((1 << eLT_LOGOUT) & mLogFlags))
		mIPLog->Log(user->mxConn, eLT_LOGOUT, user->mxConn->mCloseReason);
	return true;
}

bool cpiIPLog::OnUserLogin(cUser *user)
{
	if (user->mxConn && ((1 << eLT_LOGIN) & mLogFlags))
		mIPLog->Log(user->mxConn, eLT_LOGIN, 0);
	return true;
}

bool cpiIPLog::OnOperatorCommand(cConnDC *conn, string *str)
{
        if( mConsole.DoCommand(*str, conn) ) return false;
        return true;
}

cpiIPLog::~cpiIPLog()
{
	if (mIPLog)
		delete mIPLog;
	mIPLog = NULL;
}

	}; // namespace using namespace nIPLogPlugin;
}; // namespace nVerliHub
REGISTER_PLUGIN(nVerliHub::nIPLogPlugin::cpiIPLog);
