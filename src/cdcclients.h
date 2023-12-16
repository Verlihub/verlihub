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

#ifndef NDIRECTCONNECTDCCLIENTS_H
#define NDIRECTCONNECTDCCLIENTS_H
#include "tmysqlmemorylist.h"
#include "tlistconsole.h"
#include "cdcconsole.h"
#include <sstream>
#include "cpcre.h"
#include "cdctag.h"
#include "cdcclient.h"

using namespace std;

namespace nVerliHub {
	/*
	using namespace nTables;

	namespace nUtils {
		class cPCRE;
	};

	using nConfig::tMySQLMemoryList;
	*/

	/*
		parser for dc tag
	*/
	class cDCTagParser
	{
		public:
		// constructor
		cDCTagParser();
		// regular expressions
		nUtils::cPCRE mTagRE;
		nUtils::cPCRE mVersRE;
		nUtils::cPCRE mModeRE;
		nUtils::cPCRE mHubsRE;
		nUtils::cPCRE mSlotsRE;
		nUtils::cPCRE mLimitRE;
	};

	namespace nSocket {
		class cServerDC;
	};

	class cDCConsole;

	namespace nTables {

		typedef tMySQLMemoryList<cDCClient, nSocket::cServerDC> tClientsBase;
		class cDCClients : public tClientsBase
		{
			static cDCTagParser mParser;
			nSocket::cServerDC *mServer;
			public:
				int mPositionInDesc;

				cDCClients(nSocket::cServerDC *);
				virtual ~cDCClients(){};
				virtual void AddFields();
				cDCClient* FindTag(const string &tagID);
				virtual bool CompareDataKey(const cDCClient &D1, const cDCClient &D2);
				bool ParsePos(const string &desc);
				cDCTag* ParseTag(const string &desc);
				//bool ValidateTag(cDCTag *tag, ostream &os, cConnDC *conn, int &code);
		};


		typedef tListConsole<cDCClient, cDCClients, cDCConsole> tDCClientConsoleBase;

		class cDCClientConsole: public tDCClientConsoleBase
		{
			public:
				cDCClientConsole(cDCConsole *console);
				virtual ~cDCClientConsole();
				virtual const char * GetParamsRegex(int cmd);
				virtual cDCClients *GetTheList();
				virtual const char *CmdSuffix();
				virtual const char *CmdPrefix();
				virtual void ListHead(ostream *os);
				virtual bool IsConnAllowed(nSocket::cConnDC *conn,int cmd);
				virtual bool ReadDataFromCmd(cfBase *cmd, int CmdID, cDCClient &data);
				virtual void GetHelpForCommand(int cmd, ostream &os);
				virtual void GetHelp(ostream &os);
		};

	}; // namespace nTables
}; // namespace nVerliHub

#endif
