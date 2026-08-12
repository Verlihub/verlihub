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

#include <ostream>
#include <sstream>
#include "casyncsocketserver.h"
#include "clegacytransportbridge.h"
#include "cbanlist.h"

#if HAVE_ERRNO_H
	#include <errno.h>
#endif

#include "casyncconn.h"
#include "cprotocol.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include "ctime.h"
#include "stringutils.h"

#define sockoptval_t int

inline int closesocket(int s)
{
	return ::close(s);
}

#ifndef MSG_NOSIGNAL
	#define MSG_NOSIGNAL 0
#endif

using namespace std;

namespace nVerliHub {
	using namespace nUtils;
	using namespace nEnums;

	namespace nSocket {

vector<char> cAsyncConn::msBuffer(MAX_MESS_SIZE + 1);
unsigned long cAsyncConn::sSocketCounter = 0;

cAsyncConn::cAsyncConn(int desc, cAsyncSocketServer *s, tConnType ct):
	cObj("cAsyncConn"),
	mZLibFlag(false),
	mTLSVer(""),
	ok(desc > 0),
	mWritable(true),
	mxServer(s),
	mxMyFactory(NULL),
	mxProtocol(NULL),
	mpMsgParser(NULL),
	mSockDesc(desc),
	mSeparator('|'),
	mLineSize(0),
	mIP(0),
	mNumIP(0),
	mAddrPort(0),
	mServPort(0),
	mMaxBuffer(MAX_SEND_SIZE),
	mLineSizeMax(0),
	mType(ct),
	mxLine(NULL),
	meLineStatus(AC_LS_NO_LINE),
	mBufEnd(0),
	mBufReadPos(0),
	mCloseAfter(0, 0)
{
	if (mxServer)
		mMaxBuffer = TransportMaxOutputBuffer(mxServer);

	memset(&mAddrIN, 0, sizeof(struct sockaddr_in));

	if (mSockDesc) {
		struct sockaddr saddr;
		socklen_t addr_size = sizeof(saddr);

		if (getpeername(mSockDesc, &saddr, &addr_size) < 0) {
			if (Log(2))
				LogStream() << "Error getting peer name: " << mSockDesc << endl;

			CloseNow();
		} else {
			struct sockaddr_in *addr_in = (struct sockaddr_in*)&saddr;
			mIP = addr_in->sin_addr.s_addr;
			char *temp = inet_ntoa(addr_in->sin_addr);
			mAddrIP = temp;
			nTables::cBanList::Ip2Num(mAddrIP, mNumIP);

			if (mxServer && mxServer->mUseDNS)
				DNSLookup();

			mAddrPort = ntohs(addr_in->sin_port);

			if (getsockname(mSockDesc, &saddr, &addr_size) == 0) {
				addr_in = (struct sockaddr_in*)&saddr;
				temp = inet_ntoa(addr_in->sin_addr);
				mServAddr = temp;
				mServPort = ntohs(addr_in->sin_port);
			} else if (Log(2)) {
				LogStream() << "Error getting socket name" << endl;
			}
		}
	} else {
		CloseNow();
	}
}

cAsyncConn::cAsyncConn(const string &host, int port):
	cObj("cAsyncConn"),
	mZLibFlag(false),
	ok(false),
	mWritable(true),
	mxServer(NULL),
	mxMyFactory(NULL),
	mxProtocol(NULL),
	mpMsgParser(NULL),
	mSockDesc(-1),
	mSeparator('|'),
	mLineSize(0),
	mIP(0),
	mNumIP(0),
	mAddrPort(port),
	mServPort(0),
	mMaxBuffer(0),
	mLineSizeMax(0),
	mType(eCT_SERVER),
	mxLine(NULL),
	meLineStatus(AC_LS_NO_LINE),
	mBufEnd(0),
	mBufReadPos(0),
	mCloseAfter(0, 0)
{
	memset(&mAddrIN, 0, sizeof(struct sockaddr_in));
	Connect(host, port);
}

cAsyncConn::~cAsyncConn()
{
	if (mpMsgParser)
		DeleteParser(mpMsgParser);

	mpMsgParser = NULL;
	Close();
}

void cAsyncConn::Close()
{
	if (mSockDesc <= 0)
		return;

	mWritable = false;
	ok = false;

	if (mxServer)
		mxServer->OnConnClose(this);

	TEMP_FAILURE_RETRY(closesocket(mSockDesc));

	if (errno != EINTR) {
		sSocketCounter--;

		if (Log(3))
			LogStream() << "Closing socket: " << mSockDesc << endl;
	} else if (ErrLog(1)) {
		LogStream() << "Socket not closed: " << mSockDesc << endl;
	}

	mSockDesc = 0;
}

void cAsyncConn::Flush()
{
	if (!GetFlushSize() && !GetBufferSize())
		return;

	string empty;
	Write(empty, true);
}

int cAsyncConn::ReadLineLocal()
{
	if (!mxLine)
		throw "ReadLine with null line pointer";

	char *pos;
	char *buf = msBuffer.data() + mBufReadPos;
	int len = mBufEnd - mBufReadPos;

	if (NULL == (pos = (char*)memchr(buf, mSeparator, len))) {
		if ((mxLine->size() + len) > mLineSizeMax) {
			CloseNow();
			return 0;
		}

		mxLine->append(buf, len);
		mBufEnd = 0;
		mBufReadPos = 0;
		return len;
	}

	len = pos - buf;
	mxLine->append(buf, len);
	mBufReadPos += len + 1;
	meLineStatus = AC_LS_LINE_DONE;
	return len + 1;
}

void cAsyncConn::SetLineToRead(string *strp, char delim, int max)
{
	if (LineStatus() != AC_LS_NO_LINE)
		throw "cAsyncConn::SetLineToRead - precondition not ok";

	if (!strp)
		throw "cAsyncConn::SetLineToRead - precondition not ok - null string pointer";

	meLineStatus = AC_LS_PARTLY;
	mLineSize = 0;
	mLineSizeMax = max;
	mxLine = strp;
	mSeparator = delim;
}

void cAsyncConn::ClearLine()
{
	meLineStatus = AC_LS_NO_LINE;
	mLineSize = 0;
	mLineSizeMax = 0;
	mSeparator = '\n';
	mxLine = NULL;
}

string *cAsyncConn::GetLine()
{
	return mxLine;
}

void cAsyncConn::CloseNice(int msec)
{
	OnCloseNice();
	mWritable = false;

	if ((msec <= 0) || (!GetFlushSize() && !GetBufferSize())) {
		CloseNow();
		return;
	}

	if (mxServer)
		mCloseAfter = mxServer->mTime;
	else
		mCloseAfter.Get();

	mCloseAfter += msec;
}

void cAsyncConn::CloseNow()
{
	mWritable = false;
	ok = false;

	if (mxServer) {
		mxServer->mConnChooser.OptOut((cConnBase*)this, eCC_ALL);
		mxServer->mConnChooser.OptIn((cConnBase*)this, eCC_CLOSE);
	}
}

int cAsyncConn::ReadAll(const unsigned int tries, const unsigned int sleep)
{
	if (!ok || !mWritable)
		return -1;

	int buf_len = 0;
	unsigned int i = 0;
	mBufReadPos = 0;
	mBufEnd = 0;

	while (((buf_len = recv(mSockDesc, msBuffer.data(), MAX_MESS_SIZE, 0)) == -1) &&
		((errno == EAGAIN) || (errno == EINTR)) && (i++ <= tries))
		::usleep(sleep);

	if (buf_len <= 0) {
		if (buf_len == 0) {
			if (Log(2))
				LogStream() << "User hung up" << endl;
		} else if (Log(2)) {
			LogStream() << "Read IO error: " << errno << " = " << strerror(errno) << endl;
		}

		CloseNow();
		return -1;
	}

	if ((buf_len > 2) && (msBuffer[0] == 0x16) && (msBuffer[1] == 0x03)) {
		if (Log(2))
			LogStream() << "Closing TLS connection" << endl;

		CloseNow();
		return -1;
	}

	mBufEnd = buf_len;
	msBuffer[mBufEnd] = '\0';

	if (mxServer)
		mTimeLastIOAction = mxServer->mTime;
	else
		mTimeLastIOAction.Get();

	return buf_len;
}

int cAsyncConn::SendAll(const char *buf, size_t &len)
{
	size_t total = 0;
	size_t bytesleft = len;
	int n = 0;

#ifndef QUICK_SEND
	while (total < len) {
		n = send(mSockDesc, buf + total, bytesleft,
			MSG_NOSIGNAL | MSG_DONTWAIT);

		if (n == -1)
			break;

		total += n;
		bytesleft -= n;
	}
#else
	n = send(mSockDesc, buf + total, bytesleft, 0);
	total = n;
#endif

	len = total;
	return ((n == -1) ? -1 : 0);
}

int cAsyncConn::Connect(const string &host, int port)
{
	mSockDesc = CreateSock();

	if (mSockDesc == INVALID_SOCKET) {
		vhErr(1) << "Error getting socket" << endl;
		ok = false;
		return -1;
	}

	cTime timeout(5.0);
	SetSockOpt(SO_RCVTIMEO, &timeout, sizeof(timeval));
	SetSockOpt(SO_SNDTIMEO, &timeout, sizeof(timeval));

	struct hostent *he = gethostbyname(host.c_str());

	if (!he) {
		vhErr(2) << "Error resolving host " << host << endl;
		ok = false;
		return -1;
	}

	struct sockaddr_in dest_addr;
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = htons(port);
	dest_addr.sin_addr.s_addr = *(unsigned*)(he->h_addr_list[0]);
	memset(&(dest_addr.sin_zero), '\0', 8);
	int s = connect(mSockDesc, (struct sockaddr*)&dest_addr,
		sizeof(struct sockaddr));

	if (s == -1) {
		vhErr(1) << "Error connecting to " << host << ':' << port << endl;
		ok = false;
		return -1;
	}

	ok = true;
	return 0;
}

int cAsyncConn::SetSockOpt(int optname, const void *optval, int optlen)
{
	return setsockopt(mSockDesc, SOL_SOCKET, optname, optval, optlen);
}

tSocket cAsyncConn::CreateSock()
{
	tSocket sock = socket(AF_INET, SOCK_STREAM, 0);

	if (sock == INVALID_SOCKET)
		return INVALID_SOCKET;

	sockoptval_t yes = 1;

	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes,
		sizeof(sockoptval_t)) == INVALID_SOCKET) {
		closesocket(sock);
		return INVALID_SOCKET;
	}

