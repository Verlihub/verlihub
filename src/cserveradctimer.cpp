/*
	Copyright (C) 2006-2026 Verlihub Team, info at verlihub dot net

	Verlihub is free software; You can redistribute it
	and modify it under the terms of the GNU General
	Public License as published by the Free Software
	Foundation, either version 3 of the license, or at
	your option any later version.
*/

#include "cserveradc.h"
#include "cadcpluginbridge.h"
#include "cbanlist.h"
#include "cmaxminddb.h"
#include "cpenaltylist.h"
#include "creglist.h"

namespace nVerliHub {
	using namespace nEnums;

	namespace nSocket {

int cServerADC::OnTimer(const cTime &now)
{
	// Preserve protocol-neutral load shedding from cServerDC::OnTimer().
	mSysLoad = eSL_NORMAL;

	if (mFrequency.mNumFill > 0) {
		const double freq = mFrequency.GetMean(now);

		if (freq < (1.2 * mC.min_frequency))
			mSysLoad = eSL_PROGRESSIVE;

		if (freq < (1.0 * mC.min_frequency))
			mSysLoad = eSL_CAPACITY;

		if (freq < (0.8 * mC.min_frequency))
			mSysLoad = eSL_RECOVERY;

		if (freq < (0.5 * mC.min_frequency))
			mSysLoad = eSL_SYSTEM_DOWN;
	}

	if ((mSysLoad < eSL_PROGRESSIVE) && (mC.max_upload_kbps > 0.)) {
		double totalUpload = 0.;

		for (unsigned int zone = 0; zone <= USER_ZONES; ++zone)
			totalUpload += mUploadZone[zone].GetMean(now);

		if ((totalUpload / 1024.0) > mC.max_upload_kbps)
			mSysLoad = eSL_PROGRESSIVE;
	}

	// Ban cleanup is independent from NMDC framing and remains shared.
	if (mBanList && bool(mSlowTimer.mMinDelay) &&
		(mSlowTimer.Check(mTime, 1) == 0))
		mBanList->RemoveOldShortTempBans(mTime.Sec());

	// A synchronous reload in cServerDC ends by broadcasting an NMDC chat line.
	// ADC performs the data/cache reload directly and never invokes that output.
	if (mReloadNow) {
		mC.Load();

		if (mR && mC.use_reglist_cache)
			mR->ReloadCache();

		if (mPenList && mC.use_penlist_cache)
			mPenList->ReloadCache();

		if (mMaxMindDB)
			mMaxMindDB->ReloadAll();

		mReloadNow = false;
	}

	if (bool(mReloadcfgTimer.mMinDelay) &&
		(mReloadcfgTimer.Check(mTime, 1) == 0)) {
		mC.Load();

		if (mR && mC.use_reglist_cache)
			mR->UpdateCache();

		if (mPenList && mC.use_penlist_cache)
			mPenList->UpdateCache();

		for (unsigned int zone = 0; zone <= USER_ZONES; ++zone)
			mUploadZone[zone].Reset(mTime);

		mFrequency.Reset(mTime);

		if (Log(2))
			LogStream() << "Socket counter: " << cAsyncConn::sSocketCounter << endl;
	}

	if (mMaxMindDB && mC.mmdb_cache && mC.mmdb_cache_mins &&
		((mTime.Sec() - mMaxMindDB->mClean.Sec()) >= 60))
		mMaxMindDB->MMDBCacheClean();

	if (mBanList) {
		mBanList->mTempNickBanlist.AutoResize();
		mBanList->mTempIPBanlist.AutoResize();
	}

	// Protocol/identify/verify timeouts are ADC session state, not cConnDC
	// timeout flags. NORMAL deliberately has no synthetic idle timeout here.
	mADCProto.Sessions().CheckTimeouts(
		static_cast<long long>(mTime.MiliSec()));

	// Do not run cServerDC's NMDC hublist registration, OpChat DDoS reports,
	// trigger output or legacy timer callbacks. ADC plugins get their own event.
	return nPlugin::cADCPluginBridge::OnTimer(&mPluginManager,
		static_cast<long long>(mTime.MiliSec())) ? 1 : 0;
}

	}; // namespace nSocket
}; // namespace nVerliHub
