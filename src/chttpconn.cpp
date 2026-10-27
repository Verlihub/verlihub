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

#ifdef HAVE_CONFIG_H
	#include <config.h>
#endif

#include "chttpconn.h"
#include "cconnchoose.h"
#include "csocketapi.h"
#include <ostream>
#include <string.h>
#include "ctime.h"
#include "stringutils.h"

#define MAX_DATA 524288 // 0.5 mb
#define MAX_SEND 2097152 // 2.0 mb

#ifndef MSG_NOSIGNAL
	#define MSG_NOSIGNAL 0
#endif

using namespace std;

namespace nVerliHub {
	using namespace nUtils;

	namespace nSocket {

cHTTPConn::cHTTPConn(const string &host, const int port, cSocketCancel *cancel):
	mGood(false),
	mWrite(true),
	mSock(0),
	mHost(host),
	mPort(port),
	mEnd(0),
	mCancel(cancel)
{
	mBuf.resize(MAX_DATA + 1); // todo: use dynamic grow
	Connect(host, port);
}

cHTTPConn::~cHTTPConn()
{
	Close();
}

tSocket cHTTPConn::Create()
{
	tSocket sock;

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
		return INVALID_SOCKET;

#ifndef _WIN32 // note: see cAsyncConn::CreateSock
	const int yes = 1;

	if (SetOpt(SO_REUSEADDR, &yes, sizeof(yes), sock) == SOCKET_ERROR) {
		SockClose(sock);
		return INVALID_SOCKET;
	}
#endif

	return sock;
}

int cHTTPConn::SetOpt(int name, const void *val, int len, tSocket sock)
{
	return SockSetOpt((sock ? sock : mSock), SOL_SOCKET, name, val, len);
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

	send << " HTTP/1.1\r\n";

	if (head.find("\r\nHost: ") == head.npos) // todo: can be at position 0
		send << "Host: " << mHost << ':' << mPort << "\r\n";

	if (head.find("\r\nAccept: ") == head.npos)
		send << "Accept: */*\r\n";

	if (head.find("\r\nUser-Agent: ") == head.npos)
		send << "User-Agent: Mozilla/5.0 (compatible; " << HUB_VERSION_NAME << '/' << HUB_VERSION_VERS << "; +https://github.com/verlihub/)\r\n";

	if (head.find("\r\nContent-Type: ") == head.npos)
		send << "Content-Type: " << ((meth == "POST") ? "application/x-www-form-urlencoded" : "text/plain") << "\r\n";

	if (head.find("\r\nContent-Length: ") == head.npos)
		send << "Content-Length: " << data.size() << "\r\n";

	/*
	if (head.find("\r\nConnection: ") == head.npos)
		send << "Connection: Close\r\n";
	*/

	send << head << "\r\n"; // note: must be double before data

	if (data.size())
		send << data;

	return Write(send.str()) > 0;
}

bool cHTTPConn::ParseReply(string &repl)
{
	if (Read() <= 0)
		return false;

	string data(mBuf.data());

	if (data.empty())
		return false;

	size_t pos = data.find("\r\n\r\n"); // skip headers for now

	if (pos == data.npos)
		return false;

	data.erase(0, pos + 4);
	repl = data;
	return true;
}

int cHTTPConn::Write(const string &data)
{
	if (!mGood)
		return -1;

	size_t len = data.size();

	if (!len)
		return 0;

	if (len > MAX_SEND) {
		CloseNow();
		return -1;
	}

	if (Send(data.data(), len) == -1) { // note: socket is blocking with send timeout, so anything not sent now is never sent
		CloseNow();
		return -1;
	}

	return len;
}

int cHTTPConn::Connect(const string &host, const int port)
{
	mSock = Create();
	mGood = false;

	if (mSock == INVALID_SOCKET)
		return -1;

	if (mCancel && !mCancel->Attach(mSock)) // note: already cancelled, dont even start
		return -1;

	SockSetTimeout(mSock, SO_RCVTIMEO, 5000);
	SockSetTimeout(mSock, SO_SNDTIMEO, 5000);
	struct addrinfo hints, *res = NULL;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET; // note: socket is ipv4
	hints.ai_socktype = SOCK_STREAM;

	if ((getaddrinfo(host.c_str(), NULL, &hints, &res) != 0) || !res) // note: thread safe, unlike gethostbyname, connections are made on other threads
		return -1;

	struct sockaddr_in dest = *(struct sockaddr_in*)res->ai_addr;
	freeaddrinfo(res);
	dest.sin_port = htons(port);

	if (connect(mSock, (struct sockaddr*)&dest, sizeof(dest)) == -1)
		return -1;

	mGood = true;
	return 0;
}

int cHTTPConn::Send(const char *buf, size_t &len)
{
	if (!mGood)
		return -1;

	size_t tot = 0;

	while (tot < len) {
		const int res = send(mSock, buf + tot, (int)(len - tot), MSG_NOSIGNAL);

		if (res < 0) {
			if (SockInterrupted()) // note: interrupted by signal, try again
				continue;

			break; // note: error or send timeout expired
		}

		tot += res;
	}

	const bool done = (tot == len);
	len = tot;
	return (done ? 0 : -1);
}

int cHTTPConn::Read()
{
	if (!mGood || !mWrite)
		return -1;

	int len = 0;
	mEnd = 0;

	do { // note: socket is blocking with receive timeout, eagain means timeout expired, so retry only when interrupted by signal
		len = recv(mSock, mBuf.data(), MAX_DATA, 0);
	} while ((len == -1) && SockInterrupted());

	if (len <= 0) {
		/*
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
		*/

		CloseNow();
		return -1;
	} else {
		mEnd = len;
		mBuf[mEnd] = '\0';
	}

	return len;
}

void cHTTPConn::Close()
{
	mWrite = false;
	mGood = false;

	if (!SockValid(mSock))
		return;

	if (mCancel) // note: must be detached before closing, so cancel can not shut down reused descriptor
		mCancel->Detach();

	SockClose(mSock);
	mSock = INVALID_SOCKET;
	mBuf.clear();
}

void cHTTPConn::CloseNow()
{
	mWrite = false;
	mGood = false;
}

	}; // namespace nSocket
}; // namespace nVerliHub
