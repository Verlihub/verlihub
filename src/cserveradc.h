/*
	Copyright (C) 2006-2026 Verlihub Team, info at verlihub dot net

	Verlihub is free software; You can redistribute it
	and modify it under the terms of the GNU General
	Public License as published by the Free Software
	Foundation, either version 3 of the license, or at
	your option any later version.
*/

#ifndef CSERVERADC_H
#define CSERVERADC_H

#include "cadcidentityhost.h"
#include "cadcpluginhost.h"
#include "cadcproto.h"
#include "cadcsessionhost.h"
#include "cconnadc.h"
#include "cserverdc.h"

#include <unicode/unorm2.h>
#include <unicode/ustring.h>

namespace nVerliHub {
	namespace nSocket {

/** ADC parser used on the live hub-client path. */
class cADCServerMessage : public nProtocol::cMessageADC
{
	private:
		static bool IsUTF8NFC(const std::string &text)
		{
			bool ascii = true;

			for (size_t i = 0; i < text.size(); ++i) {
				if (static_cast<unsigned char>(text[i]) >= 0x80) {
					ascii = false;
					break;
				}
			}

			if (ascii)
				return true;

			UErrorCode error = U_ZERO_ERROR;
			int32_t length = 0;
			u_strFromUTF8(NULL, 0, &length, text.data(),
				static_cast<int32_t>(text.size()), &error);

			if (error != U_BUFFER_OVERFLOW_ERROR && U_FAILURE(error))
				return false;

			error = U_ZERO_ERROR;
			std::vector<UChar> utf16(static_cast<size_t>(length) + 1);
		u_strFromUTF8(&utf16[0], static_cast<int32_t>(utf16.size()), &length,
				text.data(), static_cast<int32_t>(text.size()), &error);

			if (U_FAILURE(error))
				return false;

			error = U_ZERO_ERROR;
			const UNormalizer2 *nfc = unorm2_getNFCInstance(&error);

			if (U_FAILURE(error) || !nfc)
				return false;

			const UBool normalized = unorm2_isNormalized(nfc, &utf16[0],
				length, &error);
			return U_SUCCESS(error) && normalized;
		}

	public:
		virtual int Parse()
		{
			if (!IsUTF8NFC(mStr)) {
				mError = true;
				mType = nEnums::eADC_INVALID;
				return mType;
			}

			return nProtocol::cMessageADC::Parse();
		}
};

/**
 * Server-side ADC protocol guard.
 *
 * ADC permits INF updates in NORMAL state, but a nickname change can alter
 * account identity and therefore requires a fresh authentication flow. Until
 * a dedicated re-authenticated rename transaction exists, reject NI changes
 * before cServerADC applies the INF delta. Other NORMAL INF updates continue
 * through the standard ADC application path.
 */
class cADCServerProto : public nProtocol::cADCProto
{
	public:
		virtual nProtocol::cMessageParser *CreateParser()
		{
			return new cADCServerMessage();
		}

		virtual int TreatMsg(nProtocol::cMessageParser *parser, cAsyncConn *conn)
		{
			const int result = nProtocol::cADCProto::TreatMsg(parser, conn);

			if (result != 1 || !parser || !conn)
				return result;

			nProtocol::cMessageADC *msg =
				dynamic_cast<nProtocol::cMessageADC*>(parser);

			if (!msg || msg->Command() != "INF")
				return result;

			const nProtocol::sADCSession *session = Sessions().Find(conn);

			if (!session || session->mState != nProtocol::eADC_STATE_NORMAL)
				return result;

			std::string nick;

			if (!msg->GetNamed("NI", nick) || nick == session->mNick)
				return result;

			std::vector<std::string> flags;
			flags.push_back("FCINF");
			std::string frame;

			if (nProtocol::cADCProto::CreateSTA(frame, "225",
				"Nickname changes require a new authenticated ADC session",
				flags)) {
				frame.push_back('\n');
				conn->Write(frame, true);
			}

			return -1;
		}
};

/** ADC connection factory. */
class cADCConnFactory : public cConnFactory
{
	public:
		cADCConnFactory(cServerDC *server, nProtocol::cADCProto *protocol);
		virtual ~cADCConnFactory();

		virtual cAsyncConn *CreateConn(tSocket sd = 0);
		virtual void DeleteConn(cAsyncConn *&connection);

	private:
		cServerDC *mServer;
		nProtocol::cADCProto *mADCProtocol;
};

/**
 * ADC-facing Verlihub server.
 *
 * cServerDC is temporarily retained as the business/configuration provider.
 * Live client sockets are cConnADC objects and therefore carry no NMDC login,
 * framing, supports, timeout or redirect state.
 */
class cServerADC : public cServerDC,
	public cADCIdentityHost,
	public cADCSessionHost,
	public nPlugin::cADCPluginHost
{
	public:
		cServerADC(string CfgBase = "./.verlihub", const string &ExecPath = "");
		virtual ~cServerADC();

		virtual int OnNewConn(cAsyncConn *conn);
		virtual void OnNewMessage(cAsyncConn *conn, string *msg);
		virtual int OnTimer(const cTime &now);
		virtual bool ValidateADCIdentity(cAsyncConn *conn,
			const string &nick, bool registered);
		virtual void OnADCSessionNormal(cAsyncConn *conn,
			const nProtocol::sADCSession &session);
		virtual void OnADCSessionDetach(cAsyncConn *conn,
			const nProtocol::sADCSession &session);
		virtual double ADCSessionTimeoutSeconds(
			const nProtocol::sADCSession &session) const;
		virtual void OnADCSessionTimeout(cAsyncConn *conn,
			const nProtocol::sADCSession &session);

		virtual nPlugin::cPluginManager *ADCPluginManager()
		{
			return &mPluginManager;
		}

		nProtocol::cADCProto &ADCProtocol() { return mADCProto; }

	private:
		/** ADC overrides the legacy socket reader so frames terminate on LF. */
		virtual int input(cAsyncConn *conn);

		bool SendFrame(cAsyncConn *conn, const string &frame, bool flush = true);
		bool SendStatus(cAsyncConn *conn, const string &code,
			const string &description, const vector<string> &flags,
			bool close = false);
		bool SendHubINF(cAsyncConn *conn);
		bool SetADCRegInfo(cConnADC *conn, const string &nick);
		bool TreatINF(nProtocol::cMessageADC *msg, cConnADC *conn);
		bool TreatPAS(nProtocol::cMessageADC *msg, cConnADC *conn);
		bool TreatNormalMessage(nProtocol::cMessageADC *msg, cConnADC *conn);
		bool EnterNormal(cConnADC *conn);
		bool BroadcastINF(cConnADC *conn);
		bool UpdateNormalINF(nProtocol::cMessageADC *msg, cConnADC *conn);
		bool PrepareInitialINF(nProtocol::cMessageADC *msg, cConnADC *conn,
			vector<string> &sanitized, string &nick, string &cid, string &pid);

		cADCServerProto mADCProto;
};

	}; // namespace nSocket
}; // namespace nVerliHub

#endif
