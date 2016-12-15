=head1 COPYRIGHT AND LICENSE

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

=cut

#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#include "ppport.h"

#undef do_open
#undef do_close

#include "const-c.inc"

#include "src/script_api.h"
#include "src/cmysql.h"
#include "plugins/perl/callbacks.h"

using namespace nVerliHub;
using namespace nVerliHub::nPerlPlugin::nCallback;
using namespace std;

bool Ban(const char *nick, const char *op, const char *reason, unsigned howlong, unsigned bantype) {
	return Ban(nick, string(op), string(reason), howlong, bantype);
}

bool SendDataToAll(const char *data, int min_class, int max_class) {
	return SendToClass(data, min_class, max_class);
}

MODULE = vh    PACKAGE = vh

PROTOTYPES: DISABLE

INCLUDE: const-xs.inc

bool
Ban(nick, op, reason, howlong, bantype)
	const char *nick
	const char *op
	const char *reason
	unsigned howlong
	int bantype

bool
CloseConnection(nick)
	const char *nick

const char *
GetUserCC(nick)
	const char *nick

const char *
GetMyINFO(nick)
	const char *nick

bool
AddRegUser(nick, uclass, passwd, op)
	const char *nick
	int uclass
	const char *passwd
	const char *op

bool
DelRegUser(nick)
	const char *nick

bool RegBot(nick, uclass, desc, speed, email, share)
	const char *nick
	int uclass
	const char *desc
	const char *speed
	const char *email
	const char *share

bool EditBot(nick, uclass, desc, speed, email, share)
	const char *nick
	int uclass
	const char *desc
	const char *speed
	const char *email
	const char *share

bool UnRegBot(nick)
	const char *nick

int
GetUserClass(nick)
	const char *nick

const char *
GetUserHost(nick)
	const char *nick

const char *
GetUserIP(nick)
	const char *nick

bool
ParseCommand(command_line, nick, pm)
	const char *command_line
	const char *nick
	int pm

bool
SendToClass(data, min_class, max_class)
	const char *data
	int min_class
	int max_class

bool
SendToAll(data)
	const char *data

bool
SendPMToAll(data, from, min_class, max_class)
	const char *data
	const char *from
	int min_class
	int max_class

bool
KickUser(op, nick, reason)
	const char *op
	const char *nick
	const char *reason

bool
SendDataToUser(data, nick)
	const char *data
	const char *nick

bool
SetConfig(configname, variable, value)
	const char *configname
	const char *variable
	const char *value

const char *
GetConfig(configname, variable)
	const char *configname
	const char *variable
PPCODE:
	char *val = GetConfig(configname, variable);
	if (!val) {
		croak("Error calling GetConfig");
		XSRETURN_UNDEF;
	}
	XPUSHs(sv_2mortal(newSVpv(val, strlen(val))));
	free((void *)val);

int
GetUsersCount()

const char *
GetNickList()

const char *
GetOPList()

const char *
GetBotList()

double
GetTotalShareSize()

const char *
GetVHCfgDir()

int
GetUpTime()

int
IsBot(nick)
	const char *nick

int
IsUserOnline(nick)
	const char *nick

void
GetHubIp()
PPCODE:
	const char *addr = GetHubIp();
	XPUSHs(sv_2mortal(newSVpv(addr, strlen(addr))));

void
GetHubSecAlias()
PPCODE:
	const char *hubsec = GetHubSecAlias();
	XPUSHs(sv_2mortal(newSVpv(hubsec, strlen(hubsec))));

int
SQLQuery(query)
	const char *query

void
SQLFetch(r)
	int r
PPCODE:
	int cols;
	MYSQL_ROW row = SQLFetch(r,cols);
	if(!row) {
		croak("Error fetching row");
		XSRETURN_UNDEF;
	}
	for(int i=0; i<cols; i++)
		if(row[i])
			XPUSHs(sv_2mortal(newSVpv(row[i],strlen(row[i]))));
		else
			XPUSHs(&PL_sv_undef);

int
SQLFree()

bool
StopHub(code, delay)
	int code
	int delay

const char *
GetTopic()

bool
SetTopic(topic)
	const char *topic

#ifdef HAVE_LIBGEOIP

string
GetIPCC(ip)
	const char *ip

string
GetIPCN(ip)
	const char *ip

#endif

bool
InUserSupports(nick, flag)
	const char *nick
	const char *flag

bool
ReportUser(nick, msg)
	const char *nick
	const char *msg

bool
SendToOpChat(msg)
	const char *msg

bool
ScriptCommand(cmd, data)
	const char *cmd
	const char *data
