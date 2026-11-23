/*
	Copyright (C) 2003-2005 Daniel Muller, dan at verliba dot cz
	Copyright (C) 2006-2026 Verlihub Team, info at verlihub dot net

	Verlihub is free software; You can redistribute it
	and modify it under the terms of the GNU General
	Public License as published by the Free Software
	Foundation, either version 3 of the license, or at
	your option any later version.

	Verlihub is distributed in the hope that it will be
	useful, but without any warranty, without even the
	implied warranty of merchantability or fitness for
	a particular purpose. See the GNU General Public
	License for more details.

	Please see https://www.gnu.org/licenses/ for a copy
	of the GNU General Public License.
*/

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <openssl/evp.h>
#include "cfdcx.h"
#include "cserverdc.h"
#include "cconndc.h"
#include "chttpconn.h"
#include "csocketapi.h"
#include "stringutils.h"

#ifdef _WIN32
	#include <windows.h> // note: MoveFileExA
#endif
#include "i18n.h"

namespace nVerliHub {
	using namespace nUtils;
	using namespace nEnums;

	namespace nSocket {

static const size_t sWaitMax = 10; // note: checks at once, each asks explorer every half minute at most
static const size_t sWaitUser = 2; // note: checks of one nick and of one address, a few users can not take all checks
static const unsigned int sTriesMax = 20; // note: payment waits for its confirmations this many times
static const long long sConfMin = 3; // note: blocks on top of the payment before it is announced, one block of a young chain is cheap to replace
static const long sRetry = 30; // note: seconds between questions about one payment
static const long sAgeMax = 3600; // note: older payment is not announced, nobody repeats an old tip
static const long sAheadMax = 7200; // note: block time can be as far ahead as fdcx nodes take it, two hours
static const long sSeenKeep = 14400; // note: longer than any payment that can still pass age and time checks, sAheadMax + sAgeMax
static const long sCoolTime = 600; // note: address that sent a notice of no real tip is not checked for this long, so a few bots can not keep all checks busy

// address of user for limits: ipv4 as it is, ipv6 by its /64, one host of ipv6 usually has a whole /64
static string LimitKey(const string &ip)
{
	if (ip.find(':') == string::npos)
		return ip;

	unsigned char raw[16];

	if (inet_pton(AF_INET6, ip.c_str(), raw) != 1)
		return ip;

	static const char hex[] = "0123456789abcdef";
	string key;

	for (int i = 0; i < 8; i++) {
		key.append(1, hex[(raw[i] >> 4) & 15]);
		key.append(1, hex[raw[i] & 15]);
	}

	return key + "/64";
}

static bool PlainHex(const string &text)
{
	for (const char ch: text) {
		if (!(((ch >= '0') && (ch <= '9')) || ((ch >= 'a') && (ch <= 'f'))))
			return false;
	}

	return true;
}

// smallest units as fdcx with as many decimals as needed, 10, 10.5 or 0.00000001
static string TipAmount(long long sats)
{
	string text(StringFrom(sats / 100000000LL));
	const long long frac = sats % 100000000LL;

	if (frac) {
		string part(StringFrom(frac));
		part.insert(0, 8 - part.size(), '0');

		while (part.size() && (part.back() == '0'))
			part.pop_back();

		text.append(1, '.');
		text.append(part);
	}

	return text;
}

cFDCX::cFDCX(cServerDC &serv):
	cObj("cFDCX"),
	mServ(serv)
{
	mWorkers.SetOnStop([this] { mCancel.Cancel(); }); // note: dont wait for explorer when hub stops
}

// note: bech32 and bech32m checksum as bip173 and bip350 define it
bool cFDCX::ValidAddress(const string &addr)
{
	static const string charset("qpzry9x8gf2tvdw0s3jn54khce6mua7l");
	const size_t sep = addr.rfind('1');

	if ((addr.size() > 90) || (sep == string::npos) || ((sep + 8) > addr.size())) // note: at least witness version and checksum
		return false;

	const string hrp(addr.substr(0, sep));

	if ((hrp != "fdcx") && (hrp != "tfdcx") && (hrp != "rfdcx"))
		return false;

	std::vector<unsigned int> values;

	for (const char ch: hrp)
		values.push_back(static_cast<unsigned char>(ch) >> 5);

	values.push_back(0);

	for (const char ch: hrp)
		values.push_back(static_cast<unsigned char>(ch) & 31);

	for (size_t pos = sep + 1; pos < addr.size(); pos++) {
		const size_t val = charset.find(addr[pos]);

		if (val == string::npos)
			return false;

		values.push_back(static_cast<unsigned int>(val));
	}

	static const uint32_t gen[5] = {0x3b6a57b2, 0x26508e6d, 0x1ea119fa, 0x3d4233dd, 0x2a1462b3};
	uint32_t chk = 1;

	for (const unsigned int val: values) {
		const uint32_t top = chk >> 25;
		chk = ((chk & 0x1ffffff) << 5) ^ val;

		for (int bit = 0; bit < 5; bit++) {
			if ((top >> bit) & 1)
				chk ^= gen[bit];
		}
	}

	const size_t first = hrp.size() * 2 + 1, len = addr.size() - sep - 7; // note: 5 bit groups after separator without checksum, first one is witness version
	const unsigned int ver = values[first];

	if ((ver == 0) ? (chk != 1) : (chk != 0x2bc830a3)) // note: version 0 uses bech32, later versions bech32m
		return false;

	unsigned int acc = 0, bits = 0;
	size_t prog = 0;

	for (size_t pos = 1; pos < len; pos++) {
		acc = (acc << 5) | values[first + pos];
		bits += 5;

		if (bits >= 8) {
			bits -= 8;
			prog++;
		}

		acc &= 0xff;
	}

	if ((bits >= 5) || (acc & ((1u << bits) - 1))) // note: padding is less than 5 bits and all zero
		return false;

	// note: versions above 1 have no rules yet, nor version 1 of other length than 32 (taproot), as fdcx clients check it
	return ((ver == 0) && ((prog == 20) || (prog == 32))) || ((ver == 1) && (prog == 32));
}

bool cFDCX::MainAddress(const string &addr)
{
	return (addr.compare(0, 5, "fdcx1") == 0) && ValidAddress(addr);
}

string cFDCX::HubAddress()
{
	string addr(mServ.mC.fdcx_address);

	for (char &ch: addr) // note: address is sent in lower case, as users send theirs
		ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));

