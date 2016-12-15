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

#include "cconndc.h"
#include "cserverdc.h"
#include "cbanlist.h"
#include "i18n.h"
#include <stdio.h>
#include "stringutils.h"

namespace nVerliHub {
	using namespace nUtils;
	using namespace nEnums;
	using namespace nSocket;
	namespace nTables {

cBanList::cBanList(cServerDC *s):
	cConfMySQL(s->mMySQL),
	mModel(s),
	mS(s)
{
	mMySQLTable.mName = "banlist";
	AddCol("ip", "varchar(15)", "", true, mModel.mIP);
	AddPrimaryKey("ip");
	AddCol("nick", "varchar(128)", "", true, mModel.mNick);
	AddPrimaryKey("nick");
	AddCol("ban_type", "tinyint(4)", "0", true, mModel.mType);
	AddCol("host", "text", "", true, mModel.mHost);
	AddCol("range_fr", "bigint(32)", "", true, mModel.mRangeMin);
	AddCol("range_to", "bigint(32)", "", true, mModel.mRangeMax);
	AddCol("date_start", "int(11)", "0", true, mModel.mDateStart);
	AddCol("date_limit", "int(11)", "", true, mModel.mDateEnd);
	AddCol("last_hit", "int(11)", "", true, mModel.mLastHit);
	AddCol("nick_op", "varchar(64)", "", true, mModel.mNickOp);
	AddCol("reason", "text", "", true, mModel.mReason);
	AddCol("share_size", "varchar(18)", "", true, mModel.mShare);
	mMySQLTable.mExtra = "UNIQUE (ip,nick), ";
	mMySQLTable.mExtra += "INDEX nick_index (nick), ";
	mMySQLTable.mExtra += "INDEX date_index (date_limit), ";
	mMySQLTable.mExtra += "INDEX range_index (range_fr)";
	SetBaseTo(&mModel);
}

cBanList::~cBanList()
{
	RemoveOldShortTempBans(0);
}

cUnBanList::cUnBanList(cServerDC* s) : cBanList(s), mModelUn(s)
{
	mMySQLTable.mName = "unbanlist";
	SetBaseTo(&mModelUn);
	AddCol("date_unban", "int(11)", "", true, mModelUn.mDateUnban);
	AddPrimaryKey("date_unban");
	AddCol("unban_op", "varchar(30)", "", true, mModelUn.mUnNickOp);
	AddCol("unban_reason", "text", "", true, mModelUn.mUnReason);
	mMySQLTable.mExtra = "UNIQUE (ip, nick, date_unban)";
}

cUnBanList::~cUnBanList(){}

void cBanList::Cleanup()
{
	time_t Now = cTime().Sec();
	mQuery.OStream() << "DELETE FROM " << mMySQLTable.mName << " WHERE date_limit IS NOT NULL AND date_limit < " << (Now - (3600 * 24 * 7));
	mQuery.Query();
	mQuery.Clear();
}

void cUnBanList::Cleanup()
{
	time_t Now = cTime().Sec();
	mQuery.OStream() << "DELETE FROM " << mMySQLTable.mName << " WHERE date_unban < " << (Now - (3600 * 24 * 30));
	mQuery.Query();
	mQuery.Clear();
}

int cBanList::UpdateBan(cBan &ban)
{
	nMySQL::cQuery query(mMySQL);
	SetBaseTo(&ban);
	UpdateFields(query.OStream());
	WherePKey(query.OStream());
	query.Query();
	return 0;
}

bool cBanList::LoadBanByKey(cBan &ban)
{
	SetBaseTo(&ban);
	return LoadPK();
}

void cBanList::NewBan(cBan &ban, cConnDC *connection, const string &nickOp, const string &reason, unsigned length, unsigned mask)
{
	if (connection) {
		ban.mIP = connection->AddrIP();
		ban.mHost = connection->AddrHost();
		ban.mDateStart = cTime().Sec();
		ban.mDateEnd = ban.mDateStart + length;
		ban.mLastHit = 0;
		ban.mReason = reason;
		ban.mNickOp = nickOp;
		ban.SetType(mask);

		if (connection->mpUser) {
			ban.mNick = connection->mpUser->mNick;
			ban.mShare = connection->mpUser->mShare;
		} else {
			ban.mNick = "nonick_" + ban.mIP;
		}
	}
}

void cBanList::AddBan(cBan &ban)
{
	//@todo nick2dbkey
	switch (1 << ban.mType) {
		case eBF_NICK:
			ban.mIP = "_nickban_";
		break;
		case eBF_IP:
			ban.mNick = "_ipban_";
		break;
		case eBF_RANGE:
			ban.mNick = "_rangeban_";
		break;
		case eBF_HOST1:
			ban.mIP = "_host1ban_";
			if(!this->GetHostSubstring(ban.mHost,ban.mNick,1))
				return;
		break;
		case eBF_HOST2:
			ban.mIP = "_host2ban_";
			if(!this->GetHostSubstring(ban.mHost,ban.mNick,2))
				return;
		break;
		case eBF_HOST3:
			ban.mIP = "_host3ban_";
			if(!this->GetHostSubstring(ban.mHost,ban.mNick,3))
				return;
		break;
		case eBF_HOSTR1:
			ban.mIP = "_hostr1ban_";
			if(!this->GetHostSubstring(ban.mHost,ban.mNick,-1))
				return;
		break;
		case eBF_SHARE:
			ban.mNick = "_shareban_";
		break;
		case eBF_PREFIX:
			ban.mIP = "_prefixban_";
		break;
		default: break;
	}

	// copy PK
	cBan OldBan(mS);
	OldBan.mIP = ban.mIP;
	OldBan.mNick = ban.mNick;
	// Load by PK to mModel
	SetBaseTo( &OldBan );
	bool update = false;

	if(LoadPK()) {
		update = true;
		mModel = OldBan;
		if(ban.mReason.size())
			mModel.mReason += " / " + ban.mReason;
		if(!ban.mDateEnd || (ban.mDateEnd > mModel.mDateEnd))
			mModel.mDateEnd = ban.mDateEnd;
		mModel.mNickOp = ban.mNickOp;

		if((1 << ban.mType) == eBF_RANGE) {
			mModel.mRangeMin = ban.mRangeMin;
			mModel.mRangeMax = ban.mRangeMax;
		}
	} else
		mModel = ban;

	SetBaseTo(&mModel);

	if(update)
		UpdatePK();
	else
		SavePK(false);
}

unsigned int cBanList::TestBan(cBan &ban, cConnDC *conn, const string &nick, unsigned mask)
{
	if (!conn)
		return 0;

	time_t Now = cTime().Sec();
	ostringstream query;
	bool firstWhere = false;
	string ip = conn->AddrIP();
	SelectFields(query);
	string host = conn->AddrHost();
	query << " where (";

	if (mask & (eBF_NICKIP | eBF_IP)) { // ip, nick and both are done by this first one
		AddTestCondition(query, ip, eBF_IP);
		query << " or ";
		firstWhere = true;
	}

	if (mask & (eBF_NICKIP | eBF_NICK))
		AddTestCondition(query, nick, eBF_NICK);

	if (mask & eBF_RANGE)
		AddTestCondition(query << " or ", ip, eBF_RANGE);

	if (conn->mpUser && (mask & eBF_SHARE)) {
		ostringstream os (ostringstream::out);
		os << conn->mpUser->mShare;

		if (firstWhere)
			query << " or ";

		AddTestCondition(query, os.str(), eBF_SHARE); // fix or condition?
	}

	if (mask & eBF_HOST1)
		AddTestCondition(query << " or ", host, eBF_HOST1);

	if (mask & eBF_HOST2)
		AddTestCondition(query << " or ", host, eBF_HOST2);

	if (mask & eBF_HOST3)
		AddTestCondition(query << " or ", host, eBF_HOST3);

	if (mask & eBF_HOSTR1)
		AddTestCondition(query << " or ", host, eBF_HOSTR1);

	if (mask & eBF_PREFIX)
		AddTestCondition(query << " or ", nick, eBF_PREFIX);

	query << ") and ((`date_limit` >= " << Now << ") or (`date_limit` is null) or (`date_limit` = 0)) order by `date_limit` desc limit 1";

	if (StartQuery(query.str()) == -1)
		return 0;

	SetBaseTo(&ban);
	unsigned int found = ((Load() >= 0) ? ((ban.mDateEnd) ? 1 : 2) : 0); // 0 = not banned, 1 = temporary ban, 2 = permanent ban
	EndQuery();

	if (found) {
		ban.mLastHit = Now;
		UpdatePK();
	}

	return found;
}

void cBanList::DelBan(cBan &Ban)
{
	SetBaseTo(&Ban);
	DeletePK();
}

int cBanList::DeleteAllBansBy(const string &ip, const string &nick, int mask)
{
	mQuery.OStream() << "DELETE FROM " << mMySQLTable.mName << " WHERE ";
	if(mask & eBF_IP)
		mQuery.OStream() << " ip = '" << ip << "'";
	if(mask & (eBF_IP | eBF_NICK))
		mQuery.OStream() << " AND";
	if(mask & eBF_NICK)
		mQuery.OStream() << " nick = '" << nick << "'";

	return mQuery.Query();
}

void cBanList::NewBan(cBan &ban, const cKick &kick, long period, int mask)
{
	ban.mIP = kick.mIP;
	ban.mDateStart = cTime().Sec();

	if (period)
		ban.mDateEnd = ban.mDateStart + period;
	else
		ban.mDateEnd = 0;

	ban.mLastHit = 0;
	ban.mReason = kick.mReason;
	ban.mNickOp = kick.mOp;
	ban.mNick = kick.mNick;
	ban.SetType(mask);
	ban.mHost = kick.mHost;
	ban.mShare = kick.mShare;
}

int cBanList::Unban(ostream &os, const string &value, const string &reason, const string &nickOp, int mask, bool deleteEntry)
{
	SelectFields(mQuery.OStream());
	if(!AddTestCondition(mQuery.OStream() << " WHERE ", value, mask)) {
		mQuery.Clear();
		return 0;
	}
	db_iterator it;
	cUnBan *unban = NULL;
	int i = 0;
	SetBaseTo(&mModel);

	for(it = db_begin(); it != db_end(); ++it) {
		mModel.DisplayComplete(os);
		if(deleteEntry) {
			unban = new cUnBan(mModel, mS);
			unban->mUnReason = reason;
			unban->mUnNickOp = nickOp;
         		unban->mDateUnban = cTime().Sec();
			mUnBanList->SetBaseTo(unban);
			mUnBanList->SavePK();
			delete unban;
		}
		i++;
	}
	mQuery.Clear();
	if(deleteEntry) {
		mQuery.OStream() << "DELETE FROM " << this->mMySQLTable.mName << " WHERE ";
		AddTestCondition(mQuery.OStream() , value, mask);
		mQuery.Query();
		mQuery.Clear();
	}
	return i;
}

bool cBanList::GetHostSubstring(const string &hostname, string &result, int level)
{
	string tmp(".");
	size_t pos;
	if(level > 0) {
		tmp += hostname;
		pos = tmp.npos;
		for(int i = 0; i < level; i++) {
			if(!pos)
				return false;
			pos = tmp.rfind('.',pos-1);
		}
		result.assign(tmp, pos, tmp.size()-pos);
	} else if(level < 0) {
		tmp = hostname;
		pos = 0;
		for (int i = 0; i < -level; i++) {
			if (pos == tmp.npos)
				return false;
			pos = tmp.find('.',pos+1);
		}
		result.assign(tmp, 0, pos);
	}

	return true;
}

bool cBanList::AddTestCondition(ostream &os, const string &value, int mask)
{
	string host;
	unsigned long num;
	switch(mask) {
		case eBF_NICK:
			os << "( nick = '"; cConfMySQL::WriteStringConstant(os, value); os << "')";
		break;
		case eBF_IP:
			os << "(ip='"; cConfMySQL::WriteStringConstant(os, value); os << "')";
		break;
		//case (int)eBF_NICK  : os << "(ip='_nickban_' AND nick='" << value << "')"; break;
		//case (int)eBF_IP    : os << "(nick='_ipban_' AND ip='" << value << "')"; break;
		case eBF_RANGE :
			num = Ip2Num(value);
			os << "(nick='_rangeban_' AND " << num << " BETWEEN range_fr AND range_to )";
		break;
		case eBF_SHARE :
			os << "(nick='_shareban_' AND share_size = '" << value << "')";
		break;
		case eBF_HOST1 :
			if(!this->GetHostSubstring(value, host, 1)) {
				os << " 0 ";
				return false;
			}
			os << "(ip='_host1ban_' AND '" << host << "' = nick)";
		break;
		case eBF_HOST2 :
			if(!this->GetHostSubstring(value, host, 2)) {
				os << " 0 ";
				return false;
			}
			os << "(ip='_host2ban_' AND '" << host << "' = nick)";
		break;
		case eBF_HOST3 :
			if(!this->GetHostSubstring(value, host, 3)) {
				os << " 0 ";
				return false;
			};
			os << "(ip='_host3ban_' AND '" << host << "' = nick)";
		break;
		case eBF_HOSTR1 :
			if(!this->GetHostSubstring(value, host, -1)) {
				os << " 0 ";
				return false;
			};
			os << "(ip='_hostr1ban_' AND '" << host << "' = nick)";
		break;
		case eBF_PREFIX :
			os << "(ip='_prefixban_' AND nick=LEFT('";cConfMySQL::WriteStringConstant(os, value); os << "',LENGTH(nick)))";
		break;
		default: return false;
	}
	return true;
}

void cBanList::List(ostream &os, int count)
{
	mQuery.Clear();
	SelectFields(mQuery.OStream());
	mQuery.OStream() << " order by date_start desc limit " << count;
	db_iterator it;
	SetBaseTo(&mModel);

	os << "\r\n\r\n\t" << _("Item");
	os << "\t\t\t\t" << _("Type");
	os << "\t\t" << _("Operator");
	os << "\t\t" << _("Time");
	os << "\t\t\t" << _("Last hit") << "\r\n";
	os << "\t" << string(160, '-') << "\r\n\r\n";

	for (it = db_begin(); it != db_end(); ++it) {
		mModel.DisplayInline(os);
		os << "\r\n";
	}

	mQuery.Clear();
}

unsigned long cBanList::Ip2Num(const string &ip)
{
	int i;
	char c;
	istringstream is(ip);
	unsigned long mask = 0;
	is >> i >> c; mask += i & 0xFF; mask <<= 8;
	is >> i >> c; mask += i & 0xFF; mask <<= 8;
	is >> i >> c; mask += i & 0xFF; mask <<= 8;
	is >> i     ; mask += i & 0xFF;
	return mask;
}

void cBanList::Num2Ip(unsigned long mask, string &ip)
{
	ostringstream os;
	unsigned char *i = (unsigned char *)&mask;
	os << int(i[3]) << ".";
	os << int(i[2]) << ".";
	os << int(i[1]) << ".";
	os << int(i[0]);
	ip = os.str();
}

void cBanList::AddNickTempBan(const string &nick, long until, const string &reason, unsigned bantype)
{
	unsigned long hash = mTempNickBanlist.HashLowerString(nick);
	sTempBan *tban = mTempNickBanlist.GetByHash(hash);

	if (tban) {
		tban->mUntil = time_t(until);
		tban->mReason = reason;
		tban->mType = bantype;
	} else {
		tban = new sTempBan(until, reason, bantype);
		mTempNickBanlist.AddWithHash(tban, hash);
	}
}

void cBanList::AddIPTempBan(const string &ip, long until, const string &reason, unsigned bantype)
{
	unsigned long hash = Ip2Num(ip);
	sTempBan *tban = mTempIPBanlist.GetByHash(hash);

	if (tban) {
		tban->mUntil = time_t(until);
		tban->mReason = reason;
		tban->mType = bantype;
	} else {
		tban = new sTempBan(until, reason, bantype);
		mTempIPBanlist.AddWithHash(tban, hash);
	}
}

void cBanList::AddIPTempBan(unsigned long ip, long until, const string &reason, unsigned bantype)
{
	sTempBan *tban = mTempIPBanlist.GetByHash(ip);

	if (tban) {
		tban->mUntil = time_t(until);
		tban->mReason = reason;
		tban->mType = bantype;
	} else {
		tban = new sTempBan(until, reason, bantype);
		mTempIPBanlist.AddWithHash(tban, ip);
	}
}

void cBanList::DelNickTempBan(const string &nick)
{
	unsigned long hash = mTempNickBanlist.HashLowerString(nick);
	sTempBan *tban = mTempNickBanlist.GetByHash(hash);

	if (tban) {
		mTempNickBanlist.RemoveByHash(hash);
		delete tban;
	}
}

void cBanList::DelIPTempBan(const string &ip)
{
	unsigned long hash = Ip2Num(ip);
	sTempBan *tban = mTempIPBanlist.GetByHash(hash);

	if (tban) {
		mTempIPBanlist.RemoveByHash(hash);
		delete tban;
	}
}

void cBanList::DelIPTempBan(unsigned long ip)
{
	sTempBan *tban = mTempIPBanlist.GetByHash(ip);

	if (tban) {
		mTempIPBanlist.RemoveByHash(ip);
		delete tban;
	}
}

bool cBanList::IsNickTempBanned(const string &nick)
{
	return mTempNickBanlist.ContainsHash(mTempNickBanlist.HashLowerString(nick));
}

bool cBanList::IsIPTempBanned(const string &ip)
{
	return mTempIPBanlist.ContainsHash(Ip2Num(ip));
}

bool cBanList::IsIPTempBanned(unsigned long ip)
{
	return mTempIPBanlist.ContainsHash(ip);
}

int cBanList::RemoveOldShortTempBans(long before)
{
	int n = 0;
	tTempNickIPBans::iterator it;
	unsigned long Hash;
	long Until;
	sTempBan *tban;

	for(it = mTempNickBanlist.begin(); it != mTempNickBanlist.end();) {
		Hash = it.mItem->mHash;
		tban = *it;
		Until = tban->mUntil;

		++it;
		if(!before || (Until< before)) {
			this->mTempNickBanlist.RemoveByHash(Hash);
			delete tban;
			n++;
		}
	}
	for(it = mTempIPBanlist.begin(); it != mTempIPBanlist.end();) {
		Hash = it.mItem->mHash;
		tban = *it;
		Until = tban->mUntil;

		++it;
		if(!before || (Until< before)) {
			this->mTempIPBanlist.RemoveByHash(Hash);
			delete tban;
			n++;
		}
	}
	return n;
}
	}; // namespace nTables
}; // namespace nVerliHub
