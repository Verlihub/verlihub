/*
	Copyright (C) 2003-2005 Daniel Muller, dan at verliba dot cz
	Copyright (C) 2006-2014 Verlihub Project, devs at verlihub-project dot org

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

#define HAVE_OSTREAM 1
#include <verlihub/ccmdr.h>

class cpiForbid;
using namespace nCmdr;

namespace nDirectConnect { class cConnDC; };
using namespace nDirectConnect;

namespace nForbid
{


/**
a console that parses commands

@author Daniel Muller
*/
class cConsole
{
public:
	cConsole(cpiForbid *);
	virtual ~cConsole();
	int DoCommand(const string &str, cConnDC * conn);
protected:
	cpiForbid *mForbid;
	enum {eMSG_SEND, eMSG_Read };
	class cfBase : public cCommand::sCmdFunc {
		public:
		cpiForbid *GetPI(){ return ((cConsole *)(mCommand->mCmdr->mOwner))->mForbid;}
	};
	class cfGetForbidden : public cfBase { virtual bool operator()();} mcfForbidGet;
	class cfAddForbidden : public cfBase { virtual bool operator()();} mcfForbidAdd;
	class cfDelForbidden : public cfBase { virtual bool operator()();} mcfForbidDel;
	cCommand mCmdForbidGet;
	cCommand mCmdForbidAdd;
	cCommand mCmdForbidDel;
	cCmdr mCmdr;
};

};

#endif
