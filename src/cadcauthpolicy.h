/*
	Copyright (C) 2006-2026 Verlihub Team, info at verlihub dot net

	Verlihub is free software; You can redistribute it
	and modify it under the terms of the GNU General
	Public License as published by the Free Software
	Foundation, either version 3 of the license, or at
	your option any later version.
*/

#ifndef CADCAUTHPOLICY_H
#define CADCAUTHPOLICY_H

namespace nVerliHub {
	namespace nSocket {
		class cAsyncConn;
	};

	namespace nProtocol {
		class cMessageADC;
		struct sADCSession;

/** ADC authentication-side security policy independent from NMDC login. */
class cADCAuthPolicy
{
	public:
		static void ObservePasswordFailure(nSocket::cAsyncConn *conn,
			const cMessageADC &msg, const sADCSession &session);
};

	}; // namespace nProtocol
}; // namespace nVerliHub

#endif
