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

#include "cpifloodprot.h"

namespace nVerliHub {
	using namespace nSocket;
	using namespace nEnums;
	namespace nFloodProtectPlugin {
cpiFloodprot::cpiFloodprot() : mFloodprotect(0)
{
	mName = "Floodprotect";
	mVersion = FLOODPROT_VERSION;
}

cpiFloodprot::~cpiFloodprot()
{
	if(mFloodprotect)
	{
		delete mFloodprotect;
		mFloodprotect = 0;
	}
}

void cpiFloodprot::OnLoad(cServerDC *server)
{
	cVHPlugin::OnLoad(server);
	mFloodprotect = new cFloodprotect(server);
}

bool cpiFloodprot::RegisterAll()
{
	RegisterCallBack("VH_OnTimer");
	RegisterCallBack("VH_OnNewConn");
	RegisterCallBack("VH_OnUserLogin");
	RegisterCallBack("VH_OnUserLogout");
	RegisterCallBack("VH_OnCloseConn");
	RegisterCallBack("VH_OnParsedMsgChat");
	RegisterCallBack("VH_OnParsedMsgPM");
	RegisterCallBack("VH_OnParsedMsgSearch");
	RegisterCallBack("VH_OnParsedMsgMyINFO");
	return true;
}

bool cpiFloodprot::OnNewConn(cConnDC *conn)
{
	if(!mFloodprotect->AddConn(conn, 1))
	{
		string omsg("Sorry, the limit of connections with your ip has been exceeded.");
		conn->Send(omsg,true);
		conn->CloseNice(500); // not sure if this is needed
		return false;
	}
	return true;
}

bool cpiFloodprot::OnUserLogin(cUser *user)
{
	if(!mFloodprotect->AddConn(user->mxConn, 1))
	{
		string omsg("Sorry, the limit of unregistered connections with your ip has been exceeded.");
		user->mxConn->Send(omsg,true);
		user->mxConn->CloseNice(500); // not sure if this is needed
		return false;
	}
	return true;
}

bool cpiFloodprot::OnUserLogout(cUser *user)
{
	mFloodprotect->AddConn(user->mxConn, -1);
	return true;
}

bool cpiFloodprot::OnCloseConn(cConnDC *conn)
{
	mFloodprotect->AddConn(conn, -1);
	return true;
}


bool cpiFloodprot::OnTimer(__int64 msec)
{
	if(!mFloodprotect->CleanUp(30))
	{
		return false;
	}

	return true;
}

bool cpiFloodprot::OnParsedMsgChat(cConnDC *conn, cMessageDC *msg)
{
	if(!mFloodprotect->CheckFlood(conn, eFT_CHAT))
		return false;

	return true;
}

bool cpiFloodprot::OnParsedMsgPM(cConnDC *conn, cMessageDC *msg)
{
	if(!mFloodprotect->CheckFlood(conn, eFT_PRIVATE))
		return false;

	return true;
}

bool cpiFloodprot::OnParsedMsgSearch(cConnDC *conn, cMessageDC *msg)
{
	if(!mFloodprotect->CheckFlood(conn, eFT_SEARCH))
		return false;

	return true;
}

bool cpiFloodprot::OnParsedMsgMyINFO(cConnDC *conn, cMessageDC *msg)
{
	if(!mFloodprotect->CheckFlood(conn, eFT_MYINFO))
		return false;

	return true;
}
	}; // namespace nFloodProtectPlugin
}; // namespace nVerliHub
REGISTER_PLUGIN(nVerliHub::nFloodProtectPlugin::cpiFloodprot);
