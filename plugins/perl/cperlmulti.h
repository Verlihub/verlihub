/*
	Copyright (C) 2003-2005 Daniel Muller, dan at verliba dot cz
	Copyright (C) 2006-2025 Verlihub Team, info at verlihub dot net

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

#ifndef NSCRIPTSCPERLMULTI_H
#define NSCRIPTSCPERLMULTI_H

#include "cperlinterpreter.h"
#include <vector>

namespace nVerliHub {
	namespace nPerlPlugin {

/* Multi perl interpreters class */

class cPerlMulti
{
public:
	cPerlMulti();
	virtual ~cPerlMulti();

	int Parse(int argc, const char *argv[]);

	bool CallArgv(const char *Function, const char *Args[]);

	int Size() { return mPerl.size(); }

	std::vector<cPerlInterpreter*> mPerl;
	std::vector<std::string> mScriptStack;
	std::vector<cPerlInterpreter*> mIntStack;
};

	}; // namespace nPerlPlugin
}; // namespace nVerlihub

#endif
