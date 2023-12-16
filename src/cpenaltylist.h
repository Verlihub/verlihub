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

#ifndef NDIRECTCONNECT_NTABLESCPENALTYLIST_H
#define NDIRECTCONNECT_NTABLESCPENALTYLIST_H
#include <string>
#include "cconfmysql.h"
#include "ctime.h"
#include "tcache.h"

using std::string;

namespace nVerliHub {
	using nConfig::cConfMySQL;

	namespace nSocket {
		class cServerDC;
	};

	using namespace nUtils;

	namespace nTables {

/*
list of temporary user penalties that are saved in database
@author Daniel Muller
*/

class cPenaltyList: public cConfMySQL
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
				long now = cTime().Sec();
				mSince = now;
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
				cTime now = cTime().Sec();

				if (mStartChat > now)
					return true;

				if (mStartSearch > now)
					return true;

				if (mStartCTM > now)
					return true;

				if (mStartPM > now)
					return true;

				if (mStopKick > now)
					return true;

				if (mStopShare0 > now)
					return true;

				if (mStopReg > now)
					return true;

				if (mStopOpchat > now)
					return true;

				return false;
			}
		};

		cPenaltyList(nMySQL::cMySQL &mysql, nSocket::cServerDC *);
		~cPenaltyList();
		void Cleanup(void);
		void CleanType(const string &type);
		bool LoadTo(sPenalty &, const string &nick);
		bool AddPenalty(sPenalty &);
		bool RemPenalty(sPenalty &);
		void ListAll(ostream &os);

		void ReloadCache()
		{
			mCache.Clear();
			mCache.LoadAll();
		}

		void UpdateCache()
		{
			mCache.Update();
		}

		nConfig::tCache<string> mCache;
	protected:
		nSocket::cServerDC *mServ;
		sPenalty mModel;
};

	}; // namespace nTables
}; // namespace nVerliHub

#endif
