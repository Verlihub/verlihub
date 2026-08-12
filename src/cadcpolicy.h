/*
	Copyright (C) 2006-2026 Verlihub Team, info at verlihub dot net

	Verlihub is free software; You can redistribute it
	and modify it under the terms of the GNU General
	Public License as published by the Free Software
	Foundation, either version 3 of the license, or at
	your option any later version.
*/

#ifndef CADCPOLICY_H
#define CADCPOLICY_H

#include <string>

namespace nVerliHub {
	namespace nSocket {
		class cConnADC;
		class cServerDC;
	};

	namespace nProtocol {
		class cMessageADC;
		struct sADCSession;

/**
 * Application access policy for ADC hub traffic.
 *
 * This deliberately consumes ADC session/message data rather than cMessageDC
 * or cUser. The legacy server object is currently used only as the provider
 * of configuration, registration penalties and the current clock.
 */
class cADCPolicy
{
	public:
		/**
		 * Return true when a NORMAL-state ADC command may be routed.
		 * On denial, reason contains a user-facing explanation suitable for STA.
		 */
		static bool AllowNormal(nSocket::cServerDC *server,
			nSocket::cConnADC *conn, const sADCSession &session,
			const cMessageADC &msg, std::string &reason);

	private:
		static bool GetINF(const sADCSession &session, const std::string &field,
			std::string &value);
		static bool HasFeature(const sADCSession &session,
			const std::string &feature);
		static bool ParseUInt64(const std::string &value,
			unsigned long long &number);
};

	}; // namespace nProtocol
}; // namespace nVerliHub

#endif
