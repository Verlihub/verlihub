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

bool Ban(char *nick, char *op, char *reason, unsigned howlong, unsigned bantype) {
	return Ban(nick,string(op),string(reason),howlong,bantype);
}

bool SendDataToAll(char *data, int min_class, int max_class) {
	return SendToClass(data, min_class, max_class);
}

MODULE = vh		PACKAGE = vh		

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

char *
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

char *
GetUserHost(nick)
	char *	nick

char *
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
	char *	configname
	char *	variable
	char *	value

char *
GetConfig(configname, variable)
	char *	configname
	char *	variable
PPCODE:
	char * val = new char[64];
	int size = GetConfig(configname, variable, val, 64);
	if (size<0) {
		croak("Error calling GetConfig");
		delete [] val;
		XSRETURN_UNDEF;
	}
	if(size >= 63) {
		delete [] val;
		val = new char[size+1];
		int size = GetConfig(configname, variable, val, size+1);
	}
	XPUSHs(sv_2mortal(newSVpv(val, size)));
	delete [] val;

int
GetUsersCount()

char *
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

char *
GetHubIp()
PPCODE:
	char * addr = GetHubIp();
	XPUSHs(sv_2mortal(newSVpv(addr, strlen(addr))));

char *
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
StopHub(code)
	int code

const char *
GetTopic()

bool
SetTopic(topic)
	char * topic

char *
GetIPCC(ip)
	char * ip

char *
GetIPCN(ip)
	char * ip

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
