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
	for (std::vector<cPerlInterpreter*>::const_iterator i = mPerl.begin(); i != mPerl.end(); i++)
		delete *i;
}


int cPerlMulti::Parse(int argc, const char *argv[]) {
	cPerlInterpreter *perl = new cPerlInterpreter();
	int ret = perl->Parse(argc, argv);
	if (ret)
		delete perl;
	else
		mPerl.push_back(perl);
	return ret;
}

bool cPerlMulti::CallArgv(const char *Function, const char *Args[]) {
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
		if (mIntStack.size()) mIntStack.back()->SetMyContext();
		if (!ret) return false;
	}
	return true;
}

	}; // namespace nPerlPlugin
}; // namespace nVerlihub
