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

#ifndef NCHATROOMCCONSOLE_H
#define NCHATROOMCCONSOLE_H

#include "src/tlistconsole.h"
#include "crooms.h"

namespace nVerliHub {
	using namespace nCmdr;
	namespace nSocket {
		class cConnDC;
	};
	namespace nChatRoom {
		class cpiChatroom;


		typedef class tListConsole<cRoom, cRooms, cpiChatroom> tRoomConsoleBase;

/**
a console that parses commands
@author Lukas Ronge
@author Daniel Muller
*/

class cRoomConsole : public tRoomConsoleBase
{
public:
	// -- required methods
	cRoomConsole(cVHPlugin *pi) : tRoomConsoleBase(pi){AddCommands();}
	virtual ~cRoomConsole();
	virtual const char *GetParamsRegex(int cmd);
	virtual const char *CmdSuffix();
	virtual const char *CmdPrefix();
	virtual cRooms *GetTheList();
	virtual bool ReadDataFromCmd(cfBase *cmd, int CmdID, cRoom &data);
	virtual void GetHelpForCommand(int cmd, ostream &os);
	virtual bool IsConnAllowed(nSocket::cConnDC *conn,int cmd);
};

	}; // namespace nChatRoom
}; // namespace nVerliHub
#endif
