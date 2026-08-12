/*
	Copyright (C) 2006-2026 Verlihub Team, info at verlihub dot net

	Verlihub is free software; You can redistribute it
	and modify it under the terms of the GNU General
	Public License as published by the Free Software
	Foundation, either version 3 of the license, or at
	your option any later version.
*/

#include "cadcbanpolicy.h"
#include "cban.h"
#include "cbanlist.h"

#include <sstream>

namespace nVerliHub {
	using namespace nEnums;

	namespace nProtocol {

namespace {

void AddCondition(nTables::cBanList *list, std::ostringstream &query,
	bool &haveCondition, const std::string &value, int flag)
{
	if (!list || value.empty())
		return;

	if (haveCondition)
		query << " or ";

	// Invalid range/host values are emitted as a false SQL expression by the
	// legacy helper, so they are still safe members of the OR group.
	list->AddTestCondition(query, value, flag);
	haveCondition = true;
}

} // namespace

unsigned int cADCBanPolicy::Test(nTables::cBanList *list, nTables::cBan &ban,
	long now, const std::string &ip, const std::string &host,
	const std::string &nick, unsigned int mask)
{
	if (!list)
		return 0;

	std::ostringstream query;
	list->SelectFields(query);
	query << " where (";
	bool haveCondition = false;

	if (mask & (eBF_NICKIP | eBF_IP))
		AddCondition(list, query, haveCondition, ip, eBF_IP);

	if (mask & (eBF_NICKIP | eBF_NICK))
		AddCondition(list, query, haveCondition, nick, eBF_NICK);

	if (mask & eBF_RANGE)
		AddCondition(list, query, haveCondition, ip, eBF_RANGE);

	if (!host.empty()) {
		if (mask & eBF_HOST1)
			AddCondition(list, query, haveCondition, host, eBF_HOST1);

		if (mask & eBF_HOST2)
			AddCondition(list, query, haveCondition, host, eBF_HOST2);

		if (mask & eBF_HOST3)
			AddCondition(list, query, haveCondition, host, eBF_HOST3);

		if (mask & eBF_HOSTR1)
			AddCondition(list, query, haveCondition, host, eBF_HOSTR1);
	}

	if (mask & eBF_PREFIX)
		AddCondition(list, query, haveCondition, nick, eBF_PREFIX);

	if (!haveCondition)
		return 0;

	query << ") and ((`date_limit` >= " << now
		<< ") or (`date_limit` is null) or (`date_limit` = 0))"
		<< " order by `date_limit` desc limit 1";

	if (list->StartQuery(query.str()) == -1)
		return 0;

	list->SetBaseTo(&ban);
	const int loaded = list->Load();
	const unsigned int found = (loaded >= 0) ? (ban.mDateEnd ? 1U : 2U) : 0U;
	list->EndQuery();

	if (found) {
		ban.mLastHit = now;
		list->UpdatePK();
	}

	return found;
}

	}; // namespace nProtocol
}; // namespace nVerliHub