	if (!MainAddress(addr)) {
		// note: also an address set before this check, from config of an older version, is never used silently; each wrong value is logged once
		if (addr.size() && (mServ.mC.fdcx_address != mBadAddress)) {
			mBadAddress = mServ.mC.fdcx_address;

			if (ErrLog(1))
				LogStream() << "fdcx_address is not a valid FDCX mainnet address (fdcx1), hub has no address: " << mBadAddress.substr(0, 100) << endl;
		}

		addr.clear();
	} else {
		mBadAddress.clear();
	}

	return addr;
}

void cFDCX::SendHubAddress(cConnDC *conn)
{
	const string addr = HubAddress();
	string omsg("$FDCXH ");

	if (addr.size()) { // note: smallest tip in whole fdcx that is announced, 0 is never, client sends no notice for smaller ones
		omsg.append(addr);
		omsg.append(1, ' ');
		omsg.append(StringFrom(mServ.mC.fdcx_mention));
	}

	if (conn) {
		if (addr.empty()) // note: hub had none at login, client knows none
			return;

		conn->Send(omsg, true);
	} else {
		mServ.mUserList.SendToAllWithFeature(omsg, eSF_FDCX, mServ.mC.delayed_myinfo, true);
	}
}

string cFDCX::Tag(const string &nick)
{
	// note: client takes hub name as it comes in $HubName, at most 1024 bytes and only part before topic
	string name(mServ.mC.hub_name, 0, 1024);
	const size_t pos = name.find(" - ");

	if ((pos != string::npos) && (pos > 0))
		name.resize(pos);

	string data(name);
	data.append(1, '\0'); // note: zero byte is in no name or nick, so no other pair gives same bytes
	data.append(nick);

	unsigned char md[EVP_MAX_MD_SIZE];
	unsigned int len = 0;

	if ((EVP_Digest(data.data(), data.size(), md, &len, EVP_sha256(), NULL) != 1) || (len < 16))
		return "";

	static const char hex[] = "0123456789abcdef";
	string tag;

	for (const char ch: string("feardc:")) { // note: memo of fearDC payment is this prefix and first 16 bytes of sha-256
		tag.append(1, hex[(static_cast<unsigned char>(ch) >> 4) & 15]);
		tag.append(1, hex[static_cast<unsigned char>(ch) & 15]);
	}

	for (unsigned int i = 0; i < 16; i++) {
		tag.append(1, hex[(md[i] >> 4) & 15]);
		tag.append(1, hex[md[i] & 15]);
	}

	return tag;
}

