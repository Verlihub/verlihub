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

#include "stdafx.h"

/*
#ifdef _WIN32
#include <windows.h>
#include <tchar.h>
#define BUFSIZE MAX_PATH
#include <local.h>
#endif
*/

#ifdef HAVE_CONFIG_H
	#include <config.h>
#endif

#include "cserverdc.h"
#include "csocketapi.h"
#include <iostream>
#include <memory>

#ifdef _WIN32
	#include <windows.h>
#endif
#include <stdlib.h>
#include <sstream>
#include <signal.h>
#include <dirent.h>
#include "script_api.h"
#include "i18n.h"
#include "dirsettings.h"
#include <getopt.h>

using namespace std;
using namespace nVerliHub;
using namespace nVerliHub::nSocket;

cObj mainLogger("main");
#define MAIN_LOG_NOTICE if (mainLogger.Log(0)) mainLogger.LogStream()
#define MAIN_LOG_ERROR if (mainLogger.ErrLog(0)) mainLogger.LogStream()

#ifndef _WIN32 // note: windows has no sigpipe, sigio, sigquit and sighup
void mySigPipeHandler(int i)
{
	signal(SIGPIPE, mySigPipeHandler);
	MAIN_LOG_NOTICE << "Received SIGPIPE, ignoring it, " << i << endl;
}

void mySigIOHandler(int i)
{
	signal(SIGIO, mySigIOHandler);
	MAIN_LOG_NOTICE << endl << "Received SIGIO, ignoring it, " << i << endl;
}
#endif

void mySigQuitHandler(int i)
{
	MAIN_LOG_NOTICE << "Received a " << i << " signal, quiting" << endl;
	cServerDC *serv = (cServerDC*)cServerDC::sCurrentServer;

	if (serv)
		serv->SyncStop();

	exit(EXIT_SUCCESS);
}

void mySigServHandler(int i)
{
	MAIN_LOG_ERROR << "Received a " << i << " signal, doing stacktrace and quiting" << endl;
	cServerDC *serv = (cServerDC*)cServerDC::sCurrentServer;

	if (serv) { // note: this is a crash, i dont think we can actually perform a proper stop()
		serv->DoStackTrace();
		serv->OnUnLoad(9); // try to unload as much as possible
	}

	exit(128 + i); // proper exit code for this signal
}

#ifndef _WIN32
void mySigHupHandler(int i)
{
	MAIN_LOG_NOTICE << "Received a " << i << " signal, reloading" << endl;
	cServerDC *serv = (cServerDC*)cServerDC::sCurrentServer;

	if (serv)
		serv->SyncReload();
}
#else
static HANDLE sStopDone = NULL; // note: set when main is done, console close waits for it

BOOL WINAPI myConsoleHandler(DWORD type) // note: windows runs it on its own thread, so it only asks main loop to stop
{
	cServerDC *serv = (cServerDC*)cServerDC::sCurrentServer;

	if (!serv || !serv->RequestStop()) // note: hub is not running or stop was already requested, so second ctrl+c ends process at once like before
		return FALSE;

	if ((type == CTRL_CLOSE_EVENT) || (type == CTRL_LOGOFF_EVENT) || (type == CTRL_SHUTDOWN_EVENT)) { // note: windows ends process when this returns, so wait for clean stop, it waits only few seconds
		if (sStopDone)
			WaitForSingleObject(sStopDone, INFINITE);
	}

	return TRUE;
}

