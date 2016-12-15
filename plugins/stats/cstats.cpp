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

#include <config.h>
#include "cstats.h"
#include <stdlib.h>
#include "src/ctime.h"

namespace nVerliHub {
	//using namespace nUtils;
	using namespace nSocket;
	using namespace nConfig;
	using namespace nTables;
	namespace nStatsPlugin {

cStats::cStats(cServerDC *server): cConfMySQL(server->mMySQL), mS(server)
{
	AddFields();
}


cStats::~cStats()
{}

void cStats::CleanUp()
{
	mQuery.Clear();
	mQuery.OStream() << "delete from " << mMySQLTable.mName << " where("
		"realtime < " << cTime().Sec() - 7 * 3600* 24 <<
		")";
	mQuery.Query();
	mQuery.Clear();
}

void cStats::AddFields()
{
	mMySQLTable.mName = "pi_stats";
	int i;
	ostringstream field_name;
	AddCol("realtime","int(11)","",false, mTime);
	AddPrimaryKey("realtime");
	AddCol("uptime", "int(11)","",true,mUptimeSec);
	AddCol("users_total","int(11)","0",true, mS->mUserCountTot);
	for(i=0; i <= USER_ZONES; i++) {
		field_name.str("");
		field_name << "users_zone" << i;
		AddCol(field_name.str().data(),"int(11)","0",true, mS->mUserCount[i]);
	}
	AddCol("upload_total","double","0",true, mUploadTotalBps);
	for(i=0; i <= USER_ZONES; i++) {
		field_name.str("");
		field_name << "upload_zone" << i;
		AddCol (field_name.str().data(),"double","0",true, mUploadInZonesBps[i]);
	}
	AddCol("share_total_gb","int(11)","0",true, mShareTotalGB);
	AddCol("freq_search_active","double","0",true, mFreqSearchA);
	AddCol("freq_search_passive","double","0",true, mFreqSearchP);
	AddCol("freq_user_login","double","0",true, mFreqUserLogin);
	AddCol("freq_user_logout","double","0",true, mFreqUserLogout);
	mMySQLTable.mExtra ="PRIMARY KEY (realtime)";

	SetBaseTo(this);
}
	}; // namespace nStatsPlugin
}; // namespace nVerliHub
