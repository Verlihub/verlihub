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

#ifndef NCONFIGLISTCONSOLE_H
#define NCONFIGLISTCONSOLE_H
#include "cdccommand.h"
#include "ccommandcollection.h"
#include "i18n.h"

namespace nVerliHub {
	namespace nSocket {
		class cConnDC;
	};
	namespace nConfig {
	using namespace nCmdr;

/**
a console that parses commands for Lists based on tMySQLMemoryList

@author Daniel Muller
*/

template <class DATA_TYPE, class LIST_TYPE, class OWNER_TYPE>
class tListConsole : public tConsoleBase<OWNER_TYPE>
{
protected:
	class cfBase;

public:
	tListConsole(void *owner):
		tConsoleBase<OWNER_TYPE>((OWNER_TYPE *)owner),
		//mOwner((OWNER_TYPE *)owner),
		mCmdr(this)
	{}

	virtual ~tListConsole()
	{};

	enum { eLC_ADD, eLC_DEL, eLC_MOD, eLC_LST, eLC_HELP, eLC_FREE };

	virtual void AddCommands()
	{
		mCmdAdd.Init(int(eLC_ADD), CmdId(eLC_ADD), GetParamsRegex(eLC_ADD), &mcfAdd);
		mCmdDel.Init(eLC_DEL, CmdId(eLC_DEL), GetParamsRegex(eLC_DEL), &mcfDel);
		mCmdMod.Init(eLC_MOD, CmdId(eLC_MOD), GetParamsRegex(eLC_MOD), &mcfMod);
		mCmdLst.Init(eLC_LST, CmdId(eLC_LST), "", &mcfLst);
		mCmdHelp.Init(eLC_HELP, CmdId(eLC_HELP), "", &mcfHelp);
		mCmdr.Add(&mCmdAdd);
		mCmdr.Add(&mCmdDel);
		mCmdr.Add(&mCmdMod);
		mCmdr.Add(&mCmdLst);
		mCmdr.Add(&mCmdHelp);
	}

	virtual int OpCommand(const string &str, nSocket::cConnDC *conn)
	{
		return this->DoCommand(str, conn);
	}

	virtual int UsrCommand(const string &str, nSocket::cConnDC *conn)
	{
		return this->DoCommand(str, conn);
	}

	virtual int DoCommand(const string &str, nSocket::cConnDC *conn)
	{
		cCommand *Cmd = mCmdr.FindCommand(str);

		if (Cmd) {
			ostringstream os;

			if (this->IsConnAllowed(conn, Cmd->GetID()))
				mCmdr.ExecuteCommand(Cmd, os, conn);
			else
				os << _("You have no rights to do this.");
		
			if (os.str().size())
				this->mOwner->mServer->DCPublicHS(os.str().c_str(), conn);

			return 1;
		}

		return 0;
	}

	virtual const char * GetParamsRegex(int) = 0;
	virtual LIST_TYPE *GetTheList() = 0;
	virtual const char *CmdSuffix() { return ""; }
	virtual const char *CmdPrefix() { return "\\+"; }
	virtual void ListHead(ostream *os) {}
	virtual bool IsConnAllowed(nSocket::cConnDC *conn, int cmd) { return true; }
	virtual bool ReadDataFromCmd(cfBase *cmd, int CmdID, DATA_TYPE &data) = 0;

	virtual const char *CmdWord(int cmd)
	{
		switch (cmd) {
			case eLC_ADD:
				return "add";
			case eLC_DEL:
				return "del";
			case eLC_MOD:
				return "mod";
			case eLC_LST:
				return "lst";
			case eLC_HELP:
				return "h";
			default:
				return "???";
		}
	}

	virtual const char *CmdSuffixWithSpace(int cmd)
	{
		static string id;
		id = CmdSuffix();

		if (!((cmd == eLC_LST) || (cmd == eLC_HELP)))
			id += ' ';

		return id.c_str();
	}

	virtual const char *CmdId(int cmd)
	{
		static string id;
		id = CmdPrefix();
		id += CmdWord(cmd);
		id += CmdSuffixWithSpace(cmd);
		return id.c_str();
	}

	virtual void GetHelpForCommand(int cmd, ostream &os)
	{
		os << this->CmdId(cmd) << this->GetParamsRegex(cmd);
	}

