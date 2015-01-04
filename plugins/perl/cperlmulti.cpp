/**************************************************************************
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
#include <iostream>
#include <vector>
#include "cperlmulti.h"
#include "cperlinterpreter.h"

namespace nVerliHub {
	namespace nPerlPlugin {

cPerlMulti::cPerlMulti()
{
}

cPerlMulti::~cPerlMulti()
{
	for(std::vector<cPerlInterpreter*>::const_iterator i = mPerl.begin(); i != mPerl.end(); i++)
		delete *i;
}


int cPerlMulti::Parse(int argc, char*argv[]) {
	cPerlInterpreter *perl = new cPerlInterpreter();
	int ret = perl->Parse(argc, argv);
	mPerl.push_back(perl);
	return ret;
}

bool cPerlMulti::CallArgv(const char *Function, char * Args [] ) {
	int s=0;
	for(std::vector<cPerlInterpreter*>::const_iterator i = mPerl.begin(); i != mPerl.end(); i++) {
		// Push scriptname and cPerlInterpreter (required for vh::ScriptCommand)
		mScriptStack.push_back((*i)->mScriptName);
		mIntStack.push_back(*i);
		s++;
		bool ret = (*i)->CallArgv(Function, Args);
		//std::cerr << "Call " << Function << " " << Args[0] << " " << " on script " << (*i)->mScriptName << " returns " << ret << std::endl;
		mScriptStack.pop_back();
		mIntStack.pop_back();
		// Restore previous context for vh::ScriptCommand
		if(mIntStack.size()) mIntStack.back()->SetMyContext();
		if(!ret) return false;
	}
	return true;
}

	}; // namespace nPerlPlugin
}; // namespace nVerlihub
