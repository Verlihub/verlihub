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

#include "cpipython.h"
#include "src/cconndc.h"
#include "src/cserverdc.h"
#include "cconsole.h"
#include "cpythoninterpreter.h"
#include "src/stringutils.h"
#include "src/i18n.h"
#include <dirent.h>

namespace nVerliHub {
using namespace nUtils;
namespace nPythonPlugin {

cConsole::cConsole(cpiPython *pyt):
	mPython(pyt),
	mCmdPythonScriptAdd(0, "!pyload ", "(\\S+)", &mcfPythonScriptAdd),
	mCmdPythonScriptGet(1, "!pylist", "", &mcfPythonScriptGet),
	mCmdPythonScriptDel(2, "!pyunload ", "(\\S+)", &mcfPythonScriptDel),
	mCmdPythonScriptRe(3, "!pyreload ", "(\\S+)", &mcfPythonScriptRe),
	mCmdPythonScriptLog(4, "!pylog ", "(\\d+)", &mcfPythonScriptLog),
	mCmdPythonScriptFiles(5, "!pyfiles", "", &mcfPythonScriptFiles),
	mCmdr(this)
{
	mCmdr.Add(&mCmdPythonScriptAdd);
	mCmdr.Add(&mCmdPythonScriptDel);
	mCmdr.Add(&mCmdPythonScriptGet);
	mCmdr.Add(&mCmdPythonScriptRe);
	mCmdr.Add(&mCmdPythonScriptLog);
	mCmdr.Add(&mCmdPythonScriptFiles);
}

cConsole::~cConsole() {}

int cConsole::DoCommand(const string &str, cConnDC *conn)
{
	nCmdr::cCommand *cmd = mCmdr.FindCommand(str);

	if (cmd != NULL) {
		int id = cmd->GetID();

		if ((id >= 0) && (id <= 5) && conn && conn->mpUser
		&& (conn->mpUser->mClass < mPython->mServer->mC.plugin_mod_class)) {
			// todo: use command list size instead of constant number
			mPython->mServer->DCPublicHS(_("You have no rights to do this."), conn);
			return 1;
		}
	}

	ostringstream os;

	if (mCmdr.ParseAll(str, os, conn) >= 0) {
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

	(*mOS) << _("Loaded Python scripts") << ":\r\n\r\n ";
	(*mOS) << setw(6) << setiosflags(ios::left) << _("ID");
	(*mOS) << toUpper(_("Script")) << "\r\n";
	(*mOS) << " " << string(6 + 20, '=') << "\r\n";

	for (int i = 0; i < GetPI()->Size(); i++) {
		(*mOS) << " " << setw(6) << setiosflags(ios::left) << GetPI()->mPython[i]->id
		<< GetPI()->mPython[i]->mScriptName << "\r\n";
	}

	return true;
}

bool cConsole::cfFilesPythonScript::operator()()
{
	DIR *dir = opendir(GetPI()->mScriptDir.c_str());
	if (!dir) {
		(*mOS) << autosprintf(_("Failed loading directory: %s"), GetPI()->mScriptDir.c_str());
		return false;
	}
	(*mOS) << autosprintf(_("Python scripts found in: %s"), GetPI()->mScriptDir.c_str()) << "\r\n\r\n ";
	(*mOS) << setw(6) << setiosflags(ios::left) << _("ID");
	(*mOS) << toUpper(_("Script")) << "\r\n";
	(*mOS) << " " << string(6 + 20, '=') << "\r\n";
	string filename;
	struct dirent *dent = NULL;
	vector<string> filenames;

	while (NULL != (dent = readdir(dir))) {
		filename = dent->d_name;
		if ((filename.size() > 3) && (StrCompare(filename, filename.size() - 3, 3, ".py") == 0))
			filenames.push_back(filename);
	}
	sort(filenames.begin(), filenames.end());

	for (size_t i = 0; i < filenames.size(); i++)
		(*mOS) << " " << setw(6) << setiosflags(ios::left) << i << filenames[i] << "\r\n";

	closedir(dir);
	return true;
}

bool cConsole::cfDelPythonScript::operator()()
{
	if (!GetPI()->online) {
		(*mOS) << _("Python interpreter is not loaded.");
		return false;
	}

	string scriptfile;
	GetParStr(1, scriptfile);
	bool number = false;
	int num = 0;

	if (GetPI()->IsNumber(scriptfile.c_str())) {
		num = atoi(scriptfile.c_str());
		number = true;
	} else if (scriptfile.find_first_of('/') == string::npos) {
		scriptfile = GetPI()->mScriptDir + scriptfile;
	}

	vector<cPythonInterpreter *>::iterator it;
	cPythonInterpreter *li;

	for (it = GetPI()->mPython.begin(); it != GetPI()->mPython.end(); ++it) {
		li = *it;

		if ((number && (num == li->id))
		|| (!number && (StrCompare(li->mScriptName, 0, li->mScriptName.size(), scriptfile) == 0))) {
			(*mOS) << autosprintf(_("Script stopped: %s"), li->mScriptName.c_str());
			delete li;
			GetPI()->mPython.erase(it);
			return true;
		}
	}

	if (number)
		(*mOS) << autosprintf(_("Script not stopped because it's not loaded: #%d"), num);
	else
		(*mOS) << autosprintf(_("Script not stopped because it's not loaded: %s"), scriptfile.c_str());

	return false;
}

bool cConsole::cfAddPythonScript::operator()()
{
	if (!GetPI()->online) {
		(*mOS) << _("Python interpreter is not loaded.");
		return false;
	}

	string scriptfile, filename;
	GetParStr(1, scriptfile);
	bool number = false;
	int num = 0;

	if (GetPI()->IsNumber(scriptfile.c_str())) {
		num = atoi(scriptfile.c_str());
		number = true;
	} else if (scriptfile.find_first_of('/') == string::npos) {
		scriptfile = GetPI()->mScriptDir + scriptfile;
	}

	if (number) {
		DIR *dir = opendir(GetPI()->mScriptDir.c_str());

		if (!dir) {
			(*mOS) << autosprintf(_("Failed loading directory: %s"), GetPI()->mScriptDir.c_str());
			return false;
		}

		struct dirent *dent = NULL;
		int i = 0;

		while (NULL != (dent = readdir(dir))) {
			filename = dent->d_name;

			if ((filename.size() > 3) && (StrCompare(filename, filename.size() - 3, 3, ".py") == 0)) {
				if (i == num) {
					scriptfile = GetPI()->mScriptDir + filename;
					break;
				}

				i++;
			}
		}

		closedir(dir);
	}

	vector<cPythonInterpreter *>::iterator it;
	cPythonInterpreter *li;

	for (it = GetPI()->mPython.begin(); it != GetPI()->mPython.end(); ++it) {
		li = *it;

		if (StrCompare(li->mScriptName, 0, li->mScriptName.size(), scriptfile) == 0) {
			(*mOS) << autosprintf(_("Script is already loaded: %s"), scriptfile.c_str());
			return false;
		}
	}

	cPythonInterpreter *ip = new cPythonInterpreter(scriptfile);

	if (!ip) {
		(*mOS) << _("Failed to allocate new Python interpreter.");
		return false;
	}

	if (ip->Init()) {
		(*mOS) << autosprintf(_("Script is now loaded: %s"), scriptfile.c_str());
		GetPI()->AddData(ip);
		return true;
	} else {
		(*mOS) << autosprintf(_("Script not found or couldn't be parsed: %s"), scriptfile.c_str());
		delete ip;
		return false;
	}
}

bool cConsole::cfReloadPythonScript::operator()()
{
	if (!GetPI()->online) {
		(*mOS) << _("Python interpreter is not loaded.");
		return false;
	}

	string scriptfile;
	GetParStr(1, scriptfile);
	bool number = false;
	int num = 0;
	int position = 0;

	if (GetPI()->IsNumber(scriptfile.c_str())) {
		num = atoi(scriptfile.c_str());
		number = true;
	} else if (scriptfile.find_first_of('/') == string::npos) {
		scriptfile = GetPI()->mScriptDir + scriptfile;
	}

	vector<cPythonInterpreter *>::iterator it;
	cPythonInterpreter *li;
	bool found = false;

	for (it = GetPI()->mPython.begin(); it != GetPI()->mPython.end(); ++it, ++position) {
		li = *it;

		if ((number && (num == li->id))
		|| (!number && (StrCompare(li->mScriptName, 0, li->mScriptName.size(), scriptfile) == 0))) {
			found = true;
			scriptfile = li->mScriptName;
			(*mOS) << autosprintf(_("Script stopped: %s"), li->mScriptName.c_str());
			delete li;
			GetPI()->mPython.erase(it);
			break;
		}
	}

	if (!found) {
		if (number)
			(*mOS) << autosprintf(_("Script not stopped because it's not loaded: #%d"), num);
		else
			(*mOS) << autosprintf(_("Script not stopped because it's not loaded: %s"), scriptfile.c_str());
	}

	cPythonInterpreter *ip = new cPythonInterpreter(scriptfile);

	if (!ip) {
		(*mOS) << " " << _("Failed to allocate new Python interpreter.");
		return false;
	}

	if (ip->Init()) {
		(*mOS) << " " << autosprintf(_("Script is now loaded: %s"), scriptfile.c_str());
		GetPI()->AddData(ip, position);
		return true;
	} else {
		(*mOS) << " " << autosprintf(_("Script not found or couldn't be parsed: %s"), scriptfile.c_str());
		delete ip;
		return false;
	}
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
	(*mOS) << autosprintf(_("Updated %s.%s from '%s' to '%s'."), "pi_python", "log_level", 
		oldValue.c_str(), newValue.c_str());
	cpiPython::me->LogLevel(atoi(level.c_str()));
	return true;
}

};  // namespace nPythonPlugin
};  // namespace nVerliHub
