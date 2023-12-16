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

#ifndef NUTILS_CTIMEOUT_H
#define NUTILS_CTIMEOUT_H
#include "ctime.h"

namespace nVerliHub {
	namespace nUtils {
/**mesures timeout and detects flooding
  *@author Daniel Muller
  */

/**
 * Minimum delay and maximum delay controler
 * */

class cTimeOut {
public:
	cTimeOut():mMinDelay(0l),mMaxDelay(0l),mLast(0l){};
	cTimeOut(double min,double max,const cTime &now):mMinDelay(min),mMaxDelay(max),mLast(now){};
	~cTimeOut();

public:
   void SetMinDelay(double mind) { mMinDelay = mind;}
   void SetMaxDelay(double maxd) { mMaxDelay = maxd;}
public: // Public attributes
	/** minimum delay between events */
	cTime mMinDelay;
	/** maximum delay between events */
	cTime mMaxDelay;
	/** last event time*/
	cTime mLast;

	/**
	 * Checks the validity, return 0 on sucess; -1 on timein; -2 on timeout
	 * if event is positive, the last event time is updated
	 * */
	int Check(const cTime &now, long event=0)
	{
		if ( !bool(mLast)) return 0;
		cTime diff(now);
		// check min delay
		if( event && (bool)mMinDelay)
		{
			diff -= mLast;
			if(mMinDelay > diff) return -1;
		}

		// check the max delay
		if( bool(mMaxDelay) && (mMaxDelay < (diff = now - mLast)) ) return -2;

		// update time of last event
		if(event) Reset(now);

		// ok np
		return 0;
	}

	/** Reset the events to now */
	void Reset(const cTime & now){ mLast = now;};
	void Disable(){ mLast= 0.;}

};

	}; // nUtils
}; // nVerliHub
#endif
