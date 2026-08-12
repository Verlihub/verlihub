/*
	Copyright (C) 2006-2026 Verlihub Team, info at verlihub dot net

	Verlihub is free software; You can redistribute it
	and modify it under the terms of the GNU General
	Public License as published by the Free Software
	Foundation, either version 3 of the license, or at
	your option any later version.
*/

#include "clegacytransportbridge.h"
#include "casyncconn.h"
#include "casyncsocketserver.h"
#include "cserverdc.h"
#include "czlib.h"

namespace nVerliHub {
	namespace nSocket {

static cServerDC *LegacyTransportServer(cAsyncSocketServer *server)
{
	return dynamic_cast<cServerDC*>(server);
}

unsigned long TransportMaxOutputBuffer(cAsyncSocketServer *server)
{
	cServerDC *legacy = LegacyTransportServer(server);
	return legacy ? legacy->mC.max_outbuf_size : MAX_SEND_SIZE;
}

bool TransportCompressionEnabled(cAsyncSocketServer *server, std::size_t size)
{
	cServerDC *legacy = LegacyTransportServer(server);
	return legacy && legacy->mZLib && !legacy->mC.disable_zlib &&
		size >= legacy->mC.zlib_min_len;
}

int TransportCompressOutput(cAsyncSocketServer *server, const char *data,
	std::size_t size, std::string &compressed, std::size_t &candidateSize,
	int &errorCode)
{
	compressed.clear();
	candidateSize = 0;
	errorCode = 0;
	cServerDC *legacy = LegacyTransportServer(server);

	if (!legacy || !legacy->mZLib)
		return 0;

	char *buffer = legacy->mZLib->Compress(data, size, candidateSize,
		errorCode, legacy->mC.zlib_compress_level);

	if (!candidateSize || !buffer)
		return -1;

	compressed.assign(buffer, candidateSize);

	if (size > candidateSize)
		legacy->mProtoSaved[0] += size - candidateSize;

	return 1;
}

unsigned long TransportMaxUnblockSize(cAsyncSocketServer *server)
{
	cServerDC *legacy = LegacyTransportServer(server);
	return legacy ? legacy->mC.max_unblock_size : MAX_SEND_UNBLOCK_SIZE;
}

unsigned long TransportMaxOutfillSize(cAsyncSocketServer *server)
{
	cServerDC *legacy = LegacyTransportServer(server);
	return legacy ? legacy->mC.max_outfill_size : MAX_SEND_FILL_SIZE;
}

void TransportLogOutput(cAsyncSocketServer *server, const std::string &message)
{
	cServerDC *legacy = LegacyTransportServer(server);

	if (legacy && legacy->mNetOutLog && legacy->mNetOutLog.is_open())
		legacy->mNetOutLog << message;
}

void TransportUserIPChanged(cAsyncSocketServer *server, cAsyncConn *conn)
{
	cServerDC *legacy = LegacyTransportServer(server);

	if (legacy && conn)
		legacy->ShowUserIP(conn);
}

	}; // namespace nSocket
}; // namespace nVerliHub
