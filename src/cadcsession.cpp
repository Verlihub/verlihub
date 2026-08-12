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

	// A SID is 20 bits. Walk the space and skip identifiers still in use.
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
			return command == "SUP" || command == "INF" ||
				command == "MSG" || command == "SCH" ||
				command == "RES" || command == "CTM" ||
				command == "RCM" || command == "QUI" ||
				command == "GET" || command == "GFI" ||
				command == "SND";
	}

	return false;
}

	}; // namespace nProtocol
}; // namespace nVerliHub
