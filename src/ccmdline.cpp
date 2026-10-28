/*
	Copyright (C) 2003-2005 Daniel Muller, dan at verliba dot cz
	Copyright (C) 2006-2026 Verlihub Team, info at verlihub dot net

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

	Please see https://www.gnu.org/licenses/ for a copy
	of the GNU General Public License.
*/

#ifdef HAVE_CONFIG_H
	#include <config.h>
#endif

#include "ccmdline.h"
#include "cdbconf.h"
#include "cmysql.h"
#include "cquery.h"
#include "cconfmysql.h"
#include "csetuplist.h"
#include "creguserinfo.h"
#include "csocketapi.h"
#include "cserverdc.h"
#include "chttpconn.h"
#include "stringutils.h"
#include "dirsettings.h"
#include "i18n.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <utility>
#include <iterator>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <time.h>
#include <dirent.h>
#include <signal.h>
#include <sys/stat.h>
#include <zlib.h>

#include <string.h>
#include <errno.h>

#ifdef _WIN32
	#include <direct.h>
	#include <io.h>
	#include <windows.h>
#else
	#include <unistd.h>
	#include <termios.h>
	#include <pwd.h>
	#include <grp.h>
	#include <fcntl.h>
	#include <sys/wait.h>
	#include <sys/resource.h>
#endif

#ifdef USE_TLS
	#include <openssl/opensslv.h>
	#include <openssl/crypto.h>
#endif

namespace nVerliHub {
	using namespace nConfig;
	using namespace nMySQL;
	using namespace nTables;
	using namespace nUtils;
	using namespace nSocket;

// colors, same as in vh script
static const char sBold[] = "\033[1m";
static const char sBlue[] = "\033[34m";
static const char sRed[] = "\033[31m";
static const char sGreen[] = "\033[32m";
static const char sGreenBold[] = "\033[1;32m";
static const char sRedBold[] = "\033[1;31m";
static int sColor[2] = {-1, -1}; // note: not checked yet, for output and error
static bool sLogFile = false; // note: console output goes to install_log, no colors there

#if defined(_WIN32) && !defined(ENABLE_VIRTUAL_TERMINAL_PROCESSING)
	#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif

static bool UseColor(bool err)
{
	if (sLogFile)
		return false;

	int &res = sColor[err ? 1 : 0];

	if (res >= 0)
		return res;

	res = 0;
	const char *no = getenv("NO_COLOR"); // note: common way to disable colors

	if (no && no[0])
		return false;

#ifdef _WIN32
	HANDLE out = GetStdHandle(err ? STD_ERROR_HANDLE : STD_OUTPUT_HANDLE);
	DWORD mode = 0;

	if (GetConsoleMode(out, &mode)) { // note: windows 10 console understands same escape codes when it's enabled
		res = (((mode & ENABLE_VIRTUAL_TERMINAL_PROCESSING) || SetConsoleMode(out, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING)) ? 1 : 0);
	} else if (GetFileType(out) == FILE_TYPE_PIPE) { // note: msys2 and cygwin terminal is a named pipe, not a console
		struct {
			FILE_NAME_INFO mInfo;
			WCHAR mName[MAX_PATH];
		} info;

		if (GetFileInformationByHandleEx(out, FileNameInfo, &info, sizeof(info))) {
			const wstring name(info.mInfo.FileName, info.mInfo.FileNameLength / sizeof(WCHAR));
			res = ((((name.find(L"msys-") != wstring::npos) || (name.find(L"cygwin-") != wstring::npos)) && (name.find(L"-pty") != wstring::npos)) ? 1 : 0);
		}
	}
#else
	const char *term = getenv("TERM");
	res = ((isatty(err ? STDERR_FILENO : STDOUT_FILENO) && (!term || (string(term) != "dumb"))) ? 1 : 0);
#endif

	return res;
}

static string Paint(const string &text, const char *code, bool err = false, const char *back = NULL) // note: back is color of surrounding text that continues after this one
{
	if (!UseColor(err))
		return text;

	string res(code);
	res += text;
	res += "\033[0m";

	if (back)
		res += back;

	return res;
}

static bool DirExists(const string &path)
{
	struct stat st;
	return ((stat(path.c_str(), &st) == 0) && S_ISDIR(st.st_mode));
}

static bool FileExists(const string &path)
{
	struct stat st;
	return ((stat(path.c_str(), &st) == 0) && S_ISREG(st.st_mode));
}

static bool FileWritable(const string &path) // note: file or folder, same as test -w of shell
{
#ifdef _WIN32
	return (_access(path.c_str(), 2) == 0);
#else
	return (access(path.c_str(), W_OK) == 0);
#endif
}

static bool MakeDir(const string &path)
{
	if (DirExists(path))
		return true;

#ifdef _WIN32
	return (_mkdir(path.c_str()) == 0);
#else
	return (mkdir(path.c_str(), 0755) == 0);
#endif
}

static bool IsSeparator(char c)
{
#ifdef _WIN32
	return ((c == '/') || (c == '\\'));
#else
	return (c == '/');
#endif
}

static bool RemoveDir(const string &path) // note: empty folder only
{
#ifdef _WIN32
	return (_rmdir(path.c_str()) == 0);
#else
	return (rmdir(path.c_str()) == 0);
#endif
}

static bool MakeDirs(const string &path) // note: like mkdir -p
{
	if (path.empty())
		return false;

	if (DirExists(path))
		return true;

	size_t pos = path.size();

	while (pos && !IsSeparator(path[pos - 1]))
		pos--;

	if (pos > 1) { // note: not root
		const string parent(path.substr(0, pos - 1));

	#ifdef _WIN32
		const bool drive = ((parent.size() == 2) && (parent[1] == ':'));
	#else
		const bool drive = false;
	#endif

		if (!drive && !MakeDirs(parent))
			return false;
	}

	return MakeDir(path);
}

static string SlashPath(const string &path) // note: windows takes both separators, show same forward slash everywhere, msys2 gives home with backslashes
{
	string res(path);

#ifdef _WIN32
	replace(res.begin(), res.end(), '\\', '/');
#endif

	return res;
}

static string StripSeparators(const string &path) // note: remove trailing separators, but keep root
{
	string res(path);

	while ((res.size() > 1) && IsSeparator(res[res.size() - 1]))
		res.erase(res.size() - 1);

	return res;
}

static bool CopyOneFile(const string &from, const string &to) // note: not named copyfile, windows has such macro
{
	ifstream in(from.c_str(), ios::in | ios::binary);

	if (!in.is_open())
		return false;

	ofstream out(to.c_str(), ios::out | ios::binary | ios::trunc);

	if (!out.is_open())
		return false;

	out << in.rdbuf();
	out.close();
	return !out.fail();
}

struct sLogRedirect { // note: hub and database classes write log to console, during installation it goes to install_log instead, also when something throws
	sLogRedirect(ostream &log):
		mOut(cout.rdbuf(log.rdbuf())),
		mErr(cerr.rdbuf(log.rdbuf()))
	{
		sLogFile = true;
	}

	~sLogRedirect()
	{
		cout.rdbuf(mOut);
		cerr.rdbuf(mErr);
		sLogFile = false;
	}

