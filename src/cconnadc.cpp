/*
	Copyright (C) 2006-2026 Verlihub Team, info at verlihub dot net

	Verlihub is free software; You can redistribute it
	and modify it under the terms of the GNU General
	Public License as published by the Free Software
	Foundation, either version 3 of the license, or at
	your option any later version.
*/

#include "cconnadc.h"
#include "cadcpluginbridge.h"
#include "cadcpluginhost.h"
#include "cpluginmanager.h"
#include "creguserinfo.h"

namespace nVerliHub {
	namespace nSocket {

cConnADC::cConnADC(int sd, cAsyncSocketServer *server):
	cAsyncConn(sd, server),
	mRegInfo(NULL),
	mADCPluginConnected(false)
{
	SetClassName("ConnADC");
	ClearLine(); // ADC client-hub framing is LF from the first byte.

	nPlugin::cADCPluginHost *host =
		dynamic_cast<nPlugin::cADCPluginHost*>(server);

	if (host && host->ADCPluginManager()) {
		mADCPluginConnected = nPlugin::cADCPluginBridge::OnConnect(
			host->ADCPluginManager(), this);

		if (!mADCPluginConnected)
			ok = false;
	}
}

cConnADC::~cConnADC()
{
	nPlugin::cADCPluginHost *host =
		dynamic_cast<nPlugin::cADCPluginHost*>(mxServer);

	if (mADCPluginConnected && host && host->ADCPluginManager())
		nPlugin::cADCPluginBridge::OnDisconnect(host->ADCPluginManager(), this);

	if (mRegInfo) {
		delete mRegInfo;
		mRegInfo = NULL;
	}
}

	}; // namespace nSocket
}; // namespace nVerliHub
