/*
	Copyright (C) 2006-2026 Verlihub Team, info at verlihub dot net

	Verlihub is free software; You can redistribute it
	and modify it under the terms of the GNU General
	Public License as published by the Free Software
	Foundation, either version 3 of the license, or at
	your option any later version.
*/

#include "cadcproto.h"

namespace nVerliHub {
	using namespace nEnums;

	namespace nProtocol {

static bool adc_proto_alpha(const char c)
{
	return (c >= 'A') && (c <= 'Z');
}

static bool adc_proto_alphanum(const char c)
{
	return adc_proto_alpha(c) || ((c >= '0') && (c <= '9'));
}

cADCProto::cADCProto():
	cProtocol()
{
	SetClassName("ADCProto");
}

cADCProto::~cADCProto()
{}

cMessageParser *cADCProto::CreateParser()
{
	return new cMessageADC();
}

void cADCProto::DeleteParser(cMessageParser *parser)
{
	if (parser)
		delete parser;
}

int cADCProto::TreatMsg(cMessageParser *parser, nSocket::cAsyncConn *conn)
{
	(void)conn;

	if (!parser)
		return -1;

	cMessageADC *msg = dynamic_cast<cMessageADC*>(parser);

	if (!msg)
		return -1;

	if (msg->mType == eMSG_UNPARSED)
		msg->Parse();

	if (msg->mType == eADC_INVALID || msg->mError)
		return -1;

	if (!msg->SplitChunks())
		return -1;

	return 0;
}

bool cADCProto::ValidCommand(const std::string &command)
{
	return command.size() == 3 && adc_proto_alpha(command[0]) &&
		adc_proto_alphanum(command[1]) && adc_proto_alphanum(command[2]);
}

bool cADCProto::Build(std::string &dest, char type, const std::string &command,
	const std::vector<std::string> &header,
	const std::vector<std::string> &parameters)
{
	dest.clear();

	if (!ValidCommand(command))
		return false;

	switch (type) {
		case 'B':
			if (header.size() != 1 || !cMessageADC::IsSID(header[0]))
				return false;
			break;
		case 'D':
		case 'E':
			if (header.size() != 2 || !cMessageADC::IsSID(header[0]) ||
				!cMessageADC::IsSID(header[1]))
				return false;
			break;
		case 'F':
			if (header.size() != 2 || !cMessageADC::IsSID(header[0]) ||
				!cMessageADC::IsFeatureSelector(header[1]))
				return false;
			break;
		case 'U':
			if (header.size() != 1 || !cMessageADC::IsCID(header[0]))
				return false;
			break;
		case 'C':
		case 'H':
		case 'I':
			if (!header.empty())
				return false;
			break;
		default:
			return false;
	}

	dest.push_back(type);
	dest.append(command);

	for (size_t i = 0; i < header.size(); ++i) {
		dest.push_back(' ');
		dest.append(header[i]);
	}

	for (size_t i = 0; i < parameters.size(); ++i) {
		std::string escaped;

		if (!cMessageADC::Escape(parameters[i], escaped)) {
			dest.clear();
			return false;
		}

		dest.push_back(' ');
		dest.append(escaped);
	}

	return true;
}

bool cADCProto::CreateHub(std::string &dest, const std::string &command,
	const std::vector<std::string> &parameters)
{
	return Build(dest, 'H', command, std::vector<std::string>(), parameters);
}

bool cADCProto::CreateInfo(std::string &dest, const std::string &command,
	const std::vector<std::string> &parameters)
{
	return Build(dest, 'I', command, std::vector<std::string>(), parameters);
}

bool cADCProto::CreateClient(std::string &dest, const std::string &command,
	const std::vector<std::string> &parameters)
{
	return Build(dest, 'C', command, std::vector<std::string>(), parameters);
}

bool cADCProto::CreateBroadcast(std::string &dest, const std::string &command,
	const std::string &sourceSID, const std::vector<std::string> &parameters)
{
	std::vector<std::string> header;
	header.push_back(sourceSID);
	return Build(dest, 'B', command, header, parameters);
}

bool cADCProto::CreateDirect(std::string &dest, const std::string &command,
	const std::string &sourceSID, const std::string &targetSID,
	const std::vector<std::string> &parameters, bool echo)
{
	std::vector<std::string> header;
	header.push_back(sourceSID);
	header.push_back(targetSID);
	return Build(dest, echo ? 'E' : 'D', command, header, parameters);
}

bool cADCProto::CreateFeature(std::string &dest, const std::string &command,
	const std::string &sourceSID, const std::string &features,
	const std::vector<std::string> &parameters)
{
	std::vector<std::string> header;
	header.push_back(sourceSID);
	header.push_back(features);
	return Build(dest, 'F', command, header, parameters);
}

bool cADCProto::CreateUDP(std::string &dest, const std::string &command,
	const std::string &sourceCID, const std::vector<std::string> &parameters)
{
	std::vector<std::string> header;
	header.push_back(sourceCID);
	return Build(dest, 'U', command, header, parameters);
}

bool cADCProto::CreateSUP(std::string &dest, bool fromHub,
	const std::vector<std::string> &features)
{
	return Build(dest, fromHub ? 'I' : 'H', "SUP",
		std::vector<std::string>(), features);
}

bool cADCProto::CreateSID(std::string &dest, const std::string &sid)
{
	if (!cMessageADC::IsSID(sid)) {
		dest.clear();
		return false;
	}

	std::vector<std::string> parameters;
	parameters.push_back(sid);
	return Build(dest, 'I', "SID", std::vector<std::string>(), parameters);
}

bool cADCProto::CreateSTA(std::string &dest, const std::string &code,
	const std::string &description, const std::vector<std::string> &flags)
{
	if (code.size() != 3 || code[0] < '0' || code[0] > '2' ||
		code[1] < '0' || code[1] > '9' || code[2] < '0' || code[2] > '9') {
		dest.clear();
		return false;
	}

	std::vector<std::string> parameters;
	parameters.push_back(code);
	parameters.push_back(description);

	for (size_t i = 0; i < flags.size(); ++i)
		parameters.push_back(flags[i]);

	return Build(dest, 'I', "STA", std::vector<std::string>(), parameters);
}

	}; // namespace nProtocol
}; // namespace nVerliHub