	streambuf *mOut, *mErr;
};

static void InstallCrashLog(const string &what) // note: same as crash handlers of hub in verlihub.cpp, but log is flushed because process ends here
{
	cObj log("main");

	if (log.ErrLog(0))
		log.LogStream() << "Received " << what << ", doing stacktrace and quiting" << endl;

	cServerDC *serv = cServerDC::sCurrentServer;

	if (serv) {
		serv->DoStackTrace();
		serv->OnUnLoad(9); // try to unload as much as possible
	}

	cout.flush();
	cerr.flush();
}

#ifdef _WIN32
static LONG WINAPI InstallCrash(EXCEPTION_POINTERS *info) // note: crash while installation creates tables
{
	ostringstream what;
	what << "exception 0x" << hex << ((info && info->ExceptionRecord) ? info->ExceptionRecord->ExceptionCode : 0);
	InstallCrashLog(what.str());
	return EXCEPTION_EXECUTE_HANDLER; // note: ends process with exception code
}
#else
static void InstallCrash(int sig) // note: crash while installation creates tables
{
	InstallCrashLog("a " + StringFrom(sig) + " signal");
	exit(128 + sig); // note: proper exit code for this signal
}
#endif

static bool IsHelpFile(const string &name)
{
	return (name.compare(0, 5, "help_") == 0);
}

static bool CopyTree(const string &from, const string &to) // note: copy folder with its files and subfolders
{
	if (!MakeDirs(to)) {
		cerr << Paint(autosprintf(_("Failed to create folder: %s"), to.c_str()), sRed, true) << endl;
		return false;
	}

	DIR *dir = opendir(from.c_str());

	if (!dir) {
		cerr << Paint(autosprintf(_("Failed to open folder: %s"), from.c_str()), sRed, true) << endl;
		return false;
	}

	bool res = true;
	struct dirent *ent;

	while ((ent = readdir(dir)) != NULL) {
		const string name(ent->d_name);

		if ((name == ".") || (name == ".."))
			continue;

		const string path(from + '/' + name);

		if (DirExists(path)) {
			if (!CopyTree(path, to + '/' + name))
				res = false;

		} else if (FileExists(path) && !CopyOneFile(path, to + '/' + name)) {
			cerr << Paint(autosprintf(_("Failed to copy file: %s"), path.c_str()), sRed, true) << endl;
			res = false;
		}
	}

	closedir(dir);
	return res;
}

#ifndef _WIN32
static void ChownTree(const string &path, uid_t uid, gid_t gid) // note: like chown -r, symbolic links are not followed
{
	struct stat st;

	if (lchown(path.c_str(), uid, gid) || (lstat(path.c_str(), &st) != 0) || !S_ISDIR(st.st_mode))
		return;

	DIR *dir = opendir(path.c_str());

	if (!dir)
		return;

	struct dirent *ent;

	while ((ent = readdir(dir)) != NULL) {
		const string name(ent->d_name);

		if ((name != ".") && (name != ".."))
			ChownTree(path + '/' + name, uid, gid);
	}

	closedir(dir);
}

static struct termios sTermSaved; // note: echo is restored when user presses ctrl+c during hidden input
static volatile sig_atomic_t sTermHidden = 0;

static void TermRestore(int sig)
{
	if (sTermHidden)
		tcsetattr(STDIN_FILENO, TCSANOW, &sTermSaved);

	signal(sig, SIG_DFL);
	raise(sig);
}
#endif

static string Req(const string &name) // note: required parameter in help
{
	return "<" + name + ">";
}

static string Opt(const string &name) // note: optional parameter in help
{
	return "[" + name + "]";
}

static size_t TextWidth(const string &text) // note: translation can be utf-8, count characters instead of bytes
{
	size_t res = 0;

	for (unsigned char c: text) {
		if ((c & 0xC0) != 0x80)
			res++;
	}

	return res;
}

cCmdLine::cCmdLine(int argc, char *argv[], int first):
	mExecPath((argc > 0) ? argv[0] : ""),
	mMultiName("vhm")
{
	for (int i = first; i < argc; i++)
		mArgs.push_back(argv[i]);
}

cCmdLine::~cCmdLine()
{
	mMySQL.reset(); // note: close database connection before its configuration
	mDBConf.reset();
}

bool cCmdLine::Run(int argc, char *argv[], int &result)
{
	string name((argc > 0) ? argv[0] : ""); // note: started as vhm, it is symbolic link to verlihub, same as verlihub -m
	const size_t pos = name.find_last_of("/\\");

	if (pos != string::npos)
		name.erase(0, pos + 1);

	if ((name.size() > 4) && (toLower(name.substr(name.size() - 4)) == ".exe"))
		name.erase(name.size() - 4);

	const bool vhm = (name == "vhm");

	if (vhm || ((argc > 1) && ((string(argv[1]) == "-m") || (string(argv[1]) == "--multi")))) {
		if (!SockStartup()) {
			cerr << Paint(_("Failed to start socket library."), sRed, true) << endl;
			result = 1;
			return true;
		}

		{
			cCmdLine line(argc, argv, (vhm ? 1 : 2));

			if (!vhm)
				line.mMultiName = "verlihub -m";

			result = line.Multi();
		}

		SockCleanup();
		return true;
	}

	if (argc < 2)
		return false;

	struct sCommand {
		const char *mShort; // note: same as in vh script, except -v that is already verbose option of hub
		const char *mLong;
		int (cCmdLine::*mFunc)();
	};

	static const sCommand list[] = {
		{"-h", "--help", &cCmdLine::Help},
		{NULL, "--version", &cCmdLine::Version},
		{"-i", "--install", &cCmdLine::Install},
		{"-r", "--run", &cCmdLine::HubRun},
		{NULL, "--stop", &cCmdLine::HubStop}, // note: -s is already syslog suffix option of hub
		{"-q", "--restart", &cCmdLine::HubRestart},
		{"-R", "--reload", &cCmdLine::HubReload},
		{"-t", "--status", &cCmdLine::HubStatus},
		{"-g", "--geoip", &cCmdLine::GeoIP},
		{"-a", "--autoload", &cCmdLine::AutoLoad},
		{"-b", "--backup", &cCmdLine::Backup},
		{"-w", "--restore", &cCmdLine::Restore},
	#ifdef _WIN32
		{NULL, "--watchdog", &cCmdLine::Watchdog}, // note: hidden, started by --run
	#endif
		{"-gc", "--getconf", &cCmdLine::GetConf},
		{"-sc", "--setconf", &cCmdLine::SetConf},
		{"-mp", "--modport", &cCmdLine::ModPort},
		{"-u", "--adduser", &cCmdLine::AddUser},
		{"-c", "--clean", &cCmdLine::Clean},
		{"-uh", "--uphelp", &cCmdLine::UpHelp}
	};

	const string arg(argv[1]);

	for (const sCommand &cmd: list) {
		if ((cmd.mShort && (arg == cmd.mShort)) || (arg == cmd.mLong)) {
		#ifndef _WIN32
			signal(SIGPIPE, SIG_IGN); // note: same as hub, closed connection gives error instead of ending process, https download writes to network
		#endif

			if (!SockStartup()) { // note: windows needs it before database connection
				cerr << Paint(_("Failed to start socket library."), sRed, true) << endl;
				result = 1;
				return true;
			}

			{
				cCmdLine line(argc, argv);
				result = (line.*cmd.mFunc)();
			} // note: database connection is closed before socket library

			SockCleanup();
			return true;
		}
	}

	return false;
}

string cCmdLine::FindConfigDir(const char *dir)
{
	string res, tmp;
	const char *home = getenv("HOME");

	if (home) {
		tmp = home;
		tmp += "/.config/verlihub";

		if (DirExists(tmp)) {
			res = tmp;
		} else {
			tmp = home;
			tmp += "/.verlihub";

			if (DirExists(tmp))
				res = tmp;
		}
	}

	if (DirExists("./.verlihub"))
		res = "./.verlihub";

	const char *env = getenv("VERLIHUB_CFG");

	if (dir && DirExists(dir))
		res = dir;
	else if (env && DirExists(env))
		res = env;

	if (res.empty() && FileExists(string(SYSCONFDIR) + "/verlihub/dbconfig")) // note: same places as vh script, only when they contain dbconfig, installation offers them
		res = string(SYSCONFDIR) + "/verlihub";

#ifndef _WIN32
	if (res.empty()) {
		struct passwd *pw = getpwnam("verlihub"); // note: home of system user verlihub

		if (pw && pw->pw_dir && pw->pw_dir[0] && FileExists(string(pw->pw_dir) + "/dbconfig"))
			res = StripSeparators(pw->pw_dir);
	}
#endif

	if (res.empty())
		res = "/etc/verlihub";

	return SlashPath(res);
}

int cCmdLine::Help()
{
	const string conf(_("config")); // note: same as in help files

	const vector<pair<string, string>> opts = {
		{"-d, --config-dir " + Req(_("path")), _("Use configuration folder at given path")},
		{"-v, --verbose", _("Increase log level, can be given more times")},
		{"-S, --syslog", _("Log to syslog instead of console")},
		{"-s, --syslog-suffix " + Req(_("suffix")), _("Add suffix to syslog name")},
		{Opt(_("port")), _("Listen on given port instead of configured one")}
	};

	const vector<pair<string, string>> cmds = {
		{"-i, --install", _("Install new hub, its configuration folder, database and master account")},
		{"-r, --run " + Opt(conf), _("Run the hub from the given configuration folder")},
		{"--stop " + Opt(conf), _("Stop the hub from the given configuration folder")},
		{"-q, --restart " + Opt(conf), _("Restart the hub from the given configuration folder")},
		{"-R, --reload " + Opt(conf), _("Force hub to reload its configuration and caches")},
		{"-t, --status " + Opt(conf), _("Check if hub is running or not")},
		{"-g, --geoip " + Opt(_("path")), _("Update all free MaxMind databases")},
		{"-a, --autoload " + Opt(_("option")), _("Load plugin automatically when hub starts, options are")},
		{"    -l, --list", _("Show list of available plugins, this is default")},
		{"    -n, --on", _("Run plugin automatically with hub")},
		{"    -f, --off", _("Disable auto load of plugin")},
		{"    -d, --configdir " + Opt(conf), _("Configuration folder of hub, optional")},
		{"-m, --multi " + Req(_("option")), _("Manage multiple hubs, same as vhm, see verlihub -m --help")},
		{"-uh, --uphelp " + Opt(conf), _("Update help files of hub")},
		{"-mp, --modport " + Req(conf) + ' ' + Req(_("main")) + ' ' + Opt(_("extra")), _("Modify main and extra hub ports")},
		{"-u, --adduser " + Req(_("nick")) + ' ' + Req(_("password")) + ' ' + Req(_("class")) + ' ' + Opt(conf), _("Register new user")},
		{"-gc, --getconf " + Req(_("variable")) + ' ' + Req(conf), _("Get hub configuration variable")},
		{"-sc, --setconf " + Req(_("variable")) + ' ' + Req(_("value")) + ' ' + Req(conf), _("Set hub configuration variable, use _ for empty value")},
		{"-c, --clean " + Opt(conf), _("Clean kicklist, banlist and unbanlist tables")},
		{"-b, --backup " + Req(_("path")) + ' ' + Opt(conf), _("Make backup of database from the given configuration folder")},
		{"-w, --restore " + Req(_("file")) + ' ' + Opt(conf), _("Import the given SQL file in the database of given configuration folder")},
		{"--version", _("Print version information")},
		{"-h, --help", _("Show this help")}
	};

	size_t width = 0; // note: same column for both lists, it depends on translation

	for (const auto &item: opts)
		width = max(width, TextWidth(item.first));

	for (const auto &item: cmds)
		width = max(width, TextWidth(item.first));

	cout << Paint(string(HUB_VERSION_NAME) + ' ' + HUB_VERSION_VERS, sBold) << "\n\n";
	cout << _("Usage") << ":\n\n";
	cout << "  verlihub " << Opt(_("options")) << ' ' << Opt(_("port")) << '\n';
	cout << "  verlihub " << Req(_("command")) << ' ' << Opt(_("parameters")) << "\n\n";
	cout << _("Options to start hub") << ":\n\n";

	for (const auto &item: opts)
		cout << "  " << Paint(item.first, sRed) << string(width - TextWidth(item.first) + 2, ' ') << item.second << '\n';

	cout << '\n' << _("Commands, they don't start hub, configuration folder is found same way as hub does when it's not given") << ":\n\n";

	for (const auto &item: cmds)
		cout << "  " << Paint(item.first, sRed) << string(width - TextWidth(item.first) + 2, ' ') << item.second << '\n';

	cout << flush;
	return 0;
}

int cCmdLine::Version()
{
	cout << Paint(string(HUB_VERSION_NAME) + ':', sBold) << ' ' << HUB_VERSION_VERS << '\n';
	cout << Paint("MySQL/MariaDB client:", sBold) << ' ' << mysql_get_client_info() << '\n';

#ifdef USE_TLS
	string ssl(OpenSSL_version(OPENSSL_VERSION)); // note: it starts with library name

	if (ssl.compare(0, 8, "OpenSSL ") == 0)
		ssl.erase(0, 8);

	cout << Paint("OpenSSL:", sBold) << ' ' << ssl << '\n';
#endif

	cout << flush;
	return 0;
}

int cCmdLine::GetConf()
{
	if ((mArgs.size() < 1) || mArgs[0].empty()) {
		cerr << Paint(_("Specify configuration variable."), sRed, true) << endl;
		return 1;
	}

	if ((mArgs.size() < 2) || mArgs[1].empty()) {
		cerr << Paint(_("Specify configuration folder."), sRed, true) << endl;
		return 1;
	}

	if (mArgs.size() > 2) {
		cerr << Paint(_("Too many parameters."), sRed, true) << endl;
		return 1;
	}

	if (!SetPath(mArgs[1]) || !Connect())
		return 1;

	if (!TableExists("SetupList")) {
		cerr << Paint(_("Table SetupList does not exist, start hub once to create it."), sRed, true) << endl;
		return 1;
	}

	string val;

	if (!LoadVar(mArgs[0], val)) {
		cerr << Paint(autosprintf(_("Configuration variable not found: %s"), mArgs[0].c_str()), sRed, true) << endl;
		return 1;
	}

	cout << '\t' << Paint(_("Variable"), sBold) << ": " << mArgs[0] << '\n';
	cout << '\t' << Paint(_("Current value"), sBold) << ": " << val << '\n';
	cout << '\t' << Paint(_("Configuration folder"), sBold) << ": " << mConfigDir << endl;
	return 0;
}

int cCmdLine::SetConf()
{
	if ((mArgs.size() < 1) || mArgs[0].empty()) {
		cerr << Paint(_("Specify configuration variable."), sRed, true) << endl;
		return 1;
	}

	if ((mArgs.size() < 2) || mArgs[1].empty()) {
		cerr << Paint(_("Specify value, use _ for empty value."), sRed, true) << endl;
		return 1;
	}

	if ((mArgs.size() < 3) || mArgs[2].empty()) {
		cerr << Paint(_("Specify configuration folder."), sRed, true) << endl;
		return 1;
	}

	if (mArgs.size() > 3) {
		cerr << Paint(_("Too many parameters."), sRed, true) << endl;
		return 1;
	}

	if (!SetPath(mArgs[2]) || !Connect())
		return 1;

	if (!TableExists("SetupList")) {
		cerr << Paint(_("Table SetupList does not exist, start hub once to create it."), sRed, true) << endl;
		return 1;
	}

	string val_old, val_new;

	if (!LoadVar(mArgs[0], val_old)) { // note: only existing variable, so typo does not create new one
		cerr << Paint(autosprintf(_("Configuration variable not found: %s"), mArgs[0].c_str()), sRed, true) << endl;
		return 1;
	}

	if (mArgs[1] != "_")
		val_new = mArgs[1];

	if (!SaveVar(mArgs[0], val_new) || !LoadVar(mArgs[0], val_new))
		return 1;

	cout << '\t' << Paint(_("Variable"), sBold) << ": " << mArgs[0] << '\n';
	cout << '\t' << Paint(_("Old value"), sBold) << ": " << val_old << '\n';
	cout << '\t' << Paint(_("New value"), sBold) << ": " << val_new << '\n';
	cout << '\t' << Paint(_("Configuration folder"), sBold) << ": " << mConfigDir << '\n';
	cout << _("Running hub uses new value after reload or restart.") << endl;
	return 0;
}

int cCmdLine::ModPort()
{
	if ((mArgs.size() < 1) || mArgs[0].empty()) {
		cerr << Paint(_("Specify configuration folder."), sRed, true) << endl;
		return 1;
	}

	if ((mArgs.size() < 2) || mArgs[1].empty()) {
		cerr << Paint(_("Specify main port."), sRed, true) << endl;
		return 1;
	}

	unsigned int first = 0, port;

	if (!ParsePort(mArgs[1], first)) {
		cerr << Paint(autosprintf(_("Port must be a number between %d and %d: %s"), 1, 65535, mArgs[1].c_str()), sRed, true) << endl;
		return 1;
	}

	vector<unsigned int> list;
	ostringstream extra;

	for (size_t i = 2; i < mArgs.size(); i++) {
		if (!ParsePort(mArgs[i], port)) {
			cerr << Paint(autosprintf(_("Port must be a number between %d and %d: %s"), 1, 65535, mArgs[i].c_str()), sRed, true) << endl;
			return 1;
		}

		if (port == first) {
			cerr << Paint(autosprintf(_("Extra ports must not contain main port: %d"), port), sRed, true) << endl;
			return 1;
		}

		for (unsigned int item: list) {
			if (item == port) {
				cerr << Paint(autosprintf(_("Extra ports must not repeat each other: %d"), port), sRed, true) << endl;
				return 1;
			}
		}

		if (list.size())
			extra << ' ';

		extra << port;
		list.push_back(port);
	}

	if (!SetPath(mArgs[0]) || !Connect())
		return 1;

	if (!TableExists("SetupList")) {
		cerr << Paint(_("Table SetupList does not exist, start hub once to create it."), sRed, true) << endl;
		return 1;
	}

	if (!SaveVar("listen_port", StringFrom(first)) || !SaveVar("extra_listen_ports", extra.str()))
		return 1;

	cout << Paint(string(_("Hub ports successfully modified")) + ':', sGreenBold) << "\n\n";
	cout << '\t' << Paint("listen_port", sBold) << " = " << Paint(StringFrom(first), sBold) << '\n';
	cout << '\t' << Paint("extra_listen_ports", sBold) << " = " << Paint((list.size() ? extra.str() : string(_("Not set"))), sBold) << '\n';
	cout << '\t' << Paint(_("Configuration folder"), sBold) << ": " << mConfigDir << '\n';
	cout << _("Running hub uses new ports after restart.") << endl;
	return 0;
}

int cCmdLine::AddUser()
{
	if ((mArgs.size() < 1) || mArgs[0].empty()) {
		cerr << Paint(_("Specify nick."), sRed, true) << endl;
		return 1;
	}

	if ((mArgs.size() < 2) || mArgs[1].empty()) {
		cerr << Paint(_("Specify password."), sRed, true) << endl;
		return 1;
	}

	static const char *classes[] = {"-1", "1", "2", "3", "4", "5", "10"}; // note: same as hub, guest class 0 can not be registered, -1 is pinger
	bool valid = false;

	if (mArgs.size() >= 3) {
		for (const char *item: classes) {
			if (mArgs[2] == item) {
				valid = true;
				break;
			}
		}
	}

	if (!valid) {
		cerr << Paint(autosprintf(_("Specify valid class: %s"), "-1, 1, 2, 3, 4, 5, 10"), sRed, true) << endl;
		return 1;
	}

	if (mArgs.size() > 4) {
		cerr << Paint(_("Too many parameters."), sRed, true) << endl;
		return 1;
	}

	if (!SetPath((mArgs.size() > 3) ? mArgs[3] : "") || !Connect())
		return 1;

	if (!TableExists("reglist") || !TableExists("SetupList")) {
		cerr << Paint(_("Table reglist or SetupList does not exist, start hub once to create them."), sRed, true) << endl;
		return 1;
	}

	const string &nick = mArgs[0];
	const int clas = StringAsLL(mArgs[2]);

	if (!CheckNewUser(nick))
		return 1;

	cout << _("User information") << ":\n\n";
	cout << '\t' << Paint(_("Nick"), sBold) << ": " << nick << '\n';
	cout << '\t' << Paint(_("Password"), sBold) << ": " << mArgs[1] << '\n';
	cout << '\t' << Paint(_("Class"), sBold) << ": " << clas << '\n';
	cout << '\t' << Paint(_("Configuration folder"), sBold) << ": " << mConfigDir << "\n\n";

	if (!AskYesNo(_("Do you want to continue?"))) {
		cout << _("Skipped.") << endl;
		return 0;
	}

	if (!InsertUser(nick, mArgs[1], clas))
		return 1;

	cout << Paint(_("User successfully registered."), sGreenBold) << endl;
	return 0;
}

bool cCmdLine::CheckNewUser(const string &nick)
{
	string security, opchat;
	LoadVar("hub_security", security);
	LoadVar("opchat_name", opchat);

	if ((toLower(nick, true) == toLower(security, true)) || (toLower(nick, true) == toLower(opchat, true))) { // note: same as hub, dont register hub security and operator chat
		cerr << Paint(autosprintf(_("Nick is used by hub bot: %s"), nick.c_str()), sRed, true) << endl;
		return false;
	}

	cQuery query(*mMySQL);
	query.OStream() << "select `nick` from `reglist` where `nick` = '";
	cConfMySQL::WriteStringConstant(query.OStream(), nick);
	query.OStream() << '\'';

	if (query.Query() <= 0) {
		cerr << Paint(_("Failed to execute database query."), sRed, true) << endl;
		return false;
	}

	const int found = query.StoreResult();
	query.Clear();

	if (found > 0) {
		cerr << Paint(autosprintf(_("User is already registered: %s"), nick.c_str()), sRed, true) << endl;
		return false;
	}

	return true;
}

bool cCmdLine::InsertUser(const string &nick, const string &pass, int clas)
{
	string security, crypt;
	LoadVar("hub_security", security);
	int method = cRegUserInfo::eCRYPT_MD5; // note: same default as hub

	if (LoadVar("default_password_encryption", crypt) && crypt.size())
		method = StringAsLL(crypt);

	if ((method < cRegUserInfo::eCRYPT_NONE) || (method > cRegUserInfo::eCRYPT_MD5))
		method = cRegUserInfo::eCRYPT_MD5;

	cRegUserInfo ui; // note: hash is made same way as in hub, so it does not depend on database server functions
	ui.SetPass(pass, cRegUserInfo::tCryptMethods(method));

	ostringstream os;
	os << "insert into `reglist` (`nick`, `class`, `reg_date`, `reg_op`, `pwd_change`, `pwd_crypt`, `login_pwd`) values ('";
	cConfMySQL::WriteStringConstant(os, nick);
	os << "', " << clas << ", " << time(NULL) << ", '";
	cConfMySQL::WriteStringConstant(os, security);
	os << "', " << (ui.mPwdChange ? 1 : 0) << ", " << ui.mPWCrypt << ", '";
	cConfMySQL::WriteStringConstant(os, ui.mPasswd);
	os << "')";
	return Execute(os.str());
}

int cCmdLine::Clean()
{
	if (mArgs.size() > 1) {
		cerr << Paint(_("Too many parameters."), sRed, true) << endl;
		return 1;
	}

	if (!SetPath(mArgs.size() ? mArgs[0] : "") || !Connect())
		return 1;

	static const char *tables[] = {"banlist", "kicklist", "unbanlist"};
	int res = 0;

	for (const char *table: tables) {
		if (!AskYesNo(autosprintf(_("Do you want to truncate table %s?"), table))) {
			cout << _("Skipped.") << endl;
			continue;
		}

		if (Execute(string("truncate table `") + table + '`'))
			cout << Paint(autosprintf(_("Truncated table: %s"), Paint(table, sBold, false, sGreenBold).c_str()), sGreenBold) << endl;
		else
			res = 1;
	}

	return res;
}

int cCmdLine::UpHelp()
{
	if (mArgs.size() > 1) {
		cerr << Paint(_("Too many parameters."), sRed, true) << endl;
		return 1;
	}

	if (!SetPath(mArgs.size() ? mArgs[0] : ""))
		return 1;

	const string from(string(DATADIR) + "/config");
	DIR *dir = opendir(from.c_str());

	if (!dir) {
		cerr << Paint(autosprintf(_("Failed to open folder: %s"), from.c_str()), sRed, true) << endl;
		return 1;
	}

	unsigned int count = 0;
	bool fail = false;
	struct dirent *ent;

	while ((ent = readdir(dir)) != NULL) { // note: help files and language folders like ru that contain them
		const string name(ent->d_name);

		if ((name == ".") || (name == ".."))
			continue;

		const string path(from + '/' + name);

		if (FileExists(path)) {
			if (!IsHelpFile(name))
				continue;

			if (CopyOneFile(path, mConfigDir + '/' + name)) {
				count++;
			} else {
				cerr << Paint(autosprintf(_("Failed to copy file: %s"), path.c_str()), sRed, true) << endl;
				fail = true;
			}

		} else if (DirExists(path)) {
			DIR *sub = opendir(path.c_str());

			if (!sub)
				continue;

			struct dirent *item;

			while ((item = readdir(sub)) != NULL) {
				const string file(item->d_name);

				if (!IsHelpFile(file) || !FileExists(path + '/' + file))
					continue;

				if (!MakeDir(mConfigDir + '/' + name)) {
					cerr << Paint(autosprintf(_("Failed to create folder: %s"), (mConfigDir + '/' + name).c_str()), sRed, true) << endl;
					fail = true;
					break;
				}

				if (CopyOneFile(path + '/' + file, mConfigDir + '/' + name + '/' + file)) {
					count++;
				} else {
					cerr << Paint(autosprintf(_("Failed to copy file: %s"), (path + '/' + file).c_str()), sRed, true) << endl;
					fail = true;
				}
			}

			closedir(sub);
		}
	}

	closedir(dir);

	if (!count) {
		cerr << Paint(autosprintf(_("No help files found in folder: %s"), from.c_str()), sRed, true) << endl;
		return 1;
	}

	cout << Paint(autosprintf(_("Updated %d help files of hub: %s"), count, Paint(mConfigDir, sBold, false, sGreenBold).c_str()), sGreenBold) << endl;
	return (fail ? 1 : 0);
}

void cCmdLine::AskAdmin(string &user, string &pass)
{
	while (true) { // note: same as _ask_admin_access of vh script
		cout << _("Please enter MySQL administrator credentials to create database, account or grant permissions.") << "\n\n";
		user = Ask(_("Username"), "root");
		pass = AskPassword(_("Password"));
		cout << "\n\n" << _("Here is information you specified about your MySQL administrator") << ":\n\n";
		cout << '\t' << Paint(_("Username"), sBold) << ": " << user << '\n';
		cout << '\t' << Paint(_("Password"), sBold) << ": " << _("<hidden>") << "\n\n";

		if (AskYesNo(_("Do you want to continue with this information?")))
			return;

		cout << _("Let's do it again.") << "\n\n";
	}
}

int cCmdLine::Install()
{
	if (mArgs.size() > 1) { // note: sleep time of vh script is accepted and not used, tables are created without running hub
		cerr << Paint(_("Too many parameters."), sRed, true) << endl;
		return 1;
	}

	const char *env = getenv("USER");

	if (!env || !env[0])
		env = getenv("USERNAME"); // note: windows

	const string user((env && env[0]) ? env : "admin");

	cout << autosprintf(_("Hello %s and welcome to Verlihub installation wizard."), Paint(user, sBold).c_str());
	cout << ' ' << autosprintf(_("To exit this wizard, press %s at any time."), Paint("Ctrl+C", sBold).c_str());
	cout << ' ' << _("Let's begin with configuration of MySQL database access.");
	cout << ' ' << _("If you have any problems during configuration, please refer to following resources") << ":\n\n";
	cout << " * " << _("Follow our official manual") << ": " << Paint("https://github.com/verlihub/verlihub/wiki/", sBold) << '\n';
	cout << " * " << _("Seek help on our support hub") << ": " << Paint("nmdcs://hub.verlihub.net:7777", sBold) << '\n';
	cout << " * " << _("Check out official MySQL manual") << ": " << Paint("https://dev.mysql.com/doc/refman/5.7/en/", sBold) << "\n\n";
	cout << Paint("[*] " + string(_("MySQL information")) + ':', sBold) << "\n\n";
	cout << _("Please enter MySQL information below for the hub being installed.");
	cout << ' ' << _("If account or database don't exist, they will be created later in process using administrator credentials.") << "\n\n";

	string db_data("verlihub"), db_user("verlihub"), db_pass, db_host("localhost"), db_charset("cp1250"), locale, encoding, mmdb; // note: same defaults as vh script

	while (true) {
		db_data = Ask(_("Choose database name"), db_data);
		db_user = Ask(autosprintf(_("MySQL user to access '%s'"), db_data.c_str()), db_user);
		db_pass = AskPassword(autosprintf(_("MySQL password for user '%s'"), db_user.c_str()));
		db_host = Ask(autosprintf(_("MySQL hostname, add \"%s\" for custom port"), (':' + Req(_("port"))).c_str()), db_host); // note: same word as in help
		db_charset = Ask(_("MySQL charset, experts only, leave empty for default"), db_charset, true);
		locale = Ask(_("Hub locale, experts only, leave empty for default"), locale, true);
		encoding = Ask(_("Hub encoding, experts only, leave empty for default"), encoding, true);
		mmdb = Ask(_("MaxMindDB path, experts only, leave empty for default"), mmdb, true);

		cout << '\n' << _("Here is information you specified about your MySQL server") << ":\n\n";
		cout << '\t' << Paint(_("Database"), sBold) << ": " << db_data << '\n';
		cout << '\t' << Paint(_("Username"), sBold) << ": " << db_user << '\n';
		cout << '\t' << Paint(_("Password"), sBold) << ": " << _("<hidden>") << '\n';
		cout << '\t' << Paint(_("Hostname"), sBold) << ": " << db_host << '\n';
		cout << '\t' << Paint(_("Charset"), sBold) << ": " << db_charset << '\n';
		cout << '\t' << Paint(_("Hub locale"), sBold) << ": " << locale << '\n';
		cout << '\t' << Paint(_("Hub encoding"), sBold) << ": " << encoding << '\n';
		cout << '\t' << Paint(_("MaxMindDB path"), sBold) << ": " << mmdb << "\n\n";

		if (AskYesNo(_("Do you want to continue with this information?")))
			break;

		cout << _("Let's do it again.") << "\n\n";
	}

	string host, addr("localhost");
	unsigned int port;
	cMySQL::SplitHost(db_host, host, port);

	if ((host != "localhost") && (host != "127.0.0.1")) { // note: account on external server is created for address of this server
		cout << '\n' << _("You are going to use database on external server, please enter IP address of current server.") << "\n\n";

		while (true) {
			addr = Ask(_("IP address"), "", true);

			if (addr.size())
				break;

			cout << '\n' << _("You have entered empty value, let's do it again.") << '\n';
		}
	}

	string db_socket;

	if ((host == "localhost") && !port) { // note: local socket, same as _mysql_export_socket of vh script, client library uses socket only for localhost
		db_socket = FindSocket(db_host, db_user, db_pass);

		if (db_socket.size())
			cout << '\n' << autosprintf(_("Using MySQL socket %s"), Paint(db_socket, sBold).c_str()) << '\n';
	}

	ostringstream os;
	string admin_user, admin_pass, err;
	const string account(db_user + '@' + db_host);
	cout << '\n' << autosprintf(_("Checking for MySQL account %s, please wait."), Paint(account, sBold).c_str()) << "\n\n";
	err = TryConnect(db_host, db_user, db_pass, "", db_socket);

	if (err.size()) {
		cout << autosprintf(_("MySQL account %s doesn't exist, it will be created now using administrator credentials."), Paint(account, sBold).c_str()) << '\n';
		cout << autosprintf(_("MySQL server said: %s"), err.c_str()) << "\n\n";
		AskAdmin(admin_user, admin_pass);
		os.str("");
		os << "create user '";
		cConfMySQL::WriteStringConstant(os, db_user);
		os << "'@'";
		cConfMySQL::WriteStringConstant(os, addr);
		os << "' identified by '";
		cConfMySQL::WriteStringConstant(os, db_pass);
		os << '\'';

		if (!AdminExecute(db_host, admin_user, admin_pass, {os.str(), "flush privileges"}, db_socket) || TryConnect(db_host, db_user, db_pass, "", db_socket).size()) {
			cerr << '\n' << Paint(autosprintf(_("MySQL account %s failed to create, please do this manually and run installation again."), Paint(account, sBold, true, sRedBold).c_str()), sRedBold, true) << endl;
			return 1;
		}

		cout << '\n' << Paint(autosprintf(_("MySQL account %s has been created."), Paint(account, sBold, false, sGreenBold).c_str()), sGreenBold) << endl;
	} else {
		cout << Paint(autosprintf(_("MySQL account %s has been found."), Paint(account, sBold, false, sGreenBold).c_str()), sGreenBold) << endl;
	}

	cout << '\n' << autosprintf(_("Checking for MySQL database %s, please wait."), Paint(db_data, sBold).c_str()) << "\n\n";
	err = TryConnect(db_host, db_user, db_pass, db_data, db_socket);

	if (err.size()) {
		cout << autosprintf(_("MySQL database %s doesn't exist, it will be created now using administrator credentials."), Paint(db_data, sBold).c_str()) << '\n';
		cout << autosprintf(_("MySQL server said: %s"), err.c_str()) << "\n\n";

		if (admin_user.empty() || admin_pass.empty()) // note: same as vh script, ask only when not asked yet
			AskAdmin(admin_user, admin_pass);

		string name(db_data); // note: backtick in name of database is doubled
		size_t pos = 0;

		while ((pos = name.find('`', pos)) != string::npos) {
			name.insert(pos, 1, '`');
			pos += 2;
		}

		vector<string> list;
		list.push_back("create database if not exists `" + name + '`');
		os.str("");
		os << "grant all on `" << name << "`.* to '";
		cConfMySQL::WriteStringConstant(os, db_user);
		os << "'@'";
		cConfMySQL::WriteStringConstant(os, addr);
		os << '\'';
		list.push_back(os.str());
		list.push_back("flush privileges");

		if (!AdminExecute(db_host, admin_user, admin_pass, list, db_socket) || TryConnect(db_host, db_user, db_pass, db_data, db_socket).size()) {
			cerr << '\n' << Paint(autosprintf(_("MySQL database %s failed to create, please do this manually and run installation again."), Paint(db_data, sBold, true, sRedBold).c_str()), sRedBold, true) << endl;
			return 1;
		}

		cout << '\n' << Paint(autosprintf(_("MySQL database %s has been created and permissions granted."), Paint(db_data, sBold, false, sGreenBold).c_str()), sGreenBold) << endl;
	} else {
		cout << Paint(autosprintf(_("MySQL database %s has been found, assuming that permissions are already granted."), Paint(db_data, sBold, false, sGreenBold).c_str()), sGreenBold) << endl;
	}

	vector<string> dirs; // note: same places as vh script offers

#ifndef _WIN32
	string vh_home;
	uid_t vh_uid = 0;
	gid_t vh_gid = 0;

	if (geteuid() == 0) {
		dirs.push_back("/etc/verlihub");

		if (string(SYSCONFDIR) != "/etc")
			dirs.push_back(string(SYSCONFDIR) + "/verlihub");

		struct passwd *pw = getpwnam("verlihub"); // note: home of system user verlihub when it exists

		if (pw && pw->pw_dir && pw->pw_dir[0]) {
			vh_home = StripSeparators(pw->pw_dir);
			vh_uid = pw->pw_uid;
			vh_gid = pw->pw_gid;
			dirs.push_back(vh_home);
		}
	} else {
		cout << '\n' << Paint(_("Please run installation as administrator to perform system wide setup."), sBlue) << '\n';
	}

	if (geteuid() != 0)
#endif
	{
		const char *env_home = getenv("HOME");

		if (env_home && env_home[0]) {
			const string home(StripSeparators(SlashPath(env_home)));
			dirs.push_back(home + "/.config/verlihub");

			if (DirExists(home + "/.verlihub"))
				dirs.push_back(home + "/.verlihub");
		}
	}

	string dir, ans;
	bool fresh = false;

	while (true) {
		cout << '\n' << _("Please choose a place for configuration files") << ":\n\n";

		for (size_t i = 0; i < dirs.size(); i++)
			cout << '\t' << Paint("[" + StringFrom(i + 1) + "] ", sBold) << dirs[i] << '\n';

		cout << '\t' << Paint("[9] ", sBold) << _("Other, choose path where to put configuration file.") << "\n\n";

		while (true) {
			ans = Ask(_("Select number"));

			if ((ans == "9") || ((ans.size() == 1) && (ans[0] >= '1') && (size_t(ans[0] - '0') <= dirs.size())))
				break;

			cout << '\n' << Paint(_("Please enter a valid number from the list."), sRedBold) << "\n\n";
		}

		if (ans == "9")
			dir = Ask(_("Type complete path"));
		else
			dir = dirs[size_t(ans[0] - '1')];

		dir = StripSeparators(SlashPath(dir));
		fresh = !DirExists(dir);

		if (fresh)
			cout << '\n' << autosprintf(_("Configuration folder %s doesn't exist and will be created now."), Paint(dir, sBold).c_str()) << "\n\n";
		else
			cout << '\n' << autosprintf(_("Configuration folder %s already exists and will be backed up now."), Paint(dir, sBold).c_str()) << "\n\n";

		if (!AskYesNo(_("Do you want to continue? Choose no to select another folder."))) {
			cout << _("Let's do it again.") << '\n';
			continue;
		}

		const string test(dir + "/_test_dir"); // note: same permission check as vh script

		if (MakeDirs(test) && RemoveDir(test))
			break;

		cout << '\n' << Paint(autosprintf(_("You don't have permission to write to %s, please select another folder, do this manually and run installation again or run installation as system administrator."), Paint(dir, sBold, false, sRedBold).c_str()), sRedBold) << '\n';
	}

	const string conf(dir + "/dbconfig");

	if (!fresh && FileExists(conf)) { // note: old configuration is moved to backup folder, nothing is deleted
		const string back(dir + ".backup." + StringFrom(time(NULL)));

		if (rename(dir.c_str(), back.c_str()) != 0) {
			cerr << '\n' << Paint(autosprintf(_("Failed to move old configuration to %s, please check write permission or run installation as system administrator."), Paint(back, sBold, true, sRedBold).c_str()), sRedBold, true) << endl;
			return 1;
		}

		cout << '\n' << Paint(autosprintf(_("Old configuration has been moved to %s."), Paint(back, sBold, false, sGreenBold).c_str()), sGreenBold) << '\n';
	}

	ofstream file;

	if (MakeDirs(dir))
		file.open(conf.c_str(), ios::out | ios::trunc);

	if (file.is_open()) {
		file << "db_host = " << db_host << '\n';
		file << "db_data = " << db_data << '\n';
		file << "db_user = " << db_user << '\n';
		file << "db_pass = " << db_pass << '\n';

		if (db_socket.size())
			file << "db_socket = " << db_socket << '\n';

		if (db_charset.size())
			file << "db_charset = " << db_charset << '\n';

		if (locale.size())
			file << "locale = " << locale << '\n';

		if (mmdb.size())
			file << "mmdb_path = " << mmdb << '\n';

		file.close();
	}

	if (!file || !FileExists(conf)) {
		cerr << '\n' << Paint(autosprintf(_("Installation is not complete, it failed to create configuration folder or file, please check write permission on %s."), Paint(dir, sBold, true, sRedBold).c_str()), sRedBold, true) << endl;
		return 1;
	}

	cout << '\n' << Paint(autosprintf(_("Configuration file has been successfully written to %s."), Paint(conf, sBold, false, sGreenBold).c_str()), sGreenBold) << "\n\n";
	cout << _("Copying files and accessing MySQL server, please wait.") << '\n';

	if (!CopyTree(string(DATADIR) + "/config", dir) || !MakeDirs(dir + "/plugins") || !MakeDirs(dir + "/scripts")) {
		cerr << Paint(autosprintf(_("Installation is not complete, it failed to create configuration folder or file, please check write permission on %s."), Paint(dir, sBold, true, sRedBold).c_str()), sRedBold, true) << endl;
		return 1;
	}

#ifdef _WIN32
	const string plug("libplug_pi.dll"); // note: windows symbolic link needs administrator, so plugin is copied
#else
	const string plug("libplug_pi.so");
#endif

	const string from(string(PLUGINDIR) + '/' + plug), to(dir + "/plugins/" + plug);

	if (FileExists(from)) { // note: plugman is loaded on start, it manages other plugins
	#ifdef _WIN32
		const bool done = CopyOneFile(from, to);
	#else
		unlink(to.c_str());
		const bool done = (symlink(from.c_str(), to.c_str()) == 0);
	#endif

		if (!done)
			cerr << Paint(autosprintf(_("Failed to add plugin to configuration folder: %s"), from.c_str()), sRed, true) << endl;
	}

	mConfigDir = dir;

	if (!CreateTables()) {
		cerr << Paint(autosprintf(_("Failed to create Verlihub database tables, please check %s for more information and run installation again."), Paint(dir + "/install_log", sBold, true, sRedBold).c_str()), sRedBold, true) << endl;
		return 1;
	}

	cout << '\n' << Paint(_("Installation is complete, now you can add master account and enter some basic settings."), sGreenBold) << "\n\n";
	cout << Paint("[*] " + string(_("Hub settings")) + ':', sBold) << "\n\n";
	cout << _("Choose master account and other hub information, don't use any special characters here, you'll be able to use them later.") << "\n\n";

	char buf[256];
	string host_name("localhost");

	if (gethostname(buf, sizeof(buf)) == 0) {
		buf[sizeof(buf) - 1] = '\0';
		host_name = buf;
	}

#ifdef _WIN32
	const string port_def("411");
#else
	const string port_def((user == "root") ? "411" : "4111"); // note: same as vh script, normal user can not listen on port below 1024
#endif

	string nick, pass, hub_host, hub_name, port_ans;
	unsigned int hub_port = 0;

	while (true) {
		nick = Ask(_("Master nick"), "[SU]" + user);

		if (!CheckNewUser(nick))
			continue;

		pass = AskPassword(_("Master password"), true);
		cout << '\n';

		while (true) {
			port_ans = Ask(_("Hub port"), port_def);

			if (ParsePort(port_ans, hub_port))
				break;

			cout << Paint(autosprintf(_("Port must be a number between %d and %d: %s"), 1, 65535, port_ans.c_str()), sRedBold) << endl;
		}

		hub_host = Ask(_("Hub host"), host_name);
		hub_name = Ask(_("Hub name"), autosprintf(_("Hub of %s"), user.c_str()));

		cout << '\n' << _("Is this information correct?") << "\n\n";
		cout << '\t' << Paint(_("Master nick"), sBold) << ": " << nick << '\n';
		cout << '\t' << Paint(_("Master password"), sBold) << ": " << _("<hidden>") << '\n';
		cout << '\t' << Paint(_("Hub name"), sBold) << ": " << hub_name << '\n';
		cout << '\t' << Paint(_("Hub address"), sBold) << ": dchub://" << hub_host << ':' << hub_port << "\n\n";

		if (AskYesNo(_("Do you want to continue with this information?")))
			break;

		cout << _("Let's try again.") << "\n\n";
	}

	cout << '\n' << _("Please wait while creating settings.") << '\n';
	bool done = SaveVar("hub_name", hub_name);
	done = SaveVar("hub_host", ((hub_port == 411) ? hub_host : (hub_host + ':' + StringFrom(hub_port)))) && done;
	done = SaveVar("listen_port", StringFrom(hub_port)) && done;

	if (encoding.size())
		done = SaveVar("hub_encoding", encoding) && done;

	if (!done)
		cerr << '\n' << Paint(_("Failed to save hub settings, please set them manually after installation."), sRedBold, true) << endl;

	if (!InsertUser(nick, pass, 10)) // note: master class
		cerr << '\n' << Paint(autosprintf(_("Failed to create master account %s, please do it manually after installation."), Paint(nick, sBold, true, sRedBold).c_str()), sRedBold, true) << endl;

	const string regs(string(DATADIR) + "/sql/default_reglist.sql"); // note: hublist pingers
	string data;

	if (!LoadFileInString(regs, data) || !Execute(data))
		cerr << '\n' << Paint(autosprintf(_("Failed to import %s, please do it manually after installation."), Paint(regs, sBold, true, sRedBold).c_str()), sRedBold, true) << endl;

#ifndef _WIN32
	if (vh_home.size() && (dir == vh_home)) { // note: hub runs as system user verlihub
		ChownTree(dir, vh_uid, vh_gid);
		chmod(dir.c_str(), 0750);
	}
#endif

	{
		ofstream log((dir + "/install_log").c_str(), ios::out | ios::app);

		if (log.is_open()) { // note: closing database connection writes log too
			sLogRedirect redirect(log);
			mMySQL.reset();
		}
	}

	cout << '\n' << Paint(_("Your hub has been installed and configured successfully, please use following command to start it") + string(":"), sGreenBold) << "\n\n";
	cout << '\t' << Paint("verlihub -d " + dir, sBold) << "\n\n";
	cout << Paint(autosprintf(_("Please use %s command for further instructions."), Paint("verlihub --help", sBold, false, sGreenBold).c_str()), sGreenBold) << "\n\n";
	cout << _("For any other questions please get in contact with us using our official support hub") << ": " << Paint("nmdcs://hub.verlihub.net:7777", sBold) << endl;
	return 0;
}

bool cCmdLine::CreateTables()
{
	const string log_name(mConfigDir + "/install_log");
	ofstream log(log_name.c_str(), ios::out | ios::trunc);

	if (!log.is_open()) {
		cerr << Paint(autosprintf(_("Failed to write file: %s"), log_name.c_str()), sRed, true) << endl;
		return false;
	}

	bool done = true;

	{
		sLogRedirect redirect(log);

	#ifdef _WIN32
		LPTOP_LEVEL_EXCEPTION_FILTER old_crash = SetUnhandledExceptionFilter(InstallCrash); // note: same as hub start, backtrace goes to install_log
	#else
		void (*old_crash)(int) = signal(SIGSEGV, InstallCrash);
	#endif

		try { // note: hub creates all tables, default data and plugin tables in constructor, it does not listen
			unique_ptr<cServerDC> serv(new cServerDC(mConfigDir, mExecPath));
		} catch (const char *ex) {
			cerr << ex << endl;
			done = false;
		}

		cServerDC::sCurrentServer = NULL; // note: hub is destroyed, crash handler must not use it

	#ifdef _WIN32
		SetUnhandledExceptionFilter(old_crash);
	#else
		signal(SIGSEGV, old_crash);
	#endif

		done = (done && Connect() && TableExists("reglist") && TableExists("SetupList")); // note: database log of this connection goes to same file
	}

	log.close();
	return done;
}

string cCmdLine::EventName(const string &dir, const char *what)
{
	string path(dir);

#ifdef _WIN32
	char *full = _fullpath(NULL, dir.c_str(), 0); // note: same folder gives same name, also when given relative

	if (full) {
		path = full;
		free(full);
	}

	path = toLower(StripSeparators(SlashPath(path))); // note: windows paths are not case sensitive
#endif

	unsigned long long hash = 14695981039346656037ULL; // note: fnv-1a, name must be short and without separators

	for (unsigned char c: path) {
		hash ^= c;
		hash *= 1099511628211ULL;
	}

	ostringstream os;
	os << "Local\\Verlihub_" << what << '_' << hex << hash;
	return os.str();
}

string cCmdLine::ExePath() const
{
#ifdef _WIN32
	char buf[MAX_PATH];
	const DWORD len = GetModuleFileNameA(NULL, buf, sizeof(buf));

	if (len && (len < sizeof(buf)))
		return SlashPath(string(buf, len));
#else
	char buf[4096];
	const ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1); // note: linux, other systems use argv

