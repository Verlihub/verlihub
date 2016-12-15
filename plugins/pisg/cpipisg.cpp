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

#include "cpipisg.h"
#include "src/cserverdc.h"
#include <time.h>

using namespace nVerliHub;

cpiPisg::cpiPisg() : 
	mStats(NULL),
	mStatsTimer(300.0,0.0,cTime().Sec()),
	mFreqSearchA(cTime(), 300.0, 10),
	mFreqSearchP(cTime(), 300.0, 10)
{
	mName = "Pisg";
	mVersion = PISG_VERSION;
}

cpiPisg::~cpiPisg()
{
	logFile.close();
}


void cpiPisg::OnLoad(cServerDC *server)
{
	cVHPlugin::OnLoad(server);
	mServer = server;
	logFile.open(server->mConfigBaseDir + "/" + "pisg.log");
}

bool cpiPisg::RegisterAll()
{
	RegisterCallBack("VH_OnParsedMsgChat");
	return true;
}

bool cpiPisg::OnParsedMsgChat(cConnDC *conn, cMessageDC *msg)
{
	if(logFile.is_open && conn != NULL && conn->mpUser != NULL && msg != NULL) {
		 time_t rawtime;
		struct tm * timeinfo;
		time(&rawtime);
		timeinfo = localtime (&rawtime);
		logFile << "[[" << (timeinfo->tm_year + 1900) << "%s-%s-%s|%s:%s]] <" << conn->mpUser->mNick.c_str() << "> " << msg->ChunkString(eCH_CH_MSG).c_str() << endl;
	//date =  os.date ("*t") io.write("[["..date.year.."-"..formatTF(date.month).."-"..formatTF(date.day).."|"..formatTF(date.hour)..":"..formatTF(date.min).."]] <"..nick.."> "..data..lf)
	}
	return true;
}

REGISTER_PLUGIN(cpiPisg);
