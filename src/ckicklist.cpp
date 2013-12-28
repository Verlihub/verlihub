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

#include "cconndc.h"
#include "cuser.h"
#include "ckicklist.h"

namespace nVerliHub {
	using namespace nMySQL;
	using namespace nConfig;
	namespace nTables {

cKickList::cKickList(cMySQL &mysql) : cConfMySQL(mysql)
{
	SetClassName("cKickList");
	mMySQLTable.mName = "kicklist";
	AddCol("nick", "varchar(64)", "", false, mModel.mNick);
	AddPrimaryKey("nick");
	AddCol("time", "int(11)", "", false, mModel.mTime);
	AddPrimaryKey("time");
	AddCol("ip", "varchar(15)", "", true, mModel.mIP);
	AddCol("host", "text", "", true, mModel.mHost);
	AddCol("share_size", "varchar(15)", "", true, mModel.mShare);
	AddCol("reason", "text", "", true, mModel.mReason);
	AddCol("op", "varchar(64)", "", false, mModel.mOp);
	AddCol("is_drop", "tinyint(1)", "", true, mModel.mIsDrop);
	mMySQLTable.mExtra = "PRIMARY KEY(nick, time), ";
	mMySQLTable.mExtra+= "INDEX op_index (op), ";
	mMySQLTable.mExtra+= "INDEX ip_index (ip), ";
	mMySQLTable.mExtra+= "INDEX drop_index (is_drop)";
	SetBaseTo(&mModel);
}

cKickList::~cKickList(){}

/*!
    \fn cKickList::Cleanup()
 */
void cKickList::Cleanup()
{
	mQuery.OStream() << "DELETE FROM " << mMySQLTable.mName << " WHERE time < " << cTime().Sec() - 24*3600*30;
	mQuery.Query();
	mQuery.Clear();
}

bool cKickList::AddKick(cConnDC *conn ,const string &OPNick, const string *reason, cKick &OldKick)
{
	if(!conn || !conn->mpUser) return false;
	if(!FindKick(OldKick, conn->mpUser->mNick, OPNick,
		300,
		reason == NULL, reason != NULL ))
	{
		OldKick.mIP = conn->AddrIP();
		OldKick.mNick = conn->mpUser->mNick;
		if(OPNick.size()) OldKick.mOp = OPNick;
		else OldKick.mOp = "VerliHub";
		OldKick.mTime = cTime().Sec();
		OldKick.mHost = conn->AddrHost();
		OldKick.mShare = conn->mpUser->mShare;
		OldKick.mIsDrop = (reason == NULL);
	}
	if(reason) OldKick.mReason += *reason; else OldKick.mIsDrop = true;

	SetBaseTo(&OldKick);
	DeletePK();
	SavePK(false);
	return true;
}


/*!
    \fn cKickList::FindKick(cConnDC*, cConnDC*, unsigned)
 */
bool cKickList::FindKick(cKick &Kick, const string &Nick, const string &OpNick, unsigned age, bool WithReason, bool WithDrop, bool IsNick)
{
	ostringstream os;

	SelectFields(os);
	os <<" WHERE time > " << cTime().Sec()-age << " AND ";
	cConfigItemBase * item = NULL;
	string var;
	if(IsNick)
	{
		Kick.mNick = Nick;
		var = "nick";
	}
	else
	{
		Kick.mIP = Nick;
		var = "ip";
	}
	item = operator[](var);
	SetBaseTo(&Kick);

	ufEqual(os, string(" AND "), true, true, false)(item);

	os <<" AND reason IS " << (WithReason?"NOT ":"") << "NULL ";
	os <<" AND is_drop = " << WithDrop;
	if(OpNick.size())
		os << " AND op = '" << OpNick << "'";
	os <<" ORDER BY time DESC LIMIT 1";

	bool found = false;
	if( -1 != StartQuery(os.str()))
	{
		found = ( 0 <= Load());
		EndQuery();
	}
	return found;
}

	}; // namespace nTables
}; // namespace nVerliHub
