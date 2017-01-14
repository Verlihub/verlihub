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

#include "crooms.h"
#include "cpichatroom.h"
#include "src/cdcproto.h"
#include "src/i18n.h"

namespace nVerliHub {
	using namespace nUtils;
	namespace nChatRoom {

cXChatRoom::cXChatRoom(const string &nick, cRoom *room) :
	cChatRoom(nick, room->mUsers, room->mServer),
	mRoom(room)
{
}

bool cXChatRoom::IsUserAllowed(cUser *user)
{
	return (user && user->mxConn && mRoom && (user->mClass >= mRoom->mMinClass));
}

cRoom::cRoom()
{
	mMinClass = 0;
	mAutoClassMin = 11;
	mAutoClassMax=4;
	mChatRoom = NULL;
	mServer = NULL;
	mPlugin = NULL;
	mUsers = NULL;
};

cRoom::~cRoom()
{
	if(mChatRoom) {
		mPlugin->DelRobot(mChatRoom);
		//delete mChatRoom;
		mChatRoom = NULL;
	}
	if(mUsers)
		delete mUsers;
	mUsers = NULL;
	mServer = NULL;
	mPlugin = NULL;
}

void cRoom::OnLoad()
{
	string omsg, start(mNick), desc("Chatroom: "), speed("\x1"), mail, share("0");

	if (!mUsers) {
		mUsers = new cUserCollection(true, false);
		mUsers->SetNickListSeparator("\r\n");
	}

	if (!mChatRoom) {
		mChatRoom = new cXChatRoom(mNick, this);
		mChatRoom->mClass = tUserCl(10);
		desc += mTopic;
		nProtocol::cDCProto::Create_MyINFO(mChatRoom->mMyINFO, mNick, desc, speed, mail, share);
		mChatRoom->mMyINFO_basic = mChatRoom->mMyINFO;
		mPlugin->AddRobot(mChatRoom);
		nProtocol::cDCProto::Create_Hello(omsg, mNick);
		mServer->mUserList.SendToAll(omsg, mServer->mC.delayed_myinfo, true);
	}
}

void cRoom::AddUser(cUser *user)
{
	if (user != NULL && user->mxConn) {
		mUsers->Add(user);
	}
}

void cRoom::DelUser(cUser *user)
{
	if (user != NULL && user->mxConn) {
		mUsers->Remove(user);
	}
}

bool cRoom::IsUserAutoJoin(cUser *user)
{
	if ((user->mClass >= mAutoClassMin) && (user->mClass <= mAutoClassMax))
		return true;
	if (mAutoCC.size() && user->mxConn && user->mxConn->mCC.size() && (mAutoCC.find(user->mxConn->mCC) !=mAutoCC.npos))
		return true;
	return false;
}

ostream& operator << (ostream &os, const cRoom &room)
{
	string roomTopic;
	os << "\t" << room.mNick;

	if (room.mTopic.empty())
		roomTopic = "--";
	else
		roomTopic = room.mTopic.substr(0, 18).c_str();

	os << "\t" << roomTopic;
	os << "\t" << room.mAutoClassMin << " - " << room.mAutoClassMax;
	os << "\t" << room.mAutoCC;
	os << "\t" << room.mMinClass;

	if (room.mUsers)
		os << room.mUsers->Size();

	return os;
}

//--------------------------

cRooms::cRooms(cVHPlugin *pi) : tRoomListBase(pi, "pi_chatroom")
{}

cRooms::~cRooms(){}

void cRooms::OnLoadData(cRoom &room)
{
	room.mServer = mOwner->mServer;
	room.mPlugin = mOwner;
	room.OnLoad();
}

void cRooms::AddFields()
{
	AddCol("nick","varchar(32)","",false, mModel.mNick);
	AddPrimaryKey("nick");
	AddCol("topic","text","",true, mModel.mTopic);
	AddCol("min_class","tinyint(4)","0",true, mModel.mMinClass);
	AddCol("auto_class_min","tinyint(4)","11",true, mModel.mAutoClassMin );
	AddCol("auto_class_max","tinyint(4)","4" ,true, mModel.mAutoClassMax );
	AddCol("auto_cc","varchar(32)","",true, mModel.mAutoCC );
	mMySQLTable.mExtra="PRIMARY KEY(nick)";

	SetBaseTo(&mModel);
}

void cRooms::AutoJoin(cUser *user)
{
	iterator it;
	for(it = begin(); it != end(); ++it) {
		cRoom *room =(*it);
		if(room->IsUserAutoJoin(user)) {
			room->AddUser(user);
		}
	}
}

bool cRooms::CompareDataKey(const cRoom &D1, const cRoom &D2)
{
	return D1.mNick == D2.mNick;
}

cRoomCfg::cRoomCfg(cServerDC *s) : mS(s)
{
	Add("min_class_add",min_class_add,5);
	Add("min_class_mod",min_class_mod,5);
	Add("min_class_del",min_class_del,5);
	Add("min_class_lst",min_class_lst,5);
}

int cRoomCfg::Load()
{
	mS->mSetupList.LoadFileTo(this,"pi_chatroom");
	return 0;
}

int cRoomCfg::Save()
{
	mS->mSetupList.SaveFileTo(this,"pi_chatroom");
	return 0;
}
	}; // namespace nChatRoom
}; // namespace nVerliHub
