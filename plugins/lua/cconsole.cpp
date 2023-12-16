/*
	Copyright (C) 2003-2005 Daniel Muller, dan at verliba dot cz
	Copyright (C) 2006-2024 Verlihub Team, info at verlihub dot net

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

extern "C"
{
#include <lua.h>
}

#include "src/cconndc.h"
#include "src/cserverdc.h"
#include "cconsole.h"
#include "cpilua.h"
#include "cluainterpreter.h"
#include "src/stringutils.h"
#include "src/i18n.h"
#include <dirent.h>

namespace nVerliHub {
	using namespace nUtils;
	namespace nLuaPlugin {

cConsole::cConsole(cpiLua *lua):
	mLua(lua),
	mCmdLuaScriptGet(0, "!lualist", "", &mcfLuaScriptGet),
	mCmdLuaScriptAdd(1, "!luaload ", "(.*)", &mcfLuaScriptAdd),
	mCmdLuaScriptDel(2, "!luaunload ", "(.*)", &mcfLuaScriptDel),
	mCmdLuaScriptRe(3, "!luareload ", "(.*)", &mcfLuaScriptRe),
	mCmdLuaScriptLog(4, "!lualog ?", "(\\d+)?", &mcfLuaScriptLog),
	mCmdLuaScriptErr(5, "!luaerr ?", "(\\d+)?", &mcfLuaScriptErr),
	mCmdLuaScriptInfo(6, "!luainfo", "", &mcfLuaScriptInfo),
	mCmdLuaScriptVersion(7, "!luaversion", "", &mcfLuaScriptVersion),
	mCmdLuaScriptFiles(8, "!luafiles", "", &mcfLuaScriptFiles),
	mCmdr(this)
{
	mCmdr.Add(&mCmdLuaScriptAdd);
	mCmdr.Add(&mCmdLuaScriptDel);
	mCmdr.Add(&mCmdLuaScriptGet);
	mCmdr.Add(&mCmdLuaScriptRe);
	mCmdr.Add(&mCmdLuaScriptInfo);
	mCmdr.Add(&mCmdLuaScriptLog);
	mCmdr.Add(&mCmdLuaScriptErr);
	mCmdr.Add(&mCmdLuaScriptVersion);
	mCmdr.Add(&mCmdLuaScriptFiles);
}

cConsole::~cConsole()
{}

int cConsole::DoCommand(const string &str, cConnDC *conn)
{
	nCmdr::cCommand *cmd = mCmdr.FindCommand(str);

	if (cmd) {
		int id = cmd->GetID();

		if ((id >= 0) && (id <= 8) && conn && conn->mpUser && this->mLua && this->mLua->mServer && (conn->mpUser->mClass < this->mLua->mServer->mC.plugin_mod_class)) { // todo: use command list size instead of constant number
			this->mLua->mServer->DCPublicHS(_("You have no rights to do this."), conn);
			return 1;
		}
	}

	ostringstream os;

	if (conn && (mCmdr.ParseAll(str, os, conn) >= 0)) {
		if (this->mLua && this->mLua->mServer)
			this->mLua->mServer->DCPublicHS(os.str().c_str(), conn);

		return 1;
	}

	return 0;
}

bool cConsole::cfVersionLuaScript::operator()()
{
	(*mOS) << "\r\n\r\n [*] " << autosprintf(_("Lua version: %s [%s]"), LUA_VERSION, LUA_RELEASE) << "\r\n";
	(*mOS) << " [*] " << autosprintf(_("Copyright: %s"), LUA_COPYRIGHT) << "\r\n";
	(*mOS) << " [*] " << autosprintf(_("Authors: %s"), LUA_AUTHORS) << "\r\n";
	return true;
}

bool cConsole::cfInfoLuaScript::operator()()
{
	unsigned __int64 size = 0;

	for (unsigned int i = 0; i < GetPI()->Size(); i++)
		size += lua_gc(GetPI()->mLua[i]->mL, LUA_GCCOUNT, 0);

	(*mOS) << "\r\n\r\n [*] " << autosprintf(_("Hub version: %s"), HUB_VERSION_VERS) << "\r\n";
	(*mOS) << " [*] " << autosprintf(_("Loaded scripts: %d"), GetPI()->Size()) << "\r\n";
	(*mOS) << " [*] " << autosprintf(_("Memory used: %s"), convertByte(size * 1024).c_str()) << "\r\n";
	return true;
}

bool cConsole::cfGetLuaScript::operator()()
{
	(*mOS) << _("Loaded Lua scripts") << ":\r\n\r\n";
	(*mOS) << "\t" << _("ID");
	(*mOS) << "\t" << _("Script");
	(*mOS) << "\t\t\t\t\t" << _("Memory") << "\r\n";
	(*mOS) << "\t" << string(85, '-') << "\r\n\r\n";

	for (unsigned int i = 0; i < GetPI()->Size(); i++) {
		(*mOS) << "\t" << i;
		(*mOS) << "\t" << GetPI()->mLua[i]->mScriptName;
		(*mOS) << "\t";

		if (GetPI()->mLua[i]->mScriptName.size() <= 8)
			(*mOS) << "\t";

		if (GetPI()->mLua[i]->mScriptName.size() <= 16)
			(*mOS) << "\t";

		if (GetPI()->mLua[i]->mScriptName.size() <= 24)
			(*mOS) << "\t";

		if (GetPI()->mLua[i]->mScriptName.size() <= 32)
			(*mOS) << "\t";

		/*
		if (GetPI()->mLua[i]->mScriptName.size() <= 40)
			(*mOS) << "\t";
		*/

		(*mOS) << convertByte(lua_gc(GetPI()->mLua[i]->mL, LUA_GCCOUNT, 0) * 1024) << "\r\n";
	}

	return true;
}

