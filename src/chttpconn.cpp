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
#include <openssl/ssl.h>
#include <stdlib.h>

#define MAX_DATA 524288 // 0.5 mb
#define MAX_SEND 2097152 // 2.0 mb

#ifndef MSG_NOSIGNAL
	#define MSG_NOSIGNAL 0
#endif

using namespace std;

namespace nVerliHub {
	using namespace nUtils;

	namespace nSocket {

cHTTPConn::cHTTPConn(const string &host, const int port, cSocketCancel *cancel, bool tls):
	mGood(false),
	mWrite(true),
	mSock(0),
	mHost(host),
	mPort(port),
	mEnd(0),
	mCancel(cancel),
	mTLS(tls)
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

	if (head.find("\r\nHost: ") == head.npos) { // todo: can be at position 0
		send << "Host: " << mHost;

		if (mPort != (mTLS ? 443 : 80)) // note: default port is not sent
			send << ':' << mPort;

		send << "\r\n";
	}

	if (head.find("\r\nAccept: ") == head.npos)
		send << "Accept: */*\r\n";

	if (head.find("\r\nUser-Agent: ") == head.npos)
		send << "User-Agent: Mozilla/5.0 (compatible; " << HUB_VERSION_NAME << '/' << HUB_VERSION_VERS << "; +https://github.com/verlihub/)\r\n";

	if (head.find("\r\nContent-Type: ") == head.npos)
		send << "Content-Type: " << ((meth == "POST") ? "application/x-www-form-urlencoded" : "text/plain") << "\r\n";

	if (head.find("\r\nContent-Length: ") == head.npos)
		send << "Content-Length: " << data.size() << "\r\n";

	if (head.find("\r\nConnection: ") == head.npos) // note: server closes connection after reply, so reply ends there
		send << "Connection: close\r\n";

	send << head << "\r\n"; // note: must be double before data

	if (data.size())
		send << data;

	return Write(send.str()) > 0;
}

bool cHTTPConn::ParseReply(string &repl, size_t max)
{
	if (!mGood || !mWrite)
		return false;

	string data;
	char buf[16384];
	size_t head = string::npos, need = string::npos;
	bool chunk = false;
	mStatus = 0;

	while (true) { // note: read until connection is closed or whole body is here
		const int len = Recv(buf, sizeof(buf));

		if (len <= 0)
			break;

		data.append(buf, len);

		if (head == string::npos) {
			head = data.find("\r\n\r\n");

			if (head != string::npos) { // note: headers are complete
				const string low(toLower(data.substr(0, head + 2)));
				size_t pos = low.find(' ');

				if ((low.compare(0, 5, "http/") == 0) && (pos != string::npos))
					mStatus = atoi(low.c_str() + pos + 1);

				pos = low.find("\r\ncontent-length:");

				if (pos != string::npos)
					need = strtoul(low.c_str() + pos + 17, NULL, 10);

				pos = low.find("\r\ntransfer-encoding:");
				chunk = ((pos != string::npos) && (low.find("chunked", pos) < low.find("\r\n", pos + 2)));
			}
		}

		if ((head != string::npos) && (data.size() > (head + 4 + max + 65536))) { // note: too big, part over limit is chunk headers
			CloseNow();
			return false;
		}

		if ((head != string::npos) && !chunk && (need != string::npos) && (data.size() >= (head + 4 + need)))
			break;
	}

	CloseNow(); // note: connection is not used again

	if (head == string::npos)
		return false;

	string body(data.substr(head + 4));

	if (chunk) { // note: chunk size in hex, then data, empty chunk ends body
		string out;
		size_t pos = 0;

		while (true) {
			const size_t end = body.find("\r\n", pos);

			if (end == string::npos)
				return false;

			const size_t size = strtoul(body.c_str() + pos, NULL, 16);

			if (!size)
				break;

			if ((end + 2 + size) > body.size())
				return false;

			out.append(body, end + 2, size);
			pos = end + 2 + size + 2;
		}

		body.swap(out);
	} else if (need != string::npos) {
		if (body.size() < need) // note: connection ended early
			return false;

		body.resize(need);
	}

	if ((body.size() > max) || (mStatus < 200) || (mStatus > 299))
		return false;

	repl.swap(body);
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

	if (mTLS && !StartTLS())
		return -1;

	mGood = true;
	return 0;
}

bool cHTTPConn::StartTLS()
{
	mCTX = SSL_CTX_new(TLS_client_method());

	if (!mCTX)
		return false;

	SSL_CTX_set_verify(mCTX, SSL_VERIFY_NONE, NULL); // note: partial client, server certificate is not verified
	mSSL = SSL_new(mCTX);

	if (!mSSL || !SSL_set_fd(mSSL, (int)mSock)) // note: windows socket fits in int, openssl takes it same way
		return false;

	SSL_set_tlsext_host_name(mSSL, mHost.c_str()); // note: servers with many sites need name of site

	while (true) { // note: socket is blocking with timeouts
		const int res = SSL_connect(mSSL);

		if (res == 1)
			return true;

		if ((SSL_get_error(mSSL, res) == SSL_ERROR_SYSCALL) && SockInterrupted())
			continue;

		return false;
	}
}

int cHTTPConn::Recv(char *buf, int len)
{
	int res;

	if (mSSL) {
		do {
			res = SSL_read(mSSL, buf, len);
		} while ((res <= 0) && (SSL_get_error(mSSL, res) == SSL_ERROR_SYSCALL) && SockInterrupted());

		if (res > 0)
			return res;

		const int err = SSL_get_error(mSSL, res);
		return (((err == SSL_ERROR_ZERO_RETURN) || ((err == SSL_ERROR_SYSCALL) && !res)) ? 0 : -1); // note: many servers close without tls close notify
	}

	do { // note: socket is blocking with receive timeout, eagain means timeout expired, so retry only when interrupted by signal
		res = recv(mSock, buf, len, 0);
	} while ((res == -1) && SockInterrupted());

	return res;
}

int cHTTPConn::Send(const char *buf, size_t &len)
{
	if (!mGood)
		return -1;

	size_t tot = 0;

	while (tot < len) {
		const int res = (mSSL ? SSL_write(mSSL, buf + tot, (int)(len - tot)) : send(mSock, buf + tot, (int)(len - tot), MSG_NOSIGNAL));

		if (res <= 0) {
			if (SockInterrupted() && (!mSSL || (SSL_get_error(mSSL, res) == SSL_ERROR_SYSCALL))) // note: interrupted by signal, try again
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

	if (mSSL) { // note: no close notify, connection is closed anyway
		SSL_free(mSSL);
		mSSL = NULL;
	}

	if (mCTX) {
		SSL_CTX_free(mCTX);
		mCTX = NULL;
	}

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
