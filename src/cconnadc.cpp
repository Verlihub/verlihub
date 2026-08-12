/*
	Copyright (C) 2006-2026 Verlihub Team, info at verlihub dot net

	Verlihub is free software; You can redistribute it
	and modify it under the terms of the GNU General
	Public License as published by the Free Software
	Foundation, either version 3 of the license, or at
	your option any later version.
*/

#include "cconnadc.h"
#include "creguserinfo.h"

namespace nVerliHub {
	namespace nSocket {

cConnADC::cConnADC(int sd, cAsyncSocketServer *server):
	cAsyncConn(sd, server),
	mRegInfo(NULL)
{
	SetClassName("ConnADC");
	ClearLine(); // ADC client-hub framing is LF from the first byte.
}

cConnADC::~cConnADC()
{
	if (mRegInfo) {
		delete mRegInfo;
		mRegInfo = NULL;
	}
}

	}; // namespace nSocket
}; // namespace nVerliHub
