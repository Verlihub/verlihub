/*
	Copyright (C) 2003-2005 Daniel Muller, dan at verliba dot cz
	Copyright (C) 2006-2019 Verlihub Team, info at verlihub dot net

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
#include <string>

using namespace std;

namespace nVerliHub {
	namespace nSocket {
 		class cHTTPConn;
		using namespace nUtils;

		class cHTTPConn:
			public cConnBase
		{
			public:
				cHTTPConn(int sock = 0);
				cHTTPConn(const string &host, int port);
				virtual ~cHTTPConn();

				virtual operator tSocket() const
				{
					return mSock;
				}

				size_t GetSize()
				{
					return mSend.size();
				}

				unsigned int GetBuf(string &buf)
				{
					buf.clear();
					buf.assign(mBuf, 0, mEnd);
					return mEnd;
				}

				int Connect(const string &host, const int port);
				int SetOpt(int name, const void *val, int len, int sock = 0);
				int Write(const string &data);
				bool Request(const string &meth, const string &req, const string &head, const string &data);
				int Read();
				void Close();
				void CloseNice(int msec = 0);
				void CloseNow();
				int OnTimerBase(cTime &now);

				cTime mLast;
				bool mGood;
				bool mWrite;
				tSocket mSock;

			protected:
				static char *mBuf;
				string mSend, mHost;
				unsigned int mPort;
				tSocket Create();
				int Send(const char *buf, size_t &len);

			private:
				int mEnd;
				cTime mClose;
		};

	}; // namespace nSocket
}; // namespace nVerliHub

#endif