	sSocketCounter++;

	if (Log(3))
		LogStream() << "New socket: " << sock << endl;

	return sock;
}

int cAsyncConn::BindSocket(int sock, int port, const char *ia)
{
	if (sock < 0)
		return INVALID_SOCKET;

	mAddrIN.sin_family = AF_INET;
	mAddrIN.sin_addr.s_addr = INADDR_ANY;

	if (ia && ia[0] != '\0')
		inet_aton(ia, &mAddrIN.sin_addr);

	mAddrIN.sin_port = htons(port);
	memset(&(mAddrIN.sin_zero), '\0', 8);

	if (::bind(sock, (struct sockaddr*)&mAddrIN, sizeof(mAddrIN)) == -1)
		return INVALID_SOCKET;

	return sock;
}

int cAsyncConn::ListenSock(int sock, const unsigned int blog)
{
	if (sock < 0)
		return INVALID_SOCKET;

	if (listen(sock, blog) == -1) {
		vhErr(0) << "Error listening" << endl;
		return INVALID_SOCKET;
	}

	return sock;
}

tSocket cAsyncConn::NonBlockSock(int sock)
{
	if (sock < 0)
		return INVALID_SOCKET;

	int flags = fcntl(sock, F_GETFL, 0);

	if (flags < 0)
		return INVALID_SOCKET;

	if (fcntl(sock, F_SETFL, flags | O_NONBLOCK) < 0)
		return INVALID_SOCKET;

	return sock;
}

