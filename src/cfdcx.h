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

#ifndef NSOCKETCFDCX_H
#define NSOCKETCFDCX_H

#include <map>
#include <string>
#include <vector>
#include "cobj.h"
#include "csocketcancel.h"
#include "cworkerpool.h"

#define FDCX_EXPLORER "https://explorer.feardc.net" // note: explorer that confirms tips and that links of announcements point to, https with certificate that system trusts

using namespace std;

namespace nVerliHub {
	namespace nSocket {
		class cServerDC;
		class cConnDC;

/*
	fdcx of hub itself: address of hub for users of fdcx, and tips to it that explorer confirms before security bot thanks for them in main chat
	a payment that is not in a block yet is asked again later, each payment is announced once, also after hub restarts
	note: main thread only, worker thread gets copies of url and gives back answer of explorer
	note: explorer is asked only over https with checked certificate, its answer is all that proves a payment
*/

class cFDCX: public cObj
{
	public:
		cFDCX(cServerDC &serv);

		// fdcx wallet address: fdcx1, tfdcx1 or rfdcx1 with bech32 or bech32m checksum, lower case
		static bool ValidAddress(const string &addr);
		// fdcx_address when it is a valid fdcx address, else empty
		string HubAddress();
		// address of hub with smallest tip that is announced, to one user or to all with FDCX support when conn is null; "$FDCXH" alone when hub has none
		void SendHubAddress(cConnDC *conn);
		// user tells it paid a tip to address of hub: explorer confirms it on worker thread, then security bot thanks the user in main chat
		void TipCheck(cConnDC *conn, const string &txid);
		// asks explorer about waiting tips that are due, called by timer of hub
		void TipTick(long now);

		bool TipsWaiting() const
		{
			return !mWaits.empty();
		}

	private:
		struct sWait // note: one check is one payment of one nick, another user can send the same id, only payer passes the memo
		{
			string mNick; // note: payer as hub knows it, memo of payment must name this nick
			string mTxid;
			string mAddr; // note: hub address at time of notice, a changed address drops the check
			string mTag; // note: memo hub expects, made from hub name at time of notice
			string mIP; // note: address of payer, ipv6 by its /64; checks of one address are limited like those of one nick
			tSocket mSock = 0; // note: payer is told why tip was not announced while still online
			unsigned long long mConnID = 0;
			unsigned int mTries = 0;
			long mNext = 0; // note: second of next question to explorer
			bool mBusy = false; // note: question is on worker thread now
		};

		cServerDC &mServ;
		cSocketCancel mCancel; // note: declared before pool, so it outlives its threads
		nThread::cWorkerPool mWorkers;
		vector<sWait> mWaits;
		std::map<string, long> mSeen; // note: announced transaction id and second of announcement, kept in file so restart repeats none
		std::map<string, long> mCool; // note: address whose notice was no payment to hub of its sender, second until it is checked again
		bool mLoaded = false; // note: file of announced ids was read

		void Ask(sWait &wait, long now);
		void Answer(const string &txid, const string &nick, int code, const string &body, const string &err);
		void End(const string &txid, const string &nick, const string &why, bool junk = false); // note: check ends, payer is told why when why is not empty; junk is a notice of no real tip of its sender, its address waits then
		string Tag(const string &nick); // note: hex of memo that names payer on this hub, same as client makes it
		string SeenFile();
		void LoadSeen(); // note: once, announced ids of last hours
		void SaveSeen();
		void PruneSeen(long now);
};

	}; // namespace nSocket
}; // namespace nVerliHub

#endif