	if (len > 0)
		return string(buf, len);

	if (mExecPath.find('/') == string::npos) // note: started from path, same as vh script
		return string(PREFIXDIR) + "/bin/verlihub";
#endif

	return mExecPath;
}

bool cCmdLine::ReadPid(const string &dir, long &pid)
{
	ifstream file((dir + "/pid").c_str());
	pid = 0;

	if (!file.is_open() || !(file >> pid))
		return false;

	return (pid > 0);
}

bool cCmdLine::IsRunning(long pid)
{
	if (pid <= 0)
		return false;

#ifdef _WIN32
	HANDLE proc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, DWORD(pid));

	if (!proc)
		return false;

	DWORD code = 0;
	char buf[MAX_PATH];
	DWORD len = sizeof(buf);
	bool res = (GetExitCodeProcess(proc, &code) && (code == STILL_ACTIVE));

	if (res && QueryFullProcessImageNameA(proc, 0, buf, &len)) // note: pid can be reused by other program
		res = (toLower(string(buf, len)).find("verlihub") != string::npos);

	CloseHandle(proc);
	return res;
#else
	if ((kill(pid_t(pid), 0) != 0) && (errno != EPERM))
		return false;

	ifstream file(("/proc/" + StringFrom(pid) + "/cmdline").c_str(), ios::in | ios::binary); // note: pid can be reused by other program, vh script checked process name too

	if (!file.is_open()) // note: no proc file system, process exists
		return true;

	string cmd;
	getline(file, cmd, '\0');
	return (cmd.find("verlihub") != string::npos);
