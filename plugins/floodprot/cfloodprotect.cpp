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

#include "src/ckick.h"
#include "src/cban.h"
#include "src/cbanlist.h"
#include "cfloodprotect.h"

namespace nVerliHub {
	using namespace nTables;
	using namespace nSocket;
	using namespace nEnums;
	namespace nFloodProtectPlugin {

cFloodCfg::cFloodCfg(cServerDC *server):
	mS(server)
{
	Add("max_conn_per_ip",mMaxConnPerIP,55);
	Add("max_users_per_ip",mMaxUsersPerIP,50);
	Add("bantime_on_flood",mBanTimeOnFlood,60*60*3);
	Load();
	Save();
}

int cFloodCfg::Load()
{
	mS->mSetupList.LoadFileTo(this,"pi_floodprot");
	return 0;
}

int cFloodCfg::Save()
{
	mS->mSetupList.SaveFileTo(this,"pi_floodprot");
	return 0;
}

cFloodprotect::cFloodprotect(cServerDC *server):
	cObj("cFloodProtect"),
	mS(server),
	mCfg(server)
{}

cFloodprotect::~cFloodprotect()
{
	CleanUp(-1);
}

bool cFloodprotect::CleanUp(int secs)
{
	// cleanup if expired
	cTime now;
	unsigned long Hash;
	sUserInfo * userinfo = 0;
	tUIIt it, it2;

	for(it = mUserInfo.begin(); it != mUserInfo.end();)
	{
		it2 = it;
		++it2;
		if(*it)
		{
			if((*it)->mLastAction.Sec() + secs < now.Sec())
    			{
				// clean up after ?? sec inactivity
				Hash = cBanList::Ip2Num((*it)->mIP);
				userinfo = mUserInfo.GetByHash(Hash);
				mUserInfo.RemoveByHash(Hash);
				if(userinfo)
				{
					delete userinfo;
					userinfo=0;
				}
			}
		}
		it = it2;
	}
	return true;
}

bool cFloodprotect::CheckFlood(cConnDC * conn, tFloodType ft)
{
	if (!conn) return true;

	if (conn && conn->mpUser && conn->mpUser->mClass >= eUC_OPERATOR) return true;

	unsigned long Hash = cBanList::Ip2Num(conn->AddrIP());

	if (mUserInfo.ContainsHash(Hash))
	{
		cTime now;
		sUserInfo * usr = 0;
		usr = mUserInfo.GetByHash(Hash);

		if (!usr || usr->mDisabled)
			return false;

		usr->mElapsedTime += now.Get() - usr->mLastAction;
		usr->mLastAction = now;
		usr->mActionCounter++;
		usr->addFloodType(ft); // registers the current action type for later analysis

		if(usr->mActionCounter == 10)
		{
			//10/10 = freq > 1 Hz
			//10/10 - freq = 1 Hz - 0.33 Hz
			//10/30 = freq < 0.33 Hz
			if(usr->mElapsedTime.Sec() < 10) // 10 actions, 1 Hz or higher is high frequency
			{
				ostringstream os;
				string text, floodtype;

				int cnt = KickAll(conn);

				switch(ft)
				{
					case eFT_CHAT: floodtype = "MAIN CHAT"; break;
					case eFT_PRIVATE: floodtype = "PRIVATE CHAT"; break;
					case eFT_SEARCH: floodtype = "SEARCH"; break;
					case eFT_MYINFO: floodtype = "MYINFO"; break;
					default: floodtype = "UNKNOWN"; break;
				}

				os << "\r\n";
				os << "FLOODPROTECT: User is trying to " << floodtype << " flood the server. Number of affected connections: " << cnt << ". Banned for " << mCfg.mBanTimeOnFlood << " secs!\r\n";
				os << "FLOODPROTECT: Frequecy is: " << usr->mActionCounter / (usr->mElapsedTime.Sec() + (usr->mElapsedTime.tv_usec / 1000000.)) << " Hz\r\n";
				os << "FLOODPROTECT: Detected flood types are: " << usr->getFloodTypes();
				text = os.str();
				mS->ReportUserToOpchat(conn, text, false);

				cBan Ban(mS);
				cKick Kick;

				Kick.mOp = mS->mC.hub_security;
				Kick.mIP = usr->mIP;
				Kick.mTime = cTime().Sec();
				Kick.mReason = "HIGH FLOOD FREQUENCY DETECTED!";
				mS->mBanList->NewBan(Ban, Kick, mCfg.mBanTimeOnFlood, eBF_IP);
				mS->mBanList->AddBan(Ban);
				usr->mDisabled = true;
				usr->mActionCounter = 0;
				usr->mElapsedTime = cTime(0,0);
				return false;
			}
			if((usr->mElapsedTime.Sec() >= 10) && (usr->mElapsedTime.Sec() < 30)) // between 0.33 Hz and 1 Hz is medium frequency
			{
				//string text;
				//ostringstream os;
				//os << "\r\n";
				//os << "FLOODPROTECT: Frequecy is: " << usr->mActionCounter / (usr->mElapsedTime.Sec() + (usr->mElapsedTime.tv_usec / 1000000.)) << " Hz\r\n";
				//os << "FLOODPROTECT: Detected flood types are: " << usr->getFloodTypes();
				//text = os.str();
				//mS->ReportUserToOpchat(conn, text, false);
				conn->CloseNow();
				usr->mActionCounter = 0;
				usr->mElapsedTime = cTime(0,0);
				return false;
			}
			if(usr->mElapsedTime.Sec() >= 30) // less than 0.33 Hz is low frequency
			{
				//string text;
				//ostringstream os;
				//os << "\r\n";
				//os << "FLOODPROTECT: Frequecy is: " << usr->mActionCounter / (usr->mElapsedTime.Sec() + (usr->mElapsedTime.tv_usec / 1000000.)) << " Hz\r\n";
				//os << "FLOODPROTECT: Detected action types are: " << usr->getFloodTypes();
				//text = os.str();
				//mS->ReportUserToOpchat(conn, text, false);
				usr->mActionCounter = 0;
				usr->mElapsedTime = cTime(0,0);
				return true;
			}
		}
	}
	else
	{
		cTime now;
		sUserInfo * usr;
		usr = new sUserInfo(conn->AddrIP());
		if(!usr) return false;
		usr->mLastAction = now;
		usr->mElapsedTime = cTime(0,0);
		mUserInfo.AddWithHash(usr, Hash);
	}
	return true;
}

int cFloodprotect::KickAll(cConnDC *conn)
{
	int cnt=0;
	cConnDC * tempConn = 0;
	cUserCollection::iterator it;

	if(!conn) return 0;

	unsigned long ip = cBanList::Ip2Num(conn->AddrIP());

	for(it=mS->mUserList.begin(); it!=mS->mUserList.end(); ++it)
	{
		tempConn = (static_cast<cUser *>(*it))->mxConn;
		if(tempConn)
		{
			if(ip == cBanList::Ip2Num(tempConn->AddrIP()))
			{
				if(tempConn->mpUser)
				{
					vhLog(2) << "KICKING: " << tempConn->mpUser->mNick << endl;
				}
				else
				{
					vhLog(2) << "KICKING: Could not determine nick!" << endl;
				}
				tempConn->CloseNow();
				cnt++;
			}
		}
	}
	vhLog(3) << "CNT: " << cnt << endl;
	return cnt;
}

bool cFloodprotect::AddConn(cConnDC *conn, short difference)
{
	// maybe special users, anyway, return true is safe
	if (!conn) return true;

	// best hash for ip is itself
	unsigned long Hash = cBanList::Ip2Num(conn->AddrIP());
	short int Count =  mConnCounter.GetByHash(Hash);

	Count += difference;

	// every user has 2 points

	short limit = 2 * mCfg.mMaxUsersPerIP;

	bool exists = mConnCounter.ContainsHash(Hash);

	if (difference > 0)
	{
		if(!exists) { mConnCounter.AddWithHash(Count, Hash); }
		else { mConnCounter.SetByHash(Hash, Count); }

	}
	else
	{
		if(exists)
		{
			if(Count == 0) { mConnCounter.RemoveByHash(Hash); }
			else { mConnCounter.SetByHash(Hash, Count); }
		}
	}

	return (Count <= limit);
}

};
};
