/*
	Copyright (C) 2003-2005 Daniel Muller, dan at verliba dot cz
	Copyright (C) 2006-2026 Verlihub Team, info at verlihub dot net

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

/*
#include <arpa/inet.h> // inet_pton
#include <netinet/in.h> // sockaddr_in
#include <sys/socket.h> // AF_INET
*/

#include "cconndc.h"
#include "cserverdc.h"
#include "cbanlist.h"
#include "cdcconsole.h" // note: GetIPRange
#include "ctime.h"
#include "i18n.h"
#include "stringutils.h"

#include <stdio.h>
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <sstream>

namespace nVerliHub {
	using namespace nUtils;
	using namespace nEnums;
	using namespace nSocket;

	namespace nTables {

cBanList::cBanList(cServerDC *s):
	cConfMySQL(s->mMySQL),
	mModel(s),
	mS(s),
	mUnBanList(NULL)
{
	mMySQLTable.mName = "banlist";
	AddCol("ip", "varchar(15)", "", true, mModel.mIP);
	AddPrimaryKey("ip");
	AddCol("nick", "varchar(128)", "", true, mModel.mNick);
	AddPrimaryKey("nick");
	AddCol("ban_type", "tinyint(4)", "0", true, mModel.mType);
	AddCol("host", "varchar(255)", "", true, mModel.mHost);
	AddCol("range_fr", "bigint(32)", "", true, mModel.mRangeMin);
	AddCol("range_to", "bigint(32)", "", true, mModel.mRangeMax);
	AddCol("date_start", "int(11)", "0", true, mModel.mDateStart);
	AddCol("date_limit", "int(11)", "", true, mModel.mDateEnd);
	AddCol("last_hit", "int(11)", "", true, mModel.mLastHit);
	AddCol("nick_op", "varchar(128)", "", true, mModel.mNickOp);
	AddCol("reason", "varchar(255)", "", true, mModel.mReason);
	AddCol("note_op", "varchar(255)", "", true, mModel.mNoteOp);
	AddCol("note_usr", "varchar(255)", "", true, mModel.mNoteUsr);
	AddCol("share_size", "varchar(18)", "", true, mModel.mShare);
	mMySQLTable.mExtra = "unique (`ip`, `nick`), ";
	mMySQLTable.mExtra += "index `nick_index` (`nick`), ";
	mMySQLTable.mExtra += "index `date_index` (`date_limit`), ";
	mMySQLTable.mExtra += "index `range_index` (`range_fr`)";
	SetBaseTo(&mModel);
}

cBanList::~cBanList()
{
	RemoveOldShortTempBans(0);
}

cUnBanList::cUnBanList(cServerDC* s):
	cBanList(s),
	mModelUn(s)
{
	mMySQLTable.mName = "unbanlist";
	SetBaseTo(&mModelUn);
	AddCol("date_unban", "int(11)", "", true, mModelUn.mDateUnban);
	AddPrimaryKey("date_unban");
	AddCol("unban_op", "varchar(128)", "", true, mModelUn.mUnNickOp);
	AddCol("unban_reason", "varchar(255)", "", true, mModelUn.mUnReason);
	mMySQLTable.mExtra = "unique (`ip`, `nick`, `date_unban`)";
}

cUnBanList::~cUnBanList()
{}

void cBanList::Cleanup()
{
	const long before = (mS->mTime.Sec() - (3600 * 24 * 7));
	mQuery.OStream() << "delete from `" << mMySQLTable.mName << "` where `date_limit` is not null and `date_limit` < " << before;
	mQuery.Query();
	mQuery.Clear();

	if (mCacheLoaded) { // note: same as above, these bans are not active anyway
		const size_t was = mCache.size();
		mCache.erase(remove_if(mCache.begin(), mCache.end(), [before](const sCacheBan &item) { return (item.mDateEnd && (item.mDateEnd < before)); }), mCache.end());

		if (mCache.size() != was) {
			mCacheChanges++;
			CacheIndex();
		}
	}
}

void cUnBanList::Cleanup()
{
	mQuery.OStream() << "delete from `" << mMySQLTable.mName << "` where `date_unban` < " << (mS->mTime.Sec() - (3600 * 24 * 30));
	mQuery.Query();
	mQuery.Clear();
}

/*
int cBanList::UpdateBan(cBan &ban)
{
	nMySQL::cQuery query(mMySQL);
	SetBaseTo(&ban);
	UpdateFields(query.OStream());
	WherePKey(query.OStream());
	query.Query();
	return 0;
}
*/

/*
bool cBanList::LoadBanByKey(cBan &ban)
{
	SetBaseTo(&ban);
	return LoadPK();
}
*/

void cBanList::NewBan(cBan &ban, cConnDC *connection, const string &nickOp, const string &reason, unsigned length, unsigned mask)
{
	if (connection) {
		ban.mIP = connection->AddrIP();
		ban.mHost = connection->AddrHost();
		ban.mDateStart = mS->mTime.Sec();
		ban.mDateEnd = (length ? (ban.mDateStart + length) : 0); // note: 0 is permanent, same as ban from kick
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

bool cBanList::AddBan(cBan &ban)
{
	switch (1 << ban.mType) { // todo: nick2dbkey, note: host ban without valid host part is not saved and false is returned
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

			if (!this->GetHostSubstring(ban.mHost, ban.mNick, 1))
				return false;

			break;

		case eBF_HOST2:
			ban.mIP = "_host2ban_";

			if (!this->GetHostSubstring(ban.mHost, ban.mNick, 2))
				return false;

			break;

		case eBF_HOST3:
			ban.mIP = "_host3ban_";

			if (!this->GetHostSubstring(ban.mHost, ban.mNick, 3))
				return false;

			break;

		case eBF_HOSTR1:
			ban.mIP = "_hostr1ban_";

			if (!this->GetHostSubstring(ban.mHost, ban.mNick, -1))
				return false;

			break;

		case eBF_SHARE:
			ban.mNick = "_shareban_";
			break;

		case eBF_PREFIX:
			ban.mIP = "_prefixban_";
			break;

		default:
			break;
	}

	cBan OldBan(mS); // copy pk
	OldBan.mIP = ban.mIP;
	OldBan.mNick = ban.mNick;
	SetBaseTo(&OldBan); // load by pk to model
	bool update = false;

	if (LoadPK()) {
		update = true;
		mModel = OldBan;

		if (ban.mReason.size()) // mModel.mReason += " / " + ban.mReason;
			mModel.mReason = ban.mReason;

		if (!ban.mDateEnd || (ban.mDateEnd > mModel.mDateEnd))
			mModel.mDateEnd = ban.mDateEnd;

		mModel.mNickOp = ban.mNickOp;

		if ((1 << ban.mType) == eBF_RANGE) {
			mModel.mRangeMin = ban.mRangeMin;
			mModel.mRangeMax = ban.mRangeMax;
		}

	} else {
		mModel = ban;
	}

	SetBaseTo(&mModel);

	if (update)
		UpdatePK();
	else
		SavePK(false);

	if (mCacheLoaded)
		CacheSave(mModel);

	return true;
}

void cBanList::DropBanned(cBan &ban, const string &reason)
{
	const int type = (1 << ban.mType);
	sCacheBan item; // note: ban has its saved keys after AddBan, same match as ban check on login
	CacheFromBan(item, ban);
	const time_t length = (ban.mDateEnd ? (ban.mDateEnd - ban.mDateStart) : 0);
	cUserCollection::iterator it;
	cUser *user;
	cConnDC *conn;
	unsigned long num;
	bool match;
	ostringstream share;

	for (it = mS->mUserList.begin(); it != mS->mUserList.end(); ++it) {
		user = (cUser*)(*it);
		conn = user->mxConn;

		if (!conn || !conn->mpUser || (user->mClass >= mS->mC.ban_bypass_class)) // note: bots have no connection
			continue;

		switch (type) {
			case eBF_NICKIP:
				match = (CacheMatch(item, conn->AddrIP(), eBF_IP) || CacheMatch(item, user->mNick, eBF_NICK));
				break;

			case eBF_NICK:
				match = CacheMatch(item, user->mNick, eBF_NICK);
				break;

			case eBF_IP:
				match = CacheMatch(item, conn->AddrIP(), eBF_IP);
				break;

			case eBF_RANGE:
				num = conn->AddrToNumber();
				match = ((num >= item.mRangeMin) && (num <= item.mRangeMax));
				break;

			case eBF_HOST1:
			case eBF_HOST2:
			case eBF_HOST3:
			case eBF_HOSTR1:
				match = CacheMatch(item, conn->AddrHost(), type);
				break;

			case eBF_SHARE:
				if ((user->mClass <= eUC_REGUSER) && (user->mShare > 0)) {
					share.str("");
					share << user->mShare; // note: same as database query
					match = CacheMatch(item, share.str(), eBF_SHARE);
				} else {
					match = false;
				}

				break;

			case eBF_PREFIX:
				match = ((user->mClass == eUC_NORMUSER) && CacheMatch(item, user->mNick, eBF_PREFIX));
				break;

			default:
				match = false;
				break;
		}

		if (!match)
			continue;

		if (mS->mC.notify_kicks_to_all > -1) { // message to all
			ostringstream toall;

			if (length) {
				if (reason.size())
					toall << autosprintf(_("%s was banned for %s by %s with reason: %s"), user->mNick.c_str(), cTimePrint(length, 0).AsPeriod().AsString().c_str(), ban.mNickOp.c_str(), reason.c_str());
				else
					toall << autosprintf(_("%s was banned for %s by %s without reason."), user->mNick.c_str(), cTimePrint(length, 0).AsPeriod().AsString().c_str(), ban.mNickOp.c_str());
			} else {
				if (reason.size())
					toall << autosprintf(_("%s was banned permanently by %s with reason: %s"), user->mNick.c_str(), ban.mNickOp.c_str(), reason.c_str());
				else
					toall << autosprintf(_("%s was banned permanently by %s without reason."), user->mNick.c_str(), ban.mNickOp.c_str());
			}

			mS->DCPublicToAll(mS->mC.hub_security, toall.str(), mS->mC.notify_kicks_to_all, int(eUC_MASTER), mS->mC.delayed_chat);
		}

		ostringstream own; // note: same message as ban check on login
		own << _("You are banned from this hub") << ":\r\n";
		ban.DisplayUser(own);
		mS->DCPublicHS(own.str(), conn);
		conn->CloseNice(1000, eCR_KICKED); // note: message goes out first, redirect for kick reason is sent if enabled
	}
}

unsigned int cBanList::TestBan(cBan &ban, cConnDC *conn, const string &nick, unsigned mask)
{
	if (mCacheLoaded && mS->mC.use_banlist_cache) // note: no database query
		return TestBanCache(ban, conn, nick, mask);

	ostringstream query;
	SelectFields(query);
	query << " where (";
	bool first = false;
	unsigned int found = 0;
	string addr, host;

	if (conn) {
		addr = conn->AddrIP();
		host = conn->AddrHost();
	}

	if ((mask & (eBF_NICKIP | eBF_IP)) && conn) { // ip, nick and both are checked in this first one
		AddTestCondition(query, addr, eBF_IP);
		query << " or ";
		first = true;
		found++;
	}

	if ((mask & (eBF_NICKIP | eBF_NICK)) && nick.size()) {
		AddTestCondition(query, nick, eBF_NICK);
		found++;
	}

	if ((mask & eBF_RANGE) && conn) {
		AddTestCondition(query << " or ", addr, eBF_RANGE);
		found++;
	}

	if ((mask & eBF_SHARE) && conn && conn->mpUser) {
		ostringstream os (ostringstream::out);
		os << conn->mpUser->mShare;

		if (first)
			query << " or ";

		AddTestCondition(query, os.str(), eBF_SHARE);
		found++;
	}

	if (conn && host.size()) {
		if (mask & eBF_HOST1) {
			AddTestCondition(query << " or ", host, eBF_HOST1);
			found++;
		}

		if (mask & eBF_HOST2) {
			AddTestCondition(query << " or ", host, eBF_HOST2);
			found++;
		}

		if (mask & eBF_HOST3) {
			AddTestCondition(query << " or ", host, eBF_HOST3);
			found++;
		}

		if (mask & eBF_HOSTR1) {
			AddTestCondition(query << " or ", host, eBF_HOSTR1);
			found++;
		}
	}

	if ((mask & eBF_PREFIX) && nick.size()) {
		AddTestCondition(query << " or ", nick, eBF_PREFIX);
		found++;
	}

	if (!found)
		return 0;

	query << ") and ((`date_limit` >= " << mS->mTime.Sec() << ") or (`date_limit` is null) or (`date_limit` = 0)) order by `date_limit` desc limit 1";

	if (StartQuery(query.str()) == -1)
		return 0;

	SetBaseTo(&ban);
	found = ((Load() >= 0) ? ((ban.mDateEnd) ? 1 : 2) : 0); // 0 = not banned, 1 = temporary ban, 2 = permanent ban
	EndQuery();

	if (found) {
		ban.mLastHit = mS->mTime.Sec();
		UpdatePK();
	}

	return found;
}

void cBanList::DelBan(cBan &Ban)
{
	SetBaseTo(&Ban);
	DeletePK();

	if (mCacheLoaded)
		CacheEraseKey(Ban.mIP, Ban.mNick);
}

int cBanList::DeleteAllBansBy(const string &ip, const string &nick, int mask)
{
	if (!(mask & (eBF_IP | eBF_NICK))) // note: nothing to match
		return 0;

	mQuery.Clear();
	mQuery.OStream() << "delete from `" << mMySQLTable.mName << "` where";

	if (mask & eBF_IP) {
		mQuery.OStream() << " `ip` = '";
		cConfMySQL::WriteStringConstant(mQuery.OStream(), ip);
		mQuery.OStream() << '\'';
	}

	if ((mask & eBF_IP) && (mask & eBF_NICK)) // note: and only between two conditions
		mQuery.OStream() << " and";

	if (mask & eBF_NICK) {
		mQuery.OStream() << " `nick` = '";
		cConfMySQL::WriteStringConstant(mQuery.OStream(), nick);
		mQuery.OStream() << '\'';
	}

	const int res = mQuery.Query();
	mQuery.Clear();

	if (mCacheLoaded) {
		const size_t was = mCache.size();
		mCache.erase(remove_if(mCache.begin(), mCache.end(), [&ip, &nick, mask](const sCacheBan &item) {
			return ((!(mask & eBF_IP) || (item.mIP == ip)) && (!(mask & eBF_NICK) || SameText(item.mNick, nick)));
		}), mCache.end());

		if (mCache.size() != was) {
			mCacheChanges++;
			CacheIndex();
		}
	}

	return res;
}

void cBanList::NewBan(cBan &ban, const cKick &kick, long period, int mask)
{
	ban.mIP = kick.mIP;
	ban.mDateStart = mS->mTime.Sec();

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
	if(!AddTestCondition(mQuery.OStream() << " where ", value, mask)) {
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
         	unban->mDateUnban = mS->mTime.Sec();
			mUnBanList->SetBaseTo(unban);
			mUnBanList->SavePK();
			delete unban;
			unban = NULL;
		}
		i++;
	}
	mQuery.Clear();
	if(deleteEntry) {
		mQuery.OStream() << "delete from " << this->mMySQLTable.mName << " where ";
		AddTestCondition(mQuery.OStream() , value, mask);
		mQuery.Query();
		mQuery.Clear();

		if (mCacheLoaded)
			CacheErase(value, mask);
	}
	return i;
}

bool cBanList::GetHostSubstring(const string &hostname, string &result, int level)
{
	if (hostname.empty()) // note: no host name, for example dns_lookup is disabled, empty part would match every user without host name
		return false;

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

	return (result.size() && (result != ".")); // note: only dot or nothing is no host part
}

bool cBanList::AddTestCondition(ostream &os, const string &value, int mask)
{
	string host;
	unsigned long num = 0;

	switch (mask) {
		case eBF_NICK:
			os << "(`nick` = '"; cConfMySQL::WriteStringConstant(os, value); os << "')";
			//os << "(`ip` = '_nickban_' and `nick` = '"; cConfMySQL::WriteStringConstant(os, value); os << "')";
			break;

		case eBF_IP:
			os << "(`ip` = '"; cConfMySQL::WriteStringConstant(os, value); os << "')";
			//os << "(`nick` = '_ipban_' and `ip` = '"; cConfMySQL::WriteStringConstant(os, value); os << "')";
			break;

		case eBF_RANGE:
			if (Ip2Num(value, num, false)) { // note: single address, all ranges that contain it
				os << "(`nick` = '_rangeban_' and (" << num << " between `range_fr` and `range_to`))";

			} else { // note: whole range, like help says, for example !unbanrange 1.2.3.4-1.2.3.9, only same range
				unsigned long last = 0;

				if (!cDCConsole::GetIPRange(value, num, last)) {
					os << " 0 ";
					return false;
				}

				os << "(`nick` = '_rangeban_' and `range_fr` = " << num << " and `range_to` = " << last << ')';
			}

			break;

		case eBF_SHARE:
			os << "(`nick` = '_shareban_' and `share_size` = '"; cConfMySQL::WriteStringConstant(os, value); os << "')";
			break;

		case eBF_HOST1:
			if (!this->GetHostSubstring(value, host, 1)) {
				os << " 0 ";
				return false;
			}

			os << "(`ip` = '_host1ban_' and `nick` = '"; cConfMySQL::WriteStringConstant(os, host); os << "')";
			break;

		case eBF_HOST2:
			if (!this->GetHostSubstring(value, host, 2)) {
				os << " 0 ";
				return false;
			}

			os << "(`ip` = '_host2ban_' and `nick` = '"; cConfMySQL::WriteStringConstant(os, host); os << "')";
			break;

		case eBF_HOST3:
			if (!this->GetHostSubstring(value, host, 3)) {
				os << " 0 ";
				return false;
			};

			os << "(`ip` = '_host3ban_' and `nick` = '"; cConfMySQL::WriteStringConstant(os, host); os << "')";
			break;

		case eBF_HOSTR1:
			if (!this->GetHostSubstring(value, host, -1)) {
				os << " 0 ";
				return false;
			};

			os << "(`ip` = '_hostr1ban_' and `nick` = '"; cConfMySQL::WriteStringConstant(os, host); os << "')";
			break;

		case eBF_PREFIX:
			os << "(`ip` = '_prefixban_' and `nick` = left('"; cConfMySQL::WriteStringConstant(os, value); os << "', length(`nick`)))";
			break;

		default:
			return false;
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

bool cBanList::Ip2Num(const string &ip, unsigned long &mask, bool zero)
{
	if ((ip.size() < 7) || (ip.size() > 15))
		return false;

	unsigned long p1 = 0, p2 = 0, p3 = 0, p4 = 0;
	stringstream comp;

	if (sscanf(ip.c_str(), "%lu.%lu.%lu.%lu", &p1, &p2, &p3, &p4) == 4) {
		if ((p1 >= 0) && (p1 <= 255)) { // p1
			if ((p2 >= 0) && (p2 <= 255)) { // p2
				if ((p3 >= 0) && (p3 <= 255)) { // p3
					if ((p4 >= 0) && (p4 <= 255)) { // p4
						comp << p1 << '.' << p2 << '.' << p3 << '.' << p4;

						if (ip == comp.str()) {
							mask = (p1 << 24) + (p2 << 16) + (p3 << 8) + p4;

							if (((zero && (mask >= 0UL)) || (!zero && (mask > 0UL))) && (mask <= 4294967295UL))
								return true;

							mask = 0; // reset for unvalidated result
						}
					}
				}
			}
		}
	}

	return false;

	/*
	struct sockaddr_in sa; // this returns different number

	if (inet_pton(AF_INET, ip.c_str(), &(sa.sin_addr)) == 1)
		return sa.sin_addr.s_addr;

	return 0;
	*/

	/*
	int i;
	char c;
	istringstream is(ip);
	unsigned long mask = 0;
	is >> i >> c;
	mask += i & 0xFF;
	mask <<= 8;
	is >> i >> c;
	mask += i & 0xFF;
	mask <<= 8;
	is >> i >> c;
	mask += i & 0xFF;
	mask <<= 8;
	is >> i;
	mask += i & 0xFF;
	return mask;
	*/
}

void cBanList::Num2Ip(unsigned long mask, string &ip)
{
	/*
	struct sockaddr_in sa; // this returns different number
	char temp[INET_ADDRSTRLEN];
	sa.sin_addr.s_addr = mask;

	if (inet_ntop(AF_INET, &(sa.sin_addr), temp, INET_ADDRSTRLEN))
		ip = temp;
	*/

	ostringstream os;
	unsigned char *i = (unsigned char*)&mask;
	os << int(i[3]) << '.';
	os << int(i[2]) << '.';
	os << int(i[1]) << '.';
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
	unsigned long hash = 0;

	if (!Ip2Num(ip, hash, false))
		return;

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
		tban = NULL;
	}
}

void cBanList::DelIPTempBan(const string &ip)
{
	unsigned long hash = 0;

	if (!Ip2Num(ip, hash, false))
		return;

	sTempBan *tban = mTempIPBanlist.GetByHash(hash);

	if (tban) {
		mTempIPBanlist.RemoveByHash(hash);
		delete tban;
		tban = NULL;
	}
}

void cBanList::DelIPTempBan(unsigned long ip)
{
	sTempBan *tban = mTempIPBanlist.GetByHash(ip);

	if (tban) {
		mTempIPBanlist.RemoveByHash(ip);
		delete tban;
		tban = NULL;
	}
}

void cBanList::ShowNickTempBan(ostream &os, const string &nick)
{
	unsigned long hash = mTempNickBanlist.HashLowerString(nick);
	sTempBan *tban = mTempNickBanlist.GetByHash(hash);

	if (tban) {
		os << "\r\n\r\n";
		os << " [*] " << autosprintf(_("Nick: %s"), nick.c_str()) << "\r\n";
		cTimePrint until(tban->mUntil - mS->mTime.Sec());
		os << " [*] " << _("Time") << ": " << autosprintf(_("%s left"), until.AsPeriod().AsString().c_str()) << "\r\n";
		os << " [*] " << _("Type") << ": ";

		switch (tban->mType) {
			case eBT_PASSW:
				os << _("Password");
				break;

			case eBT_RECON:
				os << _("Reconnect");
				break;

			case eBT_FLOOD:
				os << _("Flood");
				break;

			case eBT_CLONE:
				os << _("Clone");
				break;

			default:
				os << _("Unknown");
				break;
		}

		os << "\r\n";

		if (tban->mReason.size())
			os << " [*] " << autosprintf(_("Reason: %s"), tban->mReason.c_str()) << "\r\n";

	} else {
		os << ' ' << _("Not found");
	}
}

void cBanList::ShowIPTempBan(ostream &os, const string &ip)
{
	unsigned long hash = 0;

	if (!Ip2Num(ip, hash, false)) {
		os << ' ' << _("Bad IP specified");
		return;
	}

	sTempBan *tban = mTempIPBanlist.GetByHash(hash);

	if (tban) {
		os << "\r\n\r\n";
		os << " [*] " << autosprintf(_("IP: %s"), ip.c_str()) << "\r\n";
		cTimePrint until(tban->mUntil - mS->mTime.Sec());
		os << " [*] " << _("Time") << ": " << autosprintf(_("%s left"), until.AsPeriod().AsString().c_str()) << "\r\n";
		os << " [*] " << _("Type") << ": ";

		switch (tban->mType) {
			case eBT_PASSW:
				os << _("Password");
				break;

			case eBT_RECON:
				os << _("Reconnect");
				break;

			case eBT_FLOOD:
				os << _("Flood");
				break;

			case eBT_CLONE:
				os << _("Clone");
				break;

			default:
				os << _("Unknown");
				break;
		}

		os << "\r\n";

		if (tban->mReason.size())
			os << " [*] " << autosprintf(_("Reason: %s"), tban->mReason.c_str()) << "\r\n";

	} else {
		os << ' ' << _("Not found");
	}
}

bool cBanList::IsNickTempBanned(const string &nick)
{
	return mTempNickBanlist.ContainsHash(mTempNickBanlist.HashLowerString(nick));
}

bool cBanList::IsIPTempBanned(const string &ip)
{
	unsigned long hash = 0;
	return Ip2Num(ip, hash, false) && mTempIPBanlist.ContainsHash(hash);
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
			tban = NULL;
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
			tban = NULL;
			n++;
		}
	}
	return n;
}

/*
	ban cache
*/

string cBanList::CacheQuery()
{
	ostringstream os; // note: only active bans, column order is used by CacheFromRow()
	os << "select `ip`, `nick`, `ban_type`, `host`, `range_fr`, `range_to`, `date_start`, `date_limit`, `last_hit`, `nick_op`, `reason`, `note_op`, `note_usr`, `share_size` from `" << mMySQLTable.mName << '`';
	os << " where (`date_limit` is null) or (`date_limit` = 0) or (`date_limit` >= " << mS->mTime.Sec() << ')';
	return os.str();
}

void cBanList::CacheFromRow(sCacheBan &dest, const vector<string> &row)
{
	if (row.size() < 14)
		return;

	dest.mIP = row[0];
	dest.mNick = row[1];
	dest.mType = strtoul(row[2].c_str(), NULL, 10);
	dest.mHost = row[3];
	dest.mRangeMin = strtoul(row[4].c_str(), NULL, 10);
	dest.mRangeMax = strtoul(row[5].c_str(), NULL, 10);
	dest.mDateStart = strtol(row[6].c_str(), NULL, 10);
	dest.mDateEnd = strtol(row[7].c_str(), NULL, 10); // note: null is empty string, that is 0 and permanent like in database query
	dest.mLastHit = strtol(row[8].c_str(), NULL, 10);
	dest.mNickOp = row[9];
	dest.mReason = row[10];
	dest.mNoteOp = row[11];
	dest.mNoteUsr = row[12];
	dest.mShare = row[13];
}

void cBanList::CacheFromBan(sCacheBan &dest, const cBan &ban)
{
	dest.mIP = ban.mIP;
	dest.mNick = ban.mNick;
	dest.mType = ban.mType;
	dest.mHost = ban.mHost;
	dest.mRangeMin = ban.mRangeMin;
	dest.mRangeMax = ban.mRangeMax;
	dest.mDateStart = ban.mDateStart;
	dest.mDateEnd = ban.mDateEnd;
	dest.mLastHit = ban.mLastHit;
	dest.mNickOp = ban.mNickOp;
	dest.mReason = ban.mReason;
	dest.mNoteOp = ban.mNoteOp;
	dest.mNoteUsr = ban.mNoteUsr;
	ostringstream os;
	os << ban.mShare; // note: same as it is written to database
	dest.mShare = os.str();
}

void cBanList::CacheToBan(const sCacheBan &src, cBan &dest)
{
	dest.mIP = src.mIP;
	dest.mNick = src.mNick;
	dest.mType = src.mType;
	dest.mHost = src.mHost;
	dest.mRangeMin = src.mRangeMin;
	dest.mRangeMax = src.mRangeMax;
	dest.mDateStart = src.mDateStart;
	dest.mDateEnd = src.mDateEnd;
	dest.mLastHit = src.mLastHit;
	dest.mNickOp = src.mNickOp;
	dest.mReason = src.mReason;
	dest.mNoteOp = src.mNoteOp;
	dest.mNoteUsr = src.mNoteUsr;
	dest.mShare = strtoull(src.mShare.c_str(), NULL, 10);
}

string cBanList::CacheKey(const string &text)
{
	string key(text);

	for (auto &c: key) // note: database compares case insensitive, only ascii here
		c = tolower((unsigned char)c);

	return key;
}

bool cBanList::SameText(const string &one, const string &two)
{
	if (one.size() != two.size())
		return false;

	for (size_t i = 0; i < one.size(); i++) {
		if (tolower((unsigned char)one[i]) != tolower((unsigned char)two[i]))
			return false;
	}

	return true;
}

bool cBanList::LoadCache()
{
	mQuery.Clear();
	mQuery.OStream() << CacheQuery();

	if (mQuery.Query() <= 0) {
		mQuery.Clear();
		return false;
	}

	vector<sCacheBan> list;
	mQuery.StoreResult();
	const int cols = (mQuery.GetResult() ? mQuery.Cols() : 0);
	MYSQL_ROW row;
	vector<string> values;

	while ((row = mQuery.Row())) {
		values.clear();

		for (int col = 0; col < cols; col++)
			values.emplace_back(row[col] ? row[col] : "");

		list.emplace_back();
		CacheFromRow(list.back(), values);
	}

	mQuery.Clear();
	CacheSet(list);
	return true;
}

void cBanList::Clean()
{
	TruncateTable();

	if (mCacheLoaded) { // note: change counter is increased, so reload that is on its way is made again
		vector<sCacheBan> none;
		CacheSet(none);
	}
}

void cBanList::ReloadCache()
{
	if (!mS->mC.use_banlist_cache) { // note: cache was disabled
		if (mCacheLoaded) {
			vector<sCacheBan> none;
			CacheSet(none);
			mCacheLoaded = false;
		}

		return;
	}

	if (!mCacheLoaded) { // note: first load blocks, like other caches
		LoadCache();
		return;
	}

	if (mCacheReloading) // note: already on its way
		return;

	const unsigned long changes = mCacheChanges;
	mCacheReloading = true;

	const bool queued = mS->mMySQLWorker.Select(CacheQuery(), [this, changes](bool ok, const nMySQL::cMySQLWorker::tRows &rows) { // note: runs on main thread
		mCacheReloading = false;

		if (!ok || !mS->mC.use_banlist_cache) // note: keep old cache, error is already logged
			return;

		if (changes != mCacheChanges) { // note: cache was changed meanwhile, result might be older than that change
			ReloadCache();
			return;
		}

		vector<sCacheBan> list(rows.size());

		for (size_t i = 0; i < rows.size(); i++)
			CacheFromRow(list[i], rows[i]);

		CacheSet(list);
	});

	if (!queued) { // note: database thread is not available, reload on main connection
		mCacheReloading = false;
		LoadCache();
	}
}

void cBanList::CacheSet(vector<sCacheBan> &list)
{
	mCache.swap(list);
	mCacheLoaded = true;
	mCacheChanges++;
	CacheIndex();
}

void cBanList::CacheIndex()
{
	mCacheByIP.clear();
	mCacheByNick.clear();
	mCacheOther.clear();
	mCacheByIP.reserve(mCache.size());
	mCacheByNick.reserve(mCache.size());

	for (size_t i = 0; i < mCache.size(); i++)
		CacheIndexItem(i);
}

void cBanList::CacheIndexItem(size_t index)
{
	const sCacheBan &item = mCache[index];
	mCacheByIP.emplace(item.mIP, index);
	mCacheByNick.emplace(CacheKey(item.mNick), index);

	if (SameText(item.mNick, "_rangeban_") || SameText(item.mNick, "_shareban_") || SameText(item.mIP, "_prefixban_"))
		mCacheOther.push_back(index);
}

void cBanList::CacheSave(const cBan &ban)
{
	sCacheBan item;
	CacheFromBan(item, ban);
	mCacheChanges++;
	const auto range = mCacheByIP.equal_range(item.mIP);

	for (auto it = range.first; it != range.second; ++it) { // note: same primary key as in database, ip and nick
		sCacheBan &old = mCache[it->second];

		if (SameText(old.mNick, item.mNick)) {
			old = item; // note: keys are same, so index stays valid
			return;
		}
	}

	mCache.push_back(item);
	CacheIndexItem(mCache.size() - 1); // note: new ban is added to index alone, gives same index as full build
}

void cBanList::CacheErase(const string &value, int mask)
{
	const size_t was = mCache.size();
	mCache.erase(remove_if(mCache.begin(), mCache.end(), [this, &value, mask](const sCacheBan &item) { return CacheMatch(item, value, mask); }), mCache.end());

	if (mCache.size() != was) {
		mCacheChanges++;
		CacheIndex();
	}
}

void cBanList::CacheEraseKey(const string &ip, const string &nick)
{
	const size_t was = mCache.size();
	mCache.erase(remove_if(mCache.begin(), mCache.end(), [&ip, &nick](const sCacheBan &item) { return ((item.mIP == ip) && SameText(item.mNick, nick)); }), mCache.end());

	if (mCache.size() != was) {
		mCacheChanges++;
		CacheIndex();
	}
}

bool cBanList::CacheMatch(const sCacheBan &item, const string &value, int mask)
{
	// note: same conditions as AddTestCondition() builds for database
	string host;
	unsigned long num = 0;

	switch (mask) {
		case eBF_NICK:
			return SameText(item.mNick, value);

		case eBF_IP:
			return (item.mIP == value);

		case eBF_RANGE:
			if (Ip2Num(value, num, false))
				return (SameText(item.mNick, "_rangeban_") && (num >= item.mRangeMin) && (num <= item.mRangeMax));

			{
				unsigned long last = 0;
				return (cDCConsole::GetIPRange(value, num, last) && SameText(item.mNick, "_rangeban_") && (item.mRangeMin == num) && (item.mRangeMax == last));
			}

		case eBF_SHARE:
			return (SameText(item.mNick, "_shareban_") && (item.mShare == value));

		case eBF_HOST1:
			return (GetHostSubstring(value, host, 1) && SameText(item.mIP, "_host1ban_") && SameText(item.mNick, host));

		case eBF_HOST2:
			return (GetHostSubstring(value, host, 2) && SameText(item.mIP, "_host2ban_") && SameText(item.mNick, host));

		case eBF_HOST3:
			return (GetHostSubstring(value, host, 3) && SameText(item.mIP, "_host3ban_") && SameText(item.mNick, host));

		case eBF_HOSTR1:
			return (GetHostSubstring(value, host, -1) && SameText(item.mIP, "_hostr1ban_") && SameText(item.mNick, host));

		case eBF_PREFIX: // note: nick starts with banned prefix
			return (SameText(item.mIP, "_prefixban_") && (item.mNick.size() <= value.size()) && SameText(item.mNick, value.substr(0, item.mNick.size())));

		default:
			return false;
	}
}

unsigned int cBanList::TestBanCache(cBan &ban, cConnDC *conn, const string &nick, unsigned mask)
{
	string addr, host, share;

	if (conn) {
		addr = conn->AddrIP();
		host = conn->AddrHost();

		if (conn->mpUser) {
			ostringstream os;
			os << conn->mpUser->mShare; // note: same as database query
			share = os.str();
		}
	}

	const long now = mS->mTime.Sec();
	const unsigned long num = (conn ? conn->AddrToNumber() : 0); // note: ip number of connection, range bans compare it without parsing text for every ban
	size_t best = mCache.size(); // note: none

	auto take = [&](size_t index) {
		if ((best == mCache.size()) || (mCache[index].mDateEnd > mCache[best].mDateEnd)) // note: same order as database query, latest end date first, permanent last
			best = index;
	};

	auto check = [&](size_t index, const string &value, int type) {
		const sCacheBan &item = mCache[index];

		if (item.mDateEnd && (item.mDateEnd < now)) // note: expired
			return;

		if (CacheMatch(item, value, type))
			take(index);
	};

	auto by_key = [&](const unordered_multimap<string, size_t> &map, const string &key, const string &value, int type) {
		const auto range = map.equal_range(key);

		for (auto it = range.first; it != range.second; ++it)
			check(it->second, value, type);
	};

	if ((mask & (eBF_NICKIP | eBF_IP)) && conn)
		by_key(mCacheByIP, addr, addr, eBF_IP);

	if ((mask & (eBF_NICKIP | eBF_NICK)) && nick.size())
		by_key(mCacheByNick, CacheKey(nick), nick, eBF_NICK);

	if (conn && host.size()) { // note: host bans keep host part in nick column
		const int levels[4][2] = {{eBF_HOST1, 1}, {eBF_HOST2, 2}, {eBF_HOST3, 3}, {eBF_HOSTR1, -1}};
		string part;

		for (const auto &level: levels) {
			if ((mask & level[0]) && GetHostSubstring(host, part, level[1]))
				by_key(mCacheByNick, CacheKey(part), host, level[0]);
		}
	}

	for (const size_t index: mCacheOther) {
		if ((mask & eBF_RANGE) && conn) { // note: same match as CacheMatch for single address
			const sCacheBan &item = mCache[index];

			if ((!item.mDateEnd || (item.mDateEnd >= now)) && (num >= item.mRangeMin) && (num <= item.mRangeMax) && SameText(item.mNick, "_rangeban_"))
				take(index);
		}

		if ((mask & eBF_SHARE) && share.size())
			check(index, share, eBF_SHARE);

		if ((mask & eBF_PREFIX) && nick.size())
			check(index, nick, eBF_PREFIX);
	}

	if (best == mCache.size())
		return 0;

	sCacheBan &item = mCache[best];
	item.mLastHit = now;
	CacheToBan(item, ban);
	ostringstream query; // note: only last hit changes, write it on database thread
	query << "update `" << mMySQLTable.mName << "` set `last_hit` = " << now << " where `ip` = '";
	cConfMySQL::WriteStringConstant(query, item.mIP);
	query << "' and `nick` = '";
	cConfMySQL::WriteStringConstant(query, item.mNick);
	query << '\'';
	mS->mMySQLWorker.Write(query.str());
	return (item.mDateEnd ? 1 : 2); // note: 1 = temporary ban, 2 = permanent ban
}

	}; // namespace nTables
}; // namespace nVerliHub
