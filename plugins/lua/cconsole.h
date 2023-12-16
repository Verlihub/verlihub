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

#ifndef CCONSOLE_H
#define CCONSOLE_H

#include "src/ccommandcollection.h"

namespace nVerliHub {
	namespace nSocket {
		class cConnDC;
	};
	namespace nLuaPlugin {
		class cpiLua;

class cConsole
{
public:
	cConsole(cpiLua *);
	virtual ~cConsole();
	int DoCommand(const string &str, nSocket::cConnDC *conn);
	cpiLua *mLua;
protected:
	//enum { eMSG_SEND, eMSG_READ };
	class cfBase: public nCmdr::cCommand::sCmdFunc {
		public:
			cpiLua *GetPI() { return ((cConsole*)(mCommand->mCmdr->mOwner))->mLua; }
	};

	class cfGetLuaScript: public cfBase { virtual bool operator()(); } mcfLuaScriptGet;
	class cfAddLuaScript: public cfBase { virtual bool operator()(); } mcfLuaScriptAdd;
	class cfDelLuaScript: public cfBase { virtual bool operator()(); } mcfLuaScriptDel;
	class cfReloadLuaScript: public cfBase { virtual bool operator()(); } mcfLuaScriptRe;
	class cfLogLuaScript: public cfBase { virtual bool operator()(); } mcfLuaScriptLog;
	class cfErrLuaScript: public cfBase { virtual bool operator()(); } mcfLuaScriptErr;
	class cfInfoLuaScript: public cfBase { virtual bool operator()(); } mcfLuaScriptInfo;
	class cfVersionLuaScript: public cfBase { virtual bool operator()(); } mcfLuaScriptVersion;
	class cfFilesLuaScript: public cfBase { virtual bool operator()(); } mcfLuaScriptFiles;

	nCmdr::cCommand mCmdLuaScriptGet;
	nCmdr::cCommand mCmdLuaScriptAdd;
	nCmdr::cCommand mCmdLuaScriptDel;
	nCmdr::cCommand mCmdLuaScriptRe;
	nCmdr::cCommand mCmdLuaScriptLog;
	nCmdr::cCommand mCmdLuaScriptErr;
	nCmdr::cCommand mCmdLuaScriptInfo;
	nCmdr::cCommand mCmdLuaScriptVersion;
	nCmdr::cCommand mCmdLuaScriptFiles;
	nCmdr::cCommandCollection mCmdr;
};

	}; // namespace nLuaPlugin
}; // namespace nVerliHub

#endif
