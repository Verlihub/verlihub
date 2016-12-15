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

#include "cpistats.h"

namespace nVerliHub {
	using namespace nSocket;
	using namespace nEnums;
	namespace nStatsPlugin {
cpiStats::cpiStats() :
	mStats(NULL),
	mStatsTimer(300.0,0.0,cTime().Sec()),
	mFreqSearchA(cTime(), 300.0, 10),
	mFreqSearchP(cTime(), 300.0, 10)
{
	mName = "Stats";
	mVersion = STATS_VERSION;
}

void cpiStats::OnLoad(cServerDC *server)
{
	cVHPlugin::OnLoad(server);
	mServer = server;
	mStats = new cStats(server);
	mStats->CreateTable();
}

bool cpiStats::RegisterAll()
{
	RegisterCallBack("VH_OnUserCommand");
	RegisterCallBack("VH_OnTimer");
	RegisterCallBack("VH_OnParsedMsgSearch");
	return true;
}

bool cpiStats::OnTimer(__int64 msec)
{
	if(mStatsTimer.Check(this->mServer->mTime , 1) == 0)  {
		this->mStats->mTime = this->mServer->mTime.Sec();
		int i = 0;
		mStats->mUploadTotalBps = 0;
		double curr;
		for(i = 0; i <= USER_ZONES; i++) {
			//cout << "Zone " << i << cTime().AsDate() << endl;
			//mServer->mUploadZone[i].Dump();
			curr = mServer->mUploadZone[i].GetMean(mServer->mTime);
			//mServer->mUploadZone[i].Dump();
			mStats->mUploadInZonesBps[i] = curr;
			mStats->mUploadTotalBps += curr;
		}
		// calculate total share in GB
		mStats->mShareTotalGB = mServer->mTotalShare /(1024*1024*1024);
		// calculate the uptime
		cTime theUpTime(mServer->mTime);
		theUpTime -= mServer->mStartTime;
		mStats->mUptimeSec = theUpTime.Sec();
		// calculate search frequencys
		mStats->mFreqSearchA = this->mFreqSearchA.GetMean(mServer->mTime);
		mStats->mFreqSearchP = this->mFreqSearchP.GetMean(mServer->mTime);
		// save and clean
		this->mStats->Save();
		this->mStats->CleanUp();
	}
	return true;
}

bool cpiStats::OnParsedMsgSearch(cConnDC *, cMessageDC *msg)
{
	switch(msg->mType) {
		case eDC_MSEARCH:
		case eDC_SEARCH:
			mFreqSearchA.Insert(cTime());
			break;
		case eDC_MSEARCH_PAS:
		case eDC_SEARCH_PAS:
			mFreqSearchP.Insert(cTime());
			break;
		default:break;
	}
	return true;
}

cpiStats::~cpiStats()
{
	if (mStats)
		delete mStats;
}
	}; // namespace nStatsPlugin
}; // namespace nVerliHub
REGISTER_PLUGIN(nVerliHub::nStatsPlugin::cpiStats);
