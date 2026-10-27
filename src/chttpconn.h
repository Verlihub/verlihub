/*
	Copyright (C) 2003-2005 Daniel Muller, dan at verliba dot cz
	Copyright (C) 2006-2025 Verlihub Team, info at verlihub dot net

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

using namespace std;

namespace nVerliHub {
	namespace nSocket {
 		class cHTTPConn;
		using namespace nUtils;

		/*
			small blocking tcp client, http request and reply are used only by update check and crash report
			note: hublist registration also uses it, but only as plain tcp connection with nmdc protocol, no http there
			note: it is not part of main loop, every call blocks until data is sent or received or timeout expires
		*/
		class cHTTPConn
		{
			public:
				cHTTPConn(const string &host, int port, cSocketCancel *cancel = NULL); // note: cancel lets other thread abort blocking connection
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
				int SetOpt(int name, const void *val, int len, int sock = 0);
				int Write(const string &data);
				bool Request(const string &meth, const string &req, const string &head, const string &data);
				bool ParseReply(string &repl);
				int Read();
				void Close();
				void CloseNow();

				bool mGood;
				bool mWrite;
				tSocket mSock;

			protected:
				tSocket Create();
				int Send(const char *buf, size_t &len);

				string mHost;
				unsigned int mPort;

			private:
				vector<char> mBuf;
				unsigned int mEnd;
				cSocketCancel *mCancel = NULL;
		};

	}; // namespace nSocket
}; // namespace nVerliHub

#endif
