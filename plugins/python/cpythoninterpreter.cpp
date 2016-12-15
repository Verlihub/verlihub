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

#include "cpythoninterpreter.h"
#include "cpipython.h"
//#include <iostream>
#include "src/cserverdc.h"
#include "src/cuser.h"

using namespace std;
namespace nVerliHub {
namespace nPythonPlugin {

cPythonInterpreter::cPythonInterpreter(string scriptname) : mScriptName(scriptname)
{
	id = -1;
	online = false;
	receive_all_script_queries = false;
	name = cpiPython::GetName(scriptname.c_str());
	version = "0.0.0";
	return;
}

cPythonInterpreter::~cPythonInterpreter()
{
	if (!cpiPython::lib_unload || !cpiPython::lib_callhook) {
		log("PY: cPythonInterpreter can't use vh_python_wrapper!\n");
		return;
	}
	online = false;
	if (id > -1) cpiPython::lib_unload(id);
}

bool cPythonInterpreter::Init()
{
	if (!cpiPython::lib_reserveid || !cpiPython::lib_load || !cpiPython::lib_pack) {
		log("PY: cPythonInterpreter can't use vh_python_wrapper!\n");
		return false;
	}
	id = cpiPython::lib_reserveid();
	w_Targs *a = cpiPython::lib_pack("lssssls", id, mScriptName.c_str(), cpiPython::botname.c_str(), 
		cpiPython::opchatname.c_str(), cpiPython::me->server->mConfigBaseDir.c_str(), 
		(long)cpiPython::me->server->mStartTime.Sec(), cpiPython::me->server->mDBConf.config_name.c_str());
	if (!a) {
		id = -1;
		return false;
	}
	id = cpiPython::lib_load(a);
	if (a) free(a);
	if (id > -1) {
		log1("PY: cPythonInterpreter loaded script %d:%s\n", id, mScriptName.c_str());
		online = true;
		return true;
	}
	return false;
}

w_Targs *cPythonInterpreter::CallFunction(int func, w_Targs *args)
{
	if (!cpiPython::lib_hashook || !cpiPython::lib_callhook) {
		log("PY: cPythonInterpreter can't use vh_python_wrapper!\n");
		return NULL;
	}
	if (id < 0 || !online) {
		log2("PY: cPythonInterpreter script is unavailable\n");
		return NULL;
	}
	if (!cpiPython::lib_hashook(id, func))
		return NULL;  // true == further processing by other plugins
	w_Targs *res = cpiPython::lib_callhook(id, func, args);
	return res;
}

};  // namespace nPythonPlugin
};  // namespace nVerliHub
