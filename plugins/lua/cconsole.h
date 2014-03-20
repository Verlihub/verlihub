/*
	Copyright (C) 2003-2005 Daniel Muller, dan at verliba dot cz
	Copyright (C) 2004-2005 Janos Horvath, bourne at freemail dot hu
	Copyright (C) 2006-2012 Verlihub Team, devs at verlihub-project dot org
	Copyright (C) 2013-2014 RoLex, webmaster at feardc dot net

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
	//enum {eMSG_SEND, eMSG_Read };
	class cfBase : public nCmdr::cCommand::sCmdFunc {
		public:
		cpiLua *GetPI(){ return ((cConsole *)(mCommand->mCmdr->mOwner))->mLua;}
	};

	class cfGetLuaScript : public cfBase { virtual bool operator()();} mcfLuaScriptGet;
	class cfAddLuaScript : public cfBase { virtual bool operator()();} mcfLuaScriptAdd;
	class cfDelLuaScript : public cfBase { virtual bool operator()();} mcfLuaScriptDel;
	class cfReloadLuaScript : public cfBase { virtual bool operator()();} mcfLuaScriptRe;
	class cfLogLuaScript : public cfBase { virtual bool operator()();} mcfLuaScriptLog;
	class cfInfoLuaScript : public cfBase { virtual bool operator()();} mcfLuaScriptInfo;
	class cfVersionLuaScript : public cfBase { virtual bool operator()();} mcfLuaScriptVersion;

	nCmdr::cCommand mCmdLuaScriptGet;
	nCmdr::cCommand mCmdLuaScriptAdd;
	nCmdr::cCommand mCmdLuaScriptDel;
	nCmdr::cCommand mCmdLuaScriptRe;
	nCmdr::cCommand mCmdLuaScriptLog;
	nCmdr::cCommand mCmdLuaScriptInfo;
	nCmdr::cCommand mCmdLuaScriptVersion;
	nCmdr::cCommandCollection mCmdr;
};

	}; // namespace nLuaPlugin
}; // namespace nVerliHub

#endif