LONG WINAPI myCrashHandler(EXCEPTION_POINTERS *info) // note: crash in any thread, replaces handler of mingw runtime that calls sigsegv handler
{
	const DWORD code = ((info && info->ExceptionRecord) ? info->ExceptionRecord->ExceptionCode : 0);
	MAIN_LOG_ERROR << "Received exception 0x" << std::hex << code << std::dec << ", doing stacktrace and quiting" << endl;
	cServerDC *serv = (cServerDC*)cServerDC::sCurrentServer;

	if (serv) { // note: this is a crash, same as signal handler
		serv->DoStackTrace();
		serv->OnUnLoad(9); // try to unload as much as possible
	}

	return EXCEPTION_EXECUTE_HANDLER; // note: ends process with exception code
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
	unsigned int port = 0;
	int verbosity = 0;

	const char* short_options = "Ss:d:v";

	const struct option long_options[] = {
		{"syslog", no_argument, 0, 'S'},
		{"syslog-suffix", required_argument, 0, 's'},
		{"config-dir", required_argument, 0, 'd'},
		{"verbose", no_argument, 0, 'v'},
		{NULL, 0, NULL, 0}
	};

	int long_index = -1;
	int opt = -1;
	char *OptDirName = NULL;

	while ((opt = getopt_long(argc, argv, short_options, long_options, &long_index)) != -1 ) {
		switch (opt) {
			case 'S':
#ifdef ENABLE_SYSLOG
				cObj::msUseSyslog = 1;
#else
				MAIN_LOG_NOTICE << "Logging to syslog is disabled at compile time" << endl;
#endif
				break;

			case 's':
#ifdef ENABLE_SYSLOG
				cObj::msSyslogIdent += '-';
				cObj::msSyslogIdent += optarg;
#else
				MAIN_LOG_NOTICE << "Logging to syslog is disabled at compile time" << endl;
#endif
				break;

			case 'd':
				OptDirName = optarg;
				break;

			case 'v':
				verbosity++;
				break;

			default:
				/* unreachable */
				break;
		}
	}

	if (optind < argc) {
		stringstream arg(argv[optind]);
		arg >> port;

		if (port == 2) // install workaround
			port = 4111;
	}

	/*
	#ifdef _WIN32
		TCHAR Buffer[BUFSIZE];

		if (!GetCurrentDirectory(BUFSIZE, Buffer)) {
			MAIN_LOG_ERROR << "Unable to get current directory because: " << GetLastError() << endl;
			return 1;
		}

		ConfigBase = Buffer;
	#else
	*/
		const char *DirName = NULL;
		char *HomeDir = getenv("HOME");
		string tmp;

		if (HomeDir) {
			tmp = HomeDir;
			tmp += "/.config/verlihub";

			if (DirExists(tmp.c_str())) {
				ConfigBase = tmp;
			} else {
				tmp = HomeDir;
				tmp += "/.verlihub";

				if (DirExists(tmp.c_str()))
					ConfigBase = tmp;
			}
		}

		DirName = "./.verlihub";

		if (DirExists(DirName))
			ConfigBase = DirName;

		DirName = getenv("VERLIHUB_CFG");

		if (OptDirName && DirExists(OptDirName))
			ConfigBase = OptDirName;
		else if (DirName && DirExists(DirName))
			ConfigBase = DirName;

		if (!ConfigBase.size())
			ConfigBase = "/etc/verlihub";
	//#endif

	MAIN_LOG_NOTICE << "Configuration directory: " << ConfigBase << endl;

	if (!SockStartup()) { // note: windows needs it before any socket is created
		MAIN_LOG_ERROR << "Failed to start socket library" << endl;
		return 1;
	}

	struct sSockCleanup {
		~sSockCleanup()
		{
			SockCleanup();
		}
	} sock_cleanup; // note: declared before server, so it runs after server is destroyed

#ifdef _WIN32
	sStopDone = CreateEventA(NULL, TRUE, FALSE, NULL);

	struct sStopSignal {
		~sStopSignal()
		{
			if (sStopDone)
				SetEvent(sStopDone);
		}
	} stop_signal; // note: declared before server, so console close handler continues after server is destroyed
#endif

	std::unique_ptr<cServerDC> server_ptr;

	try { // note: constructor throws when database is not available, exit with error instead of terminate
		server_ptr.reset(new cServerDC(ConfigBase, argv[0])); // create server
	} catch (const char *ex) {
		MAIN_LOG_ERROR << ex << endl;
		return 1;
	}

	//try { // todo: whole process is running inside try?
		cServerDC &server = *server_ptr;
		cObj::msLogLevel += verbosity;

		#ifndef _WIN32
			signal(SIGPIPE, mySigPipeHandler);
			signal(SIGIO, mySigIOHandler);
			signal(SIGQUIT, mySigQuitHandler);
			signal(SIGSEGV, mySigServHandler);
			signal(SIGHUP, mySigHupHandler);
		#else
			SetConsoleCtrlHandler(myConsoleHandler, TRUE); // note: ctrl+c, ctrl+break and closing console window stop hub cleanly
			SetUnhandledExceptionFilter(myCrashHandler); // note: crash in any thread, not only main thread
		#endif

		if (!server.StartListening(port))
			return 0;

		result = server.run(); // run the main loop until it stops itself
		return result;
	/*
	} catch (const char *exception) {
		MAIN_LOG_ERROR << exception << endl;
		return 3;
	}
	*/
}
