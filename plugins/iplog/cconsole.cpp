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

#include "src/cconndc.h"
#include "cconsole.h"
#include "cpiiplog.h"
#include "ciplog.h"

namespace nVerliHub {
	using namespace nSocket;
	using namespace nEnums;
	namespace nIPLogPlugin {

cConsole::cConsole(cpiIPLog *iplog) :
	mIPLog(iplog),
	mCmdLastIp(eIL_LASTIP,"!last(ip|nick|range) ", "(\\S+)( \\d+)?", &mcfLastIp),
	mCmdHistoryOf(eIL_HISTORY,"!(ip|nick|range)(history|log) ", "(\\S+)( \\d+)?", &mcfHistoryOf),
	mCmdr(this)
{
	mCmdr.Add(&mCmdLastIp);
	mCmdr.Add(&mCmdHistoryOf);
}


cConsole::~cConsole()
{}

int cConsole::DoCommand(const string &str, cConnDC * conn)
{
	ostringstream os;
	if(mCmdr.ParseAll(str, os, conn) >= 0)
	{
		mIPLog->mServer->DCPublicHS(os.str().data(),conn);
		return 1;
	}
	return 0;
}

bool cConsole::cfLastIp::operator ( )()
{
	string who;
	int count = 10;
	enum {eIP_ALL,eIP_WHO, eIP_COUNT};
	enum {eLAST_IP,eLAST_NICK, eLAST_RANGE};
	const char *ids[]={"ip","nick","range", NULL};
	const int ids_nums[]={eLAST_IP,eLAST_NICK, eLAST_RANGE};
	int id;

	GetIDEnum(1,id, ids, ids_nums);

	GetParStr(eIP_WHO,who);
	GetParInt(eIP_COUNT,count);

	unsigned long range_min,range_max;
	switch(id) {
		case eLAST_IP:
			GetPI()->mIPLog->GetLastIP(who, count, *mOS); break;
		case eLAST_NICK:
			GetPI()->mIPLog->GetLastNick(who, count, *mOS); break;
		case eLAST_RANGE:
			GetParIPRange(eIP_WHO,range_min,range_max);
		default:break;
	}
	return true;
}

bool cConsole::cfHistoryOf::operator ( )()
{
	string who;
	int count = 10;
	enum {eHI_ALL,eHI_WHO, eHI_COUNT};
	enum {eHIST_IP,eHIST_NICK};
	const char *ids[]={"ip","nick", NULL};
	const int ids_nums[]={eHIST_IP,eHIST_NICK};
	int id=-1;

	GetIDEnum(1,id, ids, ids_nums);

	GetParStr(eHI_WHO,who);
	GetParInt(eHI_COUNT,count);
	switch(id) {
		case eHIST_IP:
			GetPI()->mIPLog->GetIPHistory(who, count, *mOS); break;
		case eHIST_NICK:
			GetPI()->mIPLog->GetNickHistory(who, count, *mOS); break;
		default: break;
	}

	return true;
}

	}; // namespace nIPLogPlugin
}; // namespace nVerliHub
