/*
	Copyright (C) 2006-2026 Verlihub Team, info at verlihub dot net

	Verlihub is free software; You can redistribute it
	and modify it under the terms of the GNU General
	Public License as published by the Free Software
	Foundation, either version 3 of the license, or at
	your option any later version.
*/

#include "cadcpolicy.h"
#include "cadcpluginbridge.h"
#include "cadcsession.h"
#include "cmessageadc.h"
#include "cconnadc.h"
#include "cpenaltylist.h"
#include "creguserinfo.h"
#include "cserverdc.h"

#include <climits>

namespace nVerliHub {
	using namespace nEnums;
	using namespace nTables;

	namespace nProtocol {

bool cADCPolicy::GetINF(const sADCSession &session, const std::string &field,
	std::string &value)
{
	value.clear();

	if (field.size() != 2)
		return false;

	for (size_t i = 0; i < session.mINF.size(); ++i) {
		const std::string &parameter = session.mINF[i];

		if (parameter.size() >= 2 && parameter.compare(0, 2, field) == 0) {
			value.assign(parameter, 2, std::string::npos);
			return true;
		}
	}

	return false;
}

bool cADCPolicy::HasFeature(const sADCSession &session,
	const std::string &feature)
{
	std::string supports;

	if (!GetINF(session, "SU", supports))
		return false;

	size_t start = 0;

	while (start <= supports.size()) {
		const size_t end = supports.find(',', start);
		const std::string current = supports.substr(start,
			(end == std::string::npos) ? std::string::npos : end - start);

		if (current == feature)
			return true;

		if (end == std::string::npos)
			break;

		start = end + 1;
	}

	return false;
}

bool cADCPolicy::ParseUInt64(const std::string &value,
	unsigned long long &number)
{
	number = 0;

	if (value.empty())
		return false;

	for (size_t i = 0; i < value.size(); ++i) {
		if (value[i] < '0' || value[i] > '9')
			return false;

		const unsigned int digit = static_cast<unsigned int>(value[i] - '0');

		if (number > (ULLONG_MAX - digit) / 10)
			return false;

		number = (number * 10) + digit;
	}

	return true;
}

bool cADCPolicy::AllowNormal(nSocket::cServerDC *server,
	nSocket::cConnADC *conn, const sADCSession &session,
	const cMessageADC &msg, std::string &reason)
{
	reason.clear();

	if (!server || !conn) {
		reason = "Invalid ADC policy context";
		return false;
	}

	if (!nPlugin::cADCPluginBridge::OnMessage(&server->mPluginManager,
		conn, &msg)) {
		reason = "Command rejected by ADC plugin";
		return false;
	}

	const int userClass = conn->mRegInfo ? conn->mRegInfo->mClass : eUC_NORMUSER;

	// Historical pinger accounts were explicitly denied chat/search/download.
	// Preserve that policy without constructing a cUser instance.
	if (userClass == eUC_PINGER) {
		reason = "This account is not allowed to use interactive hub commands";
		return false;
	}

	if (userClass < eUC_ADMIN) {
		cPenaltyList::sPenalty penalty;

		if (server->mPenList)
			server->mPenList->LoadTo(penalty, session.mNick);

		const long now = server->mTime.Sec();
		long restrictedUntil = 1;

		switch (msg.mType) {
			case eADC_MSG:
				// D/E messages are private/direct conversations; B/F are chat.
				restrictedUntil = ((msg.HeaderType() == 'D') ||
					(msg.HeaderType() == 'E')) ? penalty.mStartPM : penalty.mStartChat;
				break;

			case eADC_SCH:
				restrictedUntil = penalty.mStartSearch;
				break;

			case eADC_CTM:
			case eADC_RCM:
				restrictedUntil = penalty.mStartCTM;
				break;

			default:
				break;
		}

		if (!restrictedUntil || restrictedUntil > now) {
			reason = "Access to this command is temporarily restricted";
			return false;
		}

		// The old MyINFO path disabled search/download when a user did not meet
		// the hub-use class/share requirements. Derive active/passive directly
		// from ADC's TCP4/TCP6 features instead of NMDC connection flags.
		if ((msg.mType == eADC_SCH) || (msg.mType == eADC_CTM) ||
			(msg.mType == eADC_RCM)) {
			const bool active = HasFeature(session, "TCP4") ||
				HasFeature(session, "TCP6");
			const int minClass = active ? server->mC.min_class_use_hub :
				server->mC.min_class_use_hub_passive;

			if (userClass < minClass) {
				reason = "Your account class is not allowed to use file sharing here";
				return false;
			}

			unsigned long long minShare = 0;

			switch (userClass) {
				case eUC_NORMUSER:
					minShare = server->mC.min_share_use_hub;
					break;

				case eUC_REGUSER:
					minShare = server->mC.min_share_use_hub_reg;
					break;

				case eUC_VIPUSER:
					minShare = server->mC.min_share_use_hub_vip;
					break;

				default:
					break;
			}

			if (!active && minShare) {
				minShare = static_cast<unsigned long long>(
					static_cast<double>(minShare) *
					server->mC.min_share_factor_passive);
			}

			if (minShare) {
				std::string shareText;
				unsigned long long share = 0;

				if (!GetINF(session, "SS", shareText) ||
					!ParseUInt64(shareText, share) || share < minShare) {
					reason = "Your shared size is below the limit required for this command";
					return false;
				}
			}
		}
	}

	return true;
}

	}; // namespace nProtocol
}; // namespace nVerliHub
