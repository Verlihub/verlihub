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
#include "creguserinfo.h"

namespace nVerliHub {
	using namespace nEnums;
	using namespace nTables;

	namespace nSocket {

bool cServerADC::ValidateADCIdentity(cAsyncConn *raw, const string &nick,
	bool registered)
{
	cConnADC *conn = dynamic_cast<cConnADC*>(raw);

	if (!conn || nick.empty() || !mBanList)
		return false;

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
