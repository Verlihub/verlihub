/*
	Copyright (C) 2006-2026 Verlihub Team, info at verlihub dot net

	Verlihub is free software; You can redistribute it
	and modify it under the terms of the GNU General
	Public License as published by the Free Software
	Foundation, either version 3 of the license, or at
	your option any later version.
*/

#include "cadcsession.h"
#include "cadcsessionhost.h"
#include "casyncconn.h"

namespace nVerliHub {
	namespace nProtocol {

void cADCSessionManager::CheckTimeouts(long long nowMsec)
{
	for (tSessionMap::iterator it = mSessions.begin();
		it != mSessions.end(); ++it) {
		nSocket::cAsyncConn *conn = it->first;

		if (!conn || !conn->ok || !conn->mWritable || !conn->mxServer)
			continue;

		nSocket::cADCSessionHost *host =
			dynamic_cast<nSocket::cADCSessionHost*>(conn->mxServer);

		if (!host)
			continue;

		const double timeout = host->ADCSessionTimeoutSeconds(it->second);

		if (timeout <= 0.0)
			continue;

		const long long lastIO = static_cast<long long>(
			conn->mTimeLastIOAction.MiliSec());
		const long long limit = static_cast<long long>(timeout * 1000.0);

		if (limit > 0 && nowMsec >= (lastIO + limit))
			host->OnADCSessionTimeout(conn, it->second);
	}
}

	}; // namespace nProtocol
}; // namespace nVerliHub
