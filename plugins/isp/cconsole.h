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

#include "src/tlistconsole.h"
#include "cisps.h"

namespace nVerliHub {
	namespace nSocket {
		class cConnDC;
	};
	namespace nIspPlugin {

typedef class nConfig::tListConsole<cISP, cISPs, cpiISP> tISPConsoleBase;
/**
a console that parses commands

@author Daniel Muller
*/
class cISPConsole : public tISPConsoleBase
{
public:
	cISPConsole(nPlugin::cVHPlugin *pi) : tISPConsoleBase(pi)
	{
		AddCommands();
	}

	virtual ~cISPConsole();
	virtual const char *GetParamsRegex(int cmd);
	virtual const char *CmdSuffix();
	virtual const char *CmdPrefix();
	virtual cISPs *GetTheList();
	virtual bool ReadDataFromCmd(cfBase *cmd, int CmdID, cISP &data);
	virtual void GetHelpForCommand(int cmd, ostream &os);
};
	}; // namespace nIspPlugin
}; // namespace nVerliHub

#endif
