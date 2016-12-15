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

#ifndef CALLBACKS_H
#define CALLBACKS_H

namespace nVerliHub {
namespace nPerlPlugin {
namespace nCallback {

int SQLQuery(const char *query);
MYSQL_ROW SQLFetch(int r, int &cols);
int SQLFree();
int IsUserOnline(const char *nick);
int IsBot(const char *nick);
int GetUpTime();
const char *GetHubIp();
const char *GetHubSecAlias();
const char *GetOPList();
const char *GetBotList();
bool RegBot(const char *nick, int uclass, const char *desc, const char *speed, const char *email, const char *share);
bool EditBot(const char *nick, int uclass, const char *desc, const char *speed, const char *email, const char *share);
bool UnRegBot(const char *nick);
bool SetTopic(const char *topic);
const char *GetTopic();
bool InUserSupports(const char *nick, const char *flag);
bool ReportUser(const char *nick, const char *msg);
bool ScriptCommand(const char *, const char *);

} // nVerliHub
} // nPerlPlugin
} // nCallback

#endif
