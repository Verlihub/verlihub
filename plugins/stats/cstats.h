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

#ifndef CMGSLIST_H
#define CMGSLIST_H
#include "src/cserverdc.h"
#include "src/cconfmysql.h"

namespace nVerliHub {
	namespace nStatsPlugin {
/**
@author Daniel Muller
*/
class cStats : public nConfig::cConfMySQL
{
public:
	time_t mTime;
	double mUploadTotalBps;
	double mUploadInZonesBps[USER_ZONES+1];
	long mShareTotalGB;
	long mUptimeSec;
	double mFreqSearchA;
	double mFreqSearchP;
	double mFreqUserLogin;
	double mFreqUserLogout;
	double mFreqMyINFOSent;
	double mFreqMyINFOReceived;

	nSocket::cServerDC * mS;
	cStats(nSocket::cServerDC *server);
	void AddFields();
	virtual void CleanUp();
	virtual ~cStats();
};
	}; // namespace nStatsPlugin
}; // namespace nVerliHub
#endif
