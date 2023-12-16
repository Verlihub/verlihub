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

#ifndef CONSOLE_H
#define CONSOLE_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <tlistconsole.h>

namespace nVerliHub {
	using namespace nCmdr;

	namespace nSocket {
		class cConnDC;
	};
	namespace nPlugin{
		class cVHPlugin;
	};

	namespace nPlugMan {
		class cpiPlug;
		class cPlugs;
		class cPlug;

		typedef class nConfig::tListConsole<cPlug, cPlugs, cpiPlug> tPlugConsoleBase;

/**
a console that parses commands

@author Daniel Muller
*/

class cPlugConsole : public tPlugConsoleBase
{
public:
	// -- required methods
	cPlugConsole(nPlugin::cVHPlugin *pi) : tPlugConsoleBase(pi){AddCommands();}
	virtual ~cPlugConsole();
	virtual cPlugs *GetTheList();
	virtual void ListHead(ostream *os);
	virtual bool ReadDataFromCmd(cfBase *cmd, int CmdID, cPlug &data);

	/// ALL for commands
	virtual const char *CmdSuffix();
	virtual const char *CmdPrefix();
	virtual const char *GetParamsRegex(int cmd);
	virtual const char *CmdWord(int cmd);
	virtual bool IsConnAllowed(cConnDC* conn,int cmd);
	virtual void GetHelpForCommand(int cmd, ostream &os);
	virtual void GetHelp(ostream &os);

	enum {eLC_ON = eLC_FREE, eLC_OFF, eLC_RE};

	class cfOn  : public tPlugConsoleBase::cfBase{ virtual bool operator()();} mcfOn;
	class cfOff : public tPlugConsoleBase::cfBase{ virtual bool operator()();} mcfOff;
	class cfRe  : public tPlugConsoleBase::cfBase{ virtual bool operator()();} mcfRe;

	cDCCommand mCmdOn;
	cDCCommand mCmdOff;
	cDCCommand mCmdRe;

	virtual void AddCommands();
	// end of extra commands
};
	}; // namespace nPlugMan
}; // namespace nVerliHub

#endif
