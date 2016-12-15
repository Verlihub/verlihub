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

#ifndef CCONSOLE_H
#define CCONSOLE_H

#include "src/ccommandcollection.h"

namespace nVerliHub {
	namespace nSocket {
		class cConnDC;
	};
	namespace nPerlPlugin {
		class cpiPerl;
class cConsole
{
public:
	cConsole(cpiPerl *);
	virtual ~cConsole();
	int DoCommand(const string &str, nSocket::cConnDC * conn);
	cpiPerl *mPerl;
protected:
	//enum {eMSG_SEND, eMSG_Read };
	class cfBase : public nCmdr::cCommand::sCmdFunc {
		public:
		cpiPerl *GetPI(){ return ((cConsole *)(mCommand->mCmdr->mOwner))->mPerl;}
	};

	class cfGetPerlScript : public cfBase { virtual bool operator()();} mcfPerlScriptGet;
	class cfAddPerlScript : public cfBase { virtual bool operator()();} mcfPerlScriptAdd;
	class cfDelPerlScript : public cfBase { virtual bool operator()();} mcfPerlScriptDel;
	class cfReloadPerlScript : public cfBase { virtual bool operator()();} mcfPerlScriptRe;

	nCmdr::cCommand mCmdPerlScriptGet;
	nCmdr::cCommand mCmdPerlScriptAdd;
	nCmdr::cCommand mCmdPerlScriptDel;
	nCmdr::cCommand mCmdPerlScriptRe;
	nCmdr::cCommand mCmdPerlScriptLog;
	nCmdr::cCommand mCmdPerlScriptInfo;
	nCmdr::cCommand mCmdPerlScriptVersion;
	nCmdr::cCommandCollection mCmdr;
};

	}; // namespace nPerlPlugin
}; // namespace nVerliHub

#endif
