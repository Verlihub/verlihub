/*
	Copyright (C) 2003-2005 Daniel Muller, dan at verliba dot cz
	Copyright (C) 2006-2024 Verlihub Team, info at verlihub dot net

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

cChatConsole::cChatConsole(cServerDC *server, cChatRoom *ChatRoom):
	cDCConsoleBase(server),
	mCmdr(this),
	mChatRoom(ChatRoom)
{
}

cChatConsole::~cChatConsole()
{
}

cUserCollection* cChatConsole::GetTheList()
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

int cChatConsole::DoCommand(const string &str, cConnDC *conn)
{
	if (!conn || !conn->mpUser)
		return 0;

	ostringstream os;

	if (mCmdr.ParseAll(str, os, conn) >= 0) {
		mChatRoom->SendPMTo(conn, os.str());
		return 1;
	}

	return 0;
}

const char* cChatConsole::CmdId(int cmd)
{
	static string id;
	id = CmdPrefix();

	switch (cmd) {
		case eCHAT_INVITE:
			id += "invite";
			break;
		case eCHAT_LEAVE:
			id += "leave";
			break;
		case eCHAT_OUT:
			id += "out";
			break;
		case eCHAT_MEMBERS:
			id += "members";
			break;
		default:
			id += "???";
			break;
	}

	id += CmdSuffix();

	switch (cmd) {
		case eCHAT_LEAVE:
		case eCHAT_MEMBERS:
			break;
		default:
			id += ' ';
			break;
	}

	return id.c_str();
}

const char* cChatConsole::GetParamsRegex(int cmd)
{
	switch (cmd) { // invite|out <nick> [reason]
		case eCHAT_INVITE:
			return "^(\\S+)( (.*))?$";
		case eCHAT_OUT:
			return "^(\\S+)( (.*))?$";
		default:
			return "";
	}
}

cUserCollection* cChatConsole::cfBase::GetTheList()
{
	if (mCommand && mCommand->mCmdr && mCommand->mCmdr->mOwner)
		return ((cChatConsole*)mCommand->mCmdr->mOwner)->GetTheList();

	return NULL;
}

bool cChatConsole::cfOut::operator()()
{
	if (!mConn || !mConn->mpUser)
		return false;

	string nick;
	cUser *user;
	GetParOnlineUser(1, user, nick);

	if (!user || !user->mxConn || !user->mInList) {
		(*mOS) << autosprintf(_("User not online: %s"), nick.c_str());
		return false;
	}

	if (!GetTheList()->ContainsNick(nick)) {
		(*mOS) << autosprintf(_("User not in chatroom: %s"), nick.c_str());
		return false;
	}

	if (user->mClass > mConn->mpUser->mClass) {
		(*mOS) << autosprintf(_("You have no rights to remove user from chatroom: %s"), nick.c_str());
		return false;
	}

	/*
	string msg;
	GetParStr(3, msg);

	if (msg.size())
		mChatRoom->SendPMTo(user->mxConn, msg); // todo
	*/

	GetTheList()->Remove(user);
	(*mOS) << autosprintf(_("User removed from chatroom: %s"), nick.c_str());
	return true;
}

bool cChatConsole::cfLeave::operator()()
{
	if (!mConn || !mConn->mpUser)
		return false;

	GetTheList()->Remove(mConn->mpUser);
	(*mOS) << _("You are removed from chatroom.");
	return true;
}

bool cChatConsole::cfMembers::operator()()
{
	if (!mConn || !mConn->mpUser)
		return false;

	string NickList;
	GetTheList()->GetNickList(NickList);
	(*mOS) << _("Chatroom members") << ":\r\n\r\n" << NickList;
	return true;
}

bool cChatConsole::cfInvite::operator()()
{
	if (!mConn || !mConn->mpUser)
		return false;

	string nick;
	cUser *user;
	GetParOnlineUser(1, user, nick);

	if (!user || !user->mxConn || !user->mInList) {
		(*mOS) << autosprintf(_("User not online: %s"), nick.c_str());
		return false;
	}

	if (GetTheList()->ContainsNick(nick)) {
		(*mOS) << autosprintf(_("User is already in chatroom: %s"), nick.c_str());
		return false;
	}

	/*
	string msg;
	GetParStr(3, msg);

	if (msg.size())
		mChatRoom->SendPMTo(user->mxConn, msg); // todo
	*/

	GetTheList()->Add(user);
	(*mOS) << autosprintf(_("User added to chatroom: %s"), nick.c_str());
	return true;
}

}; // namespace nVerliHub
