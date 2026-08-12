/*
	Copyright (C) 2006-2026 Verlihub Team, info at verlihub dot net

	Verlihub is free software; You can redistribute it
	and modify it under the terms of the GNU General
	Public License as published by the Free Software
	Foundation, either version 3 of the license, or at
	your option any later version.
*/

#include "cadcadmissionpolicy.h"
#include "cadcsession.h"
#include "cbanlist.h"
#include "cconnadc.h"
#include "cmaxminddb.h"
#include "cmessageadc.h"
#include "creglist.h"
#include "creguserinfo.h"
#include "cserveradc.h"

#include <sstream>

namespace nVerliHub {
	using namespace nEnums;
	using namespace nTables;

	namespace nProtocol {

namespace {

bool HasCSVFeature(const std::string &features, const std::string &wanted)
{
	size_t start = 0;

	while (start <= features.size()) {
		const size_t end = features.find(',', start);
		const std::string current = features.substr(start,
			(end == std::string::npos) ? std::string::npos : end - start);

		if (current == wanted)
			return true;

		if (end == std::string::npos)
			break;

		start = end + 1;
	}

	return false;
}

bool SessionINF(const sADCSession &session, const std::string &key,
	std::string &value)
{
	for (size_t i = 0; i < session.mINF.size(); ++i) {
		if (session.mINF[i].size() >= 2 &&
			session.mINF[i].compare(0, 2, key) == 0) {
			value = session.mINF[i].substr(2);
			return true;
		}
	}

	value.clear();
	return false;
}

bool IsPassive(const std::string &supports)
{
	return !HasCSVFeature(supports, "TCP4") &&
		!HasCSVFeature(supports, "TCP6");
}

int ConnectionClass(nSocket::cConnADC *conn)
{
	if (!conn || !conn->mRegInfo || !conn->mRegInfo->mEnabled)
		return eUC_NORMUSER;

	return conn->mRegInfo->mClass;
}

int CandidateClass(nSocket::cServerADC *server, const std::string &nick)
{
	if (!server || !server->mR || nick.empty())
		return eUC_NORMUSER;

	cRegUserInfo info;

	if (!server->mR->FindRegInfo(info, nick) || !info.mEnabled)
		return eUC_NORMUSER;

	return info.mClass;
}

bool InConfiguredRange(const std::string &minimum, const std::string &maximum,
	unsigned long address)
{
	if (minimum.empty() || maximum.empty())
		return false;

	unsigned long low = 0;
	unsigned long high = 0;

	if (!cBanList::Ip2Num(minimum, low) || !cBanList::Ip2Num(maximum, high))
		return false;

	return low <= address && address <= high;
}

unsigned int GeoZone(nSocket::cServerADC *server, nSocket::cAsyncConn *conn)
{
	if (!server || !conn)
		return 0;

	std::string cc;

	for (unsigned int pos = 0; pos < 3; ++pos) {
		if (server->mC.cc_zone[pos].empty())
			continue;

		if (cc.empty() && server->mMaxMindDB)
			server->mMaxMindDB->GetCC(conn->AddrIP(), cc);

		if (!cc.empty() && cc != "--" &&
			(cc == server->mC.cc_zone[pos] ||
			 server->mC.cc_zone[pos].find(cc) != std::string::npos))
			return pos + 1;
	}

	const unsigned long address = conn->AddrToNumber();

	if (InConfiguredRange(server->mC.ip_zone4_min,
		server->mC.ip_zone4_max, address))
		return 4;

	if (InConfiguredRange(server->mC.ip_zone5_min,
		server->mC.ip_zone5_max, address))
		return 5;

	if (InConfiguredRange(server->mC.ip_zone6_min,
		server->mC.ip_zone6_max, address))
		return 6;

	return 0;
}

unsigned int ExtraSlots(nSocket::cServerADC *server, int userClass)
{
	if (!server)
		return 0;

	switch (userClass) {
		case eUC_PINGER:
			return server->mC.max_extra_pings;
		case eUC_REGUSER:
			return server->mC.max_extra_regs;
		case eUC_VIPUSER:
			return server->mC.max_extra_vips;
		case eUC_OPERATOR:
			return server->mC.max_extra_ops;
		case eUC_CHEEF:
			return server->mC.max_extra_cheefs;
		case eUC_ADMIN:
			return server->mC.max_extra_admins;
		default:
			return 0;
	}
}

void Deny(sADCAdmissionDecision &decision, const std::string &description)
{
	decision.mAllowed = false;
	decision.mCode = "211";
	decision.mDescription = description;
	decision.mFlags.clear();
}

} // namespace

bool cADCAdmissionPolicy::Check(nSocket::cServerADC *server,
	nSocket::cConnADC *conn, const cMessageADC &msg,
	const cADCSessionManager &sessions, sADCAdmissionDecision &decision)
{
	decision = sADCAdmissionDecision();

	if (!server || !conn)
		return true;

	std::string nick;
	std::string supports;

	// Let the normal INF validator report missing/invalid required fields.
	if (!msg.GetNamed("NI", nick) || !msg.GetNamed("SU", supports))
		return true;

	const int userClass = CandidateClass(server, nick);
	const unsigned int candidateZone = GeoZone(server, conn);
	std::vector<nSocket::cAsyncConn*> identified;
	sessions.IdentifiedConnections(identified);
	unsigned int zoneCount = 0;
	unsigned int passiveCount = 0;

	for (size_t i = 0; i < identified.size(); ++i) {
		nSocket::cAsyncConn *raw = identified[i];

		if (!raw || raw == conn || !raw->ok)
			continue;

		if (GeoZone(server, raw) == candidateZone)
			++zoneCount;

		const sADCSession *existing = sessions.Find(raw);
		std::string existingSupports;

		if (existing && SessionINF(*existing, "SU", existingSupports) &&
			IsPassive(existingSupports))
			++passiveCount;
	}

	const unsigned int totalCount = static_cast<unsigned int>(identified.size());
	const unsigned int extra = ExtraSlots(server, userClass);
	const unsigned int totalLimit = server->mC.max_users_total + extra;
	const unsigned int zoneLimit = server->mC.max_users[candidateZone] + extra;

	if (userClass < eUC_OPERATOR &&
		(totalCount >= totalLimit || zoneCount >= zoneLimit)) {
		std::ostringstream reason;

		if (zoneCount >= zoneLimit)
			reason << "User limit in zone " << candidateZone << " exceeded ("
				<< zoneCount << '/' << zoneLimit << ')';
		else
			reason << "Hub user limit exceeded (" << totalCount << '/'
				<< totalLimit << ')';

		if (server->mC.max_users_total == 0 &&
			!server->mC.hubfull_message.empty())
			reason << ": " << server->mC.hubfull_message;

		Deny(decision, reason.str());
		return false;
	}

	if (server->mC.max_users_from_ip && userClass < eUC_VIPUSER) {
		std::vector<nSocket::cAsyncConn*> connections;
		server->ADCConnections(connections);
		unsigned int sameIP = 0;

		for (size_t i = 0; i < connections.size(); ++i) {
			nSocket::cConnADC *other =
				dynamic_cast<nSocket::cConnADC*>(connections[i]);

			if (!other || !other->ok || other->AddrToNumber() != conn->AddrToNumber())
				continue;

			if (ConnectionClass(other) <= eUC_REGUSER)
				++sameIP;
		}

		if (sameIP >= server->mC.max_users_from_ip) {
			std::ostringstream reason;
			reason << "User limit from IP address " << conn->AddrIP()
				<< " exceeded (" << sameIP << '/'
				<< server->mC.max_users_from_ip << ')';
			Deny(decision, reason.str());
			return false;
		}
	}

	if (userClass != eUC_PINGER && userClass < eUC_OPERATOR &&
		server->mC.max_users_passive > -1 && IsPassive(supports) &&
		passiveCount >= static_cast<unsigned int>(server->mC.max_users_passive)) {
		std::ostringstream reason;
		reason << "Passive user limit exceeded (" << passiveCount << '/'
			<< server->mC.max_users_passive << ')';
		Deny(decision, reason.str());
		return false;
	}

	return true;
}

	}; // namespace nProtocol
}; // namespace nVerliHub
