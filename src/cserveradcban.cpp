/*
	Copyright (C) 2006-2026 Verlihub Team, info at verlihub dot net

	Verlihub is free software; You can redistribute it
	and modify it under the terms of the GNU General
	Public License as published by the Free Software
	Foundation, either version 3 of the license, or at
	your option any later version.
*/

#include "cserveradc.h"
#include "cadcbanpolicy.h"
#include "cban.h"
#include "cbanlist.h"
#include "cmaxminddb.h"
#include "creguserinfo.h"
#include "stringutils.h"

#include <sstream>

namespace nVerliHub {
	using namespace nEnums;
	using namespace nTables;
	using namespace nUtils;

	namespace nSocket {

bool cServerADC::ValidateADCIdentity(cAsyncConn *raw, const string &nick,
	bool registered)
{
	cConnADC *conn = dynamic_cast<cConnADC*>(raw);

	if (!conn || nick.empty() || !mBanList)
		return false;

	// Keep configurable hub nickname policy, but do not inherit NMDC wire
	// delimiters such as '$' and '|' as implicit forbidden characters. ADC
	// framing/escaping has its own syntax and adc_valid_nick() handles controls.
	if (!registered && !mC.nick_chars.empty()) {
		for (size_t i = 0; i < nick.size(); ++i) {
			if (mC.nick_chars.find(nick[i]) == string::npos) {
				if (conn->Log(1))
					conn->LogStream() << "ADC identity rejected by configured nick_chars" << endl;

				return false;
			}
		}
	}

	// Prefix restrictions are a hub policy rather than an NMDC framing rule.
	// Match the historical semantics for unregistered users while keeping
	// registered accounts exempt from a prefix intended for guests.
	if (!registered && !mC.nick_prefix.empty()) {
		istringstream prefixes(mC.nick_prefix);
		string prefix;
		const string candidate = mC.nick_prefix_nocase ?
			toLower(nick, true) : nick;
		bool matched = false;

		while (prefixes >> prefix) {
			if (mC.nick_prefix_nocase)
				prefix = toLower(prefix, true);

			if (candidate.size() >= prefix.size() &&
				candidate.compare(0, prefix.size(), prefix) == 0) {
				matched = true;
				break;
			}
		}

		if (!matched) {
			if (conn->Log(1))
				conn->LogStream() << "ADC identity rejected by configured nick prefix" << endl;

			return false;
		}
	}

	// Country-prefix policy is also independent from the old NMDC framing.
	// Resolve the country directly from the shared MaxMind service instead of
	// relying on cConnDC's geo cache/SetGeoZone lifecycle.
	if (!registered && mC.nick_prefix_cc && mMaxMindDB) {
		string cc;

		if (mMaxMindDB->GetCC(conn->AddrIP(), cc) && cc.size() == 2 && cc != "--") {
			const string required = "[" + cc + "]";

			if (nick.size() < required.size() ||
				nick.compare(0, required.size(), required) != 0) {
				if (conn->Log(1))
					conn->LogStream() << "ADC identity rejected by country nick prefix: "
						<< required << endl;

				return false;
			}
		}
	}

	const long now = mTime.Sec();

	// Temporary IP bans are checked on connection as well, but repeat the test
	// here so identity validation remains complete when used independently.
	if (mBanList->IsIPTempBanned(conn->AddrToNumber())) {
		cBanList::sTempBan *temp =
			mBanList->mTempIPBanlist.GetByHash(conn->AddrToNumber());

		if (temp && temp->mUntil > now) {
			if (conn->Log(1))
				conn->LogStream() << "ADC identity rejected by temporary IP ban: "
					<< temp->mReason << endl;

			return false;
		}

		mBanList->DelIPTempBan(conn->AddrToNumber());
	}

	if (mBanList->IsNickTempBanned(nick)) {
		cBanList::sTempBan *temp = mBanList->mTempNickBanlist.GetByHash(
			mBanList->mTempNickBanlist.HashLowerString(nick));

		if (temp && temp->mUntil > now) {
			if (conn->Log(1))
				conn->LogStream() << "ADC identity rejected by temporary nick ban: "
					<< temp->mReason << endl;

			return false;
		}

		mBanList->DelNickTempBan(nick);
	}

	const int userClass = conn->mRegInfo ? conn->mRegInfo->mClass : eUC_NORMUSER;

	// Preserve the legacy ban bypass semantics for registered classes. Temporary
	// bans above are intentionally not bypassed, matching ValidateUser().
	if (userClass >= mC.ban_bypass_class)
		return true;

	cBan ban(this);
	unsigned int mask = eBF_NICK | eBF_NICKIP | eBF_RANGE |
		eBF_HOST1 | eBF_HOST2 | eBF_HOST3 | eBF_HOSTR1;

	// Prefix bans historically apply to unregistered users only.
	if (!registered)
		mask |= eBF_PREFIX;

	const unsigned int banned = nProtocol::cADCBanPolicy::Test(
		mBanList, ban, now, conn->AddrIP(), conn->AddrHost(), nick, mask);

	if (!banned)
		return true;

	if (conn->Log(1)) {
		conn->LogStream() << "ADC identity rejected by persistent ban type "
			<< ban.mType;

		if (!ban.mReason.empty())
			conn->LogStream() << ": " << ban.mReason;

		conn->LogStream() << endl;
	}

	return false;
}

	}; // namespace nSocket
}; // namespace nVerliHub
