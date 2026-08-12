/*
	Copyright (C) 2006-2026 Verlihub Team, info at verlihub dot net

	Verlihub is free software; You can redistribute it
	and modify it under the terms of the GNU General
	Public License as published by the Free Software
	Foundation, either version 3 of the license, or at
	your option any later version.
*/

#ifndef CADCBANPOLICY_H
#define CADCBANPOLICY_H

#include <string>

namespace nVerliHub {
	namespace nTables {
		class cBan;
		class cBanList;
	};

	namespace nProtocol {

/** Connection-type-neutral persistent ban lookup for ADC identities. */
class cADCBanPolicy
{
	public:
		static unsigned int Test(nTables::cBanList *list, nTables::cBan &ban,
			long now, const std::string &ip, const std::string &host,
			const std::string &nick, unsigned int mask);
};

	}; // namespace nProtocol
}; // namespace nVerliHub

#endif
