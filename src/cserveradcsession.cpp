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

	}; // namespace nSocket
}; // namespace nVerliHub
