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

#ifndef NSOCKETCIP6CHECK_H
#define NSOCKETCIP6CHECK_H

#include "csocketapi.h"
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace nVerliHub {
	namespace nSocket {

/*
	checks ipv6 address of users who are connected to hub over ipv4
	user gets one time token over hub connection, connects here over ipv6 and sends it back: $MyNick <token>|
	completed tcp connection shows that address is really his, address sent by client could point to anyone
	note: this is not hub connection, nothing else is accepted here, so rest of hub stays ipv4 only
	note: client tricked into connecting here sends $MyNick <nick>|$Lock <lock>| and is reported like on hub port
*/

class cIP6Check
{
	public:
		typedef std::function<void(const std::string &nick, const std::string &addr)> tDone;
		typedef std::function<bool(const std::string &lock)> tTricked; // returns true when client should be told

		cIP6Check();
		~cIP6Check();

		// address that clients on other networks can connect to: not zero, loopback, ipv4 in ipv6 form, link local or multicast
		static bool IsPublic(const std::string &addr);

		// zero address in any form, listens on all ipv6 addresses
		static bool IsAny(const std::string &addr);

		// public and not private range fc00::/7, the only kind of address that is found by itself
		static bool IsGlobal(const std::string &addr);

		// address system would use for going out, empty when there is none, nothing is sent
		static std::string FindOwn();

		// address that clients should connect to, empty when listening on all addresses and none is found
		const std::string &TellAddress(long long now);

		bool Start(const std::string &addr, unsigned int port);
		void Stop();

		// error of last failed start
		int LastError() const
		{
			return mError;
		}

		bool IsOpen() const
		{
			return SockValid(mSock);
		}

		unsigned int Port() const
		{
			return mPort;
		}

		std::string NewToken(const std::string &nick, long long now);

		// accepts and reads what is ready without waiting, done is called with every checked address, data longer than limit is closed
		void Step(long long now, size_t limit, const tDone &done, const tTricked &tricked);

	private:
		struct sToken
		{
			std::string mNick;
			long long mUntil;
		};

		struct sPending
		{
			tSocket mSock;
			std::string mAddr;
			std::string mData;
			long long mUntil;
		};

		bool Finish(sPending &item, size_t limit, const tDone &done, const tTricked &tricked);

		tSocket mSock;
		unsigned int mPort;
		int mError;
		bool mAny;
		std::string mTell;
		long long mTellAt;
		std::map<std::string, sToken> mTokens;
		std::vector<sPending> mPending;
};

	}; // namespace nSocket
}; // namespace nVerliHub

#endif