bool cAsyncConn::ListenOnPort(int port, const char *address,
	const unsigned int blog)
{
	if (mSockDesc)
		return false;

	mSockDesc = CreateSock();

	if (mSockDesc == INVALID_SOCKET)
		return false;

	mSockDesc = BindSocket(mSockDesc, port, address);

	if (mSockDesc == INVALID_SOCKET)
		return false;

	mSockDesc = ListenSock(mSockDesc, blog);

	if (mSockDesc == INVALID_SOCKET)
		return false;

	mSockDesc = NonBlockSock(mSockDesc);

	if (mSockDesc == INVALID_SOCKET)
		return false;

	ok = mSockDesc > 0;
	return ok;
}

tSocket cAsyncConn::AcceptSock(const unsigned int sleep,
	const unsigned int tries)
{
	struct sockaddr_in client;
	socklen_t namelen = sizeof(client);
	memset(&client, 0, namelen);
	tSocket socknum = ::accept(mSockDesc, (struct sockaddr*)&client, &namelen);
	unsigned int i = 0;

	while ((socknum == INVALID_SOCKET) &&
		((errno == EAGAIN) || (errno == EINTR)) && (i++ < tries)) {
		socknum = ::accept(mSockDesc, (struct sockaddr*)&client, &namelen);
		::usleep(sleep);
	}

	if (socknum == INVALID_SOCKET)
		return INVALID_SOCKET;

	if (Log(3))
		LogStream() << "Accepted socket: " << socknum << endl;

	sSocketCounter++;
	sockoptval_t yes = 1;

	if (setsockopt(socknum, SOL_SOCKET, SO_KEEPALIVE, &yes,
		sizeof(int)) == SOCKET_ERROR) {
		TEMP_FAILURE_RETRY(closesocket(socknum));

		if (errno != EINTR) {
			sSocketCounter--;

			if (Log(3))
				LogStream() << "Closing socket: " << socknum << endl;
		} else if (ErrLog(1)) {
			LogStream() << "Socket not closed: " << socknum << endl;
		}

		return INVALID_SOCKET;
	}

	if ((socknum = NonBlockSock(socknum)) == INVALID_SOCKET)
		return INVALID_SOCKET;

	return socknum;
}

