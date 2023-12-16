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

#ifndef NDIRECTCONNECT_NTABLESCCONNTYPES_H
#define NDIRECTCONNECT_NTABLESCCONNTYPES_H
#include <string>
#include "tmysqlmemorylist.h"
#include "tlistconsole.h"

using namespace std;
namespace nVerliHub {
	//using namespace nConfig;
	namespace nSocekt {
		class cServerDC;
		class cConnDC;
	};
	class cDCConsole;

	namespace nTables {
		class cConnType
		{
			public:
				cConnType();
				virtual ~cConnType ();

				// database variables
				string mIdentifier;
				string mDescription;
				int mTagMaxSlots;
				int mTagMinSlots;
				double mTagMinLimit;
				double mTagMinLSRatio;

				// Overriding and needed methods
				virtual void OnLoad();
				friend ostream &operator << (ostream &, cConnType &);
		};

		typedef nConfig::tMySQLMemoryList<cConnType, nSocket::cServerDC> tConnTypesBase;

		/**
		Table for connection types and their configuration

		@author Daniel Muller
		*/

		class cConnTypes : public tConnTypesBase
		{
			public:
				cConnTypes(nSocket::cServerDC *server);
				virtual ~cConnTypes();
				// overiding methods
				virtual void AddFields();
				virtual bool CompareDataKey(const cConnType &D1, const cConnType &D2);

				//custom methods
				cConnType *FindConnType(const string &identifier);
		};

		typedef nConfig::tListConsole<cConnType, cConnTypes, cDCConsole> tConnTypeConsoleBase;

		class cConnTypeConsole: public tConnTypeConsoleBase
		{
			public:
				cConnTypeConsole(cDCConsole *console);
				virtual ~cConnTypeConsole();
				virtual const char * GetParamsRegex(int cmd);
				virtual cConnTypes *GetTheList();
				virtual const char *CmdSuffix();
				virtual const char *CmdPrefix();
				virtual void ListHead(ostream *os);
				virtual bool IsConnAllowed(nSocket::cConnDC *conn,int cmd);
				virtual bool ReadDataFromCmd(cfBase *cmd, int CmdID, cConnType &data);
				virtual void GetHelpForCommand(int cmd, ostream &os);
				virtual void GetHelp(ostream &os);
		};

	}; // namespace nTables
}; // namespace nVerliHub

#endif
