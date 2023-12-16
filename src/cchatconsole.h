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

#ifndef NDIRECTCONNECTCCHATCONSOLE_H
#define NDIRECTCONNECTCCHATCONSOLE_H
#include "cdccommand.h"
#include "ccommandcollection.h"

namespace nVerliHub {
	using namespace nCmdr;

	namespace nSocket {
		//class cConnDC;
		class cServerDC;
	};

	class cChatRoom;
	class cUserCollection;

	namespace nEnums {
		enum { // chat console command id
			eCHAT_INVITE,
			eCHAT_LEAVE,
			eCHAT_OUT,
			eCHAT_MEMBERS
		};
	};

/**
contains the commands that are accepted thourh the pm of any chatroom, like for example the OpChat
@author Daniel Muller
*/

class cChatConsole: public cDCConsoleBase
{
public:
	cChatConsole(nSocket::cServerDC *, cChatRoom *ChatRoom = NULL);
	virtual ~cChatConsole();

	virtual void AddCommands();
	virtual int DoCommand(const string &str, nSocket::cConnDC *conn);
	virtual int OpCommand(const string &str, nSocket::cConnDC *conn) { return this->DoCommand(str, conn); }
	virtual int UsrCommand(const string &str, nSocket::cConnDC *conn) { return this->DoCommand(str, conn); }

	virtual cUserCollection* GetTheList();
	virtual const char* GetParamsRegex(int);
	virtual const char* CmdSuffix() { return ""; }
	virtual const char* CmdPrefix() { return "\\+"; }
	virtual const char* CmdId(int cmd);

protected:
	class cfBase: public cDCCommand::sDCCmdFunc
	{
		public:
			virtual cUserCollection* GetTheList();
	};

	class cfInvite: public cfBase
	{
		virtual bool operator()();
	} mcfInvite;

	class cfLeave: public cfBase
	{
		virtual bool operator()();
	} mcfLeave;

	class cfOut: public cfBase
	{
		virtual bool operator()();
	} mcfOut;

	class cfMembers: public cfBase
	{
		virtual bool operator()();
	} mcfMembers;

	cDCCommand mCmdInvite;
	cDCCommand mCmdLeave;
	cDCCommand mCmdOut;
	cDCCommand mCmdMembers;
	cCommandCollection mCmdr;
	cChatRoom *mChatRoom;
};

}; // namespace nVerliHub

#endif
