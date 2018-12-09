/*
	Copyright (C) 2003-2005 Daniel Muller, dan at verliba dot cz
	Copyright (C) 2006-2018 Verlihub Team, info at verlihub dot net

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

#ifdef HAVE_CONFIG_H
	#include <config.h>
#endif

#if HAVE_ERRNO_H
	#include <errno.h>
#endif

#include "chttpconn.h"
#include <ostream>
#include <arpa/inet.h>
#include <sys/socket.h> // AF_INET
#include <netdb.h> // gethostbyname
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include "ctime.h"
#include "stringutils.h"

#define sockoptval_t int
#define MAX_DATA 524288 // 0.5 mb
#define MAX_SEND 2097152 // 2.0 mb

#ifndef MSG_NOSIGNAL
	#define MSG_NOSIGNAL 0
#endif

using namespace std;

namespace nVerliHub {
	using namespace nUtils;

	namespace nSocket {

char *cHTTPConn::mBuf = new char[MAX_DATA + 1];

cHTTPConn::cHTTPConn(int sock):
	mGood(sock > 0),
	mWrite(true),
	mSock(sock),
	mHost(""),
	mPort(0)
{
	mEnd = 0;
	memset(&mClose, 0, sizeof(mClose));
}

cHTTPConn::cHTTPConn(const string &host, const int port):
	mGood(false),
	mWrite(true),
	mSock(0),
	mHost(host),
	mPort(port),
	mEnd(0),
	mClose(0, 0)
{
	Connect(host, port);
}

cHTTPConn::~cHTTPConn()
{
	Close();
}

tSocket cHTTPConn::Create()
{
	tSocket sock;
	sockoptval_t yes = 1;

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
		return INVALID_SOCKET;

	if (SetOpt(SO_REUSEADDR, &yes, sizeof(sockoptval_t), sock) == INVALID_SOCKET) {
		::close(sock);
		return INVALID_SOCKET;
	}

	return sock;
}

int cHTTPConn::SetOpt(int name, const void *val, int len, int sock)
{
	return setsockopt((sock ? sock : mSock), SOL_SOCKET, name, val, len);
}

bool cHTTPConn::Request(const string &meth, const string &req, const string &head, const string &data)
{
	ostringstream send;

	if (meth.size())
		send << meth;
	else
		send << "GET";

	send << ' ';

	if (req.size())
		send << req;
	else
		send << '/';

	send << " HTTP/1.0\r\n";

	if (head.find("Host: ") == head.npos)
		send << "Host: " << mHost << ':' << mPort << "\r\n";

	if (head.find("User-Agent: ") == head.npos)
		send << "User-Agent: Mozilla/5.0 (compatible; " << HUB_VERSION_NAME << '/' << HUB_VERSION_VERS << "; +https://github.com/verlihub/)\r\n";

	if (head.find("Content-Type: ") == head.npos)
		send << "Content-Type: " << ((meth == "POST") ? "application/x-www-form-urlencoded" : "text/plain") << "\r\n";

	if ((head.find("Content-Length: ") == head.npos) && data.size())
		send << "Content-Length: " << data.size() << "\r\n\r\n";

	if (head.find("Connection: ") == head.npos)
		send << "Connection: Close\r\n";

	send << head << "\r\n";

	if (data.size())
		send << data;

	return Write(send.str()) > 0;
}

int cHTTPConn::Write(const string &data)
{
	size_t dasi = data.size();
	size_t busi = GetSize() + dasi;

	if (busi > MAX_SEND) {
		CloseNow();
		return -1;
	}

	if (dasi)
		mSend.append(data.data(), dasi);

	if (!busi)
		return 0;

	const char *buf = mSend.data();

	if (!buf)
		return 0;

	if (Send(buf, busi) == -1) {
		if ((errno != EAGAIN) && (errno != EINTR)) {
			CloseNow();
			return -1;
		}

		if (busi > 0) {
			mLast.Get();
			StrCutLeft(mSend, busi);
		} else if (bool(mClose)) {
			CloseNow();
		}
	} else {
		mSend.erase(0, busi);
		ShrinkStringToFit(mSend);

		if (bool(mClose))
			CloseNow();

		mLast.Get();
	}

	return busi;
}

int cHTTPConn::Connect(const string &host, const int port)
{
	mSock = Create();
	mGood = false;

	if (mSock == INVALID_SOCKET)
		return -1;

	cTime tout(5.0);
	SetOpt(SO_RCVTIMEO, &tout, sizeof(timeval));
	SetOpt(SO_SNDTIMEO, &tout, sizeof(timeval));
	struct hostent *name = gethostbyname(host.c_str());
	struct sockaddr_in dest;

	if (name) {
		dest.sin_family = AF_INET;
		dest.sin_port = htons(port);
		dest.sin_addr.s_addr = *(unsigned*)(name->h_addr_list[0]);
		memset(&(dest.sin_zero), '\0', 8);

		if (connect(mSock, (struct sockaddr*)&dest, sizeof(struct sockaddr)) == -1)
			return -1;

		mGood = true;
		return 0;
	}

	return -1;
}

int cHTTPConn::Send(const char *buf, size_t &len)
{
	size_t tot = 0, left = len;
	int res = 0;

#ifndef QUICK_SEND
	while (tot < len) {
		try {
			res = send(mSock, buf + tot, left, MSG_NOSIGNAL | MSG_DONTWAIT);
   		} catch (...) {
			return -1;
		}

		if (res == -1)
			break;

		tot += res;
		left -= res;
	}
#else
	res = send(mSock, buf + tot, left, 0);
	tot = res;
#endif

	len = tot;
	return res == -1 ? -1 : 0;
}

int cHTTPConn::Read()
{
	int len = 0, max = 0;
	mEnd = 0;

	if (!mGood || !mWrite)
		return -1;

	while (((len = recv(mSock, mBuf, MAX_DATA, 0)) == -1) && ((errno == EAGAIN) || (errno == EINTR)) && (max++ <= 100)) {
		::usleep(5);
	}

	if (len <= 0) {
		if (len == 0) {
			CloseNow();
			return -1;
		} else {
			switch (errno) {
				case ECONNRESET: // connection reset by peer
				case ETIMEDOUT: // connection timed out
				case EHOSTUNREACH: // no route to host
				default:
					break;
			}
		}

		CloseNow();
		return -1;
	} else {
		mEnd = len;
		mBuf[mEnd] = '\0';
		mLast.Get();
	}

	return len;
}

void cHTTPConn::Close()
{
	if (mSock <= 0)
		return;

	mWrite = false;
	mGood = false;
	TEMP_FAILURE_RETRY(::close(mSock));
	mSock = INVALID_SOCKET;
}

void cHTTPConn::CloseNow()
{
	mWrite = false;
	mGood = false;
}

void cHTTPConn::CloseNice(int msec)
{
	mWrite = false;

	if ((msec <= 0) || !GetSize()) {
		CloseNow();
		return;
	}

	mClose.Get();
	mClose += msec;
}

int cHTTPConn::OnTimerBase(cTime &now)
{

	if (bool(mClose) && (mClose > now))
		CloseNow();

	return 0;
}

	}; // namespace nSocket
}; // namespace nVerliHub
