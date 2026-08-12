/*
	Copyright (C) 2006-2026 Verlihub Team, info at verlihub dot net

	Verlihub is free software; You can redistribute it
	and modify it under the terms of the GNU General
	Public License as published by the Free Software
	Foundation, either version 3 of the license, or at
	your option any later version.
*/

#ifndef CADCPROTO_H
#define CADCPROTO_H

#include "cmessageadc.h"
#include "cadcsession.h"
#include "cprotocol.h"

#include <set>
#include <string>
#include <vector>

namespace nVerliHub {
	namespace nProtocol {

/**
 * Core ADC protocol framing and login-state support.
 *
 * The class deliberately owns only ADC wire/state concerns. Verlihub-specific
 * account, ban, plugin and permission checks can be attached at the INF/PAS
 * transition without reintroducing legacy wire framing.
 */
class cADCProto : public cProtocol
{
	public:
		cADCProto();
		virtual ~cADCProto();

		virtual cMessageParser *CreateParser();
		virtual void DeleteParser(cMessageParser *parser);
		virtual int TreatMsg(cMessageParser *msg, nSocket::cAsyncConn *conn);

		void OnDisconnect(nSocket::cAsyncConn *conn);
		cADCSessionManager &Sessions() { return mSessions; }
		const cADCSessionManager &Sessions() const { return mSessions; }

		static bool CreateHub(std::string &dest, const std::string &command,
			const std::vector<std::string> &parameters);
		static bool CreateInfo(std::string &dest, const std::string &command,
			const std::vector<std::string> &parameters);
		static bool CreateClient(std::string &dest, const std::string &command,
			const std::vector<std::string> &parameters);
		static bool CreateBroadcast(std::string &dest, const std::string &command,
			const std::string &sourceSID, const std::vector<std::string> &parameters);
		static bool CreateDirect(std::string &dest, const std::string &command,
			const std::string &sourceSID, const std::string &targetSID,
			const std::vector<std::string> &parameters, bool echo = false);
		static bool CreateFeature(std::string &dest, const std::string &command,
			const std::string &sourceSID, const std::string &features,
			const std::vector<std::string> &parameters);
		static bool CreateUDP(std::string &dest, const std::string &command,
			const std::string &sourceCID, const std::vector<std::string> &parameters);

		static bool CreateSUP(std::string &dest, bool fromHub,
			const std::vector<std::string> &features);
		static bool CreateSID(std::string &dest, const std::string &sid);
		static bool CreateSTA(std::string &dest, const std::string &code,
			const std::string &description, const std::vector<std::string> &flags);

	private:
		static bool ValidCommand(const std::string &command);
		static bool Build(std::string &dest, char type, const std::string &command,
			const std::vector<std::string> &header,
			const std::vector<std::string> &parameters);
		static bool ParseSUPFeatures(const cMessageADC &msg,
			std::set<std::string> &add, std::set<std::string> &remove);
		static bool SendFrame(nSocket::cAsyncConn *conn, const std::string &frame,
			bool flush = true);
		static bool SendSTA(nSocket::cAsyncConn *conn, const std::string &code,
			const std::string &description, const std::vector<std::string> &flags);
		bool ValidateRouting(cMessageADC *msg, nSocket::cAsyncConn *conn);
		int TreatSUP(cMessageADC *msg, nSocket::cAsyncConn *conn);

		cADCSessionManager mSessions;
};

	}; // namespace nProtocol
}; // namespace nVerliHub

#endif
