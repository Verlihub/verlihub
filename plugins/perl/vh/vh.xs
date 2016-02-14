=head1 COPYRIGHT AND LICENSE

	Copyright (C) 2003-2005 Daniel Muller, dan at verliba dot cz
	Copyright (C) 2006-2016 Verlihub Team, info at verlihub dot net

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

bool Ban(char *nick, char *op, char *reason, unsigned howlong, unsigned bantype) {
	return Ban(nick,string(op),string(reason),howlong,bantype);
}

bool SendDataToAll(char *data, int min_class, int max_class) {
	return SendToClass(data, min_class, max_class);
}

MODULE = vh		PACKAGE = vh		

PROTOTYPES: DISABLE

INCLUDE: const-xs.inc

bool
Ban(nick, op, reason, howlong, bantype)
	char *	nick
	char *  op
	char *  reason
	unsigned	howlong
	int	bantype

bool
CloseConnection(nick)
	char *	nick

char *
GetUserCC(nick)
	char * nick

const char *
GetMyINFO(nick)
	char *	nick

bool
AddRegUser(nick, uclass, passwd, op)
	char * nick
	int uclass
	char * passwd
	char * op

bool
DelRegUser(nick)
	char * nick

bool RegBot(nick, uclass, desc, speed, email, share)
	char *	nick
	int	uclass
	char *	desc
	char *	speed
	char *	email
	char *	share

bool EditBot(nick, uclass, desc, speed, email, share)
	char *	nick
	int	uclass
	char *	desc
	char *	speed
	char *	email
	char *	share

bool UnRegBot(nick)
	char *	nick

int
GetUserClass(nick)
	char *	nick

const char *
GetUserHost(nick)
	char *	nick

const char *
GetUserIP(nick)
	char *	nick

bool
ParseCommand(command_line, nick, pm)
	char *	command_line
	char *	nick
	int	pm

bool
SendToClass(data, min_class, max_class)
	char *	data
	int	min_class
	int	max_class

bool
SendToAll(data)
	char *	data

bool
SendPMToAll(data, from, min_class, max_class)
	char *	data
	char *	from
	int	min_class
	int	max_class

bool
KickUser(op, nick, reason)
	char *	op
	char *	nick
	char *	reason

bool
SendDataToUser(data, nick)
	char *	data
	char *	nick

bool
SetConfig(configname, variable, value)
	const char *	configname
	const char *	variable
	const char *	value

const char *
GetConfig(configname, variable)
	const char *	configname
	const char *	variable
PPCODE:
	const char * val = GetConfig(configname, variable);
	if (!val) {
		croak("Error calling GetConfig");
		XSRETURN_UNDEF;
	}
	XPUSHs(sv_2mortal(newSVpv(val, strlen(val))));

int
GetUsersCount()

const char *
GetNickList()

char *
GetOPList()

char *
GetBotList()

double
GetTotalShareSize()

char *
GetVHCfgDir()

int
GetUpTime()

int
IsBot(nick)
	char *  nick

int
IsUserOnline(nick)
	char *  nick

void
GetHubIp()
PPCODE:
	char * addr = GetHubIp();
	XPUSHs(sv_2mortal(newSVpv(addr, strlen(addr))));

void
GetHubSecAlias()
PPCODE:
	char * hubsec = GetHubSecAlias();
	XPUSHs(sv_2mortal(newSVpv(hubsec, strlen(hubsec))));

int
SQLQuery(query)
	char *	query

void
SQLFetch(r)
	int	r
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
	unsigned delay

const char *
GetTopic()

bool
SetTopic(topic)
	char * topic

#ifdef HAVE_LIBGEOIP

string
GetIPCC(ip)
	char * ip

string
GetIPCN(ip)
	char * ip

#endif

bool
InUserSupports(nick, flag)
	char * nick
	char * flag

bool
ReportUser(nick, msg)
	char * nick
	char * msg

bool
SendToOpChat(msg)
	char * msg

bool
ScriptCommand(cmd, data)
	char * cmd
	char * data
