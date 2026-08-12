/*
	Copyright (C) 2006-2026 Verlihub Team, info at verlihub dot net

	Verlihub is free software; You can redistribute it
	and modify it under the terms of the GNU General
	Public License as published by the Free Software
	Foundation, either version 3 of the license, or at
	your option any later version.
*/

#ifndef CADCADMISSIONPOLICY_H
#define CADCADMISSIONPOLICY_H

#include <string>
#include <vector>

namespace nVerliHub {
	namespace nSocket {
		class cConnADC;
		class cServerADC;
	};

	namespace nProtocol {
		class cMessageADC;
		class cADCSessionManager;

struct sADCAdmissionDecision
{
	sADCAdmissionDecision(): mAllowed(true), mCode("211") {}

	bool mAllowed;
	std::string mCode;
	std::string mDescription;
	std::vector<std::string> mFlags;
};

/** Hub-capacity policy for the ADC IDENTIFY -> VERIFY/NORMAL transition. */
class cADCAdmissionPolicy
{
	public:
		static bool Check(nSocket::cServerADC *server,
			nSocket::cConnADC *conn, const cMessageADC &msg,
			const cADCSessionManager &sessions,
			sADCAdmissionDecision &decision);
};

	}; // namespace nProtocol
}; // namespace nVerliHub

#endif
