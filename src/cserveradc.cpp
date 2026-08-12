/*
	Copyright (C) 2006-2026 Verlihub Team, info at verlihub dot net

	Verlihub is free software; You can redistribute it
	and modify it under the terms of the GNU General
	Public License as published by the Free Software
	Foundation, either version 3 of the license, or at
	your option any later version.
*/

#include "cserveradc.h"
#include "cadchash.h"
#include "cbanlist.h"
#include "cconndc.h"
#include "creguserinfo.h"
#include "i18n.h"

namespace nVerliHub {
	using namespace nEnums;
	using namespace nTables;

	namespace nSocket {

static bool adc_has_feature(const string &features, const string &wanted)
{
	size_t start = 0;

	while (start <= features.size()) {
		const size_t end = features.find(',', start);
		const string feature = features.substr(start,
			(end == string::npos) ? string::npos : end - start);

		if (feature == wanted)
			return true;

		if (end == string::npos)
			break;

		start = end + 1;
	}

	return false;
}

static bool adc_valid_nick(const string &nick, unsigned int minLen,
	unsigned int maxLen)
{
	if (nick.size() < minLen || nick.size() > maxLen)
		return false;

	for (size_t i = 0; i < nick.size(); ++i) {
		const unsigned char c = static_cast<unsigned char>(nick[i]);

		// UTF-8 continuation/lead bytes are > 127. ADC nick characters must be
		// above Unicode code point 32; reject ASCII controls/space and DEL here.
		if (c <= 32 || c == 127)
			return false;
	}

	return true;
}

static void adc_merge_inf(vector<string> &stored, const string &parameter)
{
	if (parameter.size() < 2)
		return;

	const string key = parameter.substr(0, 2);

	for (vector<string>::iterator it = stored.begin(); it != stored.end(); ++it) {
		if (it->size() >= 2 && it->compare(0, 2, key) == 0) {
			if (parameter.size() == 2)
				stored.erase(it);
			else
				*it = parameter;

			return;
		}
	}

	if (parameter.size() > 2)
		stored.push_back(parameter);
}

cADCConnFactory::cADCConnFactory(cServerDC *server, nProtocol::cADCProto *protocol):
	cConnFactory(protocol),
	mServer(server),
	mADCProtocol(protocol),
	mCleanupFactory(new cDCConnFactory(server))
{}

cADCConnFactory::~cADCConnFactory()
{
	if (mCleanupFactory) {
		delete mCleanupFactory;
		mCleanupFactory = NULL;
	}
}

cAsyncConn *cADCConnFactory::CreateConn(tSocket sd)
{
	if (!mServer || !mADCProtocol)
		return NULL;

	// Create the common Verlihub connection container directly. The old
	// protocol factory is not involved in connection creation or binding.
	cConnDC *conn = new cConnDC(sd, mServer);
	conn->ClearLine(); // LF framing required by ADC.
	conn->mxMyFactory = this;
	conn->mxProtocol = mADCProtocol;
	return conn;
}

void cADCConnFactory::DeleteConn(cAsyncConn *&connection)
{
	if (mADCProtocol && connection)
		mADCProtocol->OnDisconnect(connection);

	// Temporary cleanup bridge: the historical factory still owns private
	// account/plugin cleanup. It is never used for the live ADC wire path.
	if (mCleanupFactory)
		mCleanupFactory->DeleteConn(connection);
	else
		cConnFactory::DeleteConn(connection);
}

cServerADC::cServerADC(string CfgBase, const string &ExecPath):
	cServerDC(CfgBase, ExecPath),
	mADCProto()
{
	if (mFactory) {
		delete mFactory;
		mFactory = NULL;
	}

	mFactory = new cADCConnFactory(this, &mADCProto);
}

cServerADC::~cServerADC()
{}

bool cServerADC::SendFrame(cAsyncConn *conn, const string &frame, bool flush)
{
	if (!conn || frame.empty())
		return false;

	string wire(frame);
	wire.push_back('\n');
	return conn->Write(wire, flush) >= 0;
}

bool cServerADC::SendStatus(cAsyncConn *conn, const string &code,
	const string &description, const vector<string> &flags, bool close)
{
	string frame;

	if (!nProtocol::cADCProto::CreateSTA(frame, code, description, flags))
		return false;

	const bool sent = SendFrame(conn, frame, true);

	if (close && conn)
		conn->CloseNice(1000, eCR_LOGIN_ERR);

	return sent;
}

bool cServerADC::SendHubINF(cAsyncConn *conn)
{
	if (!conn)
		return false;

	vector<string> parameters;
	parameters.push_back("CT32");

	if (!mC.hub_name.empty())
		parameters.push_back("NI" + mC.hub_name);

	if (!mC.hub_desc.empty())
		parameters.push_back("DE" + mC.hub_desc);

	if (!mC.hub_version.empty())
		parameters.push_back("VE" + mC.hub_version);

	string frame;
	return nProtocol::cADCProto::CreateInfo(frame, "INF", parameters) &&
		SendFrame(conn, frame, true);
}

bool cServerADC::PrepareInitialINF(nProtocol::cMessageADC *msg, cConnDC *conn,
	vector<string> &sanitized, string &nick, string &cid, string &pid)
{
	sanitized.clear();
	nick.clear();
	cid.clear();
	pid.clear();

	if (!msg || !conn)
		return false;

	vector<string> flags;
	string features;

	if (!msg->GetNamed("NI", nick)) {
		flags.push_back("FMNI");
		SendStatus(conn, "243", "Required INF field NI missing", flags, true);
		return false;
	}

	if (!msg->GetNamed("ID", cid)) {
		flags.push_back("FMID");
		SendStatus(conn, "243", "Required INF field ID missing", flags, true);
		return false;
	}

	if (!msg->GetNamed("PD", pid)) {
		flags.push_back("FMPD");
		SendStatus(conn, "243", "Required INF field PD missing", flags, true);
		return false;
	}

	if (!msg->GetNamed("SU", features)) {
		flags.push_back("FMSU");
		SendStatus(conn, "243", "Required INF field SU missing", flags, true);
		return false;
	}

	if (!adc_valid_nick(nick, mC.min_nick, mC.max_nick)) {
		flags.push_back("FBNI");
		SendStatus(conn, "221", "Invalid ADC nickname", flags, true);
		return false;
	}

	if (!adc_has_feature(features, "BASE")) {
		flags.push_back("FCBASE");
		SendStatus(conn, "245", "BASE missing from INF SU", flags, true);
		return false;
	}

	if (!nProtocol::cADCHash::IsTigerID(cid)) {
		flags.push_back("FBID");
		SendStatus(conn, "243", "Invalid TIGR CID", flags, true);
		return false;
	}

	if (!nProtocol::cADCHash::IsTigerID(pid) ||
		!nProtocol::cADCHash::VerifyTigerCID(pid, cid)) {
		flags.push_back("FBPD");
		SendStatus(conn, "227", "Invalid PID supplied", flags, true);
		return false;
	}

	if (mADCProto.Sessions().IdentityInUse(nick, "", conn)) {
		flags.push_back("FBNI");
		SendStatus(conn, "222", "Nickname is already in use", flags, true);
		return false;
	}

	if (mADCProto.Sessions().IdentityInUse("", cid, conn)) {
		flags.push_back("FBID");
		SendStatus(conn, "224", "CID is already in use", flags, true);
		return false;
	}

	string suppliedIP;
	const bool hasI4 = msg->GetNamed("I4", suppliedIP);

	if (hasI4 && !suppliedIP.empty() && suppliedIP != "0.0.0.0" &&
		suppliedIP != conn->AddrIP()) {
		flags.push_back("I4" + conn->AddrIP());
		SendStatus(conn, "246", "Invalid IPv4 address in INF", flags, true);
		return false;
	}

	bool emittedI4 = false;
	const vector<string> &parameters = msg->Parameters();

	for (size_t i = 0; i < parameters.size(); ++i) {
		const string &parameter = parameters[i];

		if (parameter.size() < 2) {
			flags.clear();
			flags.push_back("FBINF");
			SendStatus(conn, "243", "Invalid INF parameter", flags, true);
			return false;
		}

		const string key = parameter.substr(0, 2);

		if (key == "PD")
			continue; // PID is private and must never leave the hub.

		if (key == "CT")
			continue; // Hub determines account/client type.

		if (key == "I4") {
			sanitized.push_back("I4" + conn->AddrIP());
			emittedI4 = true;
			continue;
		}

		sanitized.push_back(parameter);
	}

	if (!emittedI4)
		sanitized.push_back("I4" + conn->AddrIP());

	return true;
}

bool cServerADC::BroadcastINF(cConnDC *conn)
{
	if (!conn)
		return false;

	nProtocol::sADCSession *session = mADCProto.Sessions().Find(conn);

	if (!session || session->mState != nProtocol::eADC_STATE_NORMAL ||
		session->mSID.empty() || session->mINF.empty())
		return false;

	vector<cAsyncConn*> recipients;
	mADCProto.Sessions().NormalConnections(recipients);

	// The newly connected client receives the already connected users first.
	for (size_t i = 0; i < recipients.size(); ++i) {
		cAsyncConn *other = recipients[i];

		if (!other || other == conn || !other->ok)
			continue;

		const nProtocol::sADCSession *otherSession =
			mADCProto.Sessions().Find(other);

		if (!otherSession || otherSession->mINF.empty())
			continue;

		string frame;

		if (nProtocol::cADCProto::CreateBroadcast(frame, "INF",
			otherSession->mSID, otherSession->mINF))
			SendFrame(conn, frame, false);
	}

	// The connecting client's own INF is last and is broadcast to everyone,
	// including itself, as required for B messages.
	string own;

	if (!nProtocol::cADCProto::CreateBroadcast(own, "INF",
		session->mSID, session->mINF))
		return false;

	bool ok = true;

	for (size_t i = 0; i < recipients.size(); ++i) {
		if (!recipients[i] || !recipients[i]->ok)
			continue;

		if (!SendFrame(recipients[i], own, true))
			ok = false;
	}

	return ok;
}

bool cServerADC::EnterNormal(cConnDC *conn)
{
	if (!conn || !mADCProto.Sessions().EnterNormal(conn))
		return false;

	// cConnDC is still the shared socket container and starts the historical
	// login timer in its constructor. ADC has completed login now, so disable
	// that timer; otherwise a valid NORMAL session would later be disconnected.
	conn->ClearTimeOut(eTO_LOGIN);
	return BroadcastINF(conn);
}

bool cServerADC::TreatINF(nProtocol::cMessageADC *msg, cConnDC *conn)
{
	if (!msg || !conn)
		return false;

	nProtocol::sADCSession *session = mADCProto.Sessions().Find(conn);

	if (!session)
		return false;

	if (session->mState == nProtocol::eADC_STATE_NORMAL)
		return UpdateNormalINF(msg, conn);

	if (session->mState != nProtocol::eADC_STATE_IDENTIFY)
		return false;

	if ((msg->HeaderType() != 'B') && (msg->HeaderType() != 'H')) {
		vector<string> flags;
		flags.push_back("FCINF");
		SendStatus(conn, "244", "INF has invalid routing type", flags, true);
		return false;
	}

	vector<string> sanitized;
	string nick, cid, pid;

	if (!PrepareInitialINF(msg, conn, sanitized, nick, cid, pid))
		return false;

	if (!SetUserRegInfo(conn, nick)) {
		vector<string> flags;
		SendStatus(conn, "220", "Unable to load account information", flags, true);
		return false;
	}

	const bool registered = conn->mRegInfo && conn->mRegInfo->mEnabled;

	if (!mADCProto.Sessions().SetIdentity(conn, nick, cid, pid, registered)) {
		vector<string> flags;
		SendStatus(conn, "220", "Unable to establish ADC identity", flags, true);
		return false;
	}

	session = mADCProto.Sessions().Find(conn);

	if (!session)
		return false;

	if (registered)
		sanitized.push_back("CT2");

	session->mINF = sanitized;

	if (!registered)
		return EnterNormal(conn);

	// ADC challenge-response needs the actual UTF-8 password. Legacy one-way
	// crypt()/MD5 password records cannot produce PAS(password || salt), so they
	// must be migrated rather than silently bypassed.
	if (!conn->mRegInfo || conn->mRegInfo->mPWCrypt != cRegUserInfo::eCRYPT_NONE ||
		conn->mRegInfo->mPasswd.empty() || conn->mRegInfo->mPwdChange) {
		vector<string> flags;
		SendStatus(conn, "220",
			"Registered account requires ADC-compatible password storage",
			flags, true);
		return false;
	}

	if (!nProtocol::cADCHash::CreateTigerSalt(session->mGPA) ||
		!mADCProto.Sessions().EnterVerify(conn)) {
		vector<string> flags;
		SendStatus(conn, "220", "Unable to start password verification",
			flags, true);
		return false;
	}

	string frame;
	vector<string> parameters;
	parameters.push_back(session->mGPA);

	if (!nProtocol::cADCProto::CreateInfo(frame, "GPA", parameters))
		return false;

	return SendFrame(conn, frame, true);
}

bool cServerADC::TreatPAS(nProtocol::cMessageADC *msg, cConnDC *conn)
{
	if (!msg || !conn || msg->HeaderType() != 'H')
		return false;

	nProtocol::sADCSession *session = mADCProto.Sessions().Find(conn);

	if (!session || session->mState != nProtocol::eADC_STATE_VERIFY ||
		!session->mRegistered || msg->Parameters().size() != 1 ||
		!conn->mRegInfo || conn->mRegInfo->mPWCrypt != cRegUserInfo::eCRYPT_NONE) {
		vector<string> flags;
		flags.push_back("FCPAS");
		SendStatus(conn, "223", "Invalid password response", flags, true);
		return false;
	}

	if (!nProtocol::cADCHash::VerifyTigerPassword(conn->mRegInfo->mPasswd,
		session->mGPA, msg->Parameters()[0])) {
		vector<string> flags;
		flags.push_back("FCPAS");
		SendStatus(conn, "223", "Invalid password", flags, true);
		return false;
	}

	return EnterNormal(conn);
}

bool cServerADC::UpdateNormalINF(nProtocol::cMessageADC *msg, cConnDC *conn)
{
	if (!msg || !conn)
		return false;

	nProtocol::sADCSession *session = mADCProto.Sessions().Find(conn);

	if (!session || session->mState != nProtocol::eADC_STATE_NORMAL)
		return false;

	vector<string> delta;
	vector<string> flags;
	const vector<string> &parameters = msg->Parameters();

	for (size_t i = 0; i < parameters.size(); ++i) {
		string parameter = parameters[i];

		if (parameter.size() < 2) {
			flags.push_back("FBINF");
			SendStatus(conn, "243", "Invalid INF update", flags, false);
			return false;
		}

		const string key = parameter.substr(0, 2);
		const string value = parameter.substr(2);

		if (key == "PD")
			continue; // never retain or broadcast PID after IDENTIFY.

		if (key == "ID") {
			if (value != session->mCID) {
				flags.push_back("FBID");
				SendStatus(conn, "243", "CID cannot change during session", flags, false);
				return false;
			}

			continue;
		}

		if (key == "NI") {
			if (value.empty() || !adc_valid_nick(value, mC.min_nick, mC.max_nick)) {
				flags.push_back("FBNI");
				SendStatus(conn, "221", "Invalid ADC nickname", flags, false);
				return false;
			}

			if (session->mRegistered && value != session->mNick) {
				flags.push_back("FCINF");
				SendStatus(conn, "225", "Registered nickname cannot change", flags, false);
				return false;
			}

			if (value != session->mNick &&
				mADCProto.Sessions().IdentityInUse(value, "", conn)) {
				flags.push_back("FBNI");
				SendStatus(conn, "222", "Nickname is already in use", flags, false);
				return false;
			}

			session->mNick = value;
		}

		if (key == "SU" && (value.empty() || !adc_has_feature(value, "BASE"))) {
			flags.push_back("FCBASE");
			SendStatus(conn, "245", "BASE missing from INF SU", flags, false);
			return false;
		}

		if (key == "I4") {
			if (!value.empty() && value != "0.0.0.0" && value != conn->AddrIP()) {
				flags.push_back("I4" + conn->AddrIP());
				SendStatus(conn, "246", "Invalid IPv4 address in INF", flags, false);
				return false;
			}

			parameter = "I4" + conn->AddrIP();
		}

		if (key == "CT")
			continue; // client cannot elevate its own account type.

		adc_merge_inf(session->mINF, parameter);
		delta.push_back(parameter);
	}

	if (delta.empty())
		return true;

	string frame;

	if (!nProtocol::cADCProto::CreateBroadcast(frame, "INF", session->mSID, delta))
		return false;

	vector<cAsyncConn*> recipients;
	mADCProto.Sessions().NormalConnections(recipients);
	bool ok = true;

	for (size_t i = 0; i < recipients.size(); ++i) {
		if (recipients[i] && recipients[i]->ok && !SendFrame(recipients[i], frame, true))
			ok = false;
	}

	return ok;
}

int cServerADC::OnNewConn(cAsyncConn *nc)
{
	cConnDC *conn = dynamic_cast<cConnDC*>(nc);

	if (!conn)
		return -1;

	// ADC is client-initiated: the client sends HSUP first. No legacy greeting
	// or lock/key negotiation is performed.
	conn->SetGeoZone();
	mADCProto.Sessions().Attach(conn);

	if (mSysLoad >= eSL_RECOVERY) {
		vector<string> flags;
		SendStatus(conn, "211", "Hub is currently unable to service your request",
			flags, true);
		return -1;
	}

	if (mBanList->IsIPTempBanned(conn->AddrToNumber())) {
		cBanList::sTempBan *tban =
			mBanList->mTempIPBanlist.GetByHash(conn->AddrToNumber());

		if (tban && (tban->mUntil > mTime.Sec())) {
			vector<string> flags;
			flags.push_back("TL" + StringFrom(tban->mUntil - mTime.Sec()));
			SendStatus(conn, "232", tban->mReason, flags, true);
			return -1;
		}

		mBanList->DelIPTempBan(conn->AddrToNumber());
	}

	return 0;
}

void cServerADC::OnNewMessage(cAsyncConn *conn, string *str)
{
	if (!conn || !str || !conn->mpMsgParser || !conn->mxProtocol)
		return;

	const size_t len = str->size() + 1;
	mDownloadZone.Insert(mTime, len);
	mProtoTotal[0] += len;

	if (conn->Log(4))
		conn->LogStream() << "ADC IN [" << len << "]: " << (*str) << endl;

	conn->mpMsgParser->Parse();
	const int result = conn->mxProtocol->TreatMsg(conn->mpMsgParser, conn);
	nProtocol::cMessageADC *msg =
		dynamic_cast<nProtocol::cMessageADC*>(conn->mpMsgParser);

	if (!msg)
		return;

	cConnDC *dcConn = dynamic_cast<cConnDC*>(conn);

	if (!dcConn)
		return;

	if ((msg->mType == eADC_SUP) && (result == 0)) {
		const nProtocol::sADCSession *session = mADCProto.Sessions().Find(conn);

		if (session && session->mState == nProtocol::eADC_STATE_IDENTIFY)
			SendHubINF(conn);

		return;
	}

	if (result == 1) {
		if (msg->mType == eADC_INF) {
			TreatINF(msg, dcConn);
			return;
		}

		if (msg->mType == eADC_PAS) {
			TreatPAS(msg, dcConn);
			return;
		}
	}

	if (msg->mType == eADC_QUI)
		conn->CloseNice(0, eCR_QUIT);
}

	}; // namespace nSocket
}; // namespace nVerliHub
