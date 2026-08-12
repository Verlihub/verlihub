/*
	Copyright (C) 2006-2026 Verlihub Team, info at verlihub dot net

	Verlihub is free software; You can redistribute it
	and modify it under the terms of the GNU General
	Public License as published by the Free Software
	Foundation, either version 3 of the license, or at
	your option any later version.
*/

#ifndef CCONNADC_H
#define CCONNADC_H

#include "casyncconn.h"

namespace nVerliHub {
	namespace nTables {
		class cRegUserInfo;
	};

	namespace nSocket {

/**
 * Minimal client-hub connection container for ADC.
 *
 * Protocol login state, negotiated features, SID/CID/PID and INF data live in
 * cADCSessionManager. This object deliberately contains none of cConnDC's
 * NMDC login flags, $Supports bits, pipe framing, DC protocol flood enums or
 * $ForceMove redirect behaviour.
 */
class cConnADC : public cAsyncConn
{
	public:
		cConnADC(int sd = 0, cAsyncSocketServer *server = NULL);
		virtual ~cConnADC();

		/** Registration record loaded for the ADC nickname, if any. */
		nTables::cRegUserInfo *mRegInfo;

	private:
		bool mADCPluginConnected;
};

	}; // namespace nSocket
}; // namespace nVerliHub

#endif
