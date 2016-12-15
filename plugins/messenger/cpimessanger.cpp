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

#include "cpimessanger.h"
#include "cmsglist.h"
#include "src/cserverdc.h"
#include "src/cdcproto.h"

namespace nVerliHub {
	using namespace nSocket;
	namespace nMessangerPlugin {
cpiMessanger::cpiMessanger() : mConsole(this), mMsgs(NULL), mReloadTimer(30., 0., cTime())
{
	mName = "Messenger";
	mVersion = MESSENGER_VERSION;
}

bool cpiMessanger::OnTimer(__int64 msec)
{
	if (mReloadTimer.Check(mServer->mTime, 1)==0) {
		mMsgs->UpdateCache();
	}
	return true;
}


bool cpiMessanger::OnUserLogin(cUser *user)
{
	int iNewMsgs = mMsgs->CountMessages(user->mNick, false);
	if (iNewMsgs) {
		mMsgs->DeliverMessagesForUser(user);
	}
	return true;
}

void cpiMessanger::OnLoad(cServerDC *server)
{
	cVHPlugin::OnLoad(server);
	mMsgs = new cMsgList(server);
	mMsgs->CreateTable();
	mMsgs->CleanUp();
}

bool cpiMessanger::RegisterAll()
{
	//When user logins checkout his mailbox and eventualy tell him about messages
	//RegisterCallBack("VH_OnNewUser");
	//treat messages that use to post offline msgs, remove them, read etc...
	RegisterCallBack("VH_OnUserCommand");
	RegisterCallBack("VH_OnUserLogin");
	RegisterCallBack("VH_OnTimer");
	return true;
}

cpiMessanger::~cpiMessanger()
{
	if (mMsgs) delete mMsgs;
}

bool cpiMessanger::OnUserCommand(cConnDC *conn, string *str)
{
	if( mConsole.DoCommand(*str, conn) ) return false;
	return true;
}
	}; // namespace nMessangerPlugin
}; // namespace nVerliHub

REGISTER_PLUGIN(nVerliHub::nMessangerPlugin::cpiMessanger);
