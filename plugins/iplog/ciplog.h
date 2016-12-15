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

#include "src/cconfmysql.h"
#include "src/cserverdc.h"
#include "src/cconndc.h"

namespace nVerliHub {
	namespace nSocket {
		class cConnDC;
		class cServerDC;
	};
	namespace nEnums {
		enum {
			eLT_CONNECT =  0,
			eLT_LOGIN = 1,
			eLT_LOGOUT = 2,
			eLT_DISCONNECT = 3,
			eLT_DEFAULT = 1 << eLT_CONNECT |  1 << eLT_LOGOUT,
			eLT_ALL = 255
		};

	};
	namespace nIPLogPlugin {

struct sUserStruct {
	sUserStruct()
	{
		mDate=0;
		mIP = 0;
		mType=0;
		mInfo=0;
	}
	long mDate;
	unsigned long mIP;
	int mType;
	int mInfo;
	string mNick;
};

/**
@author Daniel Muller
*/
class cIPLog : public nConfig::cConfMySQL
{
public:
	nSocket::cServerDC * mS;
	cIPLog(nSocket::cServerDC *server);
        //bool SetVar(const string &nick, string &field, string &value);
	/** log that user logged in */
        bool Log(nSocket::cConnDC *conn, int action, int info);
	void AddFields();
	virtual void CleanUp();
	virtual ~cIPLog();

	void GetIPHistory(const string &ip, int limit, ostream &os);
	void GetNickHistory(const string &nick, int limit, ostream &os);
	void GetHistory(const string &who, bool isNick, int limit, ostream &os);

	void GetLastIP(const string &nick, int limit, ostream &os);
	void GetLastNick(const string &ip, int limit, ostream &os);
	void GetLastLogin(const string &who, bool isNick, int limit, ostream &os);

	void MakeSearchQuery(const string &who, bool IsNick, int action, int limit);
	struct sUserStruct mModel;
private:
	using nVerliHub::cObj::Log; // we hide this overloaded function on purpose
};
	}; // namespace nIPLogPlugin
}; // namespace nVerliHub
#endif
