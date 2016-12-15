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
#include "cforbidden.h"

namespace nVerliHub {

	namespace  nSocekt {
		class cConnDC;
	};
	namespace nForbidPlugin {


/**
a console that parses commands

@author Daniel Muller
*/
class cForbidConsole : public nConfig::tListConsole<cForbiddenWorker, cForbidden, cpiForbid>
{
public:
	cForbidConsole(nPlugin::cVHPlugin *pi) : tListConsole<cForbiddenWorker, cForbidden, cpiForbid>(pi)
	{
		AddCommands();
	}

	virtual ~cForbidConsole();
	virtual const char *GetParamsRegex(int cmd);
	virtual const char *CmdSuffix();
	virtual const char *CmdPrefix();
	virtual cForbidden *GetTheList();
	virtual bool ReadDataFromCmd(cfBase *cmd, int CmdID, cForbiddenWorker &data);
	virtual void GetHelpForCommand(int cmd, ostream &os);
};

	}; // namespace nForbid
}; // namespace nVerliHub

#endif
