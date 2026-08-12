/*
	Copyright (C) 2006-2026 Verlihub Team, info at verlihub dot net

	Verlihub is free software; You can redistribute it
	and modify it under the terms of the GNU General
	Public License as published by the Free Software
	Foundation, either version 3 of the license, or at
	your option any later version.
*/

#ifndef CADCHASH_H
#define CADCHASH_H

#include <string>
#include <vector>

namespace nVerliHub {
	namespace nProtocol {

/** ADC session-hash helpers. */
class cADCHash
{
	public:
		static bool DecodeBase32(const std::string &src,
			std::vector<unsigned char> &dest);
		static std::string EncodeBase32(const unsigned char *src, size_t len);

		/** Verify TIGR CID = Tiger(raw PID). */
		static bool VerifyTigerCID(const std::string &pid,
			const std::string &cid);

		/** Create a 24-byte random GPA challenge encoded as ADC Base32. */
		static bool CreateTigerSalt(std::string &salt);

		/** Verify PAS = Tiger(UTF-8 password || raw GPA challenge). */
		static bool VerifyTigerPassword(const std::string &password,
			const std::string &salt, const std::string &response);

		static bool IsTigerID(const std::string &value);

	private:
		static int Base32Value(char c);
};

	}; // namespace nProtocol
}; // namespace nVerliHub

#endif
