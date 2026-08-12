/*
	Copyright (C) 2006-2026 Verlihub Team, info at verlihub dot net

	Verlihub is free software; You can redistribute it
	and modify it under the terms of the GNU General
	Public License as published by the Free Software
	Foundation, either version 3 of the license, or at
	your option any later version.
*/

#include "cmessageadc.h"

#include <cctype>

namespace nVerliHub {
	using namespace nEnums;

	namespace nProtocol {

static bool adc_alpha(const char c)
{
	return (c >= 'A') && (c <= 'Z');
}

static bool adc_alphanum(const char c)
{
	return adc_alpha(c) || ((c >= '0') && (c <= '9'));
}

static bool adc_base32(const char c)
{
	return adc_alpha(c) || ((c >= '2') && (c <= '7'));
}

cMessageADC::cMessageADC():
	cMessageParser(32),
	mHeaderType(0)
{
	SetClassName("MessageADC");
}

cMessageADC::~cMessageADC()
{}

void cMessageADC::ReInit()
{
	cMessageParser::ReInit();
	mHeaderType = 0;
	mCommand.clear();
	mSourceSID.clear();
	mTargetSID.clear();
	mSourceCID.clear();
	mFeatures.clear();
	mParameters.clear();
}

bool cMessageADC::Tokenize(std::vector<std::string> &raw, std::vector<size_t> *offsets) const
{
	raw.clear();

	if (offsets)
		offsets->clear();

	if (mStr.size() <= 4)
		return true;

	if (mStr[4] != ' ')
		return false;

	size_t start = 5;

	if (start >= mStr.size())
		return false;

	while (start < mStr.size()) {
		size_t end = mStr.find(' ', start);

		if (end == start)
			return false; // ADC uses exactly one separator between parameters.

		if (offsets)
			offsets->push_back(start);

		if (end == std::string::npos) {
			raw.push_back(mStr.substr(start));
			break;
		}

		raw.push_back(mStr.substr(start, end - start));
		start = end + 1;

		if (start == mStr.size())
			return false;
	}

	return true;
}

bool cMessageADC::IsSID(const std::string &sid)
{
	if (sid.size() != 4)
		return false;

	for (size_t i = 0; i < sid.size(); ++i) {
		if (!adc_base32(sid[i]))
			return false;
	}

	return true;
}

bool cMessageADC::IsCID(const std::string &cid)
{
	if (cid.empty())
		return false;

	for (size_t i = 0; i < cid.size(); ++i) {
		if (!adc_base32(cid[i]))
			return false;
	}

	return true;
}

bool cMessageADC::IsFeatureSelector(const std::string &features)
{
	if (features.empty() || ((features.size() % 5) != 0))
		return false;

	for (size_t i = 0; i < features.size(); i += 5) {
		if ((features[i] != '+') && (features[i] != '-'))
			return false;

		if (!adc_alpha(features[i + 1]))
			return false;

		for (size_t j = i + 2; j < i + 5; ++j) {
			if (!adc_alphanum(features[j]))
				return false;
		}
	}

	return true;
}

bool cMessageADC::UnEscape(const std::string &src, std::string &dest)
{
	dest.clear();
	dest.reserve(src.size());

	for (size_t i = 0; i < src.size(); ++i) {
		if (src[i] != '\\') {
			if ((src[i] == '\n') || (src[i] == '\r'))
				return false;

			dest.push_back(src[i]);
			continue;
		}

		if (++i >= src.size())
			return false;

		switch (src[i]) {
			case 's': dest.push_back(' '); break;
			case 'n': dest.push_back('\n'); break;
			case '\\': dest.push_back('\\'); break;
			default: return false; // Unknown escapes are reserved and invalidate the message.
		}
	}

	return true;
}

bool cMessageADC::Escape(const std::string &src, std::string &dest)
{
	dest.clear();
	dest.reserve(src.size());

	for (size_t i = 0; i < src.size(); ++i) {
		switch (src[i]) {
			case ' ': dest.append("\\s"); break;
			case '\n': dest.append("\\n"); break;
			case '\\': dest.append("\\\\"); break;
			case '\r': return false;
			default: dest.push_back(src[i]); break;
		}
	}

	return true;
}

int cMessageADC::CommandType(const std::string &command)
{
	if (command == "STA") return eADC_STA;
	if (command == "SUP") return eADC_SUP;
	if (command == "SID") return eADC_SID;
	if (command == "INF") return eADC_INF;
	if (command == "MSG") return eADC_MSG;
	if (command == "SCH") return eADC_SCH;
	if (command == "RES") return eADC_RES;
	if (command == "CTM") return eADC_CTM;
	if (command == "RCM") return eADC_RCM;
	if (command == "QUI") return eADC_QUI;
	if (command == "GPA") return eADC_GPA;
	if (command == "PAS") return eADC_PAS;
	if (command == "GET") return eADC_GET;
	if (command == "GFI") return eADC_GFI;
	if (command == "SND") return eADC_SND;
	return eADC_UNKNOWN;
}

bool cMessageADC::ParseHeader(std::vector<std::string> &raw, size_t &firstParameter)
{
	firstParameter = 0;

	switch (mHeaderType) {
		case 'B':
			if (raw.size() < 1 || !IsSID(raw[0]))
				return false;
			mSourceSID = raw[0];
			firstParameter = 1;
			break;

		case 'D':
		case 'E':
			if (raw.size() < 2 || !IsSID(raw[0]) || !IsSID(raw[1]))
				return false;
			mSourceSID = raw[0];
			mTargetSID = raw[1];
			firstParameter = 2;
			break;

		case 'F':
			if (raw.size() < 2 || !IsSID(raw[0]) || !IsFeatureSelector(raw[1]))
				return false;
			mSourceSID = raw[0];

			for (size_t pos = 0; pos < raw[1].size(); pos += 5)
				mFeatures.push_back(raw[1].substr(pos, 5));

			firstParameter = 2;
			break;

		case 'U':
			if (raw.size() < 1 || !IsCID(raw[0]))
				return false;
			mSourceCID = raw[0];
			firstParameter = 1;
			break;

		case 'C':
		case 'H':
		case 'I':
			break;

		default:
			return false;
	}

	return true;
}

int cMessageADC::Parse()
{
	mLen = mStr.size();
	mKWSize = 4;
	mError = false;
	mHeaderType = 0;
	mCommand.clear();
	mSourceSID.clear();
	mTargetSID.clear();
	mSourceCID.clear();
	mFeatures.clear();
	mParameters.clear();

	if (mStr.empty()) {
		mType = eADC_UNKNOWN;
		return mType;
	}

	if (mStr.size() < 4) {
		mError = true;
		mType = eADC_INVALID;
		return mType;
	}

	mHeaderType = mStr[0];

	if (((mHeaderType != 'B') && (mHeaderType != 'C') &&
		 (mHeaderType != 'D') && (mHeaderType != 'E') &&
		 (mHeaderType != 'F') && (mHeaderType != 'H') &&
		 (mHeaderType != 'I') && (mHeaderType != 'U')) ||
		!adc_alpha(mStr[1]) || !adc_alphanum(mStr[2]) || !adc_alphanum(mStr[3])) {
		mError = true;
		mType = eADC_INVALID;
		return mType;
	}

	mCommand.assign(mStr, 1, 3);

	std::vector<std::string> raw;

	if (!Tokenize(raw, NULL)) {
		mError = true;
		mType = eADC_INVALID;
		return mType;
	}

	size_t firstParameter = 0;

	if (!ParseHeader(raw, firstParameter)) {
		mError = true;
		mType = eADC_INVALID;
		return mType;
	}

	for (size_t i = firstParameter; i < raw.size(); ++i) {
		std::string decoded;

		if (!UnEscape(raw[i], decoded)) {
			mError = true;
			mType = eADC_INVALID;
			return mType;
		}

		mParameters.push_back(decoded);
	}

	mType = CommandType(mCommand);
	return mType;
}

bool cMessageADC::SplitChunks()
{
	SetChunk(0, 0, mStr.length());

	std::vector<std::string> raw;
	std::vector<size_t> offsets;

	if (!Tokenize(raw, &offsets)) {
		mError = true;
		return false;
	}

	const size_t count = (raw.size() < 31) ? raw.size() : 31;

	for (size_t i = 0; i < count; ++i)
		SetChunk(i + 1, offsets[i], raw[i].size());

	return !mError;
}

bool cMessageADC::GetNamed(const std::string &name, std::string &value) const
{
	if (name.size() != 2 || !adc_alpha(name[0]) || !adc_alphanum(name[1]))
		return false;

	for (size_t i = 0; i < mParameters.size(); ++i) {
		if (mParameters[i].size() >= 2 && mParameters[i].compare(0, 2, name) == 0) {
			value.assign(mParameters[i], 2, std::string::npos);
			return true;
		}
	}

	return false;
}

	}; // namespace nProtocol
}; // namespace nVerliHub
