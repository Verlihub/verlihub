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

#ifndef NFORBIDCCONSOLE_H
#define NFORBIDCCONSOLE_H

#include "src/ccommandcollection.h"
#include "src/cdccommand.h"

namespace nVerliHub {
	namespace nSocket {
		class cConnDC;
	};
	namespace nEnums {
		enum {
			eIL_LASTIP,
			eIL_HISTORY
		};
	};
	namespace nIPLogPlugin {
		class cIPLog;
		class cpiIPLog;

/**
a console that parses commands

@author Przemek Bryniak
*/
class cConsole
{
public:
	cConsole(cpiIPLog *);
	virtual ~cConsole();
	int DoCommand(const string &str, nSocket::cConnDC * conn);
	cpiIPLog *mIPLog;
protected:

	class cfBase : public cDCCommand::sDCCmdFunc {
		public:
		cpiIPLog *GetPI(){ return ((cConsole *)(mCommand->mCmdr->mOwner))->mIPLog;}
	};
	class cfLastIp : public cfBase { virtual bool operator()();} mcfLastIp;
	class cfHistoryOf : public cfBase { virtual bool operator()();} mcfHistoryOf;
	nCmdr::cCommand mCmdLastIp;
	nCmdr::cCommand mCmdHistoryOf;
	nCmdr::cCommandCollection mCmdr;
};
	}; // namespace nIPLogPlugin
}; // namespace nVerliHub

#endif
