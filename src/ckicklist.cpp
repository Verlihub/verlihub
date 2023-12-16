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

#include "cconndc.h"
#include "cuser.h"
#include "ckicklist.h"

namespace nVerliHub {
	using namespace nMySQL;
	using namespace nConfig;
	namespace nTables {

cKickList::cKickList(cMySQL &mysql):
	cConfMySQL(mysql)
{
	SetClassName("cKickList");
	mMySQLTable.mName = "kicklist";
	AddCol("nick", "varchar(128)", "", false, mModel.mNick);
	AddPrimaryKey("nick");
	AddCol("time", "int(11)", "", false, mModel.mTime);
	AddPrimaryKey("time");
	AddCol("ip", "varchar(15)", "", true, mModel.mIP);
	AddCol("host", "varchar(255)", "", true, mModel.mHost);
	AddCol("share_size", "varchar(15)", "", true, mModel.mShare);
	AddCol("reason", "varchar(255)", "", true, mModel.mReason);
	AddCol("op", "varchar(128)", "", false, mModel.mOp);
	AddCol("is_drop", "tinyint(1)", "", true, mModel.mIsDrop);
	mMySQLTable.mExtra = "PRIMARY KEY(nick, time), ";
	mMySQLTable.mExtra += "INDEX op_index (op), ";
	mMySQLTable.mExtra += "INDEX ip_index (ip), ";
	mMySQLTable.mExtra += "INDEX drop_index (is_drop)";
	SetBaseTo(&mModel);
}

cKickList::~cKickList()
{}

void cKickList::Cleanup()
{
	mQuery.OStream() << "delete from `" << mMySQLTable.mName << "` where `time` < " << (cTime().Sec() - (24 * 3600 * 30));
	mQuery.Query();
	mQuery.Clear();
}

bool cKickList::AddKick(cConnDC *conn, const string &op, const string *why, unsigned int age, cKick &kick, bool drop)
{
	if (!conn || !conn->mpUser)
		return false;

	if (!FindKick(kick, conn->mpUser->mNick, op, age, drop, ((why != NULL) && why->size()))) {
		kick.mIP = conn->AddrIP();
		kick.mNick = conn->mpUser->mNick;

		if (op.size())
			kick.mOp = op;
		else
			kick.mOp = HUB_VERSION_NAME;

		kick.mTime = cTime().Sec();
		kick.mHost = conn->AddrHost();
		kick.mShare = conn->mpUser->mShare;
	}

	if (why && why->size())
		kick.mReason += (*why);

	kick.mIsDrop = drop;
	SetBaseTo(&kick);
	DeletePK();
	SavePK(false);
	return true;
}

bool cKickList::FindKick(cKick &dest, const string &nick, const string &op, unsigned int age, bool why, bool drop, bool is_nick)
{
	ostringstream os;
	SelectFields(os);
	os << " where `time` > " << (cTime().Sec() - age) << " and ";
	cConfigItemBase *item = NULL;
	string var;

	if (is_nick) {
		dest.mNick = nick;
		var = "nick";
	} else {
		dest.mIP = nick;
		var = "ip";
	}

	item = operator[](var);

	if (!item) return false;

	SetBaseTo(&dest);
	ufEqual(os, " and ", true, true, false)(item);
	os << " and `reason` is " << (why ? "not " : "") << "null and `is_drop` = " << drop;

	if (op.size())
		os << " and `op` = '" << op << '\'';

	os << " order by `time` desc limit 1";
	bool found = false;

	if (StartQuery(os.str()) != -1) {
		found = (0 <= Load());
		EndQuery();
	}

	return found;
}

	}; // namespace nTables
}; // namespace nVerliHub
