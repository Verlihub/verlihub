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

#ifndef CPISTATS_H
#define CPISTATS_H

#include "src/cvhplugin.h"
#include "src/ctimeout.h"
#include "src/cmeanfrequency.h"
#include "src/cmessagedc.h"
#include "src/cserverdc.h"
#include "cstats.h"
#ifndef _WIN32
#define __int64 long long
#endif

namespace nVerliHub {
	namespace nStatsPlugin {
/**
\brief a statistics plugin for verlihub

users may leave offline messages for registered users or reg users may leave offline messages for anyone

@author Daniel Muller
*/
class cpiStats : public nPlugin::cVHPlugin
{
public:
	cpiStats();
	virtual ~cpiStats();
	virtual bool RegisterAll();
	virtual bool OnParsedMsgSearch(nSocket::cConnDC *, nProtocol::cMessageDC *);
	virtual void OnLoad(nSocket::cServerDC *);
	virtual bool OnTimer(__int64 msec);
	cStats * mStats;
private:
	nUtils::cTimeOut mStatsTimer;
	nUtils::cMeanFrequency<int> mFreqSearchA;
	nUtils::cMeanFrequency<int> mFreqSearchP;
};
	}; // namespace nStatsPlugin
}; // namespace nVerliHub
#endif
