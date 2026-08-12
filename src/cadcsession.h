/*
	Copyright (C) 2006-2026 Verlihub Team, info at verlihub dot net

	Verlihub is free software; You can redistribute it
	and modify it under the terms of the GNU General
	Public License as published by the Free Software
	Foundation, either version 3 of the license, or at
	your option any later version.
*/

#ifndef CADCSESSION_H
#define CADCSESSION_H

#include <map>
#include <set>
#include <string>
#include <vector>

namespace nVerliHub {
	namespace nSocket {
		class cAsyncConn;
	};

	namespace nProtocol {

enum tADCSessionState
{
	eADC_STATE_PROTOCOL = 0,
	eADC_STATE_IDENTIFY,
	eADC_STATE_VERIFY,
	eADC_STATE_NORMAL
};

struct sADCSession
{
	sADCSession();

	tADCSessionState mState;
	std::string mSID;
	std::string mCID;
	std::string mPID;
	std::string mNick;
	std::string mGPA;
	std::set<std::string> mFeatures;
	std::vector<std::string> mINF;
	bool mRegistered;
};

class cADCSessionManager
{
	public:
		cADCSessionManager();
		~cADCSessionManager();

		sADCSession &Attach(nSocket::cAsyncConn *conn);
		void Detach(nSocket::cAsyncConn *conn);
		sADCSession *Find(nSocket::cAsyncConn *conn);
		const sADCSession *Find(nSocket::cAsyncConn *conn) const;
		nSocket::cAsyncConn *FindBySID(const std::string &sid) const;
		void NormalConnections(std::vector<nSocket::cAsyncConn*> &dest) const;
		void FeatureConnections(const std::vector<std::string> &selectors,
			std::vector<nSocket::cAsyncConn*> &dest) const;
		bool IdentityInUse(const std::string &nick, const std::string &cid,
			nSocket::cAsyncConn *except = NULL) const;
		void CheckTimeouts(long long nowMsec);

		bool AssignSID(nSocket::cAsyncConn *conn, std::string &sid);
		bool ApplySUP(nSocket::cAsyncConn *conn, const std::set<std::string> &add,
			const std::set<std::string> &remove);
		bool SetIdentity(nSocket::cAsyncConn *conn, const std::string &nick,
			const std::string &cid, const std::string &pid, bool registered);
		bool EnterVerify(nSocket::cAsyncConn *conn);
		bool EnterNormal(nSocket::cAsyncConn *conn);

		bool ClientCommandAllowed(nSocket::cAsyncConn *conn,
			const std::string &command) const;
		static bool IsBaseFeature(const std::string &feature);

	private:
		static std::string SIDFromNumber(unsigned int value);
		static bool HasINFSupport(const sADCSession &session,
			const std::string &feature);
		static bool MatchesSelectors(const sADCSession &session,
			const std::vector<std::string> &selectors);

		typedef std::map<nSocket::cAsyncConn*, sADCSession> tSessionMap;
		typedef std::map<std::string, nSocket::cAsyncConn*> tSIDMap;

		tSessionMap mSessions;
		tSIDMap mSIDIndex;
		unsigned int mNextSID;
};

	}; // namespace nProtocol
}; // namespace nVerliHub

#endif
