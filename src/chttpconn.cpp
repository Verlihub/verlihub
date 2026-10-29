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
#include <cstdio>
#include <cctype>
#include <algorithm>
#include <chrono>

#ifdef _WIN32
	#include <windows.h> // note: MoveFileExA
#endif

#define MAX_DATA 524288 // 0.5 mb
#define MAX_SEND 2097152 // 2.0 mb

#ifndef MSG_NOSIGNAL
	#define MSG_NOSIGNAL 0
#endif

using namespace std;

namespace nVerliHub {
	using namespace nUtils;

	namespace nSocket {

cHTTPConn::cHTTPConn(const string &host, const int port, cSocketCancel *cancel, bool tls, unsigned int timeout):
	mGood(false),
	mWrite(true),
	mSock(0),
	mHost(host),
	mPort(port),
	mTimeout(timeout ? timeout : 5), // note: 0 would wait forever
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

	const string low("\r\n" + toLower(head)); // note: header names are not case sensitive, and first header has no line break before it

	if (low.find("\r\nhost:") == low.npos) {
		send << "Host: " << mHost;

		if (mPort != (mTLS ? 443 : 80)) // note: default port is not sent
			send << ':' << mPort;

		send << "\r\n";
	}

	if (low.find("\r\naccept:") == low.npos)
		send << "Accept: */*\r\n";

	if (low.find("\r\nuser-agent:") == low.npos)
		send << "User-Agent: Mozilla/5.0 (compatible; " << HUB_VERSION_NAME << '/' << HUB_VERSION_VERS << "; +https://github.com/verlihub/)\r\n";

	if (low.find("\r\ncontent-type:") == low.npos)
		send << "Content-Type: " << ((meth == "POST") ? "application/x-www-form-urlencoded" : "text/plain") << "\r\n";

	if (low.find("\r\ncontent-length:") == low.npos)
		send << "Content-Length: " << data.size() << "\r\n";

	if (low.find("\r\nconnection:") == low.npos) // note: server closes connection after reply, so reply ends there
		send << "Connection: close\r\n";

	send << head << "\r\n"; // note: must be double before data
	const string top(send.str());
	size_t len = top.size();

	if (Send(top.data(), len) == -1) { // note: socket is blocking with send timeout, so anything not sent now is never sent
		SetSockError(SockError(), "Sending request to " + mHost + ':' + StringFrom(mPort));
		CloseNow();
		return false;
	}

	len = data.size(); // note: data is sent separately, so post of scripts has no size limit and is not copied

	if (len && (Send(data.data(), len) == -1)) {
		SetSockError(SockError(), "Sending request to " + mHost + ':' + StringFrom(mPort));
		CloseNow();
		return false;
	}