#endif
}

bool cCmdLine::WaitExit(long pid, unsigned int secs)
{
	for (unsigned int i = 0; i < (secs * 10); i++) {
		if (!IsRunning(pid))
			return true;

	#ifdef _WIN32
		Sleep(100);
	#else
		usleep(100000);
	#endif
	}

	return !IsRunning(pid);
}

void cCmdLine::CheckPlugins()
{
	cout << "-- " << _("Checking plugin symbolic links before hub start") << endl;
	const string plug(mConfigDir + "/plugins");
	DIR *dir = opendir(plug.c_str());

	if (!dir)
		return;

#ifdef _WIN32
	const string ext(".dll"); // note: copies of plugins, windows symbolic link needs administrator
#else
	const string ext(".so");
#endif

	struct dirent *ent;

	while ((ent = readdir(dir)) != NULL) {
		const string name(ent->d_name);

		if ((name.compare(0, 3, "lib") != 0) || (name.size() < (ext.size() + 6)) || (name.compare(name.size() - ext.size() - 3, ext.size() + 3, "_pi" + ext) != 0))
			continue;

		const string path(plug + '/' + name), from(string(PLUGINDIR) + '/' + name);
		cout << "-- " << autosprintf(_("Checking symbolic link for %s"), name.c_str()) << " .. " << flush;

	#ifdef _WIN32
		struct stat st_from, st_to;
		const bool have = (stat(from.c_str(), &st_from) == 0);
		const bool same = (have && (stat(path.c_str(), &st_to) == 0) && (st_to.st_size == st_from.st_size) && (st_to.st_mtime >= st_from.st_mtime));

		if (same)
			cout << Paint("[OK]", sGreen) << endl;
		else if (have && CopyOneFile(from, path))
			cout << Paint("[RE]", sGreen) << endl;
		else
			cout << Paint("[NO]", sRed) << endl;
	#else
		struct stat st;

		if ((lstat(path.c_str(), &st) == 0) && S_ISLNK(st.st_mode)) {
			cout << Paint("[OK]", sGreen) << endl;
		} else if (FileExists(from)) {
			unlink(path.c_str());

			if (symlink(from.c_str(), path.c_str()) == 0)
				cout << Paint("[RE]", sGreen) << endl;
			else
				cout << Paint("[NO]", sRed) << endl;
		} else {
			cout << Paint("[NO]", sRed) << endl;
		}
	#endif
	}

	closedir(dir);
}

