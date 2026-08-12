/*
	Copyright (C) 2006-2026 Verlihub Team, info at verlihub dot net

	Verlihub is free software; You can redistribute it
	and modify it under the terms of the GNU General
	Public License as published by the Free Software
	Foundation, either version 3 of the license, or at
	your option any later version.
*/

#include "cadchash.h"

#include <rhash.h>

namespace nVerliHub {
	namespace nProtocol {

int cADCHash::Base32Value(char c)
{
	if ((c >= 'A') && (c <= 'Z'))
		return c - 'A';

	if ((c >= '2') && (c <= '7'))
		return 26 + (c - '2');

	return -1;
}

bool cADCHash::DecodeBase32(const std::string &src,
	std::vector<unsigned char> &dest)
{
	dest.clear();

	unsigned int buffer = 0;
	unsigned int bits = 0;

	for (size_t i = 0; i < src.size(); ++i) {
		const int value = Base32Value(src[i]);

		if (value < 0) {
			dest.clear();
			return false;
		}

		buffer = (buffer << 5) | static_cast<unsigned int>(value);
		bits += 5;

		while (bits >= 8) {
			bits -= 8;
			dest.push_back(static_cast<unsigned char>((buffer >> bits) & 0xff));

			if (bits)
				buffer &= (1u << bits) - 1u;
			else
				buffer = 0;
		}
	}

	// ADC uses unpadded RFC 4648 Base32. Any residual low bits therefore have
	// to be zero for the representation to be canonical.
	if (bits && buffer) {
		dest.clear();
		return false;
	}

	return true;
}

std::string cADCHash::EncodeBase32(const unsigned char *src, size_t len)
{
	static const char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
	std::string dest;

	if (!src || !len)
		return dest;

	dest.reserve((len * 8 + 4) / 5);
	unsigned int buffer = 0;
	unsigned int bits = 0;

	for (size_t i = 0; i < len; ++i) {
		buffer = (buffer << 8) | src[i];
		bits += 8;

		while (bits >= 5) {
			bits -= 5;
			dest.push_back(alphabet[(buffer >> bits) & 31]);

			if (bits)
				buffer &= (1u << bits) - 1u;
			else
				buffer = 0;
		}
	}

	if (bits)
		dest.push_back(alphabet[(buffer << (5 - bits)) & 31]);

	return dest;
}

bool cADCHash::IsTigerID(const std::string &value)
{
	if (value.size() != 39)
		return false;

	for (size_t i = 0; i < value.size(); ++i) {
		if (Base32Value(value[i]) < 0)
			return false;
	}

	std::vector<unsigned char> decoded;
	return DecodeBase32(value, decoded) && decoded.size() == 24;
}

bool cADCHash::VerifyTigerCID(const std::string &pid,
	const std::string &cid)
{
	if (!IsTigerID(pid) || !IsTigerID(cid))
		return false;

	std::vector<unsigned char> rawPID;

	if (!DecodeBase32(pid, rawPID) || rawPID.size() != 24)
		return false;

	unsigned char digest[24];
	rhash_library_init();

	if (rhash_msg(RHASH_TIGER, &rawPID[0], rawPID.size(), digest) < 0)
		return false;

	return EncodeBase32(digest, sizeof(digest)) == cid;
}

	}; // namespace nProtocol
}; // namespace nVerliHub