bool cConsole::cfFilesLuaScript::operator()()
{
	DIR *dir = opendir(GetPI()->mScriptDir.c_str());

	if (!dir) {
		(*mOS) << autosprintf(_("Failed loading directory: %s"), GetPI()->mScriptDir.c_str());
		return false;
	}

	(*mOS) << autosprintf(_("Lua scripts found in: %s"), GetPI()->mScriptDir.c_str()) << "\r\n\r\n";
	(*mOS) << "\t" << _("ID");
	(*mOS) << "\t" << _("Script") << "\r\n";
	(*mOS) << "\t" << string(40, '-') << "\r\n\r\n";
	string filename;
	struct dirent *dent = NULL;
	vector<string> filenames;

	while (NULL != (dent = readdir(dir))) {
		filename = dent->d_name;

		if ((filename.size() > 4) && (StrCompare(filename, filename.size() - 4, 4, ".lua") == 0))
			filenames.push_back(filename);
	}

	closedir(dir);
	sort(filenames.begin(), filenames.end());

	for (size_t i = 0; i < filenames.size(); i++)
		(*mOS) << "\t" << i << "\t" << filenames[i] << "\r\n";

	return true;
}

bool cConsole::cfDelLuaScript::operator()()
{
	string scriptfile;
	GetParStr(1, scriptfile);
	bool number = false;
	int num = 0;

	if (IsNumber(scriptfile.c_str())) {
		num = atoi(scriptfile.c_str());
		number = true;
	} else if (scriptfile.find_first_of('/') == string::npos) {
		scriptfile = GetPI()->mScriptDir + scriptfile;
	}

	vector<cLuaInterpreter*>::iterator it;
	cLuaInterpreter *li;
	int i = 0;

	for (it = GetPI()->mLua.begin(); it != GetPI()->mLua.end(); ++it, ++i) {
		li = *it;

		if (li && ((number && (num == i)) || (!number && (StrCompare(li->mScriptName, 0, li->mScriptName.size(), scriptfile) == 0)))) {
			const char *args[] = { NULL };
			li->CallFunction("UnLoad", args);
			scriptfile = li->mScriptName;
			(*mOS) << autosprintf(_("Script stopped: %s"), li->mScriptName.c_str());
			GetPI()->mLua.erase(it);
			delete li;
			li = NULL;
			return true;
		}
	}

	if (number)
		(*mOS) << autosprintf(_("Script not stopped because it's not loaded: #%d"), num);
	else
		(*mOS) << autosprintf(_("Script not stopped because it's not loaded: %s"), scriptfile.c_str());

	return false;
}

bool cConsole::cfAddLuaScript::operator()()
{
	string scriptfile;
	GetParStr(1, scriptfile);

	if (IsNumber(scriptfile.c_str())) {
		int num = atoi(scriptfile.c_str());
		DIR *dir = opendir(GetPI()->mScriptDir.c_str());

		if (!dir) {
			(*mOS) << autosprintf(_("Failed loading directory: %s"), GetPI()->mScriptDir.c_str());
			return false;
		}

		string filename;
		struct dirent *dent = NULL;
		vector<string> filenames;

		while (NULL != (dent = readdir(dir))) {
			filename = dent->d_name;

			if ((filename.size() > 4) && (StrCompare(filename, filename.size() - 4, 4, ".lua") == 0))
				filenames.push_back(filename);
		}

		closedir(dir);
		sort(filenames.begin(), filenames.end());

		if ((num < 0) || (unsigned(num) >= filenames.size())) {
			(*mOS) << autosprintf(_("Script number %d is out of range. Get the right number using !luafiles command or specify the script path instead."), num);
			return false;
		}

		scriptfile = GetPI()->mScriptDir + filenames[num];
	} else if (scriptfile.find_first_of('/') == string::npos) {
		scriptfile = GetPI()->mScriptDir + scriptfile;
	}

	vector<cLuaInterpreter*>::iterator it;
	cLuaInterpreter *li;

	for (it = GetPI()->mLua.begin(); it != GetPI()->mLua.end(); ++it) {
		li = *it;

		if (li && (StrCompare(li->mScriptName, 0, li->mScriptName.size(), scriptfile) == 0)) {
			(*mOS) << autosprintf(_("Script is already loaded: %s"), scriptfile.c_str());
			return false;
		}
	}

	string config("config");

	if (GetPI()->server)
		config = GetPI()->server->mDBConf.config_name;

	cLuaInterpreter *ip = NULL;

	try {
		ip = new cLuaInterpreter(config, scriptfile);
	} catch (const char *ex) {
		(*mOS) << autosprintf(_("Failed to allocate new Lua interpreter for script: %s [ %s ]"), scriptfile.c_str(), ex);
		return false;
	}

	if (ip->Init()) {
		(*mOS) << autosprintf(_("Script is now loaded: %s"), scriptfile.c_str());
		GetPI()->AddData(ip);
		ip->Load();
		return true;
	} else {
		(*mOS) << autosprintf(_("Script not found or couldn't be parsed: %s"), scriptfile.c_str());
		delete ip;
		ip = NULL;
		return false;
	}
}

