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
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>

namespace nVerliHub {
namespace nSocket {
	class cConnDC;
};
namespace nPythonPlugin {

class cpiPython;

class cConsole
{
public:
	cConsole(cpiPython *);
	virtual ~cConsole();
	int DoCommand(const string &str, nSocket::cConnDC *conn);
	cpiPython *mPython;

protected:
	class cfBase : public nCmdr::cCommand::sCmdFunc {
		public:
			cpiPython *GetPI() { return ((cConsole *)(mCommand->mCmdr->mOwner))->mPython; }
	};

	class cfAddPythonScript    : public cfBase { virtual bool operator()(); } mcfPythonScriptAdd;
	class cfGetPythonScript    : public cfBase { virtual bool operator()(); } mcfPythonScriptGet;
	class cfDelPythonScript    : public cfBase { virtual bool operator()(); } mcfPythonScriptDel;
	class cfReloadPythonScript : public cfBase { virtual bool operator()(); } mcfPythonScriptRe;
	class cfLogPythonScript    : public cfBase { virtual bool operator()(); } mcfPythonScriptLog;
	class cfFilesPythonScript  : public cfBase { virtual bool operator()(); } mcfPythonScriptFiles;

	nCmdr::cCommand mCmdPythonScriptAdd;
	nCmdr::cCommand mCmdPythonScriptGet;
	nCmdr::cCommand mCmdPythonScriptDel;
	nCmdr::cCommand mCmdPythonScriptRe;
	nCmdr::cCommand mCmdPythonScriptLog;
	nCmdr::cCommand mCmdPythonScriptFiles;
	nCmdr::cCommandCollection mCmdr;
};

};  // namespace nPythonPlugin
};  // namespace nVerliHub

#endif
