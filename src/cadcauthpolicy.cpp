/*
	Copyright (C) 2006-2026 Verlihub Team, info at verlihub dot net

	Verlihub is free software; You can redistribute it
	and modify it under the terms of the GNU General
	Public License as published by the Free Software
	Foundation, either version 3 of the license, or at
	your option any later version.
*/

#include "cadcauthpolicy.h"
#include "cadchash.h"
#include "cadcsession.h"
#include "cbanlist.h"
#include "cconnadc.h"
#include "cmessageadc.h"
#include "creguserinfo.h"
#include "cserveradc.h"

namespace nVerliHub {
	using namespace nEnums;
	using namespace nTables;

	namespace nProtocol {

void cADCAuthPolicy::ObservePasswordFailure(nSocket::cAsyncConn *raw,
	const cMessageADC &msg, const sADCSession &session)
{
	if (!raw || msg.mType != eADC_PAS || msg.HeaderType() != 'H' ||
		msg.Parameters().size() != 1 || session.mState != eADC_STATE_VERIFY ||
		!session.mRegistered)
		return;

	nSocket::cConnADC *conn = dynamic_cast<nSocket::cConnADC*>(raw);

	if (!conn || !conn->mRegInfo ||
		conn->mRegInfo->mPWCrypt != cRegUserInfo::eCRYPT_NONE)
		return;

	if (cADCHash::VerifyTigerPassword(conn->mRegInfo->mPasswd,
		session.mGPA, msg.Parameters()[0]))
		return;

	nSocket::cServerADC *server =
		dynamic_cast<nSocket::cServerADC*>(conn->mxServer);

	if (!server || !server->mBanList || !server->mC.pwd_tmpban)
		return;

	const std::string reason = server->mC.wrongpass_message.empty() ?
		"Incorrect password" : server->mC.wrongpass_message;

	server->mBanList->AddIPTempBan(conn->AddrToNumber(),
		server->mTime.Sec() + server->mC.pwd_tmpban, reason, eBT_PASSW);

	if (conn->Log(2))
		conn->LogStream() << "ADC password failure; temporary IP ban applied: "
			<< reason << std::endl;
}

	}; // namespace nProtocol
}; // namespace nVerliHub
