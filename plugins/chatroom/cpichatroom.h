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

#ifndef CPICHATROOM_H
#define CPICHATROOM_H

#include "src/tlistplugin.h"
#include "src/cvhplugin.h"
#include "crooms.h"
#include "cconsole.h"

namespace nVerliHub {
	namespace nChatRoom {
		typedef nPlugin::tpiListPlugin<cRooms,cRoomConsole> tpiChatroomBase;
/**
\brief a messanger plugin for verlihub

users may leave offline messages for registered users or reg users may leave offline messages for anyone

@author Daniel Muller
*/
class cpiChatroom : public tpiChatroomBase
{
public:
	cpiChatroom();
	virtual ~cpiChatroom();
	virtual void OnLoad(cServerDC *);


	virtual bool RegisterAll();
	virtual bool OnUserCommand(cConnDC *, string *);
	virtual bool OnUserLogin(cUser *);
	virtual bool OnUserLogout(cUser *);
	virtual bool OnOperatorCommand(cConnDC *, string *);
	cRoomCfg *mCfg;
};
	}; // namespace nChatRoom
}; // namespace nVerliHub

#endif
