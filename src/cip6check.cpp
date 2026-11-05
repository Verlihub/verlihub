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

#include "cip6check.h"
#include <openssl/rand.h>
#include <random>
#include <string.h>

using namespace std;

namespace nVerliHub {
	namespace nSocket {

static const size_t IP6_MAX_PENDING = 256;
static const size_t IP6_READ_SIZE = 512;
static const int IP6_BACKLOG = 128;
static const long long IP6_READ_SECS = 5;
static const long long IP6_TOKEN_SECS = 60;
static const long long IP6_TELL_SECS = 30;

cIP6Check::cIP6Check():
	mSock(INVALID_SOCKET),
	mPort(0),
	mError(0),
	mAny(false),
	mTellAt(0)
{}

cIP6Check::~cIP6Check()
{
	Stop();
}

bool cIP6Check::IsPublic(const string &addr)
{
	unsigned char ip[16];

	if (inet_pton(AF_INET6, addr.c_str(), ip) != 1)
		return false;

	bool zero = true; // note: first 12 bytes zero is zero address, loopback and old form of ipv4

	for (int pos = 0; (pos < 12) && zero; pos++)
		zero = (ip[pos] == 0);

	if (zero)
		return false;

	bool mapped = (ip[10] == 0xff) && (ip[11] == 0xff); // ipv4 in ipv6 form

	for (int pos = 0; (pos < 10) && mapped; pos++)
		mapped = (ip[pos] == 0);

	if (mapped || (ip[0] == 0xff)) // multicast
		return false;

	return (ip[0] != 0xfe) || ((ip[1] & 0x80) == 0); // link local and old site local
}

bool cIP6Check::IsAny(const string &addr)
{
	unsigned char ip[16];

	if (inet_pton(AF_INET6, addr.c_str(), ip) != 1)
		return false;

	for (const unsigned char one: ip) {
		if (one != 0)
			return false;
	}

	return true;
}

bool cIP6Check::IsGlobal(const string &addr)
{
	unsigned char ip[16];

	if (!IsPublic(addr) || (inet_pton(AF_INET6, addr.c_str(), ip) != 1))
		return false;

	return (ip[0] & 0xfe) != 0xfc;
}

string cIP6Check::FindOwn()
{
	static const char *const targets[] = {"2001:4860:4860::8888", "2606:4700:4700::1111"}; // note: only for route lookup, udp connect sends nothing

	for (const char *const target: targets) {
		const tSocket sock = socket(AF_INET6, SOCK_DGRAM, 0);

		if (!SockValid(sock))
			return "";

		struct sockaddr_in6 to, own;
		memset(&to, 0, sizeof(to));
		to.sin6_family = AF_INET6;
		to.sin6_port = htons(53);
		socklen_t size = sizeof(own);
		char text[INET6_ADDRSTRLEN];
		bool found = false;

		if ((inet_pton(AF_INET6, target, &to.sin6_addr) == 1) && (connect(sock, (struct sockaddr*)&to, sizeof(to)) == 0) && (getsockname(sock, (struct sockaddr*)&own, &size) == 0))
			found = (inet_ntop(AF_INET6, &own.sin6_addr, text, sizeof(text)) != NULL);

		SockClose(sock);

		if (found && IsGlobal(text))
			return text;
	}

	return "";
}

const string &cIP6Check::TellAddress(long long now)
{
	if (IsOpen() && mAny && (now >= mTellAt)) {
		mTell = FindOwn();
		mTellAt = now + IP6_TELL_SECS;
	}

	return mTell;
}

bool cIP6Check::Start(const string &addr, unsigned int port)
{
	Stop();
	mError = 0;
	struct sockaddr_in6 local;
	memset(&local, 0, sizeof(local));
	local.sin6_family = AF_INET6;
	local.sin6_port = htons((unsigned short)port);

	if (inet_pton(AF_INET6, addr.c_str(), &local.sin6_addr) != 1)
		return false;

	const tSocket sock = socket(AF_INET6, SOCK_STREAM, 0);

	if (!SockValid(sock)) {
		mError = SockError();
		return false;
	}

	const int on = 1;
	SockSetOpt(sock, IPPROTO_IPV6, IPV6_V6ONLY, &on, sizeof(on));

	#ifndef _WIN32
		SockSetOpt(sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)); // note: windows would let others take the port too
	#endif

