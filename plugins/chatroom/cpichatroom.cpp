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

#include "cpichatroom.h"
#include "crooms.h"
#include "src/cserverdc.h"
#include "src/cdcproto.h"

namespace nVerliHub {
	using namespace nConfig;
	namespace nChatRoom {

cpiChatroom::cpiChatroom()
{
	mName = "Chatroom";
	mVersion = CHATROOM_VERSION;
	mCfg = NULL;
}

cpiChatroom::~cpiChatroom()
{
	if (mCfg != NULL) delete mCfg;
	mCfg = NULL;
}

void cpiChatroom::OnLoad(cServerDC *server)
{
	cUserCollection::iterator it;
	cUser *user;

	if (!mCfg) mCfg = new cRoomCfg(server);
	mCfg->Load();
	mCfg->Save();

	tpiChatroomBase::OnLoad(server);
	for (it = mServer->mUserList.begin(); it !=  mServer->mUserList.end() ; ++it) {
		user =(cUser*)*it;
		if(user && user->mxConn)
			mList->AutoJoin(user);
	}
}

bool cpiChatroom::OnUserLogin(cUser *user)
{
	mList->AutoJoin(user);
	return true;
}

bool cpiChatroom::OnUserLogout(cUser *user)
{
	cRooms::iterator it;
	for (it = mList->begin(); it != mList->end(); ++it) {
		if(*it) (*it)->DelUser(user);
	}
	return true;
}

bool cpiChatroom::RegisterAll()
{
	RegisterCallBack("VH_OnUserLogin");
	RegisterCallBack("VH_OnUserLogout");
	RegisterCallBack("VH_OnOperatorCommand");
	return true;
}

bool cpiChatroom::OnUserCommand(cConnDC *conn, string *str)
{
	if(mConsole.DoCommand(*str, conn))
		return false;
	return true;
}

bool cpiChatroom::OnOperatorCommand(cConnDC *conn, string *str)
{
	if(mConsole.DoCommand(*str, conn))
		return false;
	return true;
}

	}; // namespace nChatRoom
}; // namespace nVerliHub
REGISTER_PLUGIN(nVerliHub::nChatRoom::cpiChatroom);
