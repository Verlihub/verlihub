/*
	Copyright (C) 2006-2026 Verlihub Team, info at verlihub dot net

	Verlihub is free software; You can redistribute it
	and modify it under the terms of the GNU General
	Public License as published by the Free Software
	Foundation, either version 3 of the license, or at
	your option any later version.
*/

#ifndef CLEGACYTRANSPORTBRIDGE_H
#define CLEGACYTRANSPORTBRIDGE_H

#include <cstddef>
#include <string>

namespace nVerliHub {
	namespace nSocket {
		class cAsyncConn;
		class cAsyncSocketServer;

/**
 * Protocol-neutral transport hooks backed by the current cServerDC business
 * container. Keeping these functions in one bridge prevents cAsyncConn from
 * depending directly on cServerDC/NMDC headers while the server split is in
 * progress.
 */
unsigned long TransportMaxOutputBuffer(cAsyncSocketServer *server);
bool TransportCompressionEnabled(cAsyncSocketServer *server, std::size_t size);
int TransportCompressOutput(cAsyncSocketServer *server, const char *data,
	std::size_t size, std::string &compressed, std::size_t &candidateSize,
	int &errorCode);
unsigned long TransportMaxUnblockSize(cAsyncSocketServer *server);
unsigned long TransportMaxOutfillSize(cAsyncSocketServer *server);
void TransportLogOutput(cAsyncSocketServer *server, const std::string &message);
void TransportUserIPChanged(cAsyncSocketServer *server, cAsyncConn *conn);

	}; // namespace nSocket
}; // namespace nVerliHub

#endif
