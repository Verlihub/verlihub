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

#include "cadcproto.h"
#include "cserverdc.h"

namespace nVerliHub {
	namespace nSocket {

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
 * cServerDC is temporarily retained as the business layer for configuration,
 * users, bans, database and plugins. Client sockets themselves are ADC framed
 * from creation and no legacy greeting or parser is used on the wire.
 */
class cServerADC : public cServerDC
{
	public:
		cServerADC(string CfgBase = "./.verlihub", const string &ExecPath = "");
		virtual ~cServerADC();

		virtual int OnNewConn(cAsyncConn *conn);
		virtual void OnNewMessage(cAsyncConn *conn, string *msg);

		nProtocol::cADCProto &ADCProtocol() { return mADCProto; }

	private:
		bool SendFrame(cAsyncConn *conn, const string &frame, bool flush = true);
		bool SendStatus(cAsyncConn *conn, const string &code,
			const string &description, const vector<string> &flags,
			bool close = false);
		bool SendHubINF(cAsyncConn *conn);
		bool TreatINF(nProtocol::cMessageADC *msg, cConnDC *conn);
		bool TreatPAS(nProtocol::cMessageADC *msg, cConnDC *conn);
		bool TreatNormalMessage(nProtocol::cMessageADC *msg, cConnDC *conn);
		bool EnterNormal(cConnDC *conn);
		bool BroadcastINF(cConnDC *conn);
		bool UpdateNormalINF(nProtocol::cMessageADC *msg, cConnDC *conn);
		bool PrepareInitialINF(nProtocol::cMessageADC *msg, cConnDC *conn,
			vector<string> &sanitized, string &nick, string &cid, string &pid);

		nProtocol::cADCProto mADCProto;
};

	}; // namespace nSocket
}; // namespace nVerliHub

#endif
