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

#ifndef CROOMS_H
#define CROOMS_H

#include "src/tlistplugin.h"
#include "src/cuser.h"
#include "src/cusercollection.h"

namespace nVerliHub {
	//using namespace nConfig;
	namespace nSocket {
		class cServerDC;
	};
	class cUser;
	namespace nChatRoom {
		class cpiChatroom;
		class cRoom;

class cXChatRoom : public cChatRoom::cChatRoom
{
public:
	cXChatRoom(const string &nick, cRoom *room);
	virtual bool IsUserAllowed(cUser *);
	cRoom *mRoom;
};

class cpiChatroom;

class cRoom
{
public:
	// -- stored data
	string mNick;
	string mTopic;
	string mAutoCC;
	int mMinClass;
	int mAutoClassMin;
	int mAutoClassMax;

	// -- memory data
	cChatRoom  *mChatRoom;
	cUserCollection *mUsers;
	cServerDC *mServer;
	cpiChatroom *mPlugin;

	// -- required methods
	cRoom();
	virtual ~cRoom();
	virtual void OnLoad();
	virtual void AddUser(cUser *);
	virtual void DelUser(cUser *);
	virtual bool IsUserAutoJoin(cUser *);
	friend ostream& operator << (ostream &, const cRoom &room);
};

class cRoomCfg : public nConfig::cConfigBase
{
public:
	cRoomCfg(cServerDC *);
	int min_class_add;
	int min_class_mod;
	int min_class_del;
	int min_class_lst;

	cServerDC *mS;
	virtual int Load();
	virtual int Save();
};


typedef class nPlugin::tList4Plugin<cRoom, cpiChatroom> tRoomListBase;

/**
@author Rongic
@author Daniel Muller
*/
class cRooms : public tRoomListBase
{
public:
	// -- usable methods
	void AutoJoin(cUser *user);

	// -- required methods
	cRooms(cVHPlugin *pi);
	virtual ~cRooms();
	virtual void AddFields();
	virtual void OnLoadData(cRoom &);
	virtual bool CompareDataKey(const cRoom &D1, const cRoom &D2);
};
	}; // namepsace nChatRoom
}; // namespace nVerliHub

#endif
