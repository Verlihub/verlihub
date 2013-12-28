/*
	Copyright (C) 2007-2014 Frog, frg at otaku-anime dot net
	Copyright (C) 2006-2014 Verlihub Project, devs at verlihub-project dot org

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
//#include <iostream>
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
	w_Targs *CallFunction(int, w_Targs*);
	string mScriptName;
	int id;
	bool online;
};
	}; // namespace nPythonPlugin
}; // namespace namespace

#endif
