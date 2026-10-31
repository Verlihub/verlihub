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
#include <functional>

struct ssl_st; // note: openssl types, so plugins do not need openssl headers
struct ssl_ctx_st;

using namespace std;

namespace nVerliHub {
	namespace nSocket {
 		class cHTTPConn;
		using namespace nUtils;

		// reason of failed request, negative so script tells it apart from http status by number alone, err text is only for log
		enum tHTTPError
		{
			eHTTP_TIMEOUT = -1, // note: no connection or no data within timeout
			eHTTP_REFUSED = -2, // note: server refused connection, nothing listens on that port
			eHTTP_RESOLVE = -3, // note: host name not found
			eHTTP_TLS = -4, // note: tls handshake failed
			eHTTP_BROKEN = -5, // note: connection closed before end of reply, or reply is not valid
			eHTTP_FILE = -6, // note: download can not be written to file
			eHTTP_CANCEL = -7, // note: hub is stopping
			eHTTP_NETWORK = -8 // note: other network error, for example connection reset
		};

		// reply of http request for scripts and plugins, runs on main thread, code is http status or negative tHTTPError with err text, body is file path for download
		typedef std::function<void(unsigned long id, int code, const string &body, const string &head, const string &err)> tHTTPDone;

		/*
			small blocking tcp client, http request and reply are used by update check, crash report, --geoip and scripts
			note: hublist registration also uses it, but only as plain tcp connection with nmdc protocol, no http there
			note: it is not part of main loop, every call blocks until data is sent or received or timeout expires, so it runs on other threads
			note: partial http client, only get and post over plain tcp or tls, server certificate is not verified, so tls only encrypts data
		*/
		class cHTTPConn
		{
			public:
				cHTTPConn(const string &host, int port, cSocketCancel *cancel = NULL, bool tls = false, unsigned int timeout = 5); // note: cancel lets other thread abort blocking connection, tls is for https, timeout is seconds without data

				// split http or https url into host, port and path, path keeps query and is / when url has none
				static bool SplitUrl(const string &url, string &host, int &port, string &path, bool &tls);

				/*
					whole request, blocks calling thread, returns false when there is no reply, then code is negative tHTTPError and err tells why
					note: follows at most 2 redirects, one for moved domain and one for rewrite rule on new server, then last reply is returned
					note: code, head and body are of any reply, also 404, head is header lines without status line
					note: when file is given, body of 2xx reply goes to file instead of body, it is written to file.part first and renamed when complete
					note: retry is number of extra attempts after timeout, network error and http 408, 429, 500, 502, 503 and 504, same as curl, with pause of 1, 2, 4 seconds and so on
					note: post and other methods with data are tried again only when connection failed, so data is never sent twice
				*/
				static bool Fetch(const string &meth, const string &url, const string &head, const string &data, unsigned int timeout, cSocketCancel *cancel, int &code, string &rhead, string &body, string &err, const string &file = "", unsigned int retry = 0);

				// whole file into memory, only 2xx reply, same as wget of vh script, tried again twice
				static bool Download(const string &url, string &data);
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
				bool ReadReply(string &head, string &body, size_t max, const string &file, string &err); // note: reply with any status, max 0 is no limit, body of 2xx reply goes to file when it is given
				int Read();
				void Close();
				void CloseNow();

				bool mGood;
				bool mWrite;
				tSocket mSock;
				int mStatus = 0; // note: http status code of last reply
				int mError = 0; // note: negative tHTTPError of last failure
				string mErrText; // note: text of last failure for log

			protected:
				tSocket Create();
				int Send(const char *buf, size_t &len);
				int Recv(char *buf, int len); // note: returns 0 when connection is closed, -1 on error or timeout
				bool StartTLS();
				void SetError(int code, const string &text); // note: socket error is told by cancel first, hub stop shuts socket down
				void SetSockError(int err, const string &what);

				// one attempt of Fetch, sent tells that connection was made, so request data might have reached server
				static bool FetchOnce(const string &meth, const string &url, const string &head, const string &data, unsigned int timeout, cSocketCancel *cancel, int &code, string &rhead, string &body, string &err, const string &file, bool &sent);

				string mHost;
				unsigned int mPort;
				unsigned int mTimeout; // note: seconds

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
