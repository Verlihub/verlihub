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

namespace nVerliHub {
namespace nPerlPlugin {
namespace nCallback {

int SQLQuery(char *query);
MYSQL_ROW SQLFetch(int r, int &cols);
int SQLFree();
int IsUserOnline(char *nick);
int IsBot(char *nick);
int GetUpTime();
char *GetHubIp();
char *GetHubSecAlias();
char *GetOPList();
char *GetBotList();
bool RegBot(char *nick, int uclass, char *desc, char *speed, char *email, char *share);
bool EditBot(char *nick, int uclass, char *desc, char *speed, char *email, char *share);
bool UnRegBot(char *nick);
bool SetTopic(char *topic);
const char *GetTopic();
bool InUserSupports(char *nick, char *flag);
bool ReportUser(char *nick, char *msg);
bool ScriptCommand(char *, char *);

} // nVerliHub
} // nPerlPlugin
} // nCallback
