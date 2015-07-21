/*
	Copyright (C) 2003-2005 Daniel Muller, dan at verliba dot cz
	Copyright (C) 2006-2015 Verlihub Team, info at verlihub dot net

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

#include "cpipython.h"
#include "src/cconndc.h"
#include "src/cserverdc.h"
#include "cconsole.h"
#include "cpythoninterpreter.h"
#include "src/stringutils.h"
#include "src/i18n.h"

namespace nVerliHub {
	using namespace nUtils;
	namespace nPythonPlugin {

cConsole::cConsole(cpiPython *pyt) :
	mPython(pyt),
	mCmdPythonScriptAdd(1,"!pyload ", "(\\S+)", &mcfPythonScriptAdd),
	mCmdPythonScriptGet(0,"!pylist", "", &mcfPythonScriptGet),
	mCmdPythonScriptDel(2,"!pyunload ", "(\\S+)", &mcfPythonScriptDel),
	mCmdPythonScriptRe(3,"!pyreload ", "(\\S+)", &mcfPythonScriptRe),
	mCmdPythonScriptLog(4,"!pylog ", "(\\d+)", &mcfPythonScriptLog),
	mCmdr(this)
{
	mCmdr.Add(&mCmdPythonScriptAdd);
	mCmdr.Add(&mCmdPythonScriptDel);
	mCmdr.Add(&mCmdPythonScriptGet);
	mCmdr.Add(&mCmdPythonScriptRe);
	mCmdr.Add(&mCmdPythonScriptLog);
}

cConsole::~cConsole()
{
}

int cConsole::DoCommand(const string &str, cConnDC *conn)
{
	nCmdr::cCommand *cmd = mCmdr.FindCommand(str);

	if (cmd != NULL) {
		int id = cmd->GetID();

		if (id >= 0 && id <= 4 && conn && conn->mpUser && conn->mpUser->mClass < mPython->mServer->mC.plugin_mod_class) {
			mPython->mServer->DCPublicHS(_("You have no rights to do this."), conn);
			return 1;
		}
	}

	ostringstream os;

	if (mCmdr.ParseAll(str, os, conn) >= 0)	{
		mPython->mServer->DCPublicHS(os.str().c_str(), conn);
		return 1;
	}

	return 0;
}

bool cConsole::cfGetPythonScript::operator()()
{
	if (!GetPI()->online) {
		(*mOS) << _("Python interpreter is not loaded.");
		return true;
	}

	(*mOS) << _("Loaded Python scripts") << ":\r\n\r\n";
	(*mOS) << setw(6) << setiosflags(ios::left) << _("ID");
	(*mOS) << toUpper(_("Script")) << "\r\n";
	(*mOS) << " " << string(6 + 20, '=') << "\r\n";

	for (int i = 0; i < GetPI()->Size(); i++) {
		(*mOS) << " " << setw(6) << setiosflags(ios::left) << GetPI()->mPython[i]->id << GetPI()->mPython[i]->mScriptName << "\r\n";
	}

	return true;
}

bool cConsole::cfDelPythonScript::operator()()
{
	string scriptfile;
	GetParStr(1,scriptfile);

	if (!GetPI()->online) {
		(*mOS) << _("Python interpreter is not loaded.");
		return true;
	}

	bool found = false;
	bool number = false;
	int num = 0;
	if (GetPI()->IsNumber(scriptfile.c_str())) {
		num = atoi(scriptfile.c_str());
		number = true;
	}

	vector<cPythonInterpreter *>::iterator it;
	cPythonInterpreter *li;
	for(it = GetPI()->mPython.begin(); it != GetPI()->mPython.end(); ++it) {
		li = *it;
		if((number && num == li->id) || (!number && StrCompare(li->mScriptName,0,li->mScriptName.size(),scriptfile) == 0)) {
			found = true;
			(*mOS) << autosprintf(_("Script %s stopped."), li->mScriptName.c_str());
			delete li;
			GetPI()->mPython.erase(it);

			break;
		}
	}

	if(!found) {
		if(number)
			(*mOS) << autosprintf(_("Script #%s not stopped because it is not running."), scriptfile.c_str());
		else
			(*mOS) << autosprintf(_("Script %s not stopped because it is not running."), scriptfile.c_str());
	}
	return true;
}

bool cConsole::cfAddPythonScript::operator()()
{
	string scriptfile;
	GetParStr(1, scriptfile);

	if(!GetPI()->online) {
		(*mOS) << _("Python interpreter is not loaded.");
		return true;
	}

	cPythonInterpreter *ip = new cPythonInterpreter(scriptfile);
	if(!ip) {
		(*mOS) << _("Failed to allocate new Python interpreter.");
		return true;
	}

	GetPI()->mPython.push_back(ip);
	if(ip->Init())
		(*mOS) << autosprintf(_("Script %s is now running."), ip->mScriptName.c_str());
	else {
		(*mOS) << autosprintf(_("Script %s not found or could not be parsed."), scriptfile.c_str());
		GetPI()->mPython.pop_back();
		delete ip;
	}

	return true;
}

bool cConsole::cfLogPythonScript::operator()()
{
	if (!GetPI()->online) {
		(*mOS) << _("Python interpreter is not loaded.");
		return true;
	}
	string level;
	GetParStr(1, level);
	ostringstream ss;
   	ss << cpiPython::log_level;
   	string oldValue = ss.str();
	ss.str("");
	ss << level;
   	string newValue = ss.str();
	(*mOS) << autosprintf(_("Updated %s.%s from '%s' to '%s'."), "pi_python", "log_level", oldValue.c_str(), newValue.c_str());
	cpiPython::me->LogLevel( atoi(level.c_str()) );
	return true;
}

bool cConsole::cfReloadPythonScript::operator()()
{
	string scriptfile;
	GetParStr(1,scriptfile);

	if (!GetPI()->online) {
		(*mOS) << _("Python interpreter is not loaded.");
		return true;
	}

	bool found = false;
	bool number = false;
	int num = 0;
	if (GetPI()->IsNumber(scriptfile.c_str())) {
		num = atoi(scriptfile.c_str());
		number = true;
	}

	vector<cPythonInterpreter *>::iterator it;
	cPythonInterpreter *li;
	string name;
	for(it = GetPI()->mPython.begin(); it != GetPI()->mPython.end(); ++it) {
		li = *it;
		if((number && num == li->id) || (!number && StrCompare(li->mScriptName,0,li->mScriptName.size(),scriptfile) == 0)) {
			found = true;
			name = li->mScriptName;
			(*mOS) << autosprintf(_("Script %s stopped."), li->mScriptName.c_str()) << " ";
			delete li;
			GetPI()->mPython.erase(it);
			break;
		}
	}

	if(!found) {
		if(number)
			(*mOS) << autosprintf(_("Script #%s not stopped because it is not running."), scriptfile.c_str()) << " ";
		else
			(*mOS) << autosprintf(_("Script %s not stopped because it is not running."), scriptfile.c_str()) << " ";
		return false;
	} else {
		cPythonInterpreter *ip = new cPythonInterpreter(name);
		if(!ip) {
			(*mOS) << _("Failed to allocate new Python interpreter.");
			return true;
		}

		GetPI()->mPython.push_back(ip);
		if(ip->Init())
			(*mOS) << autosprintf(_("Script %s is now running."), ip->mScriptName.c_str());
		else {
			(*mOS) << autosprintf(_("Script %s not found or could not be parsed."), scriptfile.c_str());
			GetPI()->mPython.pop_back();
			delete ip;
		}
		return true;
	}
}

	}; // namespace nPythonPlugin
}; // namespace nVerliHub