string cFDCX::SeenFile()
{
	return mServ.mConfigBaseDir + "/fdcx_tips";
}

void cFDCX::PruneSeen(long now)
{
	for (auto it = mSeen.begin(); it != mSeen.end();) {
		if ((now - it->second) > sSeenKeep)
			it = mSeen.erase(it);
		else
			++it;
	}
}

void cFDCX::LoadSeen()
{
	mLoaded = true;
	ifstream file(SeenFile().c_str());

	if (!file.is_open()) // note: no tip was announced yet
		return;

	string line;

	while (getline(file, line)) { // note: <transaction id> <second of announcement>, other lines are left out
		istringstream is(line);
		string txid;
		long when = 0;

		if ((is >> txid >> when) && (txid.size() == 64) && PlainHex(txid))
			mSeen[txid] = when;
	}
}

void cFDCX::SaveSeen()
{
	// note: new list goes to other file first, a failed write keeps the old one and every id in it
	const string path(SeenFile()), part(path + ".part");
	ofstream file(part.c_str(), ios::out | ios::trunc);

	for (const auto &item: mSeen)
		file << item.first << ' ' << item.second << '\n';

	file.close();
	bool good = !file.fail();

	if (good) {
	#ifdef _WIN32
		good = (MoveFileExA(part.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING) != 0); // note: rename of windows does not replace a file
	#else
		good = (rename(part.c_str(), path.c_str()) == 0);
	#endif
	}

	if (!good) {
		remove(part.c_str());

		if (ErrLog(1)) // note: announcement still happened, after a restart within four hours this one could be announced again
			LogStream() << "Announced tip can not be saved to " << path << endl;
	}
}

void cFDCX::TipCheck(cConnDC *conn, const string &txid)
{
	if (!conn || !conn->mpUser)
		return;

	const string addr = HubAddress();

	if (!mServ.mC.fdcx_mention || addr.empty()) {
		mServ.DCPublicHS(_("This hub does not announce tips."), conn);
		return;
	}

	const long now = mServ.mTime.Sec();

	if (!mLoaded)
		LoadSeen();

	PruneSeen(now);

	if (mSeen.count(txid)) {
		mServ.DCPublicHS(_("This payment was announced already."), conn);
		return;
	}

	const string &nick = conn->mpUser->mNick;
	const string ip = LimitKey(conn->AddrIP());

	for (auto it = mCool.begin(); it != mCool.end();) {
		if (it->second <= now)
			it = mCool.erase(it);
		else
			++it;
	}

	if (mCool.count(ip)) {
		mServ.DCPublicHS(_("A tip notice from your address was no payment of its sender to this hub, notices from it are not checked for a few minutes."), conn);
		return;
	}

	size_t own = 0, same_ip = 0;

	for (const sWait &wait: mWaits) {
		if ((wait.mTxid == txid) && (wait.mNick == nick)) {
			mServ.DCPublicHS(_("This payment is still being checked."), conn);
			return;
		}

		if (wait.mNick == nick)
			own++;

		if (wait.mIP == ip)
			same_ip++;
	}

	if (own >= sWaitUser) {
		mServ.DCPublicHS(_("Two of your tips are still being checked, this one is not announced."), conn);
		return;
	}

	if (same_ip >= sWaitUser) {
		mServ.DCPublicHS(_("Two tips from your address are still being checked, this one is not announced."), conn);
		return;
	}

	if (mWaits.size() >= sWaitMax) {
		mServ.DCPublicHS(_("Too many tips are being checked now, this one is not announced."), conn);
		return;
	}

	const string tag = Tag(nick); // note: name of hub now, client made its memo with the name it had when it paid

	if (tag.empty()) {
		if (ErrLog(1))
			LogStream() << "Tip check of " << nick << " can not start, SHA-256 is not available" << endl;

		mServ.DCPublicHS(_("Tips can not be checked on this hub now."), conn);
		return;
	}

	sWait wait;
	wait.mNick = nick;
	wait.mTxid = txid;
	wait.mAddr = addr;
	wait.mTag = tag;
	wait.mIP = ip;
	wait.mSock = (tSocket)(*conn);
	wait.mConnID = conn->mConnID;
	wait.mNext = now;
	mWaits.push_back(wait);

	mServ.DCPublicHS(autosprintf(_("Your tip is being checked, it is announced in main chat as soon as the payment has %d confirmations."), (int)sConfMin), conn);
	TipTick(now);
}

