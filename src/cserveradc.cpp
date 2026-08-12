/*
	Copyright (C) 2006-2026 Verlihub Team, info at verlihub dot net

	Verlihub is free software; You can redistribute it
	and modify it under the terms of the GNU General
	Public License as published by the Free Software
	Foundation, either version 3 of the license, or at
	your option any later version.
*/

#include "cserveradc.h"
#include "cbanlist.h"
#include "cconndc.h"
#include "i18n.h"

namespace nVerliHub {
	using namespace nEnums;
	using namespace nTables;

	namespace nSocket {

cADCConnFactory::cADCConnFactory(cServerDC *server, nProtocol::cADCProto *protocol):
	cConnFactory(protocol),
	mServer(server),
	mADCProtocol(protocol),
	mCleanupFactory(new cDCConnFactory(server))
{}

cADCConnFactory::~cADCConnFactory()
{
	if (mCleanupFactory) {
		delete mCleanupFactory;
		mCleanupFactory = NULL;
	}
}

cAsyncConn *cADCConnFactory::CreateConn(tSocket sd)
{
	if (!mServer || !mADCProtocol)
		return NULL;

	// Create the common Verlihub connection container directly. Do not use the
	// old protocol factory here: the connection belongs to ADC from byte zero.
	cConnDC *conn = new cConnDC(sd, mServer);
	conn->ClearLine(); // LF framing required by ADC.
	conn->mxMyFactory = this;
	conn->mxProtocol = mADCProtocol;
	return conn;
}

void cADCConnFactory::DeleteConn(cAsyncConn *&connection)
{
	if (mADCProtocol && connection)
		mADCProtocol->OnDisconnect(connection);

	// cDCConnFactory still owns account/plugin cleanup that has not yet been
	// extracted from cServerDC. It is deliberately used only on disconnect;
	// it never creates an ADC connection and never binds the wire protocol.
	if (mCleanupFactory)
		mCleanupFactory->DeleteConn(connection);
	else
		cConnFactory::DeleteConn(connection);
}

cServerADC::cServerADC(string CfgBase, const string &ExecPath):
	cServerDC(CfgBase, ExecPath),
	mADCProto()
{
	// The business-layer constructor installs its historical connection
	// factory. Replace it before listening so every accepted socket is ADC.
	if (mFactory) {
		delete mFactory;
		mFactory = NULL;
	}

	mFactory = new cADCConnFactory(this, &mADCProto);
}

cServerADC::~cServerADC()
{}

int cServerADC::OnNewConn(cAsyncConn *nc)
{
	cConnDC *conn = dynamic_cast<cConnDC*>(nc);

	if (!conn)
		return -1;

	// ADC is client-initiated: the client sends HSUP first. Do not call the
	// business-layer OnNewConn(), because its historical wire handshake is not
	// part of ADC.
	conn->SetGeoZone();
	mADCProto.Sessions().Attach(conn);

	if (mSysLoad >= eSL_RECOVERY) {
		std::vector<std::string> flags;
		std::string frame;
		nProtocol::cADCProto::CreateSTA(frame, "211",
			"Hub is currently unable to service your request", flags);

		if (!frame.empty()) {
			frame.push_back('\n');
			conn->Write(frame, true);
		}

		conn->CloseNice(1000, eCR_HUB_LOAD);
		return -1;
	}

	if (mBanList->IsIPTempBanned(conn->AddrToNumber())) {
		cBanList::sTempBan *tban =
			mBanList->mTempIPBanlist.GetByHash(conn->AddrToNumber());

		if (tban && (tban->mUntil > mTime.Sec())) {
			std::vector<std::string> flags;
			flags.push_back("TL" + StringFrom(tban->mUntil - mTime.Sec()));
			std::string frame;
			nProtocol::cADCProto::CreateSTA(frame, "232", tban->mReason, flags);

			if (!frame.empty()) {
				frame.push_back('\n');
				conn->Write(frame, true);
			}

			conn->CloseNice(1000, eCR_KICKED);
			return -1;
		}

		mBanList->DelIPTempBan(conn->AddrToNumber());
	}

	return 0;
}

	}; // namespace nSocket
}; // namespace nVerliHub