bool cConsole::cfReloadLuaScript::operator()()
{
	string scriptfile;
	GetParStr(1, scriptfile);
	bool number = false;
	int num = 0;

	if (IsNumber(scriptfile.c_str())) {
		num = atoi(scriptfile.c_str());
		number = true;
	} else if (scriptfile.find_first_of('/') == string::npos) {
		scriptfile = GetPI()->mScriptDir + scriptfile;
	}

	vector<cLuaInterpreter*>::iterator it;
	cLuaInterpreter *li;
	bool found = false;
	int i = 0;

	for (it = GetPI()->mLua.begin(); it != GetPI()->mLua.end(); ++it, ++i) {
		li = *it;

		if (li && ((number && (num == i)) || (!number && (StrCompare(li->mScriptName, 0, li->mScriptName.size(), scriptfile) == 0)))) {
			found = true;
			const char *args[] = { NULL };
			li->CallFunction("UnLoad", args);
			(*mOS) << autosprintf(_("Script stopped: %s"), li->mScriptName.c_str());
			scriptfile = li->mScriptName;
			GetPI()->mLua.erase(it);
			delete li;
			li = NULL;
			break;
		}
	}

	if (!found) {
		if (number)
			(*mOS) << autosprintf(_("Script not stopped because it's not loaded: #%d"), num);
		else
			(*mOS) << autosprintf(_("Script not stopped because it's not loaded: %s"), scriptfile.c_str());
	}

	string config("config");

	if (GetPI()->server)
		config = GetPI()->server->mDBConf.config_name;

	cLuaInterpreter *ip = NULL;

	try {
		ip = new cLuaInterpreter(config, scriptfile);
	} catch (const char *ex) {
		(*mOS) << autosprintf(_("Failed to allocate new Lua interpreter for script: %s [ %s ]"), scriptfile.c_str(), ex);
		return false;
	}

	if (ip->Init()) {
		(*mOS) << ' ' << autosprintf(_("Script is now loaded: %s"), scriptfile.c_str());
		GetPI()->AddData(ip);
		ip->Load();
		return true;
	} else {
		(*mOS) << ' ' << autosprintf(_("Script not found or couldn't be parsed: %s"), scriptfile.c_str());
		delete ip;
		ip = NULL;
		return false;
	}
}

bool cConsole::cfLogLuaScript::operator()()
{
	int level;

	if (GetParInt(1, level)) {
		stringstream ss;
		ss << GetPI()->log_level;
		string oldValue = ss.str();
		ss.str("");
		ss << level;
		string newValue = ss.str();
		GetPI()->SetLogLevel(level);
		(*mOS) << autosprintf(_("Updated configuration %s.%s from '%s' to '%s'."), "pi_lua", "log_level", oldValue.c_str(), newValue.c_str());
	} else {
		(*mOS) << autosprintf(_("Current log level: %d"), GetPI()->log_level);
	}

	return true;
}

bool cConsole::cfErrLuaScript::operator()()
{
	int eclass;

	if (GetParInt(1, eclass)) {
		stringstream ss;
		ss << GetPI()->err_class;
		string oldValue = ss.str();
		ss.str("");
		ss << eclass;
		string newValue = ss.str();
		GetPI()->SetErrClass(eclass);
		(*mOS) << autosprintf(_("Updated configuration %s.%s: %s -> %s"), "pi_lua", "err_class", oldValue.c_str(), newValue.c_str());
	} else {
		(*mOS) << autosprintf(_("Current error class: %d"), GetPI()->err_class);
	}

	return true;
}

	}; // namespace nLuaPlugin
}; // namespace nVerliHub
