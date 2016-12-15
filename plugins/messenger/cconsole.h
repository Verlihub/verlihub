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

#ifndef NMESSANGERCCONSOLE_H
#define NMESSANGERCCONSOLE_H

#include "src/ccommandcollection.h"

namespace nVerliHub {
	namespace nSocket {
		class cConnDC;
	};
	namespace nMessangerPlugin {
		class cpiMessanger;

/**
a console that parses commands

@author Daniel Muller
*/
class cConsole
{
public:
	cConsole(cpiMessanger *);

	virtual ~cConsole();
	int DoCommand(const string &str, nSocket::cConnDC * conn);

	cpiMessanger *mMessanger;
protected:
	enum {eMSG_SEND, eMSG_Read };
  class cfBase : public nCmdr::cCommand::sCmdFunc {
		public:
		cpiMessanger *GetMessanger(){ return ((cConsole *)(mCommand->mCmdr->mOwner))->mMessanger;}
	};
	class cfMessageSend : public cfBase { virtual bool operator()();} mcfMsgSend;
	class cfMessageRead : public cfBase { virtual bool operator()();} mcfMsgRead;
	nCmdr::cCommand mCmdMsgSend;
	nCmdr::cCommand mCmdMsgRead;
	nCmdr::cCommandCollection mCmdr;
};

	}; // namespace nMessangerPlugin
}; // namespace nVerliHub

#endif