cConnFactory *cAsyncConn::GetAcceptingFactory()
{
	if (mxServer && mxServer->mFactory)
		return mxServer->mFactory;

	return NULL;
}

cAsyncConn *cAsyncConn::Accept(const unsigned int sleep, const unsigned int tries)
{
	tSocket sd = AcceptSock(sleep, tries);

	if (sd == INVALID_SOCKET)
		return NULL;

	if (mxServer)
		mTimeLastIOAction = mxServer->mTime;
	else
		mTimeLastIOAction.Get();

	cConnFactory *acceptingFactory = GetAcceptingFactory();
	cAsyncConn *newConn = NULL;

	if (acceptingFactory)
		newConn = acceptingFactory->CreateConn(sd);

	if (!newConn)
		throw "Unable to create connection";

	return newConn;
}

tConnType cAsyncConn::GetType()
{
	return mType;
}

int cAsyncConn::OnTimerBase(const cTime &now)
{
	if (bool(mCloseAfter) && (mCloseAfter < now)) {
		CloseNow();
		return 0;
	}

	Flush();
	OnTimer(now);
	return 0;
}

int cAsyncConn::OnTimer(const cTime &now)
{
	return 0;
}

int cAsyncConn::Write(const string &data, bool flush)
{
	size_t flush_size = GetFlushSize();
	size_t buf_size = GetBufferSize();
	const size_t data_size = data.size();
	size_t calc_size = flush_size + buf_size + data_size;

	if (calc_size > mMaxBuffer) {
		if (Log(2)) {
			LogStream() << "Output buffer is too big, closing: " << flush_size
				<< " + " << buf_size << " + " << data_size << " = "
				<< calc_size << " of " << mMaxBuffer << endl;
		}

		CloseNow();
		return -1;
	}

	if (data_size) {
		mBufFlush.append(data.data(), data_size);
		flush_size += data_size;
	}

	buf_size += flush_size;
	flush = flush || (buf_size > (mMaxBuffer >> 1));

	if (!buf_size || !flush)
		return 0;

	cAsyncSocketServer *serv = mxServer;

	if (!serv && Log(5))
		LogStream() << "Server not available for write operations" << endl;

	const char *send_buf = mBufFlush.data();

	if (flush_size) {
		if (mZLibFlag && serv &&
			TransportCompressionEnabled(serv, flush_size)) {
			if (send_buf[flush_size - 1] == '|') {
				string compressed;
				size_t candidate_size = 0;
				int comp_err = 0;
				const int comp = TransportCompressOutput(serv, send_buf,
					flush_size, compressed, candidate_size, comp_err);

				if (comp == 1) {
					buf_size -= flush_size;
					buf_size += compressed.size();
					mBufSend.append(compressed);
				} else {
					mBufSend.append(send_buf, flush_size);

					if (comp < 0) {
						if (candidate_size) {
							if (Log(5)) {
								LogStream() << "Compressed ZLib data is larger, fall back: "
									<< candidate_size << " vs " << flush_size << endl;
							}
						} else if (comp_err > -100) {
							if (Log(0))
								LogStream() << "Reallocation of ZLib buffer failed, fall back: "
									<< comp_err << endl;
						} else {
							LogStream() << "Failed compressing data with ZLib, fall back: "
								<< comp_err << endl;
						}
					}
				}

				mBufFlush.clear();
				ShrinkStringToFit(mBufFlush);
			} else if (Log(1)) {
				LogStream() << "Missing ending pipe in compress data: "
					<< mBufFlush << endl;
			}
		} else {
			mBufSend.append(send_buf, flush_size);
			mBufFlush.clear();
			ShrinkStringToFit(mBufFlush);
		}
	}

	send_buf = mBufSend.data();
	calc_size = buf_size;

	if (SendAll(send_buf, calc_size) == -1) {
		if (Log(6) && serv) {
			ostringstream os;
			os << '[' << AddrIP() << "] Failed sending all data, "
				<< calc_size << " of " << buf_size << ", " << errno << '='
				<< strerror(errno) << ": " << mBufSend << endl;
			TransportLogOutput(serv, os.str());
		}

		if ((errno != EAGAIN) && (errno != EINTR)) {
			if (Log(2))
				LogStream() << "Error during writing, closing: " << errno << endl;

			CloseNow();
			return -1;
		}

		if (calc_size > 0) {
			if (serv)
				mTimeLastIOAction = serv->mTime;
			else
				mTimeLastIOAction.Get();

			StrCutLeft(mBufSend, calc_size);
			buf_size -= calc_size;
		} else if (bool(mCloseAfter)) {
			CloseNow();
		}

		if (serv && ok) {
			serv->mConnChooser.OptIn(this, eCC_OUTPUT);
			const unsigned long unblock = TransportMaxUnblockSize(serv);
			const unsigned long outfill = TransportMaxOutfillSize(serv);

			if (buf_size < unblock) {
				serv->mConnChooser.OptIn(this, eCC_INPUT);

				if (Log(5)) {
					ostringstream os;
					os << "Unblocking read operation on socket: " << buf_size
						<< " of " << unblock << endl;
					TransportLogOutput(serv, os.str());
					LogStream() << "Unblocking input: " << buf_size
						<< " of " << unblock << endl;
				}
			} else if (buf_size >= outfill) {
				serv->mConnChooser.OptOut(this, eCC_INPUT);

				if (Log(5)) {
					ostringstream os;
					os << "Blocking read operation on socket: " << buf_size
						<< " of " << outfill << endl;
					TransportLogOutput(serv, os.str());
					LogStream() << "Blocking input: " << buf_size
						<< " of " << outfill << endl;
				}
			}
		}
	} else {
		mBufSend.clear();
		ShrinkStringToFit(mBufSend);

		if (bool(mCloseAfter))
			CloseNow();

		if (serv && ok) {
			serv->mConnChooser.OptOut(this, eCC_OUTPUT);

			if (Log(5))
				LogStream() << "Blocking output" << endl;
		}

		if (serv)
			mTimeLastIOAction = serv->mTime;
		else
			mTimeLastIOAction.Get();
	}

	return calc_size;
}

