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

#ifndef NUTILS_CMEANFREQUENCY_H
#define NUTILS_CMEANFREQUENCY_H
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "ctime.h"
#include <string.h>
#include <iostream>

using namespace std;
namespace nVerliHub {
	namespace nUtils {
/**
  *@author Daniel Muller
  */

/**
 * Mesuers a frequency over a given period of incomming events
 * events are counted by type T, they are summed
 * the given interval is divined into several subintervals
 * to make better resolution
 * */

template<class T, int max_size=20> class cMeanFrequency
{
	public:
	/** period length over which it is measured */
	cTimePrint mOverPeriod;
	/** length of every part or resolution */
	cTimePrint mPeriodPart;
	/** start and end of mesured time */
	cTimePrint mStart, mEnd;
	/** start (maybe end) of the partly period where we are now */
	cTimePrint mPart;
	/** resolution of mesure */
	int mResolution;
	/** counts of events in every part of pertiod/resolution */
	T mCounts[max_size];
	int mStartIdx;
	/** number of filled periods */
	int mNumFill;

	void Dump(void)
	{
		cout << "mOverPeriod: " << mOverPeriod.AsPeriod()
			<< " mStart, mEnd: " << mStart.AsDate() << ' ' << mEnd.AsDate()
			<< " mPart, mPeriodPart " << mPart.AsDate() << ' ' << mPeriodPart.AsPeriod()
			<< " mResolution:" << mResolution
			<< " mCounts[" ;
		for (int i = 0; i < max_size; i++) cout << mCounts[i] << ", ";
		cout << "] " << "mStartIdx:" << mStartIdx << " mNumFill:" << mNumFill << endl;
	}

	/************** conctruction */
	cMeanFrequency()
	{
		cTimePrint now;
		mResolution = max_size;
		SetPeriod(0.);
		Reset(now);
	}

	cMeanFrequency(const cTimePrint &now)
	{
		mResolution = max_size;
		SetPeriod(1.);
		Reset(now);
	}

	cMeanFrequency(const cTimePrint &now, double per, int res):
		mOverPeriod(per),
		mPeriodPart(per/res),
		mResolution(res)
	{
		Reset(now);
	}

	/** insert/add */
	void Insert(const cTimePrint &now, T data=1)
	{
		Adjust(now);
		mCounts[(mStartIdx+mNumFill) % mResolution] += data;
	}

	double GetMean(const cTimePrint &now)
	{
		T sum = CountAll(now);
		double Sum = sum;
		if (! mNumFill) return 0.;
		Sum *= double(mResolution / mNumFill);
		Sum /= double(mOverPeriod);
		return Sum;
	}

	/** calculate count over all period */
	T CountAll(const cTimePrint &now)
	{
		Adjust(now);
		T sum=0;
		int i,j=mStartIdx;
		for(i=0; i < mNumFill; i++)
		{
			sum += mCounts[j++];
			if(j >= mResolution) j = 0;
		}
		return sum;
	};

	/** adjust state to current time */
	void Adjust(const cTimePrint &now)
	{
		// need adjustment
		if( mEnd < now )
		{
			cTimePrint t(mEnd);
			t += mOverPeriod; // if last adjustment happend more thern period ago
			// in this case we can empty all
			if( t < now ) { Reset(now); return; }
			// shift until it's ok
			while(mEnd < now) Shift();
		}
		else if( mNumFill < mResolution)
		{
			while(( mPart < now ) && ( mNumFill < mResolution))
			{
				mPart += mPeriodPart;
				mNumFill ++;
			}
		}
	};

	// left (remove first part period)
	void Shift()
	{
		mEnd   += mPeriodPart;
		mStart += mPeriodPart;
		mCounts[mStartIdx] = 0;
		if(mNumFill > 0) mNumFill --;
		mStartIdx ++;
		if (mStartIdx >= mResolution) mStartIdx -= mResolution;
	}

	void Reset(const cTimePrint &now)
	{
		memset(&mCounts,0, sizeof(mCounts));
		mStart = now;
		mEnd =  mStart;
		mEnd += mOverPeriod;
		mNumFill = 0;
		mStartIdx = 0;
		mPart = mStart;
		mPart += mPeriodPart;
	}

	/** setup the opeverperiod and the resolution */
	void SetPeriod(double per)
	{
		mOverPeriod = cTimePrint(per);
		mPeriodPart = mOverPeriod / mResolution;
	}

};

	}; // namespace nUtils
}; // namespace nVerliHub
#endif