void cFDCX::TipTick(long now)
{
	vector<pair<string, string>> failed;

	for (sWait &wait: mWaits) {
		if (wait.mBusy || (wait.mNext > now))
			continue;

		Ask(wait, now);

		if (!wait.mBusy && (wait.mTries >= sTriesMax)) // note: question could not start, last try is gone
			failed.push_back(make_pair(wait.mTxid, wait.mNick));
	}

	for (const auto &item: failed) // note: list is changed only after loop
		End(item.first, item.second, _("Explorer could not be asked, tip is not announced."));
}

void cFDCX::Ask(sWait &wait, long now)
{
	const string url(FDCX_EXPLORER "/api/wallet/tip/" + wait.mAddr + '/' + wait.mTxid);
	wait.mTries++;
	wait.mNext = now + sRetry;
	cSocketCancel *cancel = &mCancel;
	const string txid(wait.mTxid), nick(wait.mNick);

	const bool posted = mServ.StartPool(mWorkers, "tip", 1, sWaitMax) && mWorkers.Post([this, url, txid, nick, cancel]() -> nThread::cWorkerPool::tResult { // note: runs on worker thread
		int code = 0;
		string head, body, err;

		try { // note: pool drops a job that throws without result, check would stay busy and hold its place
			cHTTPConn::Fetch("GET", url, "", "", 20, cancel, code, head, body, err, "", 0, 4096, true); // note: answer is one short line, certificate of explorer is checked
		} catch (...) {
			code = eHTTP_NETWORK;
			body.clear();
			err = "Request failed";
		}

		return [this, txid, nick, code, body, err] { Answer(txid, nick, code, body, err); }; // note: runs on main thread
	});

	if (posted)
		wait.mBusy = true;
}