int cAsyncConn::OnCloseNice(void)
{
	return 0;
}

cMessageParser *cAsyncConn::CreateParser()
{
	if (mxProtocol)
		return mxProtocol->CreateParser();

	return NULL;
}

void cAsyncConn::DeleteParser(cMessageParser *oldParser)
{
	if (mxProtocol)
		mxProtocol->DeleteParser(oldParser);
	else {
		delete oldParser;
		oldParser = NULL;
	}
}

string *cAsyncConn::FactoryString()
{
	if (!mpMsgParser)
		mpMsgParser = CreateParser();

	if (!mpMsgParser)
		return NULL;

	mpMsgParser->ReInit();
	return &(mpMsgParser->GetStr());
}

bool cAsyncConn::SetSecConn(const string &addr, string &vers)
{
	if (mTLSVer.size() || vers.empty() || (vers.size() > 3))
		return false;

	unsigned long num = 0;

	if (!nTables::cBanList::Ip2Num(addr, num, false))
		return false;

	mNumIP = num;
	mAddrIP = addr;
	mIP = inet_addr(addr.c_str());

	if (mxServer && mxServer->mUseDNS &&
		(mAddrHost.empty() || (mAddrHost == "localhost"))) {
		mAddrHost.clear();
		DNSLookup();
	}

	if (vers.size() == 1) {
		if (vers[0] == 'S')
			vers = "1.0";
		else
			vers = "0.0";
	}

	mTLSVer = vers;
	return true;
}