void cCmdLine::RaiseFileLimit()
{
#ifndef _WIN32 // note: windows has no such limit, select and wepoll have their own
	cout << "-- " << _("Checking open files limit before hub start") << endl;
	struct rlimit lim;

	if (getrlimit(RLIMIT_NOFILE, &lim) != 0) {
		cout << "-- " << _("Failed to get current open files limit, it will remain unchanged") << endl;
		return;
	}

	string val;
	bool load = false;

	{
		ostringstream sink; // note: database log is not shown, same as vh script
		sLogRedirect redirect(sink);
		load = (Connect() && LoadVar("max_users", val) && val.size() && (val.find_first_not_of("0123456789") == string::npos));
		mMySQL.reset(); // note: close before fork, child must not use this connection
	}

	if (!load) {
		cout << "-- " << _("Failed to get hub user limit, open files limit will remain unchanged") << endl;
		return;
	}

	const rlim_t cur = lim.rlim_cur, add = rlim_t(StringAsLL(val)), want = cur + add; // note: one socket per user, tls is no longer handled by separate proxy

	if (cur > want) {
		cout << "-- " << _("Current open files limit is higher than hub user limit, change not required") << endl;
		return;
	}

	struct rlimit set = lim;
	set.rlim_cur = want;

	if ((set.rlim_max != RLIM_INFINITY) && (set.rlim_max < want)) // note: same as ulimit -n, it changes both limits, only super user can raise hard one
		set.rlim_max = want;

	if ((setrlimit(RLIMIT_NOFILE, &set) == 0) && (getrlimit(RLIMIT_NOFILE, &lim) == 0) && (lim.rlim_cur == want))
		cout << "-- " << _("Successfully changed open files limit to") << ' ' << Paint(StringFrom((long long)want), sGreen) << endl;
	else
		cout << "-- " << _("Failed to set new open files limit, it will remain unchanged") << endl;
#endif
}

