/**************************************************************************
*   Copyright (C) 2004 by Dan Muller                                      *
*   dan at verliba.cz                                                     *
*                                                                         *
*   Copyright (C) 2004 by Janos Horvath                                   *
*   bourne at freemail dot hu                                             *
*                                                                         *
*   Copyright (C) 2011 by Shurik                                          *
*   shurik at sbin.ru                                                     *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
*   This program is distributed in the hope that it will be useful,       *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
*   GNU General Public License for more details.                          *
*                                                                         *
*   You should have received a copy of the GNU General Public License     *
*   along with this program; if not, write to the                         *
*   Free Software Foundation, Inc.,                                       *
*   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
***************************************************************************/
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
