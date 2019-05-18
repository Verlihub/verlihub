/*
	Copyright (C) 2003-2005 Daniel Muller, dan at verliba dot cz
	Copyright (C) 2006-2019 Verlihub Team, info at verlihub dot net

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

#ifndef NDIRECTCONNECTCREDIRECTS_H
#define NDIRECTCONNECTCREDIRECTS_H

#include "ccustomredirect.h"
#include "tmysqlmemorylist.h"
#include "tlistconsole.h"
#include "cdcconsole.h"

namespace nVerliHub {
	namespace nSocket {
		class cServerDC;
		class cConnDC;
	};

	class cDCConsole;

	namespace nTables {

		typedef nConfig::tMySQLMemoryList<cRedirect, nSocket::cServerDC> tRedirectsBase;

		class cRedirects: public tRedirectsBase
		{
			public:
				cRedirects(nSocket::cServerDC *);
				virtual ~cRedirects() {};
				virtual void AddFields();
				cRedirect* Find(const string &name);
				virtual bool CompareDataKey(const cRedirect &D1, const cRedirect &D2);
				char* MatchByType(unsigned int rype);
				int MapTo(unsigned int rype);
				void Random(int &key);
		};

		typedef nConfig::tListConsole<cRedirect, cRedirects, cDCConsole> tRedirectConsoleBase;

		class cRedirectConsole: public tRedirectConsoleBase
		{
			public:
				cRedirectConsole(cDCConsole *console);
				virtual ~cRedirectConsole();
				virtual const char* GetParamsRegex(int cmd);
				virtual cRedirects* GetTheList();
				virtual const char* CmdSuffix();
				virtual const char* CmdPrefix();
				virtual void ListHead(ostream *os);
				virtual bool IsConnAllowed(nSocket::cConnDC *conn, int cmd);
				virtual bool ReadDataFromCmd(cfBase *cmd, int CmdID, cRedirect &data);
				virtual void GetHelpForCommand(int cmd, ostream &os);
				virtual void GetHelp(ostream &os);
		};

	}; // namespace nTables
}; // namespace nVerliHub

#endif
