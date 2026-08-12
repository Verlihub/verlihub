/*
	Copyright (C) 2006-2026 Verlihub Team, info at verlihub dot net

	Verlihub is free software; You can redistribute it
	and modify it under the terms of the GNU General
	Public License as published by the Free Software
	Foundation, either version 3 of the license, or at
	your option any later version.
*/

#ifndef CADCSESSIONHOST_H
#define CADCSESSIONHOST_H

namespace nVerliHub {
	namespace nProtocol {
		struct sADCSession;
	};

	namespace nSocket {
		class cAsyncConn;

/** Native ADC session lifecycle sink, independent from cUser/cConnDC. */
class cADCSessionHost
{
	public:
		virtual ~cADCSessionHost() {}
		virtual void OnADCSessionNormal(cAsyncConn *conn,
			const nProtocol::sADCSession &session) = 0;
		virtual void OnADCSessionDetach(cAsyncConn *conn,
			const nProtocol::sADCSession &session) = 0;
		virtual double ADCSessionTimeoutSeconds(
			const nProtocol::sADCSession &session) const = 0;
		virtual void OnADCSessionTimeout(cAsyncConn *conn,
			const nProtocol::sADCSession &session) = 0;
};

	}; // namespace nSocket
}; // namespace nVerliHub

#endif
