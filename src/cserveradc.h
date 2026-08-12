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
 * Connection factory that reuses cConnDC as Verlihub's connection/account
 * container while replacing the attached wire protocol with ADC.
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
		cDCConnFactory *mLegacyFactory;
};

/**
 * ADC-facing Verlihub server.
 *
 * cServerDC is retained as the business layer (configuration, users, bans,
 * database and plugins). New client sockets are ADC framed and are not sent
 * the NMDC $Lock greeting.
 */
class cServerADC : public cServerDC
{
	public:
		cServerADC(string CfgBase = "./.verlihub", const string &ExecPath = "");
		virtual ~cServerADC();

		virtual int OnNewConn(cAsyncConn *conn);

		nProtocol::cADCProto &ADCProtocol() { return mADCProto; }

	private:
		nProtocol::cADCProto mADCProto;
};

	}; // namespace nSocket
}; // namespace nVerliHub

#endif
