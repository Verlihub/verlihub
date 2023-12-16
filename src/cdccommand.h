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

#ifndef CDCCOMMAND_H
#define CDCCOMMAND_H
#include "ccommand.h"
#include "cobj.h"

namespace nVerliHub {
	class cUser;
	namespace nSocket {
		class cServerDC;
		class cConnDC;
	};

	template<class OwnerType>
	class tConsoleBase: public cObj
	{
		public:
		tConsoleBase(OwnerType *s) :
			cObj("nDC::Console"),
			mOwner(s)
			{}
		OwnerType *mOwner;
		virtual ~tConsoleBase(){}
		virtual int OpCommand(const string &, nSocket::cConnDC*) = 0;
		virtual int UsrCommand(const string & , nSocket::cConnDC * ) = 0;
	};

	typedef tConsoleBase<nSocket::cServerDC> cDCConsoleBase;

	class cDCCommand : public nCmdr::cCommand
	{
	public:
		cDCCommand();
		virtual ~cDCCommand(){};

		//long mActionType;

		class sDCCmdFunc : public nCmdr::cCommand::sCmdFunc
		{
		public:
			nSocket::cServerDC *mS;
			nSocket::cConnDC * mConn;
			cDCConsoleBase *mCo;

			virtual ~sDCCmdFunc(){};
			virtual bool GetIDEnum(int rank, int &id, const char *ids[], const int enums[]);
			virtual bool GetParUnEscapeStr(int rank, string &dest);
			virtual bool GetParIPRange(int rank, unsigned long &ipmin, unsigned long &ipmax);
			virtual bool GetParRegex(int rank, string &dest);
			virtual bool GetParOnlineUser(int rank, cUser *&dest, string &nick);
			void Bind(cDCConsoleBase *co);
			virtual bool operator() (nUtils::cPCRE &idrex, nUtils::cPCRE &parrex, ostream &os, void *extra);
		};

		cDCCommand(int ID, const char *IdRegex, const char *ParRegex, sDCCmdFunc *CmdFunc/*, long Action = -1*/);
		virtual void Init(void *co);
		virtual void Init(int ID, const char *IdRegex, const char *, sCmdFunc*);
	};

}; // namespace nVerliHub

#endif
