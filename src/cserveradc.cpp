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
	mLegacyFactory(new cDCConnFactory(server))
{}

cADCConnFactory::~cADCConnFactory()
{
	if (mLegacyFactory) {
		delete mLegacyFactory;
		mLegacyFactory = NULL;
	}
}

cAsyncConn *cADCConnFactory::CreateConn(tSocket sd)
{
	if (!mServer || !mADCProtocol || !mLegacyFactory)
		return NULL;

	cAsyncConn *conn = mLegacyFactory->CreateConn(sd);

	if (!conn)
		return NULL;

	// cAsyncConn historically starts with the NMDC '|' separator. ClearLine()
	// resets the framing delimiter to LF, which is the ADC message terminator.
	conn->ClearLine();
	conn->mxMyFactory = this;
	conn->mxProtocol = mADCProtocol;
	return conn;
}

void cADCConnFactory::DeleteConn(cAsyncConn *&connection)
{
	if (mADCProtocol && connection)
		mADCProtocol->OnDisconnect(connection);

	if (mLegacyFactory)
		mLegacyFactory->DeleteConn(connection);
	else
		cConnFactory::DeleteConn(connection);
}

cServerADC::cServerADC(string CfgBase, const string &ExecPath):
	cServerDC(CfgBase, ExecPath),
	mADCProto()
{
	// cServerDC creates an NMDC-bound factory in its constructor. Replace it
	// before StartListening() so all accepted client sockets use ADC instead.
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

	// ADC is client-initiated: the client sends HSUP first. In particular, do
	// not call cServerDC::OnNewConn(), because it sends the NMDC $Lock greeting.
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
