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

#include "cchatconsole.h"
#include "cserverdc.h"
#include "cuser.h"
#include "i18n.h"

namespace nVerliHub {
	using namespace nEnums;
cChatConsole::cChatConsole(cServerDC *server, cChatRoom *ChatRoom) :
	cDCConsoleBase(server),
	mCmdr(this),
	mChatRoom(ChatRoom)
{}


cChatConsole::~cChatConsole()
{}

cUserCollection *cChatConsole::GetTheList()
{
	return mChatRoom->mCol;
}

void cChatConsole::AddCommands()
{
	mCmdInvite.Init(eCHAT_INVITE, CmdId(eCHAT_INVITE), GetParamsRegex(eCHAT_INVITE), &mcfInvite);
	mCmdLeave.Init(eCHAT_LEAVE, CmdId(eCHAT_LEAVE), GetParamsRegex(eCHAT_LEAVE), &mcfLeave);
	mCmdOut.Init(eCHAT_OUT, CmdId(eCHAT_OUT), GetParamsRegex(eCHAT_OUT), &mcfOut);
	mCmdMembers.Init(eCHAT_MEMBERS, CmdId(eCHAT_MEMBERS), GetParamsRegex(eCHAT_MEMBERS), &mcfMembers);
	mCmdr.Add(&mCmdInvite);
	mCmdr.Add(&mCmdLeave);
	mCmdr.Add(&mCmdOut);
	mCmdr.Add(&mCmdMembers);
	mCmdr.InitAll(this);
}

int cChatConsole::DoCommand(const string &str, cConnDC * conn)
{
	ostringstream os;
	if (!conn || !conn->mpUser) return 0;
	if(mCmdr.ParseAll(str, os, conn) >= 0)
	{
		mChatRoom->SendPMTo(conn, os.str());
		return 1;
	}
	return 0;
}

const char *cChatConsole::CmdId(int cmd)
{
	static string id;
	id = CmdPrefix();
	switch(cmd)
	{
		case eCHAT_INVITE: id += "invite"; break;
		case eCHAT_LEAVE: id += "leave"; break;
		case eCHAT_OUT: id += "out"; break;
		case eCHAT_MEMBERS: id += "members"; break;
		default: id += "???";
	}
	id += CmdSuffix();

	switch(cmd)
	{
		case eCHAT_LEAVE: break;
		case eCHAT_MEMBERS: break;
		default :id += " ";
	}

	return id.c_str();
}

const char * cChatConsole::GetParamsRegex(int cmd)
{
	switch(cmd)
	{
		// +invite <nick>[ <by these words>]
		case eCHAT_INVITE: return "^(\\S+)( (.*))?$"; break;
		case eCHAT_OUT: return "^(\\S+)( (.*))?$"; break;
		default: return "";
	}
}

cUserCollection *cChatConsole::cfBase::GetTheList()
{
	if (mCommand && mCommand->mCmdr && mCommand->mCmdr->mOwner)
		return ((cChatConsole*) mCommand->mCmdr->mOwner)->GetTheList();
	return NULL;
}

bool cChatConsole::cfOut::operator()()
{
	string nick, msg;
	cUser *user;

	GetParOnlineUser(1, user,nick);
	if(!user ||!user->mxConn || !GetTheList()->ContainsNick(nick)) {
		*mOS << autosprintf(_("User '%s' is not in this room."), nick.c_str());
		return false;
	}
	if (user->mClass > mConn->mpUser->mClass) {
		*mOS << autosprintf(_("You cannot send %s out of the room because he has higher privilegies."), nick.c_str());
		return false;
	}

	GetParStr(3, msg);

	GetTheList()->Remove(user);
	return true;
}

bool cChatConsole::cfLeave::operator()()
{
	if (mConn && mConn->mpUser) {
		GetTheList()->Remove(mConn->mpUser);
		return true;
	} else return false;
}

bool cChatConsole::cfMembers::operator()()
{
	string NickList;
	if (mConn && mConn->mpUser) {
		NickList = GetTheList()->GetNickList();
		*mOS << autosprintf(_("Members: \n%s"), NickList.c_str());;
		return true;
	} else return false;
}

bool cChatConsole::cfInvite::operator()()
{
	string nick, msg;
	cUser *user;

	GetParOnlineUser(1, user,nick);
	if(!user ||!user->mxConn)
	{
		*mOS << autosprintf(_("User '%s' is not online, so you cannot invite him."), nick.c_str());
		return false;
	}
	GetParStr(3, msg);

	GetTheList()->Add(user);
	return true;
}

}; // namespace nVerliHub
