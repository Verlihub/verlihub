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

#include "src/cconndc.h"
#include "src/cserverdc.h"
#include "src/stringutils.h"
#include "src/i18n.h"
#include "cconsole.h"
#include "cpiperl.h"
#include "cperlinterpreter.h"
#include <dirent.h>
#include <cctype>

namespace nVerliHub {
	using namespace nUtils;
	namespace nPerlPlugin {

static bool IsNumber(const char* s)
{
	if (!s || !strlen(s))
		return false;

	for (unsigned int i = 0; i < strlen(s); i++) {
		if (!isdigit(s[i]))
			return false;
	}

	return true;
}

cConsole::cConsole(cpiPerl *perl):
	mPerl(perl),
	mCmdPerlScriptGet(0,"!perllist", "", &mcfPerlScriptGet),
	mCmdPerlScriptAdd(1,"!perlload ", "(.*)", &mcfPerlScriptAdd),
	mCmdPerlScriptDel(2,"!perlunload ", "(.*)", &mcfPerlScriptDel),
	mCmdPerlScriptRe(3,"!perlreload ", "(.*)", &mcfPerlScriptRe),
	mCmdr(this)
{
	mCmdr.Add(&mCmdPerlScriptAdd);
	mCmdr.Add(&mCmdPerlScriptDel);
	mCmdr.Add(&mCmdPerlScriptGet);
	mCmdr.Add(&mCmdPerlScriptRe);
}

cConsole::~cConsole()
{
}

bool cConsole::cfGetPerlScript::operator()()
{
	(*mOS) << _("Loaded Perl scripts") << ":\r\n\r\n";
	(*mOS) << "\t" << _("ID");
	(*mOS) << "\t" << _("Script") << "\r\n";
	(*mOS) << "\t" << string(40, '-') << "\r\n";

	for (unsigned int i = 0; i < GetPI()->Size(); i++)
		(*mOS) << "\t" << i << "\t" << GetPI()->mPerl.mPerl[i]->mScriptName << "\r\n";

	return true;
}

int cConsole::DoCommand(const string &str, cConnDC * conn)
{
	ostringstream os;

	if(mCmdr.ParseAll(str, os, conn) >= 0)
	{
		mPerl->mServer->DCPublicHS(os.str().c_str(),conn);
		return 1;
	}
	return 0;
}

bool cConsole::cfDelPerlScript::operator()()
{
	string scriptfile;
	GetParStr(1,scriptfile);

	bool found = false;
	bool number = false;
	int i = 0, num = 0;
	vector<cPerlInterpreter *>::iterator it;
	cPerlInterpreter *pi;

	if (IsNumber(scriptfile.c_str())) {
		num = atoi(scriptfile.c_str());
		number = true;
	}

	for(it = GetPI()->mPerl.mPerl.begin(); it != GetPI()->mPerl.mPerl.end(); ++it, ++i) {
		pi = *it;
		if ((number && num == i) || (!number && StrCompare(pi->mScriptName,0,pi->mScriptName.size(),scriptfile)==0)) {
			found = true;
			scriptfile = pi->mScriptName;
			(*mOS) << autosprintf(_("Script %s stopped."), pi->mScriptName.c_str()) << " ";
			delete pi;
			GetPI()->mPerl.mPerl.erase(it);
			break;
		}
	}

	if(!found) {
		if(number)
			(*mOS) << autosprintf(_("Script #%s not stopped because it is not running."), scriptfile.c_str()) << " ";
		else
			(*mOS) << autosprintf(_("Script %s not stopped because it is not running."), scriptfile.c_str()) << " ";
		return false;
	}
	return true;
}

bool cConsole::cfAddPerlScript::operator()()
{
	string scriptfile, pathname, filename;
	bool number = false;
	int num = 0;
	GetParStr(1, scriptfile);
	vector<cPerlInterpreter *>::iterator it;
	cPerlInterpreter *pi;

	if (IsNumber(scriptfile.c_str())) {
		num = atoi(scriptfile.c_str());
		number = true;
	}
	cServerDC *server= cServerDC::sCurrentServer;
	pathname = server->mConfigBaseDir;


	if(number) {
		DIR *dir = opendir(pathname.c_str());
		int i = 0;
		if(!dir) {
			(*mOS) << autosprintf(_("Failed loading directory %s."), pathname.c_str()) << " ";
			return false;
		}
		struct dirent *dent = NULL;

		while(NULL != (dent=readdir(dir))) {

			filename = dent->d_name;
			if((filename.size() > 4) && (StrCompare(filename,filename.size()-3,3,".pl")==0)) {
				if(i == num)
					scriptfile = pathname + "/" + filename;
				i++;
			}

		}
		closedir(dir);
	}
	const char *argv[] = { "", scriptfile.c_str(), NULL };
	for(it = GetPI()->mPerl.mPerl.begin(); it != GetPI()->mPerl.mPerl.end(); ++it) {
		pi = *it;
		if (StrCompare(pi->mScriptName,0,pi->mScriptName.size(),scriptfile)==0) {
			(*mOS) << autosprintf(_("Script %s is already running."), scriptfile.c_str()) << " ";
			return false;
		}
	}
	if(GetPI()->mPerl.Parse(2, argv))
		(*mOS) << autosprintf(_("Script not found or couldn't be parsed: %s"), scriptfile.c_str());
	else
		(*mOS) << autosprintf(_("Script %s is now running."), scriptfile.c_str()) << " ";
	return true;
}

bool cConsole::cfReloadPerlScript::operator()()
{
	string scriptfile;
	bool found = false, number = false;
	int i, num;
	i = num = 0;

	GetParStr(1,scriptfile);
	if (IsNumber(scriptfile.c_str())) {
		num = atoi(scriptfile.c_str());
		number = true;
	}

	cPerlInterpreter *pi;
	vector<cPerlInterpreter *>::iterator it;
	for(it = GetPI()->mPerl.mPerl.begin(); it != GetPI()->mPerl.mPerl.end(); ++it, ++i) {
		pi = *it;
		if ((number && num == i) || (!number && StrCompare(pi->mScriptName,0,pi->mScriptName.size(),scriptfile)==0)) {
			found = true;
			(*mOS) << autosprintf(_("Script %s stopped."), pi->mScriptName.c_str()) << " ";
			scriptfile = pi->mScriptName;
			delete pi;
			GetPI()->mPerl.mPerl.erase(it);
			break;
		}
	}

	if(!found) {
		if(number)
			(*mOS) << autosprintf(_("Script #%s not stopped because it is not running."), scriptfile.c_str()) << " ";
		else
			(*mOS) << autosprintf(_("Script %s not stopped  because it is not running."), scriptfile.c_str()) << " ";
		return false;
	} else {
		const char *argv[] = { "", scriptfile.c_str(), NULL };
		GetPI()->mPerl.Parse(2, argv);
		if(GetPI()->mPerl.Parse(2, argv))
			(*mOS) << autosprintf(_("Script not found or couldn't be parsed: %s"), scriptfile.c_str());
		else
			(*mOS) << autosprintf(_("Script %s is now running."), scriptfile.c_str()) << " ";
		return true;
	}
}

	}; // namespace nPerlPlugin
}; // namespace nVerliHub