	virtual void GetHelp(ostream &os)
	{
		os << _("No help available for this command.");
	}

	virtual OWNER_TYPE * GetPlugin() { return this->mOwner; }

protected:
	class cfBase: public cDCCommand::sDCCmdFunc
	{
		public:
			~cfBase()
			{};

			tListConsole<DATA_TYPE, LIST_TYPE, OWNER_TYPE> *GetConsole()
			{
				return (tListConsole<DATA_TYPE, LIST_TYPE, OWNER_TYPE> *)(mCommand->mCmdr->mOwner);
			}

			virtual LIST_TYPE *GetTheList()
			{
				if (this->GetConsole() != NULL)
					return this->GetConsole()->GetTheList();
				else
					return NULL;
			}

			virtual void GetSyntaxHelp(ostream &os, cCommand *cmd)
			{
				this->GetConsole()->GetHelpForCommand(cmd->GetID(), os);
			}
	};

	class cfAdd: public cfBase
	{
	public:
		virtual ~cfAdd()
		{};

		virtual bool operator()()
		{
			DATA_TYPE Data;

			if (this->GetConsole() && this->GetConsole()->ReadDataFromCmd(this, eLC_ADD, Data)) {
				LIST_TYPE *list = this->GetTheList();

				if (list) {
					if (!list->FindData(Data)) {
						DATA_TYPE *AddedData = list->AddData(Data);

						if (AddedData) {
							list->OnLoadData(*AddedData);
							(*this->mOS) << _("Item successfully added") << ": " << (*AddedData);
							return true;
						} else {
							(*this->mOS) << _("Unable to add item.");
						}
					} else {
						(*this->mOS) << _("Item already exists.");
					}
				}
			//} else {
				//(*this->mOS) << "\r\n";
			}

			return false;
		}
	} mcfAdd;

	class cfDel: public cfBase
	{
	public:
		virtual ~cfDel()
		{};

		virtual bool operator()()
		{
			DATA_TYPE Data;

			if (this->GetConsole() && this->GetConsole()->ReadDataFromCmd(this, eLC_DEL, Data)) {
				if (this->GetTheList() && this->GetTheList()->FindData(Data)) {
					this->GetTheList()->DelData(Data);
					(*this->mOS) << _("Item successfully deleted.");
					return true;
				}
			}

			(*this->mOS) << _("Item not found.");
			return false;
		}
	} mcfDel;

	class cfMod: public cfBase
	{
	public:
		virtual ~cfMod()
		{};

		virtual bool operator()()
		{
			DATA_TYPE Data, * pOrig;
			tListConsole<DATA_TYPE, LIST_TYPE, OWNER_TYPE> *Console;
			Console = this->GetConsole();

			if (Console && Console->ReadDataFromCmd(this, eLC_MOD, Data)) {
				if (this->GetTheList() && (pOrig = this->GetTheList()->FindData(Data))) {
					if (Console->ReadDataFromCmd(this, eLC_MOD, *pOrig)) {
						this->GetTheList()->UpdateData(*pOrig);
						(*this->mOS) << _("Item successfully modified") << ": " << (*pOrig);
						return true;
					} else {
						(*this->mOS) << _("Error in item data.");
						return false;
					}
				}
			}

			(*this->mOS) << _("Item not found.");
			return false;
		}
	} mcfMod;

	class cfLst: public cfBase
	{
	public:
		virtual ~cfLst()
		{};

		virtual bool operator()()
		{
			DATA_TYPE *pData;
			this->GetConsole()->ListHead(this->mOS);

			for(int i = 0; i < this->GetTheList()->Size(); i++) {
				pData = (*this->GetTheList())[i];
				(*this->mOS) << "\r\n" << (*pData);
			}

			return true;
		}
	} mcfLst;

	class cfHelp: public cfBase
	{
		public:
			virtual ~cfHelp()
			{};

			virtual bool operator()()
			{
				this->GetConsole()->GetHelp(*this->mOS);
				return true;
			}
	} mcfHelp;

	cDCCommand mCmdAdd;
	cDCCommand mCmdDel;
	cDCCommand mCmdMod;
	cDCCommand mCmdLst;
	cDCCommand mCmdHelp;

	//OWNER_TYPE *mOwner;
	cCommandCollection mCmdr;
};

	}; // namespace nConfig
}; // namespace nVerliHub

#endif
