/*
	Copyright (C) 2003-2005 Daniel Muller, dan at verliba dot cz
	Copyright (C) 2006-2024 Verlihub Team, info at verlihub dot net

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

#ifndef CFREQLIMITER_H
#define CFREQLIMITER_H

#include "ctimeout.h"
#include "cmeanfrequency.h"

/**
  * Various utility classes, very useful
  *@author Daniel Muller
  */
namespace nVerliHub {
	namespace nUtils {

/** a simple frequency limitter
 * You can setup:
 * 	minimum delay between two events
 * 	maximum frequency mesured over a given time period
 * If Any is not verified, you know it.
 *@author Daniel Muller
 *
 */

class cFreqLimiter
{
public:
	cFreqLimiter(double min_f, double period, long max_cnt, const cTimePrint & now):
		mTmOut(min_f,0,now),
		mFreq(now,period,5),
		mMaxCnt(max_cnt)
	{}

	void SetParams(double min_f, double period, long max_cnt)
	{
		mTmOut.SetMinDelay(min_f);
		mFreq.SetPeriod(period);
		mMaxCnt = max_cnt;
	}

	cFreqLimiter():
		mMaxCnt(0)
	{}

	virtual ~cFreqLimiter() {}

	/** to mesure min delay */
	cTimeOut mTmOut;
	/** to mesure frequency */
	cMeanFrequency<long> mFreq;
	/** the frequency limiter */
	long mMaxCnt;

	/** return 0 if ok, -1 if short delay, -3 if high frequency */
	virtual int Check(const cTimePrint &now)
	{
		int r = mTmOut.Check(now,1);
		if(r) return r;
		mFreq.Insert(now,1);
		if(mFreq.CountAll(now) > mMaxCnt) return -3;
		return 0;
	}
};

	}; // namespace nUtils
}; // namespace nVerliHub

#endif
