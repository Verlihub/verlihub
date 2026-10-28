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

	Please see http://www.gnu.org/licenses/ for a copy
	of the GNU General Public License.
*/

#ifndef CASYNHTTPCCONN_H
#define CASYNHTTPCCONN_H

#include "cobj.h"
#include "ctime.h"
#include "cconnbase.h"
#include "csocketcancel.h"
#include <string>
#include <vector>

struct ssl_st; // note: openssl types, so plugins do not need openssl headers
struct ssl_ctx_st;

using namespace std;

namespace nVerliHub {
	namespace nSocket {
 		class cHTTPConn;
		using namespace nUtils;

		/*
			small blocking tcp client, http request and reply are used only by update check and crash report
			note: hublist registration also uses it, but only as plain tcp connection with nmdc protocol, no http there
			note: it is not part of main loop, every call blocks until data is sent or received or timeout expires
			note: partial http client, only get and post over plain tcp or tls, server certificate is not verified, so tls only encrypts data
		*/
		class cHTTPConn
		{
			public:
				cHTTPConn(const string &host, int port, cSocketCancel *cancel = NULL, bool tls = false); // note: cancel lets other thread abort blocking connection, tls is for https
				~cHTTPConn();

				cHTTPConn(const cHTTPConn&) = delete;
				cHTTPConn& operator=(const cHTTPConn&) = delete;

				unsigned int GetBuf(string &buf)
				{
					if (mEnd) {
						buf.clear();
						buf.assign(mBuf.data(), 0, mEnd);
					}

					return mEnd;
				}

				int Connect(const string &host, const int port);
				int SetOpt(int name, const void *val, int len, tSocket sock = 0);
				int Write(const string &data);
				bool Request(const string &meth, const string &req, const string &head, const string &data);
				bool ParseReply(string &repl, size_t max = 524288); // note: reads whole reply, returns body when status is 2xx, max is limit of body size
				int Read();
				void Close();
				void CloseNow();

				bool mGood;
				bool mWrite;
				tSocket mSock;
				int mStatus = 0; // note: http status code of last reply

			protected:
				tSocket Create();
				int Send(const char *buf, size_t &len);
				int Recv(char *buf, int len); // note: returns 0 when connection is closed, -1 on error or timeout
				bool StartTLS();

				string mHost;
				unsigned int mPort;

			private:
				vector<char> mBuf;
				unsigned int mEnd;
				cSocketCancel *mCancel = NULL;
				bool mTLS = false;
				struct ssl_st *mSSL = NULL;
				struct ssl_ctx_st *mCTX = NULL;
		};

	}; // namespace nSocket
}; // namespace nVerliHub

#endif
