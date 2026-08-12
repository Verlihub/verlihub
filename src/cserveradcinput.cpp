/*
	Copyright (C) 2006-2026 Verlihub Team, info at verlihub dot net

	Verlihub is free software; You can redistribute it
	and modify it under the terms of the GNU General
	Public License as published by the Free Software
	Foundation, either version 3 of the license, or at
	your option any later version.
*/

#include "cserveradc.h"

namespace nVerliHub {
	namespace nSocket {

int cServerADC::input(cAsyncConn *conn)
{
	if (!conn || conn->ReadAll(mNoReadTry, mNoReadDelay) <= 0)
		return 0;

	int justRead = 0;

	while (conn->ok && conn->mWritable) {
		if (conn->LineStatus() == nEnums::AC_LS_NO_LINE)
			conn->SetLineToRead(FactoryString(conn), '\n', mMaxLineLength);

		justRead += conn->ReadLineLocal();

		if (conn->LineStatus() == nEnums::AC_LS_LINE_DONE) {
			OnNewMessage(conn, conn->GetLine());
			conn->ClearLine();
		}

		if (conn->BufferEmpty())
			break;
	}

	return justRead;
}

	}; // namespace nSocket
}; // namespace nVerliHub