	return true;
}

bool cHTTPConn::ParseReply(string &repl, size_t max)
{
	string head, body, err;

	if (!ReadReply(head, body, max, "", err) || (mStatus < 200) || (mStatus > 299))
		return false;

	repl.swap(body);
	return true;
}

bool cHTTPConn::ReadReply(string &head, string &body, size_t max, const string &file, string &err)
{
	head.clear();
	body.clear();
	mStatus = 0;

	if (!mGood || !mWrite) {
		SetError(eHTTP_NETWORK, "Not connected");
		err = mErrText;
		return false;
	}

	string data; // note: received bytes that are not handled yet
	char buf[16384];
	size_t need = string::npos, got = 0; // note: content length and body bytes so far
	bool have = false, chunk = false, done = false, full = false;
	int state = 0; // note: chunk decoder, 0 = size line, 1 = data, 2 = line break after data, 3 = trailer after last chunk
	size_t left = 0; // note: bytes left in current chunk
	FILE *out = NULL;

	auto put = [&](const char *ptr, size_t len) -> bool { // note: body goes to file or memory
		if (!len)
			return true;

		if (out)
			return (fwrite(ptr, 1, len, out) == len);

		if (max && ((body.size() + len) > max)) {
			full = true;
			return false;
		}

		body.append(ptr, len);
		return true;
	};

	auto unchunk = [&]() -> int { // note: chunk size in hex, then data, empty chunk ends body, 1 = ok, 0 = bad data, -1 = body not stored
		size_t pos = 0;

		while (!done) {
			if (state == 0) {
				const size_t end = data.find("\r\n", pos);

				if (end == string::npos)
					break;

				if (!isxdigit((unsigned char)data[pos]))
					return 0;

				const size_t size = strtoul(data.c_str() + pos, NULL, 16); // note: chunk extension after semicolon is ignored
				pos = end + 2;

				if (size) {
					left = size;
					state = 1;
				} else {
					state = 3;
				}

			} else if (state == 1) {
				const size_t take = std::min(left, data.size() - pos);

				if (!take)
					break;

				if (!put(data.data() + pos, take))
					return -1;

				pos += take;
				left -= take;

				if (!left)
					state = 2;

			} else if (state == 2) {
				if ((data.size() - pos) < 2)
					break;

				if (data.compare(pos, 2, "\r\n") != 0)
					return 0;

				pos += 2;
				state = 0;

			} else { // note: trailer lines until empty line
				const size_t end = data.find("\r\n", pos);

				if (end == string::npos)
					break;

				if (end == pos)
					done = true;

				pos = end + 2;
			}
		}

		data.erase(0, pos);
		return 1;
	};

	while (!done) { // note: read until whole body is here or connection is closed
		const int len = Recv(buf, sizeof(buf));

		if (len < 0) // note: error is set by receive
			break;

		if (len == 0) {
			if (have && ((!chunk && (need == string::npos)) || (chunk && (state == 3)))) // note: body ends with connection, or server closed without empty line after last chunk
				done = true;
			else
				SetError(eHTTP_BROKEN, (have ? "Connection closed before end of reply from " : "Connection closed before reply from ") + mHost + ':' + StringFrom(mPort));

			break;
		}

		data.append(buf, len);

		if (!have) {
			const size_t end = data.find("\r\n\r\n");

			if (end == string::npos)
				continue;

			const string low(toLower(data.substr(0, end + 2))); // note: status line, then headers, each with line break
			const size_t line = low.find("\r\n");
			size_t pos = low.find(' ');

			if ((low.compare(0, 5, "http/") == 0) && (pos != string::npos))
				mStatus = atoi(low.c_str() + pos + 1);

			pos = low.find("\r\ncontent-length:");

			if (pos != string::npos)
				need = strtoul(low.c_str() + pos + 17, NULL, 10);

			pos = low.find("\r\ntransfer-encoding:");
			chunk = ((pos != string::npos) && (low.find("chunked", pos) < low.find("\r\n", pos + 2)));
			head = data.substr(line + 2, end + 2 - (line + 2));
			data.erase(0, end + 4);
			have = true;

			if (file.size() && (mStatus >= 200) && (mStatus <= 299)) { // note: only real file, not error page
				out = fopen(file.c_str(), "wb");

				if (!out) {
					SetError(eHTTP_FILE, "Unable to write file: " + file);
					break;
				}
			}
		}

		if (chunk) {
			const int res = unchunk();

			if (res == 0) {
				SetError(eHTTP_BROKEN, "Bad chunked data in reply from " + mHost + ':' + StringFrom(mPort));
				break;
			}

			if (res < 0) {
				if (full)
					SetError(eHTTP_BROKEN, "Reply is too big");
				else
					SetError(eHTTP_FILE, "Unable to write file: " + file);

				break;
			}

		} else {
			size_t take = data.size();

			if ((need != string::npos) && (take > (need - got)))
				take = need - got;

			if (!put(data.data(), take)) {
				if (full)
					SetError(eHTTP_BROKEN, "Reply is too big");
				else
					SetError(eHTTP_FILE, "Unable to write file: " + file);

				break;
			}

			got += take;
			data.clear();

			if ((need != string::npos) && (got >= need))
				done = true;
		}
	}

	CloseNow(); // note: connection is not used again

	if (out) {
		if ((fclose(out) != 0) && done) {
			SetError(eHTTP_FILE, "Unable to write file: " + file);
			done = false;
		}

		if (!done) // note: incomplete file is not left behind
			remove(file.c_str());
	}

	if (!done)
		err = mErrText;

	return done;
}

static string HeadValue(const string &head, const string &name) // note: value of first header with given name in lower case, without spaces around
{
	const string low("\r\n" + toLower(head));
	const size_t pos = low.find("\r\n" + name + ':');

	if (pos == string::npos)
		return "";

	size_t beg = pos + name.size() + 3; // note: same position in head, low has two more characters before it
	const size_t end = low.find("\r\n", beg);
	beg -= 2;
	string val(head.substr(beg, ((end == string::npos) ? string::npos : (end - 2 - beg))));
	const size_t first = val.find_first_not_of(" \t");

	if (first == string::npos)
		return "";

	val.erase(0, first);
	val.erase(val.find_last_not_of(" \t") + 1);
	return val;
}

static string JoinUrl(const string &base, const string &loc) // note: location of redirect can be relative
{
	if ((toLower(loc.substr(0, 7)) == "http://") || (toLower(loc.substr(0, 8)) == "https://"))
		return loc;

	string host, path;
	int port = 0;
	bool tls = false;
	cHTTPConn::SplitUrl(base, host, port, path, tls);

	if (loc.compare(0, 2, "//") == 0) // note: same scheme, other host
		return string(tls ? "https:" : "http:") + loc;

	string root(string(tls ? "https://" : "http://") + host);

	if (port != (tls ? 443 : 80))
		root += ':' + StringFrom(port);

	if (loc.size() && (loc[0] == '/'))
		return root + loc;

	string dir(path.substr(0, path.find('?'))); // note: relative to folder of current path
	dir.erase(dir.rfind('/') + 1);
	return root + dir + loc;
}

static bool MoveOver(const string &from, const string &to) // note: rename that replaces existing file, windows rename does not
{
#ifdef _WIN32
	return (MoveFileExA(from.c_str(), to.c_str(), MOVEFILE_REPLACE_EXISTING) != 0);
#else
	return (rename(from.c_str(), to.c_str()) == 0);
#endif
}

bool cHTTPConn::SplitUrl(const string &url, string &host, int &port, string &path, bool &tls) // note: only http and https
{
	size_t pos;
	const string low(toLower(url.substr(0, 8)));

	if (low == "https://") {
		tls = true;
		port = 443;
		pos = 8;
	} else if (low.compare(0, 7, "http://") == 0) {
		tls = false;
		port = 80;
		pos = 7;
	} else {
		return false;
	}

	const size_t end = url.find_first_of("/?#", pos);
	host = url.substr(pos, ((end == string::npos) ? string::npos : (end - pos)));

	if ((end == string::npos) || (url[end] == '#'))
		path = "/";
	else if (url[end] == '?') // note: query without path
		path = '/' + url.substr(end);
	else
		path = url.substr(end);

	const size_t frag = path.find('#'); // note: fragment is never sent to server

	if (frag != string::npos)
		path.erase(frag);

	const size_t colon = host.find(':');

	if (colon != string::npos) {
		port = atoi(host.c_str() + colon + 1);
		host.erase(colon);
	}

	return (host.size() && (port > 0) && (port <= 65535));
}

bool cHTTPConn::Fetch(const string &meth, const string &url, const string &head, const string &data, unsigned int timeout, cSocketCancel *cancel, int &code, string &rhead, string &body, string &err, const string &file)
{
	string cur(url), how(meth.size() ? toUpper(meth) : "GET"), send(data), extra(head);
	code = 0;
	rhead.clear();
	body.clear();
	err.clear();

	if (extra.size() && ((extra.size() < 2) || (extra.compare(extra.size() - 2, 2, "\r\n") != 0))) // note: every header line must end with line break
		extra.append("\r\n");

	const string part(file.size() ? (file + ".part") : "");

	for (int hop = 0; ; hop++) {
		string host, path;
		int port = 0;
		bool tls = false;

		if (!SplitUrl(cur, host, port, path, tls)) { // note: first url is checked before request, so this is bad location of redirect
			code = eHTTP_BROKEN;
			err = "Invalid URL: " + cur;
			return false;
		}

		cHTTPConn conn(host, port, cancel, tls, timeout);

		if (!conn.mGood || !conn.Request(how, path, extra, send) || !conn.ReadReply(rhead, body, 0, part, err)) { // note: no limit, script decides what it downloads
			code = (conn.mError ? conn.mError : eHTTP_NETWORK);
			err = conn.mErrText;
			rhead.clear();
			body.clear();
			return false;
		}

		code = conn.mStatus;

		if (((code != 301) && (code != 302) && (code != 303) && (code != 307) && (code != 308)) || (hop >= 2)) // note: two redirects, then last reply is returned
			break;

		const string loc(HeadValue(rhead, "location"));

		if (loc.empty())
			break;

		cur = JoinUrl(cur, loc);

		if ((code != 307) && (code != 308) && (how != "GET") && (how != "HEAD")) { // note: same as browsers, post becomes get, only 307 and 308 repeat post with data
			how = "GET";
			send.clear();
		}
	}

	if (part.size() && (code >= 200) && (code <= 299) && !MoveOver(part, file)) {
		remove(part.c_str());
		code = eHTTP_FILE;
		err = "Unable to write file: " + file;
		return false;
	}

	return true;
}

bool cHTTPConn::Download(const string &url, string &data) // note: same as wget of vh script
{
	int code = 0;
	string head, err;
	return (Fetch("GET", url, "", "", 5, NULL, code, head, data, err) && (code >= 200) && (code <= 299));
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
	const string addr(host + ':' + StringFrom(port));
	mSock = Create();
	mGood = false;
	mError = 0;
	mErrText.clear();

	if (mSock == INVALID_SOCKET) {
		SetSockError(SockError(), "Socket for " + addr);
		return -1;
	}

	if (mCancel && !mCancel->Attach(mSock)) { // note: already cancelled, dont even start
		SetError(eHTTP_CANCEL, "");
		return -1;
	}

	SockSetTimeout(mSock, SO_RCVTIMEO, mTimeout * 1000); // note: time without data, not whole request, so big download works as long as data comes
	SockSetTimeout(mSock, SO_SNDTIMEO, mTimeout * 1000);
	struct addrinfo hints, *res = NULL;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET; // note: socket is ipv4
	hints.ai_socktype = SOCK_STREAM;

	if ((getaddrinfo(host.c_str(), NULL, &hints, &res) != 0) || !res) { // note: thread safe, unlike gethostbyname, connections are made on other threads
		SetError(eHTTP_RESOLVE, "Unable to resolve host: " + host);
		return -1;
	}

	struct sockaddr_in dest = *(struct sockaddr_in*)res->ai_addr;
	freeaddrinfo(res);
	dest.sin_port = htons(port);

	if (!SockNonBlock(mSock)) { // note: connect without blocking, so timeout is same on windows, where connect ignores send timeout and waits about 21 seconds
		SetSockError(SockError(), "Connection to " + addr);
		return -1;
	}

	if (connect(mSock, (struct sockaddr*)&dest, sizeof(dest)) == -1) {
		if (!SockInProgress()) {
			SetSockError(SockError(), "Connection to " + addr);
			return -1;
		}

		const std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now() + std::chrono::seconds(mTimeout);

		while (true) {
			const int wait = SockWaitConnect(mSock, 200); // note: short steps, so cancel is noticed, shutdown does not wake connecting socket on every system

			if (mCancel && mCancel->IsCancelled()) {
				SetError(eHTTP_CANCEL, "");
				return -1;
			}

			if (wait > 0)
				break;

			if ((wait < 0) && !SockInterrupted()) {
				SetSockError(SockError(), "Connection to " + addr);
				return -1;
			}

			if (std::chrono::steady_clock::now() >= end) {
				SetError(eHTTP_TIMEOUT, "Connection to " + addr + " timed out after " + StringFrom(mTimeout) + " seconds");
				return -1;
			}
		}

		const int err = SockConnectError(mSock);

		if (err) {
			SetSockError(err, "Connection to " + addr);
			return -1;
		}
	}

	if (!SockNonBlock(mSock, false)) { // note: rest is blocking with timeouts
		SetSockError(SockError(), "Connection to " + addr);
		return -1;
	}

	if (mTLS && !StartTLS()) // note: error is set there
		return -1;

	mGood = true;
	return 0;
}

void cHTTPConn::SetError(int code, const string &text)
{
	if ((code == eHTTP_CANCEL) || (mCancel && mCancel->IsCancelled())) { // note: hub stop shuts socket down, so any error after that is cancel
		mError = eHTTP_CANCEL;
		mErrText = "Request cancelled, hub is stopping";
		return;
	}

	mError = code;
	mErrText = text;
}

void cHTTPConn::SetSockError(int err, const string &what)
{
	if (SockIsTimeout(err))
		SetError(eHTTP_TIMEOUT, what + " timed out after " + StringFrom(mTimeout) + " seconds");
	else if (SockIsRefused(err))
		SetError(eHTTP_REFUSED, what + " refused");
	else
		SetError(eHTTP_NETWORK, what + " failed: " + SockErrorText(err));
}

bool cHTTPConn::StartTLS()
{
	const string addr(mHost + ':' + StringFrom(mPort));
	mCTX = SSL_CTX_new(TLS_client_method());

	if (!mCTX) {
		SetError(eHTTP_TLS, "TLS handshake with " + addr + " failed: no TLS context");
		return false;
	}

	SSL_CTX_set_verify(mCTX, SSL_VERIFY_NONE, NULL); // note: partial client, server certificate is not verified
	mSSL = SSL_new(mCTX);

	if (!mSSL || !SSL_set_fd(mSSL, (int)mSock)) { // note: windows socket fits in int, openssl takes it same way
		SetError(eHTTP_TLS, "TLS handshake with " + addr + " failed: no TLS session");
		return false;
	}

	SSL_set_tlsext_host_name(mSSL, mHost.c_str()); // note: servers with many sites need name of site

	while (true) { // note: socket is blocking with timeouts
		const int res = SSL_connect(mSSL);

		if (res == 1)
			return true;

		if (SSL_get_error(mSSL, res) == SSL_ERROR_SYSCALL) {
			if (SockInterrupted())
				continue;

			const int err = SockError();

			if (SockIsTimeout(err)) {
				SetSockError(err, "TLS handshake with " + addr);
				return false;
			}
		}

		SetError(eHTTP_TLS, "TLS handshake with " + addr + " failed");
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

		if ((err == SSL_ERROR_ZERO_RETURN) || ((err == SSL_ERROR_SYSCALL) && !res)) // note: many servers close without tls close notify
			return 0;

		if (err == SSL_ERROR_SYSCALL)
			SetSockError(SockError(), "Reading reply from " + mHost + ':' + StringFrom(mPort));
		else
			SetError(eHTTP_BROKEN, "TLS error in reply from " + mHost + ':' + StringFrom(mPort));

		return -1;
	}

	do { // note: socket is blocking with receive timeout, eagain means timeout expired, so retry only when interrupted by signal
		res = recv(mSock, buf, len, 0);
	} while ((res == -1) && SockInterrupted());

	if (res < 0)
		SetSockError(SockError(), "Reading reply from " + mHost + ':' + StringFrom(mPort));

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
