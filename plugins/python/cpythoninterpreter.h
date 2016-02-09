/*
	Copyright (C) 2003-2005 Daniel Muller, dan at verliba dot cz
	Copyright (C) 2006-2016 Verlihub Team, info at verlihub dot net

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

#ifndef CPYTHONINTERPRETER_H
#define CPYTHONINTERPRETER_H

#include "wrapper.h"
#include <string>
#include "src/script_api.h"
#include <iostream>
#include <string>

using namespace std;
namespace nVerliHub {
namespace nPythonPlugin {

class cPythonInterpreter
{
public:
	cPythonInterpreter(string scriptname);
	~cPythonInterpreter();
	bool Init();
	w_Targs *CallFunction(int, w_Targs *);
	string mScriptName;
	string name;
	string version;
	int id;
	bool online;
	bool receive_all_script_queries;
};

};  // namespace nPythonPlugin
};  // namespace namespace

#endif
