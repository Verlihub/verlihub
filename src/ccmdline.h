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

#ifndef NVERLIHUB_CCMDLINE_H
#define NVERLIHUB_CCMDLINE_H

#include <string>
#include <vector>
#include <memory>
#include <ostream>
#include <mysql/mysql.h> // note: mysql names its struct differently than mariadb, so it can not be declared here

using namespace std;

namespace nVerliHub {
	namespace nMySQL {
		class cMySQL;
	};

	namespace nConfig {
		class cDBConf;
	};

/*
	command line of verlihub executable, it replaces shell scripts step by step
	note: command must be first argument, same as in vh script, otherwise hub starts with its usual options
*/

class cCmdLine
{
	public:
		// run command when first argument is one, returns false when hub should start instead, result is exit code
		static bool Run(int argc, char *argv[], int &result);

		// find configuration folder same way for hub and commands, dir is from -d option or null
		static string FindConfigDir(const char *dir);
		// name of event that stops or reloads hub of configuration folder, windows has no signals
		static string EventName(const string &dir, const char *what);

	private:
		cCmdLine(int argc, char *argv[], int first = 2); // note: first argument after command

		// hub in list of multiple hubs, same as line of vhm script
		struct sHub {
			string mPath;
			string mNick;
			string mOwner;
			long mTime = 0;
		};
		~cCmdLine();

		// commands, each returns exit code
		int Help();
		int Version();
		int GetConf();
		int SetConf();
		int ModPort();
		int AddUser();
		int Clean();
		int UpHelp();
		int Install();
		int HubRun();
		int HubStop();
		int HubRestart();
		int HubReload();
		int HubStatus();
		int GeoIP();
		int AutoLoad();
		int Backup();
		int Restore();

		// connect to hub database with plain client handle, it is needed for rows with lengths and escaping, it reports error itself
		MYSQL* RawConnect();
		// write structure and data of one table as sql statements, same as mysqldump
		static bool DumpTable(MYSQL *handle, const string &name, ostream &os);
	#ifdef _WIN32
		int Watchdog();
	#endif

		// manage multiple hubs, same as vhm script
		int Multi();
		int MultiHelp();
		// file with list of hubs
		static string HubsFile();
		// parse one line of hub list
		static bool ParseHub(string line, sHub &hub);
		// load list of hubs, returns false when file does not exist
		static bool LoadHubs(vector<sHub> &list);
		// print list of hubs with their status
		bool PrintHubs(bool legend);
		// ask for number of hub or A for all
		static string AskHub();
		// run command for hub as its process owner, same as su of vhm script
		bool RunAs(const sHub &hub, const char *action);

		// start hub in background with watchdog, same as run of vh script
		bool StartHub();
		// stop hub and wait until it ends
		bool KillHub();
		// refresh plugin links or copies in configuration folder
		void CheckPlugins();
		// raise open files limit by user limit of hub
		void RaiseFileLimit();
		// run hub and restart it when it crashes or asks for restart, same as vh_daemon script, returns exit code
		static int Watch(const string &exe, const string &dir);
		// full path of this executable
		string ExePath() const;
		// pid of watchdog from pid file in configuration folder
		static bool ReadPid(const string &dir, long &pid);
		// check that process exists and is verlihub
		static bool IsRunning(long pid);
		// wait until process ends, returns false on timeout
		static bool WaitExit(long pid, unsigned int secs);

		// check that nick can be registered, it reports error itself
		bool CheckNewUser(const string &nick);
		// register user with password hash made same way as hub does
		bool InsertUser(const string &nick, const string &pass, int clas);
		// create all tables and default data by creating hub once without listening, its log goes to install_log
		bool CreateTables();

		// use given configuration folder or find default one, it must contain dbconfig
		bool SetPath(const string &dir);
		// load dbconfig and connect to database of hub
		bool Connect();
		// check that table exists in hub database
		bool TableExists(const string &name);
		// run query that returns nothing, it reports error itself
		bool Execute(const string &query);
		// load and save variable of hub configuration
		bool LoadVar(const string &var, string &val);
		bool SaveVar(const string &var, const string &val);

		// ask question on console and return true for yes
		static bool AskYesNo(const string &question);
		// ask question on console, empty answer gives default value, empty value is allowed only when told so
		static string Ask(const string &question, const string &def = "", bool empty = false);
		// ask password without showing it, optionally twice
		static string AskPassword(const string &question, bool repeat = false);
		// ask for database administrator credentials, same as vh script
		static void AskAdmin(string &user, string &pass);
		// read line from console, optionally without showing it, process exits when input ends
		static void ReadLine(string &line, bool hidden = false);
		// connect to database server, it returns error text or empty string on success, empty database checks account only
		static string TryConnect(const string &host, const string &user, const string &pass, const string &data, const string &sock = "");
		// run queries on database server as administrator, it reports error itself
		static bool AdminExecute(const string &host, const string &user, const string &pass, const vector<string> &list, const string &sock = "");
		// find local socket of database server, same as _mysql_export_socket of vh script, empty when default of client library is used
		static string FindSocket(const string &host, const string &user, const string &pass);
		// parse port number in range 1 - 65535
		static bool ParsePort(const string &str, unsigned int &port);

		vector<string> mArgs; // note: arguments after command
		string mExecPath; // note: path of executable, hub needs it when installation creates tables
		string mMultiName; // note: vhm or verlihub -m, shown in messages
		string mConfigDir;
		unique_ptr<nConfig::cDBConf> mDBConf;
		unique_ptr<nMySQL::cMySQL> mMySQL;
};

}; // namespace nVerliHub

#endif
