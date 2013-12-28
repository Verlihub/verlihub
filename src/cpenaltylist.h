/*
	Copyright (C) 2003-2005 Daniel Muller, dan at verliba dot cz
	Copyright (C) 2006-2014 Verlihub Project, devs at verlihub-project dot org

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

#ifndef NDIRECTCONNECT_NTABLESCPENALTYLIST_H
#define NDIRECTCONNECT_NTABLESCPENALTYLIST_H
#include <string>
#include "cconfmysql.h"
#include "ctime.h"
#include "tcache.h"

using std::string;

namespace nVerliHub {
	using nUtils::cTime;
	using nConfig::cConfMySQL;
	using namespace nUtils;
	namespace nTables {

/**
has the list of temporary user penalties... that save in database

@author Daniel Muller
*/

class cPenaltyList : public cConfMySQL
{
	public:
		/**structure representing the MySQL table data */
		struct sPenalty
		{
			string mNick;
			string mOpNick;
			long mSince;
			long mStartChat;
			long mStartSearch;
			long mStartCTM;
			long mStartPM;
			long mStopKick;
			long mStopShare0;
			long mStopReg;
			long mStopOpchat;

			sPenalty()
			{
				long Now = cTime().Sec();
				mSince = Now;
				mStartChat = 1;
				mStartSearch = 1;
				mStartCTM = 1;
				mStartPM = 1;
				mStopKick = 1;
				mStopShare0 = 1;
				mStopReg = 1;
				mStopOpchat = 1;
			}

			bool ToKeepIt()
			{
				long Now = cTime().Sec();
				if (mStartChat >  Now) return true;
				if (mStartSearch > Now) return true;
				if (mStartCTM > Now) return true;
				if (mStartPM > Now) return true;
				if (mStopKick > Now) return true;
				if (mStopShare0 > Now) return true;
				if (mStopReg > Now) return true;
				if (mStopOpchat > Now) return true;
				return false;
			}
			friend ostream &operator << (ostream &, const sPenalty &);
		};


		cPenaltyList(nMySQL::cMySQL &mysql);
		~cPenaltyList();
		void Cleanup(void);
		bool LoadTo(sPenalty &, const string &Nick);
		bool AddPenalty(sPenalty &);
		bool RemPenalty(sPenalty &);
		void ReloadCache()
		{
			mCache.Clear(); mCache.LoadAll();
		}
		nConfig::tCache<string> mCache;
	protected:
		sPenalty mModel;
};

	}; // namespace nTables
}; // namespace nVerliHub

#endif
