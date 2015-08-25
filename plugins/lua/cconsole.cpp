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

extern "C"
{
#include <lua.h>
}

#include "src/cconndc.h"
#include "src/cserverdc.h"
#include "cconsole.h"
#include "cpilua.h"
#include "cluainterpreter.h"
#include "curr_date_time.h"
#include "src/stringutils.h"
#include "src/i18n.h"
#include <dirent.h>

#define PADDING 25

namespace nVerliHub {
	using namespace nUtils;
	namespace nLuaPlugin {

cConsole::cConsole(cpiLua *lua):
	mLua(lua),
	mCmdLuaScriptGet(0, "!lualist", "", &mcfLuaScriptGet),
	mCmdLuaScriptAdd(1, "!luaload ", "(.*)", &mcfLuaScriptAdd),
	mCmdLuaScriptDel(2, "!luaunload ", "(.*)", &mcfLuaScriptDel),
	mCmdLuaScriptRe(3, "!luareload ", "(.*)", &mcfLuaScriptRe),
	mCmdLuaScriptLog(4, "!lualog", "(.*)", &mcfLuaScriptLog), //(\\d+)
	mCmdLuaScriptVersion(5, "!luaversion", "", &mcfLuaScriptVersion),
	mCmdLuaScriptInfo(6, "!luainfo", "", &mcfLuaScriptInfo),
	mCmdLuaScriptFiles(7, "!luafiles", "", &mcfLuaScriptFiles),
	mCmdr(this)
{
	mCmdr.Add(&mCmdLuaScriptAdd);
	mCmdr.Add(&mCmdLuaScriptDel);
	mCmdr.Add(&mCmdLuaScriptGet);
	mCmdr.Add(&mCmdLuaScriptRe);
	mCmdr.Add(&mCmdLuaScriptInfo);
	mCmdr.Add(&mCmdLuaScriptLog);
	mCmdr.Add(&mCmdLuaScriptVersion);
	mCmdr.Add(&mCmdLuaScriptFiles);
}

cConsole::~cConsole()
{
}

int cConsole::DoCommand(const string &str, cConnDC *conn)
{
	nCmdr::cCommand *cmd = mCmdr.FindCommand(str);

	if (cmd != NULL) {
		int id = cmd->GetID();

		if ((id >= 0) && (id <= 7) && conn && conn->mpUser && (conn->mpUser->mClass < mLua->mServer->mC.plugin_mod_class)) { // todo: use command list size instead of constant number
			mLua->mServer->DCPublicHS(_("You have no rights to do this."), conn);
			return 1;
		}
	}

	ostringstream os;

	if (mCmdr.ParseAll(str, os, conn) >= 0)	{
		mLua->mServer->DCPublicHS(os.str().c_str(), conn);
		return 1;
	}

	return 0;
}

bool cConsole::cfVersionLuaScript::operator()()
{
	(*mOS) << "\r\n [*] " << setw(PADDING) << setiosflags(ios::left) << _("Lua version") << LUA_VERSION << "\r\n";
	(*mOS) << " [*] " << setw(PADDING) << setiosflags(ios::left) << _("Lua library version") << LUA_RELEASE << "\r\n";
	(*mOS) << " [*] " << setw(PADDING) << setiosflags(ios::left) << _("Copyright") << LUA_COPYRIGHT << "\r\n";
	(*mOS) << " [*] " << setw(PADDING) << setiosflags(ios::left) << _("Authors") << LUA_AUTHORS;
	return true;
}

bool cConsole::cfInfoLuaScript::operator()()
{
	int size = 0;

	if (GetPI()->Size() > 0)
		size = lua_gc(GetPI()->mLua[0]->mL, LUA_GCCOUNT, 0);

	(*mOS) << "\r\n [*] " << setw(PADDING) << setiosflags(ios::left) << _("Version date") << __CURR_DATE_TIME__ << "\r\n";
	(*mOS) << " [*] " << setw(PADDING) << setiosflags(ios::left) << _("Loaded scripts") << GetPI()->Size() << "\r\n";
	(*mOS) << " [*] " << setw(PADDING) << setiosflags(ios::left) << _("Memory used") << convertByte(size * 1024, false).c_str();
	return true;
}

bool cConsole::cfGetLuaScript::operator()()
{
	(*mOS) << _("Loaded Lua scripts") << ":\r\n\r\n ";
	(*mOS) << setw(6) << setiosflags(ios::left) << _("ID");
	(*mOS) << toUpper(_("Script")) << "\r\n";
	(*mOS) << " " << string(6 + 20, '=') << "\r\n";

	for (int i = 0; i < GetPI()->Size(); i++) {
		(*mOS) << " " << setw(6) << setiosflags(ios::left) << i << GetPI()->mLua[i]->mScriptName << "\r\n";
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

	(*mOS) << autosprintf(_("Lua scripts found in: %s"), GetPI()->mScriptDir.c_str()) << "\r\n\r\n ";
	(*mOS) << setw(6) << setiosflags(ios::left) << _("ID");
	(*mOS) << toUpper(_("Script")) << "\r\n";
	(*mOS) << " " << string(6 + 20, '=') << "\r\n";
	string filename;
	struct dirent *dent = NULL;
	int i = 0;

	while (NULL != (dent = readdir(dir))) {
		filename = dent->d_name;

		if ((filename.size() > 4) && (StrCompare(filename, filename.size() - 4, 4, ".lua") == 0)) {
			(*mOS) << " " << setw(6) << setiosflags(ios::left) << i << filename << "\r\n";
			i++;
		}
	}

	closedir(dir);
	return true;
}

bool cConsole::cfDelLuaScript::operator()()
{
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

	vector<cLuaInterpreter *>::iterator it;
	cLuaInterpreter *li;
	int i = 0;
	bool found = false;

	for (it = GetPI()->mLua.begin(); it != GetPI()->mLua.end(); ++it, ++i) {
		li = *it;

		if ((number && num == i) || (!number && StrCompare(li->mScriptName, 0, li->mScriptName.size(), scriptfile) == 0)) {
			found = true;
			scriptfile = li->mScriptName;
			(*mOS) << autosprintf(_("Script %s stopped."), li->mScriptName.c_str());
			delete li;
			GetPI()->mLua.erase(it);
			break;
		}
	}

	if (!found) {
		if (number)
			(*mOS) << autosprintf(_("Script #%s not stopped because it's not loaded."), scriptfile.c_str());
		else
			(*mOS) << autosprintf(_("Script %s not stopped because it's not loaded."), scriptfile.c_str());

		return false;
	}

	return true;
}

bool cConsole::cfAddLuaScript::operator()()
{
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

	if (number) {
		DIR *dir = opendir(GetPI()->mScriptDir.c_str());

		if (!dir) {
			(*mOS) << autosprintf(_("Failed loading directory: %s"), GetPI()->mScriptDir.c_str());
			return false;
		}

		string filename;
		struct dirent *dent = NULL;
		int i = 0;

		while (NULL != (dent = readdir(dir))) {
			filename = dent->d_name;

			if ((filename.size() > 4) && (StrCompare(filename, filename.size() - 4, 4, ".lua") == 0)) {
				if (i == num) {
					scriptfile = GetPI()->mScriptDir + filename;
					break;
				}

				i++;
			}
		}

		closedir(dir);
	}

	cLuaInterpreter *ip = new cLuaInterpreter(scriptfile);

	if (!ip) {
		(*mOS) << _("Failed to allocate new Lua interpreter.");
		return false;
	}

	vector<cLuaInterpreter *>::iterator it;
	cLuaInterpreter *li;

	if (ip->Init()) {
		for (it = GetPI()->mLua.begin(); it != GetPI()->mLua.end(); ++it) {
			li = *it;

			if (StrCompare(li->mScriptName, 0, li->mScriptName.size(), scriptfile) == 0) {
				(*mOS) << autosprintf(_("Script %s is already loaded."), scriptfile.c_str());
				delete ip;
				return false;
			}
		}

		(*mOS) << autosprintf(_("Script %s is now loaded."), scriptfile.c_str());
		GetPI()->AddData(ip);
		ip->Load();
	} else {
		(*mOS) << autosprintf(_("Script %s not found or couldn't be parsed."), scriptfile.c_str());
		delete ip;
		return false;
	}

	return true;
}

bool cConsole::cfReloadLuaScript::operator()()
{
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

	vector<cLuaInterpreter *>::iterator it;
	cLuaInterpreter *li;
	bool found = false;
	int i = 0;

	for (it = GetPI()->mLua.begin(); it != GetPI()->mLua.end(); ++it, ++i) {
		li = *it;

		if ((number && num == i) || (!number && StrCompare(li->mScriptName, 0, li->mScriptName.size(), scriptfile) == 0)) {
			found = true;
			(*mOS) << autosprintf(_("Script %s stopped."), li->mScriptName.c_str());
			scriptfile = li->mScriptName;
			delete li;
			GetPI()->mLua.erase(it);
			break;
		}
	}

	if (!found) {
		if (number)
			(*mOS) << autosprintf(_("Script #%s not stopped because it's not loaded."), scriptfile.c_str());
		else
			(*mOS) << autosprintf(_("Script %s not stopped because it's not loaded."), scriptfile.c_str());

		return false;
	} else {
		cLuaInterpreter *ip = new cLuaInterpreter(scriptfile);

		if (!ip) {
			(*mOS) << " " << _("Failed to allocate new Lua interpreter.");
			return false;
		}

		if (ip->Init()) {
			(*mOS) << " " << autosprintf(_("Script %s is now loaded."), scriptfile.c_str());
			GetPI()->AddData(ip);
			ip->Load();
			return true;
		} else {
			(*mOS) << " " << autosprintf(_("Script %s not found or couldn't be parsed."), scriptfile.c_str());
			delete ip;
			return false;
		}
	}
}

bool cConsole::cfLogLuaScript::operator()()
{
	int level;
	ostringstream msg;

	if (GetParInt(1, level)) {
		stringstream ss;
		ss << cpiLua::log_level;
		string oldValue = ss.str();
		ss.str("");
		ss << level;
		string newValue = ss.str();
		(*mOS) << autosprintf(_("Updated %s.%s from '%s' to '%s'."), "pi_lua", "log_level", oldValue.c_str(), newValue.c_str());
		cpiLua::me->SetLogLevel(level);
	} else {
		msg << autosprintf(_("Current log level: %d"), cpiLua::log_level);
		(*mOS) << msg.str();
	}

	return true;
}

	}; // namespace nLuaPlugin
}; // namespace nVerliHub
