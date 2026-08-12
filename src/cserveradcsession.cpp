/*
	Copyright (C) 2006-2026 Verlihub Team, info at verlihub dot net

	Verlihub is free software; You can redistribute it
	and modify it under the terms of the GNU General
	Public License as published by the Free Software
	Foundation, either version 3 of the license, or at
	your option any later version.
*/

#include "cserveradc.h"
#include "cadcpluginbridge.h"

namespace nVerliHub {
	using namespace nEnums;

	namespace nSocket {

void cServerADC::OnADCSessionNormal(cAsyncConn *raw,
	const nProtocol::sADCSession &session)
{
	cConnADC *conn = dynamic_cast<cConnADC*>(raw);

	if (!conn)
		return;

	nPlugin::cADCPluginBridge::OnLogin(&mPluginManager, conn, &session);
}

void cServerADC::OnADCSessionDetach(cAsyncConn *raw,
	const nProtocol::sADCSession &session)
{
	cConnADC *conn = dynamic_cast<cConnADC*>(raw);

	if (!conn)
		return;

	nPlugin::cADCPluginBridge::OnLogout(&mPluginManager, conn, &session);
}

double cServerADC::ADCSessionTimeoutSeconds(
	const nProtocol::sADCSession &session) const
{
	switch (session.mState) {
		case nProtocol::eADC_STATE_PROTOCOL:
		case nProtocol::eADC_STATE_IDENTIFY:
			return mC.timeout_length[eTO_LOGIN];

		case nProtocol::eADC_STATE_VERIFY:
			return mC.timeout_length[eTO_SETPASS];

		case nProtocol::eADC_STATE_NORMAL:
			return 0.0;
	}

	return 0.0;
}

void cServerADC::OnADCSessionTimeout(cAsyncConn *raw,
	const nProtocol::sADCSession &session)
{
	if (!raw || !raw->ok || !raw->mWritable)
		return;

	vector<string> flags;

	switch (session.mState) {
		case nProtocol::eADC_STATE_PROTOCOL:
			flags.push_back("FCSUP");
			break;

		case nProtocol::eADC_STATE_IDENTIFY:
			flags.push_back("FCINF");
			break;

		case nProtocol::eADC_STATE_VERIFY:
			flags.push_back("FCPAS");
			break;

		case nProtocol::eADC_STATE_NORMAL:
			return;
	}

	SendStatus(raw, "220", "ADC login timed out", flags, true);
}

	}; // namespace nSocket
}; // namespace nVerliHub
