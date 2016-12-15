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

#ifndef _CFLOODPROTECT_H_
#define _CFLOODPROTECT_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "src/tchashlistmap.h"
#include "src/thasharray.h"
#include "src/ctime.h"
#include "src/cconfigbase.h"
#include "src/cserverdc.h"
#include "src/cconndc.h"

#include <vector>
namespace nVerliHub {
	namespace nEnums {
		typedef enum {
			eFT_CHAT,
			eFT_PRIVATE,
			eFT_SEARCH,
			eFT_MYINFO
		} tFloodType;
	};
	namespace nFloodProtectPlugin {

struct sUserInfo
{
	nUtils::cTime mLastAction;
	nUtils::cTime mElapsedTime;
	unsigned short mActionCounter;
	string mIP;
	string mFTStr;
	bool mDisabled;
	list<nEnums::tFloodType> mFloodTypes;

	sUserInfo(string ip):
		mActionCounter(0),
		mIP(ip),
		mDisabled(false)
	{}

	~sUserInfo()
	{}

	void addFloodType(nEnums::tFloodType ft)
	{
	    if(mFloodTypes.size() == 10)
	    {
		mFloodTypes.pop_front(); // drop the first
	    }
	    mFloodTypes.push_back(ft);
	}
	string & getFloodTypes()
	{
	    mFTStr.clear();
	    for(list<nEnums::tFloodType>::iterator it=mFloodTypes.begin(); it!=mFloodTypes.end(); ++it)
	    {
		switch(*it)
		{
			case nEnums::eFT_CHAT: mFTStr += "CHAT ";
		    break;
			case nEnums::eFT_PRIVATE: mFTStr += "PRIVATE ";
		    break;
			case nEnums::eFT_SEARCH: mFTStr += "SEARCH ";
		    break;
			case nEnums::eFT_MYINFO: mFTStr += "MYINFO ";
		    break;
		    default: mFTStr += "UNKNOWN ";
		    break;
		}
	    }
	    return mFTStr;
	}
};

class cFloodCfg : public nConfig::cConfigBase
{
public:
	cFloodCfg(nSocket::cServerDC *);
	int mMaxConnPerIP;
	int mMaxUsersPerIP;
	int mBanTimeOnFlood;
	nSocket::cServerDC *mS;
	virtual int Load();
	virtual int Save();
};

class cFloodprotect: public cObj
{
public:
	cFloodprotect(nSocket::cServerDC *);
	~cFloodprotect();
	bool CleanUp(int secs);
	bool CheckFlood(nSocket::cConnDC *, nEnums::tFloodType);
	bool AddConn(nSocket::cConnDC *, short diff = 1);
	int KickAll(nSocket::cConnDC *);

	nSocket::cServerDC * mS;

	typedef tcHashListMap<sUserInfo *> tUserInfo;
	//FIXME: DataType in tHashArray template must be a pointer!
	typedef tHashArray<short int> tConnCounter;
	typedef tUserInfo::iterator tUIIt;
	tUserInfo mUserInfo;
	tConnCounter mConnCounter;
	cFloodCfg mCfg;
};

	}; // namespace nFloodProtectPlugin
}; // namespace nVerliHub

#endif
