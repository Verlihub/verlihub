/**************************************************************************
*   Copyright (C) 2011-2013 by Shurik                                     *
*   shurik at sbin.ru                                                     *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
*   This program is distributed in the hope that it will be useful,       *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
*   GNU General Public License for more details.                          *
*                                                                         *
*   You should have received a copy of the GNU General Public License     *
*   along with this program; if not, write to the                         *
*   Free Software Foundation, Inc.,                                       *
*   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
***************************************************************************/

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
