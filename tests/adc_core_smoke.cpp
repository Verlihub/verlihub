/*
	Copyright (C) 2006-2026 Verlihub Team, info at verlihub dot net

	Verlihub is free software; You can redistribute it
	and modify it under the terms of the GNU General
	Public License as published by the Free Software
	Foundation, either version 3 of the license, or at
	your option any later version.
*/

#include "cadchash.h"
#include "cadcproto.h"
#include "cserveradc.h"

#include <iostream>
#include <string>
#include <vector>

using namespace nVerliHub;
using namespace nVerliHub::nEnums;
using namespace nVerliHub::nProtocol;
using namespace nVerliHub::nSocket;

namespace {

bool Check(bool condition, const char *message)
{
	if (condition)
		return true;

	std::cerr << "ADC smoke test failed: " << message << std::endl;
	return false;
}

bool TestParser()
{
	cADCServerMessage sup;
	sup.GetStr() = "HSUP ADBASE ADTIGR";

	if (!Check(sup.Parse() == eADC_SUP, "valid SUP was rejected") ||
		!Check(sup.HeaderType() == 'H', "SUP header type") ||
		!Check(sup.Parameters().size() == 2, "SUP parameter count"))
		return false;

	cADCServerMessage badFeature;
	badFeature.GetStr() = "HSUP AD1ABC";

	if (!Check(badFeature.Parse() == eADC_INVALID,
		"invalid ADC feature code was accepted"))
		return false;

	cADCServerMessage escaped;
	escaped.GetStr() = "BMSG AAAA hello\\sworld";

	if (!Check(escaped.Parse() == eADC_MSG, "escaped BMSG was rejected") ||
		!Check(escaped.Parameters().size() == 1, "BMSG parameter count") ||
		!Check(escaped.Parameters()[0] == "hello world", "ADC space escape"))
		return false;

	cADCServerMessage nfc;
	nfc.GetStr() = std::string("BMSG AAAA Caf") + "\xC3\xA9";

	if (!Check(nfc.Parse() == eADC_MSG, "NFC UTF-8 was rejected"))
		return false;

	cADCServerMessage nfd;
	nfd.GetStr() = std::string("BMSG AAAA Cafe") + "\xCC\x81";

	return Check(nfd.Parse() == eADC_INVALID,
		"non-NFC UTF-8 was accepted");
}

bool TestFrames()
{
	std::string frame;

	if (!Check(cADCProto::CreateSID(frame, "ABCD"), "SID builder") ||
		!Check(frame == "ISID ABCD", "SID wire format"))
		return false;

	std::vector<std::string> features;
	features.push_back("ADBASE");
	features.push_back("ADTIGR");

	if (!Check(cADCProto::CreateSUP(frame, true, features), "SUP builder") ||
		!Check(frame == "ISUP ADBASE ADTIGR", "SUP wire format"))
		return false;

	std::vector<std::string> flags;
	flags.push_back("FCINF");

	if (!Check(cADCProto::CreateSTA(frame, "225", "Nick change blocked", flags),
		"STA builder"))
		return false;

	return Check(frame == "ISTA 225 Nick\\schange\\sblocked FCINF",
		"STA escaping/wire format");
}

bool TestHashHelpers()
{
	const unsigned char raw[] = {0x00, 0x01, 0x02, 0x03, 0x10, 0x20, 0xfe, 0xff};
	const std::string encoded = cADCHash::EncodeBase32(raw, sizeof(raw));
	std::vector<unsigned char> decoded;

	if (!Check(!encoded.empty(), "Base32 encoder") ||
		!Check(cADCHash::DecodeBase32(encoded, decoded), "Base32 decoder") ||
		!Check(decoded.size() == sizeof(raw), "Base32 decoded size"))
		return false;

	for (size_t i = 0; i < sizeof(raw); ++i) {
		if (!Check(decoded[i] == raw[i], "Base32 round trip"))
			return false;
	}

	std::string salt;

	if (!Check(cADCHash::CreateTigerSalt(salt), "Tiger GPA salt generation") ||
		!Check(cADCHash::IsTigerID(salt), "Tiger GPA salt shape"))
		return false;

	std::vector<unsigned char> invalid;
	return Check(!cADCHash::DecodeBase32("B", invalid),
		"non-canonical Base32 residual bits were accepted");
}

} // namespace

int main()
{
	if (!TestParser() || !TestFrames() || !TestHashHelpers())
		return 1;

	std::cout << "ADC core smoke test passed" << std::endl;
	return 0;
}