bool cAsyncConn::SetUserIP(const string &addr)
{
	unsigned long num = 0;

	if (!nTables::cBanList::Ip2Num(addr, num, false))
		return false;

	if (mNumIP == num)
		return true;

	mNumIP = num;
	mAddrIP = addr;
	mIP = inet_addr(addr.c_str());

	if (mxServer) {
		TransportUserIPChanged(mxServer, this);

		if (mxServer->mUseDNS &&
			(mAddrHost.empty() || (mAddrHost == "localhost"))) {
			mAddrHost.clear();
			DNSLookup();
		}
	}

	return true;
}

bool cAsyncConn::DNSLookup()
{
	if (mAddrHost.size())
		return true;

	struct hostent *hp = gethostbyaddr((char*)&mIP, sizeof(mIP), AF_INET);

	if (hp)
		mAddrHost = hp->h_name;

	return hp != NULL;
}

unsigned long cAsyncConn::DNSResolveHost(const string &host)
{
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(sockaddr_in));
	struct hostent *he = gethostbyname(host.c_str());

	if (he)
		addr.sin_addr = *((struct in_addr*)he->h_addr);

	return addr.sin_addr.s_addr;
}

bool cAsyncConn::DNSResolveReverse(const string &ip, string &host)
{
	struct in_addr addr;

	if (!inet_aton(ip.c_str(), &addr))
		return false;

	struct hostent *hp = gethostbyaddr((char*)&addr, sizeof(addr), AF_INET);

	if (hp)
		host = hp->h_name;

	return hp != NULL;
}

cAsyncConn *cConnFactory::CreateConn(tSocket sd)
{
	cAsyncConn *conn = new cAsyncConn(sd);
	conn->mxMyFactory = this;
	return conn;
}

void cConnFactory::DeleteConn(cAsyncConn *&conn)
{
	if (conn) {
		conn->Close();
		delete conn;
		conn = NULL;
	}
}

	}; // namespace nSocket
}; // namespace nVerliHub
