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

#include "stdafx.h"
#ifdef _WIN32
#include <windows.h>
#include <tchar.h>
#define BUFSIZE MAX_PATH
#include <local.h>
#endif
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "cserverdc.h"
#include <iostream>
#include <stdlib.h>
#include <sstream>
#include <signal.h>
#include <dirent.h>
#include "script_api.h"
#include "i18n.h"
#include "dirsettings.h"

using namespace std;
using namespace nVerliHub;
using namespace nVerliHub::nSocket;

#if ! defined _WIN32
void mySigPipeHandler(int i)
{
	signal(SIGPIPE,mySigPipeHandler);
	cout << "Received SIGPIPE, ignoring it, " << i << endl;
}

void mySigIOHandler(int i)
{
	signal(SIGIO  ,mySigIOHandler  );
	cout << endl << "Received SIGIO, ignoring it, " << i << endl;
}

void mySigQuitHandler(int i)
{
	cout << "Received a " << i << " signal, quiting";
	exit(0);
}

void mySigServHandler(int i)
{
	cerr << "Received a " << i << " signal, doing stacktrace and quiting" << endl;
	cServerDC *serv = (cServerDC*)cServerDC::sCurrentServer;

	if (serv)
		serv->DoStackTrace();

	exit(0);
}

void mySigHupHandler(int i)
{
	signal(SIGPIPE,mySigHupHandler);
	cout << "Received a " << i << " signal";
	cServerDC *Hub = (cServerDC *)cServerDC::sCurrentServer;
	if (Hub) Hub->mC.Load();
}

#endif

bool DirExists(const char *dirname)
{
	DIR *dir ;
	dir = opendir(dirname);
	if( dir == NULL)
	{
		return false;
	}
	else
	{
		closedir(dir);
		return true;
	}
}

int main(int argc, char *argv[])
{
	int result = 0;
	string ConfigBase;

	#ifdef _WIN32
		TCHAR Buffer[BUFSIZE];

		if (!GetCurrentDirectory(BUFSIZE, Buffer)) {
			cout << "Unable to get current directory because: " << GetLastError() << endl;
			return 1;
		}

		ConfigBase = Buffer;
	#else
		const char *DirName = NULL;
		char *HomeDir = getenv("HOME");
		string tmp;

		if (HomeDir) {
			tmp = HomeDir;
			tmp += "/.verlihub";
			DirName = tmp.c_str();

			if (DirExists(DirName))
				ConfigBase = DirName;
		}

		DirName = "./.verlihub";

		if (DirExists(DirName))
			ConfigBase = DirName;

		DirName = getenv("VERLIHUB_CFG");

		if ((DirName != NULL) && DirExists(DirName))
			ConfigBase = DirName;

		if (!ConfigBase.size())
			ConfigBase = "/etc/verlihub";
	#endif

	int port = 0;

	vector<string> args(argv+1, argv+argc);
	for (int i = 0; i < args.size(); i++)
	{
	#ifdef ENABLE_SYSLOG
		if (args[i] == "-S" || args[i] == "--syslog")
		{
			cObj::msUseSyslog = 1;
		}
		else
	#endif
		{
			stringstream arg(args[i]);
			arg >> port;
		}
	}

	cout << "Configuration directory: " << ConfigBase << endl;
	cServerDC server(ConfigBase, argv[0]); // create server

	if (!server.mDBConf.locale.empty()) { // set locale when defined
		cout << "Found locale configuration: " << server.mDBConf.locale << endl;
		cout << "Setting environment variable LANG: " << ((setenv("LANG", server.mDBConf.locale.c_str(), 1) == 0) ? "OK" : "Error") << endl;
		cout << "Unsetting environment variable LANGUAGE: " << ((unsetenv("LANGUAGE") == 0) ? "OK" : "Error") << endl;
		char *res = setlocale(LC_ALL, server.mDBConf.locale.c_str());
		cout << "Setting hub locale: " << ((res) ? res : "Error") << endl;
		res = bindtextdomain("verlihub", LOCALEDIR);
		cout << "Setting locale message directory: " << ((res) ? res : "Error") << endl;
		res = textdomain("verlihub");
		cout << "Setting locale message domain: " << ((res) ? res : "Error") << endl;
	}

	#ifndef _WIN32
		signal(SIGPIPE, mySigPipeHandler);
		signal(SIGIO, mySigIOHandler);
		signal(SIGQUIT, mySigQuitHandler);
		signal(SIGSEGV, mySigServHandler);
	#endif

	server.StartListening(port);
	result = server.run(); // run the main loop until it stops itself
	return result;
}
