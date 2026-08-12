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

/**
 * ADC connection factory.
 *
 * Client connections are created directly and are attached to cADCProto from
 * the first byte received. cConnDC is currently retained only as the common
 * Verlihub connection/account container while that class is being generalized.
 */
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

		// Temporary cleanup bridge only. It is never used to create or bind
		// client connections; ADC owns the complete live wire path.
		cDCConnFactory *mCleanupFactory;
};

/**
 * ADC-facing Verlihub server.
 *
 * cServerDC is temporarily retained as the business layer for configuration,
 * users, bans, database and plugins. Client sockets themselves are ADC framed
 * from creation and no legacy greeting or legacy parser is used on the wire.
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
		bool SendHubINF(cAsyncConn *conn);
		nProtocol::cADCProto mADCProto;
};

	}; // namespace nSocket
}; // namespace nVerliHub

#endif
