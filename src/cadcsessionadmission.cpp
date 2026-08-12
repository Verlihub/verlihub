/*
	Copyright (C) 2006-2026 Verlihub Team, info at verlihub dot net

	Verlihub is free software; You can redistribute it
	and modify it under the terms of the GNU General
	Public License as published by the Free Software
	Foundation, either version 3 of the license, or at
	your option any later version.
*/

#include "cadcsession.h"
#include "casyncconn.h"

namespace nVerliHub {
	namespace nProtocol {

void cADCSessionManager::IdentifiedConnections(
	std::vector<nSocket::cAsyncConn*> &dest) const
{
	dest.clear();

	for (tSessionMap::const_iterator it = mSessions.begin();
		it != mSessions.end(); ++it) {
		if (!it->first || !it->first->ok || it->second.mNick.empty())
			continue;

		if (it->second.mState == eADC_STATE_VERIFY ||
			it->second.mState == eADC_STATE_NORMAL)
			dest.push_back(it->first);
	}
}

	}; // namespace nProtocol
}; // namespace nVerliHub
