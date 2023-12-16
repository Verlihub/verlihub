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

#ifndef CREGLIST_H
#define CREGLIST_H
#include <string>
#include "cconfmysql.h"
#include "creguserinfo.h"
#include "tcache.h"

using namespace std;

namespace nVerliHub {
	using namespace nMySQL;
	namespace nSocket {
		class cConnDC;
		class cServerDC;
	};

	namespace nTables {

	class cRegUserInfo;

/**a reg users list
  *@author Daniel Muller
  */

class cRegList : public nConfig::cConfMySQL
{
public:
	cRegList(nMySQL::cMySQL &mysql, nSocket::cServerDC *);
	virtual ~cRegList();
	/** find nick in reglist
		if not foud return 0
		else return 1 and fill in the reuserinfo parameter */
	bool FindRegInfo(cRegUserInfo &, const string &nick);
	int ShowUsers(cConnDC *op, ostringstream &os, int cls);
	/** add registered user */
	bool AddRegUser(const string &nick, nSocket::cConnDC *op, int cl, const char *password = NULL);
	// set user password
	bool ChangePwd(const string &nick, const string &pwd, cConnDC *conn = NULL);
	/** No descriptions */
	bool SetVar(const string &nick, string &field, string &value);
	/** log that user logged in */
	bool Login(nSocket::cConnDC *conn, const string &nick);
	bool Logout(const string &nick);
	/** log that user logged in with wrong passwd*/
	bool LoginError(nSocket::cConnDC *conn, const string &nick);
	bool DelReg(const string &nick);
	void ReloadCache(){mCache.Clear(); mCache.LoadAll();}
	void UpdateCache(){mCache.Update();}
	nConfig::tCache<string> mCache;
protected: // Protected attributes
	/** reference to a server */
	nSocket::cServerDC *mS;
	cRegUserInfo mModel;
};
};
};

#endif