	if (!SockNonBlock(sock) || (::bind(sock, (struct sockaddr*)&local, sizeof(local)) != 0) || (listen(sock, IP6_BACKLOG) != 0)) {
		mError = SockError(); // note: closing socket clears it
		SockClose(sock);
		return false;
	}

	mSock = sock;
	mPort = port;
	mAny = IsAny(addr);
	mTellAt = 0;

	if (!mAny)
		mTell = addr;

	return true;
}

void cIP6Check::Stop()
{
	for (sPending &item: mPending)
		SockClose(item.mSock);

	mPending.clear();
	mTokens.clear();

	if (SockValid(mSock))
		SockClose(mSock);

	mSock = INVALID_SOCKET;
	mPort = 0;
	mAny = false;
	mTell.clear();
}

string cIP6Check::NewToken(const string &nick, long long now)
{
	unsigned char data[16];

	if (RAND_bytes(data, sizeof(data)) != 1) {
		random_device rnd;

		for (unsigned char &one: data)
			one = (unsigned char)rnd();
	}

	static const char hex[] = "0123456789abcdef";
	string token;
	token.reserve(sizeof(data) * 2);

	for (const unsigned char one: data) {
		token.append(1, hex[one >> 4]);
		token.append(1, hex[one & 15]);
	}

	sToken &item = mTokens[token];
	item.mNick = nick;
	item.mUntil = now + IP6_TOKEN_SECS;
	return token;
}

bool cIP6Check::Finish(sPending &item, size_t limit, const tDone &done, const tTricked &tricked)
{
	if (item.mData.size() > limit) // note: same limits as on hub port, nothing is reported
		return true;

	const size_t pos = item.mData.find('|');

	if (pos == string::npos)
		return false; // note: not complete yet

	static const string nick_cmd("$MyNick ");

	if (item.mData.compare(0, nick_cmd.size(), nick_cmd) != 0)
		return true;

	const auto token = mTokens.find(item.mData.substr(nick_cmd.size(), pos - nick_cmd.size()));

	if (token != mTokens.end()) {
		const string nick(token->second.mNick);
		mTokens.erase(token); // note: one time only
		done(nick, item.mAddr);
		return true;
	}

	const size_t end = item.mData.find('|', pos + 1);

	if (end == string::npos)
		return false; // note: not complete yet

	static const string lock_cmd("$Lock ");

	if (item.mData.compare(pos + 1, lock_cmd.size(), lock_cmd) != 0)
		return true;

	string lock(item.mData, pos + 1 + lock_cmd.size(), end - pos - 1 - lock_cmd.size());
	const size_t null = lock.find('\0'); // note: bug in some clients that send null character in referer

	if (null != string::npos)
		lock.resize(null);

	if (lock.size() && tricked(lock)) {
		static const string notify("$Error CTM2HUB|");
		send(item.mSock, notify.c_str(), (int)notify.size(), MSG_NOSIGNAL);
	}

	return true;
}

void cIP6Check::Step(long long now, size_t limit, const tDone &done, const tTricked &tricked)
{
	if (!IsOpen())
		return;

	for (auto it = mTokens.begin(); it != mTokens.end();) {
		if (it->second.mUntil <= now)
			it = mTokens.erase(it);
		else
			++it;
	}

	while (mPending.size() < IP6_MAX_PENDING) {
		struct sockaddr_in6 from;
		socklen_t size = sizeof(from);
		const tSocket sock = accept(mSock, (struct sockaddr*)&from, &size);

		if (!SockValid(sock))
			break;

		char text[INET6_ADDRSTRLEN];

		if (!SockNonBlock(sock) || !inet_ntop(AF_INET6, &from.sin6_addr, text, sizeof(text))) {
			SockClose(sock);
			continue;
		}

		sPending item;
		item.mSock = sock;
		item.mAddr = text;
		item.mUntil = now + IP6_READ_SECS;
		mPending.push_back(item);
	}

	for (size_t pos = 0; pos < mPending.size();) {
		sPending &item = mPending[pos];
		char data[IP6_READ_SIZE];
		const int size = (int)recv(item.mSock, data, sizeof(data), 0);
		bool over = false;

		if (size > 0) {
			item.mData.append(data, size);
			over = Finish(item, limit, done, tricked);

		} else if ((size == 0) || !SockTryAgain()) { // note: other side closed or connection failed
			over = true;
		}

		if (!over && (item.mUntil > now)) {
			pos++;
			continue;
		}

		SockClose(item.mSock);
		mPending.erase(mPending.begin() + pos);
	}
}

	}; // namespace nSocket
}; // namespace nVerliHub