void cFDCX::Answer(const string &txid, const string &nick, int code, const string &body, const string &err)
{
	auto it = std::find_if(mWaits.begin(), mWaits.end(), [&txid, &nick](const sWait &wait) { return (wait.mTxid == txid) && (wait.mNick == nick); });

	if (it == mWaits.end()) // note: check ended meanwhile
		return;

	it->mBusy = false;
	const bool last = (it->mTries >= sTriesMax);
	const string tag(it->mTag);

	if (!mServ.mC.fdcx_mention) {
		End(txid, nick, _("This hub does not announce tips now."));
		return;
	}

	if (HubAddress() != it->mAddr) {
		End(txid, nick, _("FDCX address of this hub changed, tip is not announced."));
		return;
	}

	if (code == eHTTP_CERT) { // note: certificate of explorer is not trusted, asking again changes nothing; a broken handshake is asked again below
		if (ErrLog(1))
			LogStream() << "Tip check of " << nick << " stopped, explorer can not be trusted: " << err << endl;

		End(txid, nick, _("Explorer of this hub can not be checked, tip is not announced."));
		return;
	}

	if (code == 404) { // note: client sends notice only after node of explorer took the payment, so explorer that does not know it means there is none
		End(txid, nick, _("Payment was not found, tip is not announced."), true);
		return;
	}

	if (code == 400) {
		if (ErrLog(1))
			LogStream() << "Tip check of " << nick << " refused by explorer: " << body.substr(0, 200) << endl;

		End(txid, nick, _("Explorer did not take this payment, tip is not announced."), true);
		return;
	}

	if (code != 200) { // note: no answer, busy or limit, asked again later
		if (last) {
			if (ErrLog(1))
				LogStream() << "Tip check of " << nick << " got no answer from explorer, last code " << code << ": " << err << endl;

			End(txid, nick, _("Explorer did not answer, tip is not announced."));
		}

		return;
	}

	istringstream is(body);
	long long conf = -1, paid = -1, block = -1;
	string memo, rest;

	if (!(is >> conf >> paid >> block >> memo) || (is >> rest) || (conf < 0) || (paid < 0) || (block < 0)) {
		if (ErrLog(1))
			LogStream() << "Tip check of " << nick << " got answer that can not be read: " << body.substr(0, 200) << endl;

		End(txid, nick, _("Explorer gave an answer that can not be read, tip is not announced."));
		return;
	}

	// note: what a payment pays and whom it names is known before its block, a wrong one ends at once
	if (!paid) { // note: explorer counts nothing for a payment to another address and for one that the address of hub made itself
		End(txid, nick, _("This payment brings no coins to the FDCX address of this hub."), true);
		return;
	}

	if (paid < ((long long)mServ.mC.fdcx_mention * 100000000LL)) {
		End(txid, nick, autosprintf(_("Tips from %u FDCX are announced here, this payment is %s FDCX."), mServ.mC.fdcx_mention, TipAmount(paid).c_str()), true);
		return;
	}

	if (memo.compare(0, tag.size(), tag) != 0) { // note: payer signed its own nick into the payment, so nobody can announce the tip of another user
		End(txid, nick, _("This payment does not name you as its payer, only the FearDC user who sent it can announce it."), true);
		return;
	}

	if (conf < sConfMin) { // note: payment waits for its confirmations
		if (last)
			End(txid, nick, autosprintf(_("Payment did not get %d confirmations in 10 minutes, tip is not announced."), (int)sConfMin));

		return;
	}

	const long long now = mServ.mTime.Sec();

	if (block > (now + sAheadMax)) {
		End(txid, nick, _("This payment has a block time in the future, tip is not announced."), true);
		return;
	}

	if ((now - block) > sAgeMax) {
		End(txid, nick, _("This payment is older than one hour, tip is not announced."), true);
		return;
	}

	if (mSeen.count(txid)) { // note: same payment from another check of same payer that ended first
		End(txid, nick, _("This payment was announced already."));
		return;
	}

	ostringstream os;
	os << autosprintf(_("Thank you for supporting this hub with %s FDCX: %s"), TipAmount(paid).c_str(), nick.c_str()) << "\r\n";
	os << autosprintf(_("FDCX is a real coin that you can earn and send with the FearDC client: %s"), "https://client.feardc.net/") << "\r\n";
	os << autosprintf(_("See this payment: %s"), (FDCX_EXPLORER "/tx/" + txid).c_str());

	mSeen[txid] = (long)now;
	SaveSeen();
	End(txid, nick, "");
	mServ.DCPublicHSToAll(os.str(), true, true);
}

void cFDCX::End(const string &txid, const string &nick, const string &why, bool junk)
{
	auto it = std::find_if(mWaits.begin(), mWaits.end(), [&txid, &nick](const sWait &wait) { return (wait.mTxid == txid) && (wait.mNick == nick); });

	if (it == mWaits.end())
		return;

	if (junk) // note: explorer answered and it was no real tip of this sender, its address waits before it can take a check again
		mCool[it->mIP] = mServ.mTime.Sec() + sCoolTime;

	if (why.size()) { // note: payer still online with same connection hears why
		cAsyncConn *found = mServ.FindConn(it->mSock, it->mConnID);
		cConnDC *conn = static_cast<cConnDC*>(found);

		if (conn && conn->mpUser && (conn->mpUser->mNick == it->mNick))
			mServ.DCPublicHS(why, conn);
	}

	mWaits.erase(it);
}

	}; // namespace nSocket
}; // namespace nVerliHub
