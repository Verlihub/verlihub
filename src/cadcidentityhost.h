/*
	Copyright (C) 2006-2026 Verlihub Team, info at verlihub dot net

	Verlihub is free software; You can redistribute it
	and modify it under the terms of the GNU General
	Public License as published by the Free Software
	Foundation, either version 3 of the license, or at
	your option any later version.
*/

#ifndef CADCIDENTITYHOST_H
#define CADCIDENTITYHOST_H

#include <string>

namespace nVerliHub {
	namespace nSocket {
		class cAsyncConn;

/**
 * Protocol-neutral identity gate used by the ADC session manager.
 */
class cADCIdentityHost
{
	public:
		virtual ~cADCIdentityHost() {}
		virtual bool ValidateADCIdentity(cAsyncConn *conn,
			const std::string &nick, bool registered) = 0;
};

	}; // namespace nSocket
}; // namespace nVerliHub

#endif
