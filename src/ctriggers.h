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

#ifndef NDIRECTCONNECTCTRIGGERS_H
#define NDIRECTCONNECTCTRIGGERS_H
#include "ctrigger.h"
#include "tmysqlmemorylist.h"
#include "tlistconsole.h"
#include "cdcconsole.h"

namespace nVerliHub {
	namespace nSocket {
		class cServerDC;
	};
	class cDCConsole;
	namespace nTables {

		typedef nConfig::tMySQLMemoryList<cTrigger, nSocket::cServerDC> tTriggersBase;

  /**
  This class contains the list of all triggers and is able to add, edit and delete them

  @author Daniel Muller
  @author Simoncelli Davide

  */
class cTriggers : public tTriggersBase
{
public:
	cTriggers(nSocket::cServerDC *);
	virtual ~cTriggers(){};
	virtual void AddFields();
	cTrigger * Find(const string &name); //@todo Trigger stuff
	virtual bool CompareDataKey(const cTrigger &D1, const cTrigger &D2);
	void OnTimer(long now);
	// useful functions
	void TriggerAll(int FlagMask, nSocket::cConnDC *conn);
	bool DoCommand(nSocket::cConnDC *conn, const string &cmd, istringstream &cmd_line, nSocket::cServerDC &server);
};


typedef nConfig::tListConsole<cTrigger, cTriggers, cDCConsole> tTriggerConsoleBase;

  /**

  Trigger console to manage triggers

  @author Daniel Muller
  @author Simoncelli Davide

  */

class cTriggerConsole: public tTriggerConsoleBase
{
public:
	cTriggerConsole(cDCConsole *console);
	virtual ~cTriggerConsole();
	virtual const char * GetParamsRegex(int cmd);
	virtual cTriggers *GetTheList();
	virtual const char *CmdSuffix();
	virtual const char *CmdPrefix();
	virtual void ListHead(ostream *os);
	virtual bool IsConnAllowed(nSocket::cConnDC *conn,int cmd);
	virtual bool ReadDataFromCmd(cfBase *cmd, int CmdID, cTrigger &data);
	virtual void GetHelpForCommand(int cmd, ostream &os);
	virtual void GetHelp(ostream &os);
	bool CheckData(cfBase *cmd, cTrigger &tmp);
};

	}; // namespace nTables
}; // namespace nVerliHub

#endif
