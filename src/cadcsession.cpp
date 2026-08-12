/*
	Copyright (C) 2006-2026 Verlihub Team, info at verlihub dot net

	Verlihub is free software; You can redistribute it
	and modify it under the terms of the GNU General
	Public License as published by the Free Software
	Foundation, either version 3 of the license, or at
	your option any later version.
*/

#include "cadcsession.h"

namespace nVerliHub {
	namespace nProtocol {

sADCSession::sADCSession():
	mState(eADC_STATE_PROTOCOL),
	mRegistered(false)
{}

cADCSessionManager::cADCSessionManager():
	mNextSID(1)
{}

cADCSessionManager::~cADCSessionManager()
{}

sADCSession &cADCSessionManager::Attach(nSocket::cAsyncConn *conn)
{
	return mSessions[conn];
}

void cADCSessionManager::Detach(nSocket::cAsyncConn *conn)
{
	tSessionMap::iterator it = mSessions.find(conn);

	if (it == mSessions.end())
		return;

	if (!it->second.mSID.empty())
		mSIDIndex.erase(it->second.mSID);

	mSessions.erase(it);
}

sADCSession *cADCSessionManager::Find(nSocket::cAsyncConn *conn)
{
	tSessionMap::iterator it = mSessions.find(conn);
	return (it == mSessions.end()) ? NULL : &it->second;
}

const sADCSession *cADCSessionManager::Find(nSocket::cAsyncConn *conn) const
{
	tSessionMap::const_iterator it = mSessions.find(conn);
	return (it == mSessions.end()) ? NULL : &it->second;
}

nSocket::cAsyncConn *cADCSessionManager::FindBySID(const std::string &sid) const
{
	tSIDMap::const_iterator it = mSIDIndex.find(sid);
	return (it == mSIDIndex.end()) ? NULL : it->second;
}

void cADCSessionManager::NormalConnections(
	std::vector<nSocket::cAsyncConn*> &dest) const
{
	dest.clear();

	for (tSessionMap::const_iterator it = mSessions.begin();
		it != mSessions.end(); ++it) {
		if (it->first && it->second.mState == eADC_STATE_NORMAL)
			dest.push_back(it->first);
	}
}

bool cADCSessionManager::HasINFSupport(const sADCSession &session,
	const std::string &feature)
{
	for (size_t i = 0; i < session.mINF.size(); ++i) {
		const std::string &parameter = session.mINF[i];

		if (parameter.size() < 2 || parameter.compare(0, 2, "SU") != 0)
			continue;

		const std::string supports = parameter.substr(2);
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
	}

	return false;
}

bool cADCSessionManager::MatchesSelectors(const sADCSession &session,
	const std::vector<std::string> &selectors)
{
	for (size_t i = 0; i < selectors.size(); ++i) {
		if (selectors[i].size() != 5)
			return false;

		const bool present = HasINFSupport(session, selectors[i].substr(1, 4));

		if ((selectors[i][0] == '+' && !present) ||
			(selectors[i][0] == '-' && present))
			return false;
	}

	return true;
}

void cADCSessionManager::FeatureConnections(
	const std::vector<std::string> &selectors,
	std::vector<nSocket::cAsyncConn*> &dest) const
{
	dest.clear();

	for (tSessionMap::const_iterator it = mSessions.begin();
		it != mSessions.end(); ++it) {
		if (it->first && it->second.mState == eADC_STATE_NORMAL &&
			MatchesSelectors(it->second, selectors))
			dest.push_back(it->first);
	}
}

bool cADCSessionManager::IdentityInUse(const std::string &nick,
	const std::string &cid, nSocket::cAsyncConn *except) const
{
	for (tSessionMap::const_iterator it = mSessions.begin();
		it != mSessions.end(); ++it) {
		if (it->first == except)
			continue;

		if ((!nick.empty() && it->second.mNick == nick) ||
			(!cid.empty() && it->second.mCID == cid))
			return true;
	}

	return false;
}

std::string cADCSessionManager::SIDFromNumber(unsigned int value)
{
	static const char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
	std::string sid(4, 'A');

	value &= 0x000fffff;

	for (int i = 3; i >= 0; --i) {
		sid[i] = alphabet[value & 31];
		value >>= 5;
	}

	return sid;
}

bool cADCSessionManager::AssignSID(nSocket::cAsyncConn *conn, std::string &sid)
{
	if (!conn)
		return false;

	sADCSession &session = Attach(conn);

	if (session.mState != eADC_STATE_PROTOCOL)
		return false;

	if (!session.mSID.empty()) {
		sid = session.mSID;
		session.mState = eADC_STATE_IDENTIFY;
		return true;
	}

	for (unsigned int attempts = 0; attempts < 0x00100000; ++attempts) {
		const unsigned int candidate = mNextSID++ & 0x000fffff;
		const std::string encoded = SIDFromNumber(candidate);

		if (mSIDIndex.find(encoded) != mSIDIndex.end())
			continue;

		session.mSID = encoded;
		mSIDIndex[encoded] = conn;
		session.mState = eADC_STATE_IDENTIFY;
		sid = encoded;
		return true;
	}

	return false;
}

bool cADCSessionManager::IsBaseFeature(const std::string &feature)
{
	return feature == "BASE";
}

bool cADCSessionManager::ApplySUP(nSocket::cAsyncConn *conn,
	const std::set<std::string> &add, const std::set<std::string> &remove)
{
	if (!conn)
		return false;

	sADCSession &session = Attach(conn);

	if ((session.mState != eADC_STATE_PROTOCOL) &&
		(session.mState != eADC_STATE_NORMAL))
		return false;

	for (std::set<std::string>::const_iterator it = remove.begin();
		it != remove.end(); ++it)
		session.mFeatures.erase(*it);

	for (std::set<std::string>::const_iterator it = add.begin();
		it != add.end(); ++it)
		session.mFeatures.insert(*it);

	if (session.mState == eADC_STATE_PROTOCOL &&
		session.mFeatures.find("BASE") == session.mFeatures.end())
		return false;

	return true;
}

bool cADCSessionManager::SetIdentity(nSocket::cAsyncConn *conn,
	const std::string &nick, const std::string &cid, const std::string &pid,
	bool registered)
{
	sADCSession *session = Find(conn);

	if (!session || session->mState != eADC_STATE_IDENTIFY ||
		nick.empty() || cid.empty() || pid.empty())
		return false;

	if (IdentityInUse(nick, cid, conn))
		return false;

	session->mNick = nick;
	session->mCID = cid;
	session->mPID = pid;
	session->mRegistered = registered;
	return true;
}

bool cADCSessionManager::EnterVerify(nSocket::cAsyncConn *conn)
{
	sADCSession *session = Find(conn);

	if (!session || session->mState != eADC_STATE_IDENTIFY ||
		!session->mRegistered)
		return false;

	session->mState = eADC_STATE_VERIFY;
	return true;
}

bool cADCSessionManager::EnterNormal(nSocket::cAsyncConn *conn)
{
	sADCSession *session = Find(conn);

	if (!session)
		return false;

	if (session->mState == eADC_STATE_IDENTIFY) {
		if (session->mRegistered)
			return false;
	} else if (session->mState != eADC_STATE_VERIFY) {
		return false;
	}

	session->mState = eADC_STATE_NORMAL;
	session->mGPA.clear();
	return true;
}

bool cADCSessionManager::ClientCommandAllowed(nSocket::cAsyncConn *conn,
	const std::string &command) const
{
	const sADCSession *session = Find(conn);

	if (!session)
		return command == "SUP" || command == "STA";

	if (command == "STA")
		return true;

	switch (session->mState) {
		case eADC_STATE_PROTOCOL:
			return command == "SUP";

		case eADC_STATE_IDENTIFY:
			return command == "INF" || command == "QUI";

		case eADC_STATE_VERIFY:
			return command == "PAS" || command == "QUI";

		case eADC_STATE_NORMAL:
			// This manager represents the client-hub TCP session. GET/GFI/SND
			// are BASE client-client (C-context) commands and must be handled on
			// the separate transfer connection, never by the hub adapter.
			return command == "SUP" || command == "INF" ||
				command == "MSG" || command == "SCH" ||
				command == "RES" || command == "CTM" ||
				command == "RCM" || command == "QUI";
	}

	return false;
}

	}; // namespace nProtocol
}; // namespace nVerliHub
