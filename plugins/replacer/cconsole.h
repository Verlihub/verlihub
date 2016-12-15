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

#ifndef NREPLACECCONSOLE_H
#define NREPLACECCONSOLE_H

#include "src/ccommandcollection.h"
#include "src/cconndc.h"

namespace nVerliHub {
	namespace nReplacePlugin {
		class cpiReplace;
/**
a console that parses commands
@author Daniel Muller
changes by Pralcio
*/
class cConsole
{
public:
	cConsole(cpiReplace *);
	virtual ~cConsole();
	int DoCommand(const string &str, nSocket::cConnDC * conn);
protected:
	cpiReplace *mReplace;
	enum {eMSG_SEND, eMSG_Read };
	class cfBase : public nCmdr::cCommand::sCmdFunc {
		public:
		cpiReplace *GetPI(){ return ((cConsole *)(mCommand->mCmdr->mOwner))->mReplace;}
	};
	class cfAddReplacer : public cfBase { virtual bool operator()();} mcfReplaceAdd;
	class cfGetReplacer : public cfBase { virtual bool operator()();} mcfReplaceGet;
	class cfDelReplacer : public cfBase { virtual bool operator()();} mcfReplaceDel;
	nCmdr::cCommand mCmdReplaceAdd;
	nCmdr::cCommand mCmdReplaceGet;
	nCmdr::cCommand mCmdReplaceDel;
	nCmdr::cCommandCollection mCmdr;
};
	}; // namespace nReplacePlugin
}; // namespace nReplacePlugin

#endif