bool cCmdLine::StartHub()
{
	long pid = 0;

	if (ReadPid(mConfigDir, pid) && IsRunning(pid)) {
		cout << "-- " << autosprintf(_("Hub is already running with PID %ld, to stop it use --stop option"), pid) << endl;
		return false;
	}

	cout << "-- " << autosprintf(_("Starting hub from directory '%s', this could take some time"), mConfigDir.c_str()) << endl;
	CheckPlugins();
	const string log_file(mConfigDir + "/log"), err_file(mConfigDir + "/err"), pid_file(mConfigDir + "/pid");

	if (!FileWritable(mConfigDir)) {
		cout << Paint("-- " + string(autosprintf(_("You don't have permission to write to directory '%s'"), mConfigDir.c_str())), sRed) << endl;
		return false;
	}

	const string files[][2] = {
		{log_file, _("You don't have permission to write to log file '%s'")},
		{err_file, _("You don't have permission to write to error file '%s'")},
		{pid_file, _("You don't have permission to write to PID file '%s'")}
	};

	for (const auto &item: files) {
		if (FileExists(item[0]) && !FileWritable(item[0])) {
			cout << Paint("-- " + string(autosprintf(item[1].c_str(), item[0].c_str())), sRed) << endl;
			return false;
		}
	}

	if (FileExists(err_file)) { // note: backup logs
		remove((err_file + ".old").c_str()); // note: windows rename does not replace
		rename(err_file.c_str(), (err_file + ".old").c_str());
	}

	if (FileExists(log_file)) {
		remove((log_file + ".old").c_str());
		rename(log_file.c_str(), (log_file + ".old").c_str());
	}

	RaiseFileLimit();
	const string exe(ExePath());

#ifdef _WIN32
	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(sa);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE; // note: watchdog and hub write to these files
	HANDLE out = CreateFileA(log_file.c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, &sa, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	HANDLE err = CreateFileA(err_file.c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, &sa, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	HANDLE in = CreateFileA("NUL", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, &sa, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if ((out == INVALID_HANDLE_VALUE) || (err == INVALID_HANDLE_VALUE) || (in == INVALID_HANDLE_VALUE)) {
		cout << Paint("-- " + string(_("Error occured while trying to create log files")), sRed) << endl;
		return false;
	}

	STARTUPINFOA si;
	PROCESS_INFORMATION pi;
	memset(&si, 0, sizeof(si));
	memset(&pi, 0, sizeof(pi));
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESTDHANDLES;
	si.hStdInput = in;
	si.hStdOutput = out;
	si.hStdError = err;
	string cmd("\"" + exe + "\" --watchdog \"" + mConfigDir + "\""); // note: hidden command, watchdog runs hub and restarts it, like vh_daemon script
	const bool made = CreateProcessA(exe.c_str(), &cmd[0], NULL, NULL, TRUE, CREATE_NO_WINDOW | CREATE_NEW_PROCESS_GROUP, NULL, NULL, &si, &pi);
	CloseHandle(out);
	CloseHandle(err);
	CloseHandle(in);

	if (!made) {
		cout << Paint("-- " + string(_("Error occured while trying to get hub PID")), sRed) << endl;
		return false;
	}

	pid = long(pi.dwProcessId);
	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);
#else
	cout << flush;
	const pid_t child = fork();

	if (child < 0) {
		cout << Paint("-- " + string(_("Error occured while trying to get hub PID")), sRed) << endl;
		return false;
	}

	if (child == 0) { // note: watchdog, it runs hub and restarts it, like vh_daemon script
		setsid();
		const int in = open("/dev/null", O_RDONLY), out = open(log_file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644), err = open(err_file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);

		if ((in < 0) || (out < 0) || (err < 0))
			_exit(1);

		dup2(in, STDIN_FILENO);
		dup2(out, STDOUT_FILENO);
		dup2(err, STDERR_FILENO);
		close(in);
		close(out);
		close(err);
		exit(Watch(exe, mConfigDir));
	}

	pid = long(child);
#endif

	ofstream file(pid_file.c_str(), ios::out | ios::trunc);

	if (file.is_open()) {
		file << pid << '\n';
		file.close();
	}

#ifdef _WIN32
	Sleep(2000);
#else
	sleep(2);
#endif

	if (!IsRunning(pid)) {
		cout << Paint("-- " + string(autosprintf(_("Hub is not running, please see '%s' and '%s' for more information"), log_file.c_str(), err_file.c_str())), sRed) << endl;
		remove(pid_file.c_str());
		return false;
	}

	cout << "-- " << autosprintf(_("Starting hub with PID %ld"), pid) << " .. " << Paint("[OK]", sGreen) << endl;
	return true;
}

bool cCmdLine::KillHub()
{
	long pid = 0;
	const string pid_file(mConfigDir + "/pid");

	if (!ReadPid(mConfigDir, pid) || !IsRunning(pid)) {
		cout << Paint("-- " + string(autosprintf(_("Hub with config path '%s' is not running"), mConfigDir.c_str())), sRed) << endl;

		if (FileExists(pid_file) && (remove(pid_file.c_str()) != 0))
			cout << Paint("-- " + string(autosprintf(_("PID file %s already exists. Can't remove it, please do it manually"), pid_file.c_str())), sRed) << endl;

		return false;
	}

	cout << "-- " << autosprintf(_("Killing verlihub (PID: %ld)..."), pid) << flush;

#ifdef _WIN32
	HANDLE ev = OpenEventA(EVENT_MODIFY_STATE, FALSE, EventName(mConfigDir, "stop").c_str()); // note: windows has no signals, hub waits for this event
	const bool sent = (ev && SetEvent(ev));

	if (ev)
		CloseHandle(ev);

	if (!sent) {
		cout << Paint(' ' + string(_("USER HAS NO RIGHT TO STOP THE PROCESS")), sRed) << endl;
		return false;
	}
#else
	if (kill(pid_t(pid), SIGTERM) != 0) {
		cout << Paint(' ' + string(_("USER HAS NO RIGHT TO STOP THE PROCESS")), sRed) << endl;
		cout << Paint("-- " + string(autosprintf(_("Please use 'kill -9 %ld' as root"), pid)), sRed) << endl;
		return false;
	}
#endif

	WaitExit(pid, 30); // note: vh script waited 2 seconds, hub can need more to finish database writes
	remove(pid_file.c_str());
	cout << ' ' << _("Done") << endl;
	return true;
}

int cCmdLine::HubRun()
{
	if (mArgs.size() > 1) {
		cerr << Paint(_("Too many parameters."), sRed, true) << endl;
		return 1;
	}

	if (mArgs.empty())
		cout << "-- " << _("Searching config directory in default path") << endl;

	if (!SetPath(mArgs.size() ? mArgs[0] : ""))
		return 1;

	return (StartHub() ? 0 : 1);
}

int cCmdLine::HubStop()
{
	if (mArgs.size() > 1) {
		cerr << Paint(_("Too many parameters."), sRed, true) << endl;
		return 1;
	}

	if (!SetPath(mArgs.size() ? mArgs[0] : ""))
		return 1;

	return (KillHub() ? 0 : 1);
}

int cCmdLine::HubRestart()
{
	if (mArgs.size() > 1) {
		cerr << Paint(_("Too many parameters."), sRed, true) << endl;
		return 1;
	}

	if (!SetPath(mArgs.size() ? mArgs[0] : ""))
		return 1;

	KillHub(); // note: same as vh script, hub is started also when it was not running
	return (StartHub() ? 0 : 1);
}

int cCmdLine::HubReload()
{
	if (mArgs.size() > 1) {
		cerr << Paint(_("Too many parameters."), sRed, true) << endl;
		return 1;
	}

	if (!SetPath(mArgs.size() ? mArgs[0] : ""))
		return 1;

	long pid = 0;
	const string pid_file(mConfigDir + "/pid");

	if (!ReadPid(mConfigDir, pid) || !IsRunning(pid)) {
		cout << Paint("-- " + string(autosprintf(_("Hub with config path '%s' is not running"), mConfigDir.c_str())), sRed) << endl;

		if (FileExists(pid_file) && (remove(pid_file.c_str()) != 0))
			cout << Paint("-- " + string(autosprintf(_("PID file %s already exists. Can't remove it, please do it manually"), pid_file.c_str())), sRed) << endl;

		return 1;
	}

	cout << "-- " << autosprintf(_("Reloading verlihub (PID: %ld)..."), pid) << flush;

#ifdef _WIN32
	HANDLE ev = OpenEventA(EVENT_MODIFY_STATE, FALSE, EventName(mConfigDir, "reload").c_str()); // note: windows has no sighup, hub waits for this event
	const bool sent = (ev && SetEvent(ev));

	if (ev)
		CloseHandle(ev);
#else
	const bool sent = (kill(pid_t(pid), SIGHUP) == 0);
#endif

	if (!sent) {
		cout << Paint(' ' + string(_("USER HAS NO RIGHT TO SEND SIGNALS TO THE PROCESS")), sRed) << endl;
		return 1;
	}

	cout << Paint("[OK]", sGreen) << endl;
	return 0;
}

int cCmdLine::HubStatus()
{
	if (mArgs.size() > 1) {
		cerr << Paint(_("Too many parameters."), sRed, true) << endl;
		return 1;
	}

	if (!SetPath(mArgs.size() ? mArgs[0] : ""))
		return 1;

	long pid = 0;
	const bool run = (ReadPid(mConfigDir, pid) && IsRunning(pid));
	cout << "-- " << autosprintf(_("Hub with config path '%s' is"), mConfigDir.c_str()) << ' ' << (run ? Paint(_("running"), sGreen) : Paint(_("not running"), sRed)) << endl;
	return (run ? 0 : 1);
}

#ifdef _WIN32
int cCmdLine::Watchdog() // note: hidden command, windows has no fork, so --run starts this one
{
	if (mArgs.size() != 1)
		return 1;

	return Watch(ExePath(), mArgs[0]);
}
#endif

#ifndef _WIN32
static volatile pid_t sWatchHub = 0;
static volatile sig_atomic_t sWatchStop = 0;

static void WatchSignal(int sig) // note: same as vh_daemon script, stop and reload are passed to hub
{
	if (sig == SIGHUP) {
		if (sWatchHub > 0)
			kill(sWatchHub, SIGHUP);

		return;
	}

	sWatchStop = 1;

	if (sWatchHub > 0)
		kill(sWatchHub, SIGQUIT); // note: hub stops cleanly on it
}
#endif

int cCmdLine::Watch(const string &exe, const string &dir)
{
	time_t last = 0;
	unsigned int crashes = 0;
	int code = 0;

#ifndef _WIN32
	signal(SIGTERM, WatchSignal);
	signal(SIGINT, WatchSignal);
	signal(SIGQUIT, WatchSignal);
	signal(SIGHUP, WatchSignal);
#endif

#ifdef _WIN32
	HANDLE stop_ev = CreateEventA(NULL, TRUE, FALSE, EventName(dir, "stop").c_str()); // note: manual reset, hub and watchdog both see it
#endif

	while (true) {
	#ifdef _WIN32
		if (stop_ev && (WaitForSingleObject(stop_ev, 0) == WAIT_OBJECT_0))
			return 0;
	#else
		if (sWatchStop) // note: stop came while hub was not running
			return 0;
	#endif

	#ifdef _WIN32
		STARTUPINFOA si;
		PROCESS_INFORMATION pi;
		memset(&si, 0, sizeof(si));
		memset(&pi, 0, sizeof(pi));
		si.cb = sizeof(si);
		si.dwFlags = STARTF_USESTDHANDLES; // note: same log files as watchdog
		si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
		si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
		si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
		string cmd("\"" + exe + "\" -d \"" + dir + "\"");

		if (!CreateProcessA(exe.c_str(), &cmd[0], NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
			cout << "-- " << _("Failed to start hub") << endl;
			return 1;
		}

		CloseHandle(pi.hThread);
		WaitForSingleObject(pi.hProcess, INFINITE);
		DWORD res = 0;
		GetExitCodeProcess(pi.hProcess, &res);
		CloseHandle(pi.hProcess);
		code = int(res);
		const bool crash = ((code == 3) || (res >= 0xC0000000UL)); // note: crash ends hub with exception code
		const bool stop = (stop_ev && (WaitForSingleObject(stop_ev, 0) == WAIT_OBJECT_0));
	#else
		cout << flush;
		const pid_t child = fork();

		if (child < 0) {
			cout << "-- " << _("Failed to start hub") << endl;
			return 1;
		}

		if (child == 0) {
			signal(SIGTERM, SIG_DFL);
			signal(SIGINT, SIG_DFL);
			signal(SIGQUIT, SIG_DFL);
			signal(SIGHUP, SIG_DFL);
			execl(exe.c_str(), exe.c_str(), "-d", dir.c_str(), (char*)NULL);
			_exit(127);
		}

		sWatchHub = child;
		int st = 0;

		while (waitpid(child, &st, 0) < 0) { // note: signal interrupts wait, same as loop in vh_daemon script
			if (errno != EINTR) {
				st = 0;
				break;
			}
		}

		sWatchHub = 0;
		code = (WIFEXITED(st) ? WEXITSTATUS(st) : (WIFSIGNALED(st) ? (128 + WTERMSIG(st)) : 0));
		const bool crash = ((code == 3) || (code == 139)); // note: vh_daemon script meant both, its pipe checked only 139
		const bool stop = sWatchStop;
	#endif

		if ((code == 1) && !stop) {
			cout << "-- " << _("Restarting in a while...") << flush;

		#ifdef _WIN32
			if (stop_ev)
				WaitForSingleObject(stop_ev, 5000); // note: stop ends waiting, loop checks it
			else
				Sleep(5000);
		#else
			sleep(5);
		#endif

			cout << ' ' << _("Done.") << endl;
		} else if (crash && !stop) {
			cout << "-- " << autosprintf(_("Hub exits with code %d"), code) << endl;
			const time_t now = time(NULL);

			if ((now - last) > 60) {
				crashes = 1;
			} else if (++crashes > 3) {
				cout << "-- " << _("Too many crashes in short time, stopping.") << endl;
				break;
			}

			last = now;
			cout << "-- " << _("Restarting...") << flush;

		#ifdef _WIN32
			if (stop_ev)
				WaitForSingleObject(stop_ev, 2000);
			else
				Sleep(2000);
		#else
			sleep(2);
		#endif

			cout << ' ' << _("Done.") << endl;
		} else {
			cout << "-- " << _("Stopping...") << flush;

		#ifdef _WIN32
			Sleep(1000);
		#else
			sleep(1);
		#endif

			cout << ' ' << _("Done.") << endl;
			code = 0;
			break;
		}
	}

#ifdef _WIN32
	if (stop_ev)
		CloseHandle(stop_ev);
#endif

	return code;
}

string cCmdLine::HubsFile()
{
#if defined(_WIN32) || defined(HAVE_FREEBSD)
	return string(SYSCONFDIR) + "/verlihub.conf"; // note: same as vhm script on freebsd
#else
	return "/etc/verlihub.conf";
#endif
}

bool cCmdLine::ParseHub(string line, sHub &hub)
{
	if (line.size() && (line[line.size() - 1] == '\r'))
		line.erase(line.size() - 1);

	size_t pos = line.rfind(':'); // note: path:nick:owner:time, path is split from right, because windows path contains colon

	if ((pos == string::npos) || !pos)
		return false;

	hub.mTime = long(StringAsLL(line.substr(pos + 1)));
	line.erase(pos);
	pos = line.rfind(':');

	if ((pos == string::npos) || !pos)
		return false;

	hub.mOwner = line.substr(pos + 1);
	line.erase(pos);
	pos = line.rfind(':');

	if ((pos == string::npos) || !pos)
		return false;

	hub.mNick = line.substr(pos + 1);
	hub.mPath = line.substr(0, pos);
	return true;
}

bool cCmdLine::LoadHubs(vector<sHub> &list)
{
	list.clear();
	ifstream file(HubsFile().c_str());

	if (!file.is_open())
		return false;

	string line;
	sHub hub;

	while (getline(file, line)) {
		if (ParseHub(line, hub))
			list.push_back(hub);
	}

	return true;
}

static string FormatRow(const string &text, size_t width, const char *style = NULL) // note: same as _format_row of vhm script
{
	string res(style ? Paint(text, style) : text);
	const size_t len = TextWidth(text);

	if (len < width)
		res.append(width - len, ' ');

	return res;
}

bool cCmdLine::PrintHubs(bool legend)
{
	vector<sHub> list;

	if (!LoadHubs(list) || list.empty()) {
		cout << Paint("-- " + string(autosprintf(_("Config file is empty. Add new hub using %s"), (mMultiName + " --add").c_str())), sRed) << endl;
		return false;
	}

	cout << Paint(" ID   ", sBold) << FormatRow(_("Owner"), 20, sBold) << FormatRow(_("Path to dbconfig"), 35, sBold) << FormatRow(_("Process owner"), 20, sBold) << Paint(_("Last update"), sBold) << '\n';

	for (size_t i = 0; i < list.size(); i++) {
		long pid = 0;
		const bool run = (ReadPid(list[i].mPath, pid) && IsRunning(pid));
		const time_t when = time_t(list[i].mTime);
		char date[32] = "";
		struct tm *tm = gmtime(&when);

		if (tm)
			strftime(date, sizeof(date), "%Y-%m-%d %H:%M:%S", tm);

		cout << Paint(" [" + StringFrom(i + 1) + "]  ", (run ? sGreenBold : sRedBold)) << FormatRow(list[i].mNick, 20) << FormatRow(list[i].mPath, 35) << FormatRow(list[i].mOwner, 20) << date << '\n';
	}

	if (legend)
		cout << '\n' << _("Hub status") << ": " << Paint(_("Running"), sGreenBold) << " | " << Paint(_("Stopped"), sRedBold) << "\n\n";

	cout << flush;
	return true;
}

string cCmdLine::AskHub()
{
	while (true) { // note: same as _ask_identifier of vhm script
		const string ans(Ask(_("Select number")));

		if ((ans == "A") || (ans.find_first_not_of("0123456789") == string::npos))
			return ans;

		cout << Paint("-- " + string(_("Insert number or A to choose all hubs")), sRed) << endl;
	}
}

bool cCmdLine::RunAs(const sHub &hub, const char *action)
{
	cout << Paint("-- " + string(autosprintf(_("Performing action for '%s'"), hub.mPath.c_str())), sBold) << endl;

#ifdef _WIN32
	cCmdLine line(0, NULL); // note: windows has no process owners, action runs here
	line.mExecPath = mExecPath;
	line.mArgs.push_back(hub.mPath);
	const string what(action);

	if (what == "--run")
		return (line.HubRun() == 0);

	if (what == "--stop")
		return (line.HubStop() == 0);

	if (what == "--restart")
		return (line.HubRestart() == 0);

	if (what == "--reload")
		return (line.HubReload() == 0);

	if (what == "--uphelp")
		return (line.UpHelp() == 0);

	return false;
#else
	struct passwd *pw = getpwnam(hub.mOwner.c_str());

	if (!pw) {
		cout << Paint("-- " + string(_("Can't find process owner")), sRed) << endl;
		return false;
	}

	const string exe(ExePath()), name(pw->pw_name), home(pw->pw_dir ? pw->pw_dir : "/");
	const uid_t uid = pw->pw_uid;
	const gid_t gid = pw->pw_gid;
	cout << flush;
	const pid_t child = fork();

	if (child < 0)
		return false;

	if (child == 0) { // note: like su of vhm script, but without shell, so it works also for user without login shell
		if ((initgroups(name.c_str(), gid) != 0) || (setgid(gid) != 0) || (setuid(uid) != 0)) {
			cerr << Paint("-- " + string(autosprintf(_("Failed to switch to user %s"), name.c_str())), sRed, true) << endl;
			_exit(1);
		}

		setenv("HOME", home.c_str(), 1);
		setenv("USER", name.c_str(), 1);
		setenv("LOGNAME", name.c_str(), 1);
		execl(exe.c_str(), exe.c_str(), action, hub.mPath.c_str(), (char*)NULL);
		_exit(127);
	}

	int st = 0;

	while (waitpid(child, &st, 0) < 0) {
		if (errno != EINTR)
			return false;
	}

	return (WIFEXITED(st) && (WEXITSTATUS(st) == 0));
#endif
}

int cCmdLine::MultiHelp()
{
	const vector<pair<string, string>> cmds = { // note: same as help_me of vhm script
		{"-l, --list", _("Show a list")},
		{"-a, --add", _("Add a new hub")},
		{"-d, --del " + Opt("ID"), _("Delete an hub")},
		{"-r, --run " + Opt("ID"), _("Run a single hub or all hubs together")},
		{"-s, --stop " + Opt("ID"), _("Stop a single hub or all hubs together")},
		{"-q, --restart " + Opt("ID"), _("Restart a single hub or all hubs together")},
		{"-R, --reload " + Opt("ID"), _("Force hub to reload its configuration and caches")},
		{"-uh, --uphelp", _("Update help files of all hubs")},
		{"-v, --version", _("Print version information")},
		{"-h, --help", _("Show this help")}
	};

	size_t width = 0;

	for (const auto &item: cmds)
		width = max(width, TextWidth(item.first));

	cout << Paint(_("Multi-Verlihub"), sBold) << '\n';
	cout << autosprintf(_("This is a utility to run, restart and stop hubs with Verlihub, usage: %s"), (mMultiName + ' ' + Req(_("option"))).c_str()) << "\n\n";

	for (const auto &item: cmds)
		cout << "  " << Paint(item.first, sRed) << string(width - TextWidth(item.first) + 2, ' ') << item.second << '\n';

	cout << '\n' << autosprintf(_("List of hubs is in %s."), HubsFile().c_str());

#ifndef _WIN32
	cout << ' ' << _("Only super user can use it, each hub runs as its process owner."); // note: windows has no process owners yet
#endif

	cout << endl;
	return 0;
}

int cCmdLine::Multi()
{
	const string cmd(mArgs.size() ? mArgs[0] : "");

	if (cmd.empty() || (cmd == "-h") || (cmd == "--help"))
		return MultiHelp();

	if ((cmd == "-v") || (cmd == "--version"))
		return Version();

#ifndef _WIN32
	if (geteuid() != 0) { // note: same as vhm script, list file is in system folder and hubs run as other users
		cout << Paint("-- " + string(_("You can run vhm only with root account.")), sRed) << endl;
		return 1;
	}
#endif

	vector<sHub> list;

	if ((cmd == "-l") || (cmd == "--list")) {
		cout << "-- " << _("This is a list of available hubs") << "\n\n";
		return (PrintHubs(true) ? 0 : 1);
	}

	if ((cmd == "-uh") || (cmd == "--uphelp")) {
		unsigned int count = 0;
		LoadHubs(list);

		for (const sHub &hub: list) {
			count++;
			RunAs(hub, "--uphelp");
		}

		cout << Paint(autosprintf(_("Help file update done for %s hubs."), Paint(StringFrom(count), sBold, false, sGreenBold).c_str()), sGreenBold) << endl;
		return 0;
	}

	if ((cmd == "-a") || (cmd == "--add")) {
		const string conf(HubsFile());

		if (!FileExists(conf)) {
			ofstream file(conf.c_str(), ios::out | ios::app);

			if (!file.is_open()) {
				cout << Paint("-- " + string(autosprintf(_("You don't have the write permission in '%s' to create config file"), conf.c_str())), sRed) << endl;
				return 1;
			}
		} else if (!FileWritable(conf)) {
			cout << Paint("-- " + string(autosprintf(_("You don't have the write permission in '%s' to edit config file"), conf.c_str())), sRed) << endl;
			return 1;
		}

		cout << "-- " << _("Please insert some information for new entry") << endl;
		string nick, owner, path;

		while (true) {
			nick = Ask(_("Choose nickname of hub owner"));

			while (true) {
				owner = Ask(_("Choose user that should run verlihub"));

			#ifndef _WIN32
				if (!getpwnam(owner.c_str())) { // note: exact name, vhm script matched only beginning of line in passwd
					cout << Paint(_("User does not exist in the system"), sRed) << endl;
					continue;
				}
			#endif

				break;
			}

			while (true) {
				path = StripSeparators(SlashPath(Ask(_("Type path to dbconfig"))));
				bool dup = false;
				LoadHubs(list);

				for (const sHub &hub: list) {
					if (hub.mPath == path)
						dup = true;
				}

				if (!FileExists(path + "/dbconfig"))
					cout << Paint(_("Please insert a valid config folder with dbconfig file"), sRed) << endl;
				else if (dup)
					cout << Paint(_("The path already exists in config file"), sRed) << endl;
				else
					break;
			}

			cout << _("This is information you choose") << ":\n";
			cout << '\t' << Paint(_("Nickname"), sBold) << ": " << nick << '\n';
			cout << '\t' << Paint(_("Process owner"), sBold) << ": " << owner << '\n';
			cout << '\t' << Paint(_("Config path"), sBold) << ": " << path << '\n';

			if (AskYesNo(_("Do you want to add this settings?")))
				break;

			cout << _("Let's do it again") << "\n\n";
		}

		ofstream file(conf.c_str(), ios::out | ios::app);
		file << path << ':' << nick << ':' << owner << ':' << time(NULL) << '\n';
		file.close();

		if (!file) {
			cout << Paint("-- " + string(autosprintf(_("You don't have the write permission in '%s' to edit config file"), conf.c_str())), sRed) << endl;
			return 1;
		}

		cout << "-- " << autosprintf(_("Successfully added entry '%s' to config file"), path.c_str()) << endl;
		return 0;
	}

	const char *action = NULL;
	const bool del = ((cmd == "-d") || (cmd == "--del"));

	if ((cmd == "-r") || (cmd == "--run"))
		action = "--run";
	else if ((cmd == "-s") || (cmd == "--stop"))
		action = "--stop";
	else if ((cmd == "-q") || (cmd == "--restart"))
		action = "--restart";
	else if ((cmd == "-R") || (cmd == "--reload"))
		action = "--reload";

	if (!action && !del) {
		cout << Paint(autosprintf(_("Unknown option %s, try %s --help"), cmd.c_str(), mMultiName.c_str()), sRed) << endl;
		return 1;
	}

	string id((mArgs.size() > 1) ? mArgs[1] : "");

	if (id.empty() || ((id != "A") && (id.find_first_not_of("0123456789") != string::npos))) { // note: same as vhm script, list is shown when number is not given
		cout << "-- " << _("Choose hub from list:") << "\n\n";

		if (!PrintHubs(false))
			return 0;

		if (!del)
			cout << Paint("[A] ", sBold) << _("All hubs") << '\n';

		cout << '\n';
		id = AskHub();
	}

	LoadHubs(list);

	if (del) {
		const size_t num = size_t(StringAsLL(id));

		if ((id == "A") || !num || (num > list.size())) {
			cout << Paint("-- " + string(_("A error happens while trying to delete entry")), sRed) << endl;
			return 1;
		}

		const string conf(HubsFile());
		ifstream in(conf.c_str());
		ofstream back((conf + ".bak").c_str(), ios::out | ios::trunc); // note: same backup as sed of vhm script
		vector<string> lines;
		string line;
		size_t index = 0;
		sHub hub;

		while (getline(in, line)) {
			back << line << '\n';

			if (ParseHub(line, hub) && (++index == num)) // note: same numbering as list
				continue;

			lines.push_back(line);
		}

		in.close();
		back.close();
		ofstream out(conf.c_str(), ios::out | ios::trunc);

		for (const string &item: lines)
			out << item << '\n';

		out.close();

		if (!out) {
			cout << Paint("-- " + string(_("A error happens while trying to delete entry")), sRed) << endl;
			return 1;
		}

		cout << "-- " << _("Done") << endl;
		return 0;
	}

	if (id == "A") { // note: all hubs
		for (const sHub &hub: list)
			RunAs(hub, action);

		return 0;
	}

	const size_t num = size_t(StringAsLL(id));

	if (!num || (num > list.size())) {
		cout << Paint("-- " + string(_("Can't find selected entry")), sRed) << endl;
		return 1;
	}

	return (RunAs(list[num - 1], action) ? 0 : 1);
}

static bool SplitUrl(const string &url, string &host, int &port, string &path, bool &tls) // note: only http and https
{
	size_t pos;

	if (url.compare(0, 8, "https://") == 0) {
		tls = true;
		port = 443;
		pos = 8;
	} else if (url.compare(0, 7, "http://") == 0) {
		tls = false;
		port = 80;
		pos = 7;
	} else {
		return false;
	}

	const size_t end = url.find('/', pos);
	host = url.substr(pos, ((end == string::npos) ? string::npos : (end - pos)));
	path = ((end == string::npos) ? "/" : url.substr(end));
	const size_t colon = host.find(':');

	if (colon != string::npos) {
		port = atoi(host.c_str() + colon + 1);
		host.erase(colon);
	}

	return (host.size() && (port > 0) && (port <= 65535));
}

static bool Download(const string &url, string &data) // note: same as wget of vh script
{
	string host, path;
	int port = 0;
	bool tls = false;

	if (!SplitUrl(url, host, port, path, tls))
		return false;

	cHTTPConn conn(host, port, NULL, tls);
	return (conn.mGood && conn.Request("GET", path, "", "") && conn.ParseReply(data, 268435456)); // note: 256 mb, city database is biggest
}

static bool UnpackFile(const string &gz, const string &name, string &out) // note: same as tar of vh script, file with given name is taken from gzip compressed tar archive
{
	z_stream zs;
	memset(&zs, 0, sizeof(zs));

	if (inflateInit2(&zs, 16 + MAX_WBITS) != Z_OK) // note: gzip header
		return false;

	zs.next_in = (Bytef*)gz.data();
	zs.avail_in = uInt(gz.size());
	string tar;
	char buf[65536];
	int res;

	do {
		zs.next_out = (Bytef*)buf;
		zs.avail_out = sizeof(buf);
		res = inflate(&zs, Z_NO_FLUSH);

		if ((res != Z_OK) && (res != Z_STREAM_END)) {
			inflateEnd(&zs);
			return false;
		}

		tar.append(buf, sizeof(buf) - zs.avail_out);

		if (tar.size() > 1073741824UL) { // note: 1 gb, not a database
			inflateEnd(&zs);
			return false;
		}
	} while ((res != Z_STREAM_END) && (zs.avail_in || !zs.avail_out));

	inflateEnd(&zs);
	size_t pos = 0;

	while ((pos + 512) <= tar.size()) { // note: 512 byte header, then data rounded up to 512 bytes
		const char *head = tar.data() + pos;

		if (!head[0]) // note: end of archive
			break;

		const char *end = (const char*)memchr(head, '\0', 100);
		string file(head, (end ? size_t(end - head) : 100));
		const size_t size = strtoul(string(head + 124, 12).c_str(), NULL, 8);
		const char type = head[156];
		pos += 512;

		if ((pos + size) > tar.size())
			return false;

		const size_t slash = file.find_last_of('/');

		if (((type == '0') || (type == '\0')) && (file.substr((slash == string::npos) ? 0 : (slash + 1)) == name)) {
			tar.erase(pos + size); // note: no copy of big database
			tar.erase(0, pos);
			out.swap(tar);
			return true;
		}

		pos += (((size + 511) / 512) * 512);
	}

	return false;
}

int cCmdLine::GeoIP()
{
	if (mArgs.size() > 1) {
		cerr << Paint(_("Too many parameters."), sRed, true) << endl;
		return 1;
	}

#ifndef _WIN32
	if (geteuid() != 0) { // note: same as vh script, default folder is system folder
		cout << Paint(_("You can only run this command using administrator account."), sRedBold) << endl;
		return 1;
	}
#endif

	string path;

	if (mArgs.size()) { // note: we have path parameter
		path = StripSeparators(SlashPath(mArgs[0]));
	} else {
	#if defined(_WIN32)
		cout << Paint(_("Specify MaxMindDB path, same as mmdb_path in dbconfig."), sRedBold) << endl; // note: windows has no system folder for it
		return 1;
	#elif defined(HAVE_FREEBSD)
		path = "/usr/local/share/GeoIP"; // note: freebsd default, same as vh script
	#else
		path = "/usr/share/GeoIP"; // note: linux default, same as vh script
	#endif
	}

	if (!DirExists(path)) { // note: check if directory exists
		cout << Paint(autosprintf(_("Error accessing database directory: %s"), path.c_str()), sRedBold) << endl;
		return 1;
	}

	static const char *list[][3] = { // note: same databases and addresses as vh script
		{"Country", "https://ledo.feardc.net/mmdb/country.tar.gz", "GeoLite2-Country.mmdb"},
		{"City", "https://ledo.feardc.net/mmdb/city.tar.gz", "GeoLite2-City.mmdb"},
		{"ASN", "https://ledo.feardc.net/mmdb/asn.tar.gz", "GeoLite2-ASN.mmdb"}
	};

	for (const auto &item: list) {
		cout << Paint(autosprintf(_("Processing MaxMind %s database."), item[0]), sBold) << endl;
		string data, file;

		if (!Download(item[1], data)) {
			cout << Paint(autosprintf(_("Error downloading MaxMind %s database."), item[0]), sRedBold) << endl;
			return 1;
		}

		if (!UnpackFile(data, item[2], file)) {
			cout << Paint(autosprintf(_("Error decompressing MaxMind %s database."), item[0]), sRedBold) << endl;
			return 1;
		}

		data.clear();
		const string dest(path + '/' + item[2]), temp(dest + ".tmp"); // note: written next to old one, then replaced, so hub never reads half file
		ofstream out(temp.c_str(), ios::out | ios::binary | ios::trunc);

		if (out.is_open()) {
			out.write(file.data(), file.size());
			out.close();
		}

	#ifdef _WIN32
		if (out && FileExists(temp))
			remove(dest.c_str()); // note: windows rename does not replace
	#endif

		if (!out || (rename(temp.c_str(), dest.c_str()) != 0)) {
			remove(temp.c_str());
			cout << Paint(autosprintf(_("Error moving MaxMind %s database."), item[0]), sRedBold) << endl;
			return 1;
		}
	}

	cout << Paint(_("Done updating all free MaxMind databases, please use !reload command in your hubs."), sGreenBold) << endl;
	return 0;
}

int cCmdLine::AutoLoad()
{
	string action("list"), dir;

	for (size_t i = 0; i < mArgs.size(); i++) { // note: same options as vh script
		const string &arg = mArgs[i];

		if ((arg == "-l") || (arg == "--list")) {
			action = "list";
		} else if ((arg == "-n") || (arg == "--on")) {
			action = "on";
		} else if ((arg == "-f") || (arg == "--off")) {
			action = "off";
		} else if (((arg == "-d") || (arg == "--configdir")) && ((i + 1) < mArgs.size())) {
			dir = mArgs[++i];
		} else {
			cout << Paint(autosprintf(_("Unknown parameter %s"), arg.c_str()), sRed) << endl;
			return 1;
		}
	}

	if (!SetPath(dir))
		return 1;

#ifdef _WIN32
	const string ext(".dll"); // note: windows symbolic link needs administrator, so plugin is copied
#else
	const string ext(".so");
#endif

	vector<string> plugs;
	DIR *pdir = opendir(PLUGINDIR);

	if (pdir) {
		struct dirent *ent;

		while ((ent = readdir(pdir)) != NULL) {
			const string name(ent->d_name);

			if ((name.compare(0, 3, "lib") == 0) && (name.size() >= (ext.size() + 6)) && (name.compare(name.size() - ext.size() - 3, ext.size() + 3, "_pi" + ext) == 0) && FileExists(string(PLUGINDIR) + '/' + name))
				plugs.push_back(name);
		}

		closedir(pdir);
	}

	sort(plugs.begin(), plugs.end()); // note: same order as ls of vh script

	if (action == "list")
		cout << "-- " << _("This is a list of available plugins") << '\n';

	cout << '\n' << Paint(" ID  ", sBold) << Paint(' ' + string(_("Plugin path")), sBold) << '\n';
	const string conf(mConfigDir + "/plugins/");

	for (size_t i = 0; i < plugs.size(); i++) {
	#ifdef _WIN32
		const bool on = FileExists(conf + plugs[i]);
	#else
		struct stat st;
		const bool on = ((lstat((conf + plugs[i]).c_str(), &st) == 0) && S_ISLNK(st.st_mode));
	#endif

		cout << Paint(" [" + StringFrom(i + 1) + "] ", (on ? sGreenBold : sRedBold)) << PLUGINDIR << '/' << plugs[i] << '\n';
	}

	if (action == "list") {
		cout << '\n' << _("Status") << ": " << Paint(_("Start automatically"), sGreenBold) << " | " << Paint(_("Manually"), sRedBold) << "\n\n" << flush;
		return 0;
	}

	cout << endl;
	size_t num = 0;

	while (true) { // note: ask id
		const string ans(Ask(_("Select number")));

		if (ans.size() && (ans.find_first_not_of("0123456789") == string::npos)) {
			num = size_t(StringAsLL(ans));

			if (num && (num <= plugs.size()))
				break;
		}

		cout << Paint("-- " + string(_("Insert a ID from the list below")), sRed) << endl;
	}

	const string name(plugs[num - 1]), from(string(PLUGINDIR) + '/' + name), to(conf + name);

#ifdef _WIN32
	const bool on = FileExists(to);
#else
	struct stat st;
	const bool on = ((lstat(to.c_str(), &st) == 0) && S_ISLNK(st.st_mode));
#endif

	if (on) {
		if (action == "on") {
			cout << Paint("-- " + string(_("Plugin already starts automatically")), sRed) << endl;
		} else if (remove(to.c_str()) == 0) {
			cout << "-- " << _("Done") << endl;
		} else {
			cout << Paint("-- " + string(autosprintf(_("Cannot remove symbolic link in '%s'; please check permission"), conf.c_str())), sRed) << endl;
			return 1;
		}
	} else {
		if (action == "on") {
		#ifdef _WIN32
			const bool done = CopyOneFile(from, to);
		#else
			unlink(to.c_str());
			const bool done = (symlink(from.c_str(), to.c_str()) == 0);
		#endif

			if (done) {
				cout << "-- " << _("Done") << endl;
			} else {
				cout << Paint("-- " + string(autosprintf(_("Cannot create symbolic link in '%s'; please check permission"), conf.c_str())), sRed) << endl;
				return 1;
			}
		} else {
			cout << Paint("-- " + string(_("Plugin is already disabled from autoload")), sRed) << endl;
		}
	}

	return 0;
}

MYSQL* cCmdLine::RawConnect()
{
	mDBConf.reset(new cDBConf(mConfigDir + "/dbconfig"));
	MYSQL *handle = mysql_init(NULL);

	if (!handle)
		return NULL;

	const string &charset = (mDBConf->db_charset.size() ? mDBConf->db_charset : string(DEFAULT_CHARSET)); // note: same charset as hub, so data is written and read back unchanged

	if (charset.size())
		mysql_options(handle, MYSQL_SET_CHARSET_NAME, charset.c_str());

	if (!cMySQL::RealConnect(handle, mDBConf->db_host, mDBConf->db_user, mDBConf->db_pass, mDBConf->db_data, mDBConf->db_tls, mDBConf->db_socket)) {
		cerr << Paint(autosprintf(_("Failed connecting to MySQL/MariaDB server: %s"), mysql_error(handle)), sRed, true) << endl;
		mysql_close(handle);
		return NULL;
	}

	return handle;
}

static string Escape(MYSQL *handle, const char *data, unsigned long len) // note: same escaping as mysqldump, binary data is safe too
{
	string res;
	res.resize((len * 2) + 1);
	res.resize(mysql_real_escape_string(handle, &res[0], data, len));
	return res;
}

static string Ident(const string &name) // note: backtick in name is doubled
{
	string res("`");

	for (char c: name) {
		if (c == '`')
			res += '`';

		res += c;
	}

	res += '`';
	return res;
}

bool cCmdLine::DumpTable(MYSQL *handle, const string &name, ostream &os)
{
	string query("show create table " + Ident(name));

	if (mysql_query(handle, query.c_str()))
		return false;

	MYSQL_RES *res = mysql_store_result(handle);
	MYSQL_ROW row = (res ? mysql_fetch_row(res) : NULL);

	if (!row || !row[1]) {
		if (res)
			mysql_free_result(res);

		return false;
	}

	os << "\n--\n-- Table structure for table " << Ident(name) << "\n--\n\n";
	os << "DROP TABLE IF EXISTS " << Ident(name) << ";\n";
	os << row[1] << ";\n";
	mysql_free_result(res);
	query = "select * from " + Ident(name);

	if (mysql_query(handle, query.c_str()))
		return false;

	res = mysql_use_result(handle); // note: rows are read one by one, big table does not need memory

	if (!res)
		return false;

	const unsigned int cols = mysql_num_fields(res);
	MYSQL_FIELD *fields = mysql_fetch_fields(res);
	size_t size = 0;
	bool first = true, head = true;

	while ((row = mysql_fetch_row(res)) != NULL) {
		unsigned long *lens = mysql_fetch_lengths(res);

		if (head) {
			os << "\n--\n-- Dumping data for table " << Ident(name) << "\n--\n\n";
			head = false;
		}

		if (first) {
			os << "INSERT INTO " << Ident(name) << " VALUES ";
			size = 0;
		} else {
			os << ',';
		}

		ostringstream line;
		line << '(';

		for (unsigned int i = 0; i < cols; i++) {
			if (i)
				line << ',';

			if (!row[i])
				line << "NULL";
			else if (IS_NUM(fields[i].type) && (fields[i].type != MYSQL_TYPE_TIMESTAMP)) // note: numbers are written as they are
				line << string(row[i], lens[i]);
			else
				line << '\'' << Escape(handle, row[i], lens[i]) << '\'';
		}

		line << ')';
		const string data(line.str());
		os << data;
		size += data.size();
		first = (size > 1048576); // note: new statement after 1 mb, same as extended insert of mysqldump, server packet limit is not reached

		if (first)
			os << ";\n";
	}

	const bool done = !mysql_errno(handle);
	mysql_free_result(res);

	if (!first)
		os << ";\n";

	return done;
}

int cCmdLine::Backup()
{
	if (mArgs.size() > 2) {
		cerr << Paint(_("Too many parameters."), sRed, true) << endl;
		return 1;
	}

	const string dest(mArgs.size() ? StripSeparators(SlashPath(mArgs[0])) : "");

	if (dest.empty() || !DirExists(dest)) { // note: check destination folder
		cout << "-- " << _("Please provide a valid destination folder") << endl;
		return 1;
	}

	if (!FileWritable(dest)) { // note: check permission
		cout << Paint("-- " + string(autosprintf(_("You don't have the write permission in '%s'"), dest.c_str())), sRed) << endl;
		return 1;
	}

	if (!SetPath((mArgs.size() > 1) ? mArgs[1] : ""))
		return 1;

	MYSQL *handle = RawConnect();

	if (!handle)
		return 1;

	const string file(dest + '/' + StringFrom(time(NULL)) + ".sql"); // note: same name as vh script
	ofstream os(file.c_str(), ios::out | ios::binary | ios::trunc);
	bool done = os.is_open();

	if (done) {
		const char *charset = mysql_character_set_name(handle);
		os << "-- " << HUB_VERSION_NAME << ' ' << HUB_VERSION_VERS << " backup of database " << Ident(mDBConf->db_data) << ", made by verlihub --backup\n";
		os << "-- It can be imported with verlihub --restore or with mysql client\n\n";

		if (charset && charset[0])
			os << "/*!40101 SET NAMES " << charset << " */;\n";

		os << "/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;\n";
		vector<string> tables;
		done = !mysql_query(handle, "show full tables where Table_type = 'BASE TABLE'");
		MYSQL_RES *res = (done ? mysql_store_result(handle) : NULL);

		if (res) {
			MYSQL_ROW row;

			while ((row = mysql_fetch_row(res)) != NULL) {
				if (row[0])
					tables.push_back(row[0]);
			}

			mysql_free_result(res);
		} else {
			done = false;
		}

		for (size_t i = 0; done && (i < tables.size()); i++)
			done = DumpTable(handle, tables[i], os);

		os << "\n/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;\n";
		os.close();
		done = (done && os);
	}

	if (!done && mysql_errno(handle))
		cerr << Paint(autosprintf(_("MySQL server said: %s"), mysql_error(handle)), sRed, true) << endl;

	mysql_close(handle);

	if (!done) {
		cout << Paint("-- " + string(_("A error occured with backup")), sRed) << endl;
		remove(file.c_str());
		return 1;
	}

	cout << "-- " << autosprintf(_("Backup file '%s' successfully saved for '%s'"), file.c_str(), mConfigDir.c_str()) << endl;
	return 0;
}

static bool NextStatement(const string &data, size_t &pos, string &delim, string &out) // note: same as mysql client, comments are skipped, quotes and delimiter command are respected
{
	out.clear();
	bool line = true; // note: at start of line, delimiter command is only there

	while (pos < data.size()) {
		const char c = data[pos];

		if (line && out.find_first_not_of(" \t\r\n") == string::npos) {
			size_t start = data.find_first_not_of(" \t", pos);

			if ((start != string::npos) && (toLower(data.substr(start, 10)) == "delimiter ")) { // note: client command, it is not sent to server
				const size_t end = data.find('\n', start);
				string val(data.substr(start + 10, ((end == string::npos) ? string::npos : (end - start - 10))));
				const size_t first = val.find_first_not_of(" \t\r");

				if (first != string::npos) {
					val.erase(0, first);
					val.erase(val.find_last_not_of(" \t\r") + 1);
					delim = val;
				}

				pos = ((end == string::npos) ? data.size() : (end + 1));
				out.clear();
				continue;
			}
		}

		line = false;

		if ((c == '#') || ((c == '-') && (data.compare(pos, 2, "--") == 0) && (((pos + 2) >= data.size()) || isspace((unsigned char)data[pos + 2])))) { // note: comment until end of line
			const size_t end = data.find('\n', pos);
			pos = ((end == string::npos) ? data.size() : (end + 1));
			out += '\n';
			line = true;
			continue;
		}

		if ((c == '/') && (data.compare(pos, 2, "/*") == 0) && (data.compare(pos, 3, "/*!") != 0)) { // note: comment, but version comment is sent to server, it runs it
			const size_t end = data.find("*/", pos + 2);
			pos = ((end == string::npos) ? data.size() : (end + 2));
			out += ' ';
			continue;
		}

		if ((c == '\'') || (c == '"') || (c == '`')) { // note: quoted text, delimiter inside does not count
			out += c;
			pos++;

			while (pos < data.size()) {
				const char q = data[pos];
				out += q;
				pos++;

				if ((q == '\\') && (c != '`') && (pos < data.size())) { // note: escaped character
					out += data[pos];
					pos++;
				} else if (q == c) {
					if ((pos < data.size()) && (data[pos] == c)) { // note: doubled quote
						out += c;
						pos++;
					} else {
						break;
					}
				}
			}

			continue;
		}

		if (data.compare(pos, delim.size(), delim) == 0) { // note: end of statement
			pos += delim.size();
			const size_t first = out.find_first_not_of(" \t\r\n");

			if (first == string::npos) { // note: empty statement
				out.clear();
				continue;
			}

			return true;
		}

		out += c;

		if (c == '\n')
			line = true;

		pos++;
	}

	return (out.find_first_not_of(" \t\r\n") != string::npos); // note: last statement without delimiter
}

int cCmdLine::Restore()
{
	if (mArgs.size() > 2) {
		cerr << Paint(_("Too many parameters."), sRed, true) << endl;
		return 1;
	}

	const string file(mArgs.size() ? SlashPath(mArgs[0]) : "");

	if (file.empty() || !FileExists(file) || (file.size() < 5) || (toLower(file.substr(file.size() - 4)) != ".sql")) { // note: check sql file, same as vh script
		cout << Paint("-- " + string(_("Please provide a valid SQL file")), sRed) << endl;
		return 1;
	}

	if (!SetPath((mArgs.size() > 1) ? mArgs[1] : ""))
		return 1;

	ifstream in(file.c_str(), ios::in | ios::binary);
	string data((istreambuf_iterator<char>(in)), istreambuf_iterator<char>());
	in.close();
	MYSQL *handle = RawConnect();

	if (!handle)
		return 1;

	size_t pos = 0;
	string delim(";"), query;
	unsigned int count = 0;
	bool done = true;

	while (NextStatement(data, pos, delim, query)) {
		count++;

		if (mysql_real_query(handle, query.data(), (unsigned long)query.size())) {
			cerr << Paint(autosprintf(_("Statement %u failed: %s"), count, mysql_error(handle)), sRed, true) << endl;
			done = false;
			break;
		}

		do { // note: free result of statements that return rows
			MYSQL_RES *res = mysql_store_result(handle);

			if (res)
				mysql_free_result(res);
		} while (!mysql_next_result(handle));
	}

	mysql_close(handle);

	if (!done) {
		cout << Paint("-- " + string(_("A error occured with file importing")), sRed) << endl;
		return 1;
	}

	cout << "-- " << _("File imported successfully") << endl;
	return 0;
}

bool cCmdLine::SetPath(const string &dir)
{
	mConfigDir = (dir.size() ? SlashPath(dir) : FindConfigDir(NULL));

	if (!FileExists(mConfigDir + "/dbconfig")) {
		cerr << Paint(autosprintf(_("Folder is not valid configuration folder, it must contain dbconfig file: %s"), mConfigDir.c_str()), sRed, true) << endl;
		return false;
	}

	return true;
}

bool cCmdLine::Connect()
{
	mDBConf.reset(new cDBConf(mConfigDir + "/dbconfig"));

	try { // note: constructor connects and throws on failure, it already wrote error of database client
		mMySQL.reset(new cMySQL(mDBConf->db_host, mDBConf->db_user, mDBConf->db_pass, mDBConf->db_data, mDBConf->db_charset, mDBConf->db_tls, mDBConf->db_socket));
	} catch (const char *) {
		cerr << Paint(_("Failed to connect to database of hub."), sRed, true) << endl;
		return false;
	}

	return true;
}

bool cCmdLine::TableExists(const string &name)
{
	cQuery query(*mMySQL);
	query.OStream() << "show tables like '";
	cConfMySQL::WriteStringConstant(query.OStream(), name, true);
	query.OStream() << '\'';

	if (query.Query() <= 0)
		return false;

	const int res = query.StoreResult();
	query.Clear();
	return (res > 0);
}

bool cCmdLine::Execute(const string &text)
{
	cQuery query(*mMySQL);
	query.OStream() << text;
	const bool res = (query.Query() > 0);
	query.Clear();

	if (!res)
		cerr << Paint(_("Failed to execute database query."), sRed, true) << endl;

	return res;
}

bool cCmdLine::LoadVar(const string &var, string &val)
{
	cSetupList list(*mMySQL);
	cConfigItemBaseString item(val, var);
	return list.LoadItem(mDBConf->config_name.c_str(), &item);
}

bool cCmdLine::SaveVar(const string &var, const string &val)
{
	cSetupList list(*mMySQL);
	string data(val);
	cConfigItemBaseString item(data, var);
	list.SaveItem(mDBConf->config_name.c_str(), &item); // note: same as hub, it replaces old value
	return true;
}

bool cCmdLine::AskYesNo(const string &question)
{
	cout << question << " (y/N) " << flush;
	string ans;
	ReadLine(ans);
	return ((ans == "y") || (ans == "Y"));
}

string cCmdLine::Ask(const string &question, const string &def, bool empty)
{
	string ans;

	while (true) {
		cout << question;

		if (def.size())
			cout << " (" << def << ')';

		cout << ": " << flush;
		ReadLine(ans);

		if (ans.empty())
			ans = def;

		if (ans.size() || empty)
			return ans;
	}
}

string cCmdLine::AskPassword(const string &question, bool repeat)
{
	string pass, again;

	while (true) {
		cout << question << ": " << flush;
		ReadLine(pass, true);

		if (pass.empty())
			continue;

		if (!repeat)
			return pass;

		cout << _("Repeat password") << ": " << flush;
		ReadLine(again, true);

		if (pass == again)
			return pass;

		cout << _("Passwords do not match, let's do it again.") << endl;
	}
}

void cCmdLine::ReadLine(string &line, bool hidden)
{
	line.clear();
	bool quiet = false;

#ifdef _WIN32
	HANDLE in = GetStdHandle(STD_INPUT_HANDLE);
	DWORD mode = 0;

	if (hidden && GetConsoleMode(in, &mode) && SetConsoleMode(in, mode & ~ENABLE_ECHO_INPUT)) // note: not a console in msys2 terminal, there input is shown
		quiet = true;
#else
	if (hidden && isatty(STDIN_FILENO) && (tcgetattr(STDIN_FILENO, &sTermSaved) == 0)) {
		struct termios term = sTermSaved;
		term.c_lflag &= ~ECHO;
		sTermHidden = 1;
		signal(SIGINT, TermRestore);
		quiet = (tcsetattr(STDIN_FILENO, TCSANOW, &term) == 0);
	}
#endif

	const bool res = bool(getline(cin, line));

	if (quiet) {
	#ifdef _WIN32
		SetConsoleMode(in, mode);
	#else
		tcsetattr(STDIN_FILENO, TCSANOW, &sTermSaved);
	#endif

		cout << endl; // note: enter was not shown
	}

#ifndef _WIN32
	if (sTermHidden) {
		sTermHidden = 0;
		signal(SIGINT, SIG_DFL);
	}
#endif

	if (!res) { // note: input ended, questions would repeat forever
		cerr << endl << _("Input ended, exiting.") << endl;
		exit(1);
	}

	if (line.size() && (line[line.size() - 1] == '\r')) // note: windows line end from pipe
		line.erase(line.size() - 1);

	if (!hidden) { // note: same as read of shell, spaces around answer are removed, password stays exactly as typed
		const size_t first = line.find_first_not_of(" \t");

		if (first == string::npos) {
			line.clear();
		} else {
			line.erase(0, first);
			line.erase(line.find_last_not_of(" \t") + 1);
		}
	}
}

string cCmdLine::TryConnect(const string &host, const string &user, const string &pass, const string &data, const string &sock)
{
	MYSQL *handle = mysql_init(NULL);

	if (!handle)
		return _("Unable to initialize MySQL structure");

	unsigned int wait = 5; // note: same as vh script
	mysql_options(handle, MYSQL_OPT_CONNECT_TIMEOUT, &wait);
	string res;

	if (!cMySQL::RealConnect(handle, host, user, pass, data, false, sock))
		res = mysql_error(handle);

	mysql_close(handle);
	return res;
}

bool cCmdLine::AdminExecute(const string &host, const string &user, const string &pass, const vector<string> &list, const string &sock)
{
	MYSQL *handle = mysql_init(NULL);

	if (!handle)
		return false;

	unsigned int wait = 5;
	mysql_options(handle, MYSQL_OPT_CONNECT_TIMEOUT, &wait);
	bool res = cMySQL::RealConnect(handle, host, user, pass, "", false, sock);

	if (!res)
		cerr << Paint(autosprintf(_("Failed to connect as administrator: %s"), mysql_error(handle)), sRed, true) << endl;

	for (size_t i = 0; res && (i < list.size()); i++) {
		if (mysql_query(handle, list[i].c_str())) {
			cerr << Paint(autosprintf(_("Failed to execute database query: %s"), mysql_error(handle)), sRed, true) << endl;
			res = false;
		}
	}

	mysql_close(handle);
	return res;
}

string cCmdLine::FindSocket(const string &host, const string &user, const string &pass)
{
	string res;

#ifndef _WIN32 // note: windows server uses tcp or named pipe
	MYSQL *handle = mysql_init(NULL);

	if (!handle)
		return res;

	unsigned int wait = 5;
	mysql_options(handle, MYSQL_OPT_CONNECT_TIMEOUT, &wait);
	unsigned int code = 0;

	if (cMySQL::RealConnect(handle, host, user, pass, "", false)) { // note: default path of client library works, server tells its own path, hub uses same one
		if (!mysql_query(handle, "select @@socket")) {
			MYSQL_RES *rows = mysql_store_result(handle);
			MYSQL_ROW row = (rows ? mysql_fetch_row(rows) : NULL);

			if (row && row[0])
				res = row[0];

			if (rows)
				mysql_free_result(rows);
		}
	} else {
		code = mysql_errno(handle);
	}

	mysql_close(handle);

	if (code == 2002) { // note: cant connect through default socket of client library, server has it elsewhere, try usual places
		static const char *list[] = {"/run/mysqld/mysqld.sock", "/var/run/mysqld/mysqld.sock", "/var/lib/mysql/mysql.sock", "/var/run/mysql/mysql.sock", "/tmp/mysql.sock", "/tmp/mysqld.sock"};
		struct stat st;

		for (const char *item: list) {
			if ((stat(item, &st) == 0) && S_ISSOCK(st.st_mode) && TryConnect(host, user, pass, "", item).find("socket") == string::npos) { // note: account error is fine here, socket itself works
				res = item;
				break;
			}
		}
	}
#else
	(void)host;
	(void)user;
	(void)pass;
#endif

	return res;
}

bool cCmdLine::ParsePort(const string &str, unsigned int &port)
{
	if (str.empty() || (str.size() > 5) || (str.find_first_not_of("0123456789") != string::npos))
		return false;

	const unsigned long val = strtoul(str.c_str(), NULL, 10);

	if (!val || (val > 65535))
		return false;

	port = val;
	return true;
}

}; // namespace nVerliHub
