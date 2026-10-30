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
#include "casyncsocketserver.h"
#include "cserverdc.h"
#include "cbanlist.h"

#include "casyncconn.h"
#include "cprotocol.h"
#include "csocketapi.h"
#include <unistd.h>
#include <string.h>
#include "ctime.h"
#include "stringutils.h"

using namespace std;

namespace nVerliHub {
	using namespace nUtils;
	using namespace nEnums;

	namespace nSocket {

vector<char> cAsyncConn::msBuffer(MAX_MESS_SIZE + 1); // todo: use dynamic grow
unsigned long cAsyncConn::sSocketCounter = 0;
std::atomic<unsigned long long> cAsyncConn::sConnIDCounter{0};

cAsyncConn::cAsyncConn(tSocket desc, cAsyncSocketServer *s, tConnType ct): // incoming connection
	cObj("cAsyncConn"),
	mZLibFlag(false),
	mTLSVer(""),
	ok(SockValid(desc)),
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
	if (mxServer) {
		nVerliHub::cServerDC *serv = (nVerliHub::cServerDC*)mxServer;
		mMaxBuffer = serv->mC.max_outbuf_size;
	}

	memset(&mAddrIN, 0, sizeof(struct sockaddr_in));

	if (SockValid(mSockDesc)) {
		struct sockaddr saddr;
		socklen_t addr_size = sizeof(saddr);

		if (0 > getpeername(mSockDesc, &saddr, &addr_size)) {
			if (Log(2))
				LogStream() << "Error getting peer name: " << mSockDesc << endl;

			CloseNow(); // note: this uses mCloseAfter

		} else {
			struct sockaddr_in *addr_in = (struct sockaddr_in*)&saddr;
			mIP = addr_in->sin_addr.s_addr; // copy ip
			char *temp = inet_ntoa(addr_in->sin_addr);
			mAddrIP = temp; // ip address
			cBanList::Ip2Num(mAddrIP, mNumIP); // not validated
			mAddrPort = ntohs(addr_in->sin_port); // port number

			if (getsockname(mSockDesc, &saddr, &addr_size) == 0) { // get server address and port that user is connected to
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

cAsyncConn::cAsyncConn(const string &host, int port/*, bool udp*/): // outgoing connection
	cObj("cAsyncConn"),
	mZLibFlag(false),
	ok(false),
	mWritable(true),
	mxServer(NULL),
	mxMyFactory(NULL),
	mxProtocol(NULL),
	mpMsgParser(NULL),
//#if !defined _WIN32
	mSockDesc(-1),
/*
#else
	mSockDesc(0),
#endif
*/
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

	/*
	if (udp) {
		mType = eCT_SERVERUDP;
		SetupUDP(host, port);
	} else {
	*/
		Connect(host, port);
	//}
}

cAsyncConn::~cAsyncConn()
{
	if (mpMsgParser)
		this->DeleteParser(mpMsgParser);

	mpMsgParser = NULL;
	this->Close();
}

void cAsyncConn::Close()
{
	if (!SockValid(mSockDesc))
		return;

	mWritable = false;
	ok = false;


	if (mxServer)
		mxServer->OnConnClose(this);

	sSocketCounter--; // note: socket is released even when close reports error, it must not be closed again

	if (SockClose(mSockDesc) == 0) {
		if (Log(3))
			LogStream() << "Closing socket: " << mSockDesc << endl;
	} else if (ErrLog(1)) {
		const int err = SockError();
		LogStream() << "Error closing socket: " << mSockDesc << ", " << err << " = " << SockErrorText(err) << endl;
	}

	mSockDesc = 0;
}

void cAsyncConn::Flush()
{
	if (!GetFlushSize() && !GetBufferSize())
		return;

	string empty;
	Write(empty, true); // write buffers
}

int cAsyncConn::ReadLineLocal()
{
	if (!mxLine)
		throw "ReadLine with null line pointer";

	char *pos, *buf = msBuffer.data() + mBufReadPos;
	int len = mBufEnd - mBufReadPos;

	if (NULL == (pos = (char*)memchr(buf, mSeparator, len))) {
		if ((mxLine->size() + len) > mLineSizeMax) {
			CloseNow();
			return 0;
		}

		mxLine->append((char*)buf, len);
		mBufEnd = 0;
		mBufReadPos = 0;
		return len;
	}

	len = pos - buf;

	if ((mxLine->size() + len) > mLineSizeMax) { // note: same limit for line that ends in this read
		CloseNow();
		return 0;
	}

	mxLine->append((char*)buf, len);
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

string* cAsyncConn::GetLine()
{
	return mxLine;
}

void cAsyncConn::CloseNice(int msec)
{
	OnCloseNice(); // must be first

#ifdef USE_TLS
	if (mTLS && !mTLSBusy && mTLS->IsReady() && mWritable && ok) { // note: when worker has session, handshake is not done yet anyway
		Flush(); // note: waiting data must be encrypted before close notify

		if (ok) {
			string out;
			mTLS->Shutdown(out);
			mBufSend.append(out);
		}
	}
#endif
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

	if (mxServer && mxServer->mConnChooser.HasConn((cConnBase*)this)) { // note: only registered connection, closing before it is added or after it is removed must not leave entry in chooser
		mxServer->mConnChooser.OptOut((cConnBase*)this, eCC_ALL);
		mxServer->mConnChooser.OptIn((cConnBase*)this, eCC_CLOSE);
	}
}

int cAsyncConn::ReadAll(const unsigned int tries, const unsigned int sleep)
{
	if (!ok)
		return -1;

	if (!mWritable) { // note: close nice, data that user still sends is read and dropped, connection stays until its last data is sent or close time is over, unread data would make system reset connection on close
		mBufReadPos = 0;
		mBufEnd = 0;
		const int drop = recv(mSockDesc, msBuffer.data(), MAX_MESS_SIZE, 0);

		if ((drop > 0) || ((drop == -1) && SockTryAgain())) // note: buffer stays empty, so nothing is parsed
			return 1;

		CloseNow(); // note: user hung up or error, nothing more can be sent
		return -1;
	}

	int buf_len = 0; //addr_len = sizeof(struct sockaddr)
	unsigned int i = 0;
	mBufReadPos = 0;
	mBufEnd = 0;
	//bool udp = (this->GetType() == eCT_CLIENTUDP);

	//if (!udp) {
		#ifdef USE_TLS
			const size_t max_len = ((mTLS || (mStartWait & eSW_DETECT)) ? (MAX_MESS_SIZE - cTLSSession::sMaxExtra) : MAX_MESS_SIZE); // note: decrypted data must fit into read buffer
		#else
			const size_t max_len = MAX_MESS_SIZE;
		#endif

			while (((buf_len = recv(mSockDesc, msBuffer.data(), (int)max_len, 0)) == -1) && SockTryAgain() && (i++ <= tries)) {
			//#if !defined _WIN32
				::usleep(sleep);
			//#endif
			}
	/*
	} else {
		while (((buf_len = recvfrom(mSockDesc, msBuffer.data(), MAX_MESS_SIZE, 0, (struct sockaddr*)&mAddrIN, (socklen_t*)&addr_len)) == -1) && (i++ <= tries)) {
	#if !defined _WIN32
		::usleep(sleep);
	#endif
		}
	}
	*/

	if (buf_len <= 0) {
		//if (!udp) {
			if (buf_len == 0) {
				if (Log(2)) // connection hung up
					LogStream() << "User hung up" << endl;

			} else {
				if (Log(2)) {
					const int err = SockError();
					LogStream() << "Read IO error: " << err << " = " << SockErrorText(err) << endl;
				}

				/*
				switch (errno) {
					case ECONNRESET: // connection reset by peer
						break;
					case ETIMEDOUT: // connection timed out
						break;
					case EHOSTUNREACH: // no route to host
						break;
					default:
						break;
				}
				*/
			}

			CloseNow();
			return -1;
		//}

	} else { // received data
		if (mxServer)
			mTimeLastIOAction = mxServer->mTime;
		else
			mTimeLastIOAction.Get();

		OnReceived(buf_len); // note: before decrypting, so tls data is counted too

	#ifdef USE_TLS
		if ((mStartWait & eSW_DETECT) && mxServer) { // note: first data decides, client that wants tls starts handshake at once
			if ((msBuffer[0] == 0x16) && ((buf_len < 2) || (msBuffer[1] == 0x03)) && StartTLS(mxServer->mTLSContext.Get())) {
				if (Log(3))
					LogStream() << "TLS connection detected" << endl;

				mxServer->TLSDetected(this, true);
				return TLSRead(buf_len);
			}

			mxServer->TLSDetected(this, false); // note: plain client that did not wait for $Lock

			if (!ok)
				return -1;
		}

		if (mTLS)
			return TLSRead(buf_len);
	#endif

		if ((buf_len > 2) && (msBuffer[0] == 0x16) && (msBuffer[1] == 0x03)) { // detect tls connection
			if (Log(2))
				LogStream() << "Closing TLS connection" << endl;

			CloseNow(); // todo: eCR_TLS_SESS
			return -1;
		}

		mBufEnd = buf_len;
		msBuffer[mBufEnd] = '\0'; // end string
	}

	return buf_len;
}

#ifdef USE_TLS

void cAsyncConn::OnTLSReady()
{
	if (mxServer)
		mxServer->TLSReady(this);
}

bool cAsyncConn::StartTLS(ssl_ctx_st *ctx)
{
	if (mTLS || !ctx)
		return false;

	mTLS = std::make_shared<cTLSSession>(ctx);

	if (!mTLS->IsValid()) {
		mTLS.reset();
		return false;
	}

	return true;
}

int cAsyncConn::TLSRead(int len)
{
	if (mTLSBusy) { // note: worker is doing handshake with this session, data waits until it is done
		if ((mTLSQueue.size() + len) > MAX_MESS_SIZE) { // note: real handshake of client is few kb, openssl refuses certificate chain over 100 kb, more is never handshake
			if (Log(2))
				LogStream() << "TLS handshake data is too big, closing: " << (mTLSQueue.size() + len) << endl;

			CloseNow();
			return -1;
		}

		mTLSQueue.append(msBuffer.data(), len);
		mBufEnd = 0;
		return len;
	}

	if (!mTLS->IsReady() && mxServer && mxServer->PostTLS(this, mTLS, string(msBuffer.data(), len))) { // note: handshake signs with private key, do it on worker thread
		mTLSBusy = true;
		mBufEnd = 0;
		return len;
	}

	string plain, out, err; // note: on this thread, after handshake or when workers are not available
	const bool was_ready = mTLS->IsReady();
	const bool good = mTLS->Feed(msBuffer.data(), len, plain, out, err);

	if (!TLSFed(was_ready, good, plain, out, err))
		return -1;

	return len; // note: data was received even when nothing was decrypted yet
}

bool cAsyncConn::TLSFed(bool was_ready, bool good, const string &plain, const string &out, const string &err)
{
	mBufReadPos = 0;
	mBufEnd = 0;

	if (out.size() && ok) { // note: handshake, session tickets or alert, send them at once
		mBufSend.append(out);
		string empty;
		Write(empty, true);
	}

	if (!good) {
		if ((mxServer && mxServer->mTLSLog) ? Log(0) : Log(2))
			LogStream() << "TLS error: " << err << endl;

		CloseNow();
		return false;
	}

	if (mTLS->IsClosed()) { // note: same as hang up
		if (Log(2))
			LogStream() << "User closed TLS session" << endl;

		CloseNow();
		return false;
	}

	if (plain.size() > MAX_MESS_SIZE) { // note: can not happen, receive size leaves room for it
		if (Log(0))
			LogStream() << "Decrypted data does not fit into read buffer: " << plain.size() << endl;

		CloseNow();
		return false;
	}

	if (plain.size())
		memcpy(msBuffer.data(), plain.data(), plain.size());

	mBufEnd = plain.size();
	msBuffer[mBufEnd] = '\0'; // end string

	if (!was_ready && mTLS->IsReady()) {
		mTLSVer = mTLS->Version();

		if (Log(3))
			LogStream() << "TLS handshake done: " << mTLSVer << endl;

		OnTLSReady();
	}

	return true;
}

void cAsyncConn::TLSResult(bool good, const string &plain, const string &out, const string &err)
{
	mTLSBusy = false;

	if (!ok || !mTLS) // note: connection is closing
		return;

	if (!TLSFed(false, good, plain, out, err) || !ok) // note: result is posted only before handshake is done
		return;

	if (!BufferEmpty() && mxServer) // note: data that came together with end of handshake
		mxServer->ParseInput(this);

	if (!ok)
		return;

	if (mTLSHold.size()) { // note: data that was written while worker had session
		string hold;
		hold.swap(mTLSHold);

		if (!AppendSend(hold.data(), hold.size())) {
			CloseNow();
			return;
		}

		string empty;
		Write(empty, true);
	}

	TLSFeedQueue();
}

void cAsyncConn::TLSFeedQueue()
{
	if (mTLSQueue.empty() || !ok || mTLSBusy)
		return;

	string data;
	data.swap(mTLSQueue);

	if (!mTLS->IsReady() && mxServer && mxServer->PostTLS(this, mTLS, data)) { // note: handshake is still not done
		mTLSBusy = true;
		return;
	}

	string plain, out, err;
	const bool was_ready = mTLS->IsReady();
	const bool good = mTLS->Feed(data.data(), data.size(), plain, out, err);

	if (TLSFed(was_ready, good, plain, out, err) && ok && !BufferEmpty() && mxServer)
		mxServer->ParseInput(this);
}

#endif

bool cAsyncConn::AppendSend(const char *data, size_t len)
{
#ifdef USE_TLS
	if (mTLS && mTLSBusy) { // note: worker has session, encrypt when it is back
		mTLSHold.append(data, len);
		return true;
	}

	if (mTLS) {
		string out, err;
		const bool good = mTLS->Encrypt(data, len, out, err);
		mBufSend.append(out);

		if (!good) {
			if ((mxServer && mxServer->mTLSLog) ? Log(0) : Log(2))
				LogStream() << "TLS error: " << err << endl;

			return false;
		}

		return true;
	}
#endif

	mBufSend.append(data, len);
	return true;
}

int cAsyncConn::SendAll(const char *buf, size_t &len)
{
	size_t total = 0; // how many bytes weve sent
	size_t bytesleft = len; // how many we have left to send
	int n = 0; //, err = 0;
	//int repetitions = 0;
	//bool udp = (this->GetType() == eCT_SERVERUDP);

#ifndef QUICK_SEND
	while (total < len) {
		//try {
			//if (!udp) {
//#if !defined _WIN32
					n = send(mSockDesc, buf + total, (int)bytesleft, MSG_NOSIGNAL | MSG_DONTWAIT);
/*
#else
				int RetryCount = 0;

				do {
					if ((n = send(mSockDesc, buf + total, (int)bytesleft, 0)) != SOCKET_ERROR)
						break;

					if (WSAGetLastError() == WSAEWOULDBLOCK) {
						if (ErrLog(3))
							LogStream() << "cAsynConn Warning, resource unavailable, retrying" << ++RetryCount <<endl;

						::Sleep(50);
					}
				} while (WSAGetLastError() == WSAEWOULDBLOCK);
*/
/*
#endif
*/
			/*
			} else {
				n = sendto(mSockDesc, buf + total, bytesleft, 0, (struct sockaddr*)&mAddrIN, sizeof(struct sockaddr));
			}
			*/
			/*
		} catch (...) {
			if (ErrLog(2))
				LogStream() << "Exception in SendAll(buf, " << len << ") total=" << total << " left=" << bytesleft << " rep=" << repetitions << " n=" << n << endl;

			return -1;
		}

		repetitions++;
		*/

		if (n == -1)
			break;

		total += n;
		bytesleft -= n;
	}
#else
	//if (!udp)
			n = send(mSockDesc, buf + total, (int)bytesleft, 0);
	/*
	else
		n = sendto(mSockDesc, buf + total, bytesleft, 0, (struct sockaddr*)&mAddrIN, sizeof(struct sockaddr));
	*/
	total = n;
#endif

	len = total; // number of bytes actually sent
	return ((n == -1) ? -1 : 0); // -1 on failure, 0 on success
}

/*
int cAsyncConn::SetupUDP(const string &host, int port)
{
	mSockDesc = CreateSock(true);

	if(mSockDesc == INVALID_SOCKET) {
		vhErr(1) << "Error getting socket." << endl;
		ok = false;
		return -1;
	}

	struct hostent *he = gethostbyname(host.c_str());
	if(he != NULL) {
		memset(&mAddrIN, 0, sizeof(struct sockaddr_in));
		mAddrIN.sin_family = AF_INET;
		mAddrIN.sin_port = htons(port);
		mAddrIN.sin_addr = *((struct in_addr *)he->h_addr);
		memset(&(mAddrIN.sin_zero), '\0', 8);
		ok = true;
		return 0;
	} else {
		vhErr(2) << "Error resolving host " << host << endl;
		ok = false;
		return -1;
	}
}

int cAsyncConn::SendUDPMsg(const string &host, int port, const string &data)
{
	int result;
	cAsyncConn conn(host, port, true);
	if (conn.ok)
		result = conn.Write(data, true);
	else
		return -1;
	if(conn.mSockDesc != INVALID_SOCKET)
		conn.Close();
	return result;
}
*/

int cAsyncConn::Connect(const string &host, int port)
{
	mSockDesc = CreateSock();

	if (mSockDesc == INVALID_SOCKET) {
		vhErr(1) << "Error getting socket" << endl;
		ok = false;
		return -1;
	}

	SockSetTimeout(mSockDesc, SO_RCVTIMEO, 5000);
	SockSetTimeout(mSockDesc, SO_SNDTIMEO, 5000);

	struct hostent *he = gethostbyname(host.c_str());

	if (he) {
		struct sockaddr_in dest_addr;
		dest_addr.sin_family = AF_INET;
		dest_addr.sin_port = htons(port);
		dest_addr.sin_addr.s_addr = *(unsigned*)(he->h_addr_list[0]); //inet_addr(host.c_str())
		memset(&(dest_addr.sin_zero), '\0', 8);
		int s = connect(mSockDesc, (struct sockaddr*)&dest_addr, sizeof(struct sockaddr));

		if (s == -1) {
			vhErr(1) << "Error connecting to " << host << ':' << port << endl;
			ok = false;
			return -1;
		}

		ok = true;
		return 0;

	} else {
		vhErr(2) << "Error resolving host " << host << endl;
		ok = false;
		return -1;
	}
}

int cAsyncConn::SetSockOpt(int optname, const void *optval, int optlen)
{
	return SockSetOpt(this->mSockDesc, SOL_SOCKET, optname, optval, optlen);
}

/*
int cAsyncConn::GetSockOpt(int optname, void *optval, int &optlen)
{
	socklen_t _optlen; // int &optlen ?
	return getsockopt(this->mSockDesc, SOL_SOCKET, optname, optval, &_optlen);
}
*/

tSocket cAsyncConn::CreateSock(/*bool udp*/)
{
	tSocket sock;

	//if (!udp) {
		if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) // create tcp socket
			return INVALID_SOCKET;

	#ifndef _WIN32 // note: on windows it allows other process to bind same port and steal connections, and windows has no address already in use problem after restart
		const int yes = 1;

		if (SockSetOpt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == SOCKET_ERROR) { // fix the address already in use error
			SockClose(sock);
			return INVALID_SOCKET;
		}
	#endif
	/*
	} else {
		if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET) // create udp socket
			return INVALID_SOCKET;
	}
	*/

	sSocketCounter++;

	if (Log(3))
		LogStream() << "New socket: " << sock << endl;

	return sock;
}

tSocket cAsyncConn::BindSocket(tSocket sock, int port, const char *ia)
{
	if (!SockValid(sock))
		return INVALID_SOCKET;

	mAddrIN.sin_family = AF_INET;
	mAddrIN.sin_addr.s_addr = INADDR_ANY; // default listen address

	if (ia && (ia[0] != '\0'))
		inet_pton(AF_INET, ia, &mAddrIN.sin_addr); // override it

	mAddrIN.sin_port = htons(port);
	memset(&(mAddrIN.sin_zero), '\0', 8);

	if (::bind(sock, (struct sockaddr*)&mAddrIN, sizeof(mAddrIN)) == -1) // bind socket to port
		return INVALID_SOCKET;

	return sock;
}

tSocket cAsyncConn::ListenSock(tSocket sock, const unsigned int blog)
{
	if (!SockValid(sock))
		return INVALID_SOCKET;

	if (listen(sock, blog) == -1) { // note: this is backlog
		vhErr(0) << "Error listening" << endl;
		return INVALID_SOCKET;
	}

	return sock;
}

tSocket cAsyncConn::NonBlockSock(tSocket sock)
{
	if (!SockValid(sock) || !SockNonBlock(sock))
		return INVALID_SOCKET;

	return sock;
}

bool cAsyncConn::ListenOnPort(int port, const char *address, const unsigned int blog/*, bool udp*/)
{
	if (SockValid(mSockDesc))
		return false;

	const tSocket sock = CreateSock(/*udp*/);

	if (sock == INVALID_SOCKET) {
		mSockDesc = 0;
		return false;
	}

	if ((BindSocket(sock, port, address) == INVALID_SOCKET) || (ListenSock(sock, blog) == INVALID_SOCKET) || (NonBlockSock(sock) == INVALID_SOCKET)) { // note: dont leak socket on failure
		SockClose(sock);
		sSocketCounter--;
		mSockDesc = 0;
		return false;
	}

	mSockDesc = sock;
	ok = true;
	return ok;
}

tSocket cAsyncConn::AcceptSock(const unsigned int sleep, const unsigned int tries)
{
	//#if !defined _WIN32
		struct sockaddr_in client;
	/*
	#else
		struct sockaddr client;
	#endif
	*/

	socklen_t namelen = sizeof(client); // get a socket for the connected user
	memset(&client, 0, namelen);

	//#if !defined _WIN32
		tSocket socknum = ::accept(mSockDesc, (struct sockaddr*)&client, &namelen);
	/*
	#else
		tSocket socknum = accept(mSockDesc, (struct sockaddr*)&client, &namelen);
	#endif
	*/

	unsigned int i = 0;

	while ((socknum == INVALID_SOCKET) && SockTryAgain() && (i++ < tries)) {
		//#if !defined _WIN32
			socknum = ::accept(mSockDesc, (struct sockaddr*)&client, (socklen_t*)&namelen);
		/*
		#else
			socknum = accept(mSockDesc, (struct sockaddr*)&client, &namelen);
		#endif
		*/

		//#if !defined _WIN32
			::usleep(sleep);
		/*
		#else
			::Sleep(1);
		#endif
		*/
	}

	if (socknum == INVALID_SOCKET) {
		/*
		#ifdef _WIN32
			vhErr(1) << WSAGetLastError() << "  " << sizeof(fd_set) << endl;
		#endif
		*/

		return INVALID_SOCKET;
	}

	if (Log(3))
		LogStream() << "Accepted socket: " << socknum << endl;


	sSocketCounter++;
	const int yes = 1;

	if ((SockSetOpt(socknum, SOL_SOCKET, SO_KEEPALIVE, &yes, sizeof(yes)) == SOCKET_ERROR) || (NonBlockSock(socknum) == INVALID_SOCKET)) { // note: dont leak socket on failure
		SockClose(socknum);
		sSocketCounter--;

		if (Log(3))
			LogStream() << "Closing socket: " << socknum << endl;

		return INVALID_SOCKET;
	}

	return socknum;
}

cConnFactory* cAsyncConn::GetAcceptingFactory()
{
	if (mxServer && mxServer->mFactory)
		return mxServer->mFactory;

	return NULL;
}

cAsyncConn* cAsyncConn::Accept(const unsigned int sleep, const unsigned int tries)
{
	tSocket sd = AcceptSock(sleep, tries);

	if (sd == INVALID_SOCKET)
		return NULL;

	if (mxServer)
		mTimeLastIOAction = mxServer->mTime;
	else
		mTimeLastIOAction.Get();

	cConnFactory *AcceptingFactory = this->GetAcceptingFactory();
	cAsyncConn *new_conn = NULL;

	if (AcceptingFactory)
		new_conn = AcceptingFactory->CreateConn(sd);

	if (!new_conn)
		throw "Unable to create connection";

	return new_conn;
}

/*
const tConnType& cAsyncConn::getType()
{
	return mType;
}
*/

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

/*
void cAsyncConn::OnFlushDone()
{}
*/

int cAsyncConn::Write(const string &data, bool flush, bool pipe) // note: data can actually be empty when we perform a timed flush
{
	size_t flush_size = GetFlushSize(), buf_size = GetBufferSize(), data_size = (data.size() + (pipe ? 1 : 0)); // note: pipe is added after data in buffer, data itself is not changed
	size_t calc_size = flush_size + buf_size + data_size;

	if (!flush_size && !buf_size && (data_size > mMaxBuffer)) // note: one message bigger than limit while nothing is waiting is not slow user, it is allowed, more data on top of it only up to normal limit
		mBigWrite = data_size;

	if (calc_size > (mMaxBuffer + mBigWrite)) { // disconnect user who is receiving too slow and his buffer is overfilled, we cant waste memory forever
		if (Log(2))
			LogStream() << "Output buffer is too big, closing: " << flush_size << " + " << buf_size << " + " << data_size << " = " << calc_size << " of " << mMaxBuffer << endl;

		CloseNow();
		return -1;
	}

	if (data_size) { // we have something new to append
		mBufFlush.append(data.data(), data.size());

		if (pipe)
			mBufFlush.append(1, '|');

		flush_size += data_size;
	}

	const bool send_wait = (buf_size && ok && mxServer); // note: unsent data in send buffer means socket is full, output event sends the rest together with flush buffer
	buf_size += flush_size;
	flush = (flush || (!send_wait && (buf_size > (mMaxBuffer >> 1)))); // force flush when buffer is big, except while waiting for output event, send would only fail with try again

	if (!buf_size || !flush) // nothing to send or send it later
		return 0;

	nVerliHub::cServerDC *serv = NULL;

	if (mxServer)
		serv = (nVerliHub::cServerDC*)mxServer;
	else if (Log(5))
		LogStream() << "Server not available for write operations" << endl;

	const char *send_buf = mBufFlush.data(); // pointer to flush buffer

	if (flush_size) { // check if there is something to flush, else send old remaining data
		if (mZLibFlag && serv && !serv->mC.disable_zlib && (flush_size >= serv->mC.zlib_min_len)) { // compress data only when flushing or we will destroy everything, only if minimum length is reached
			if (send_buf[flush_size - 1] == '|') {
				calc_size = 0; // we dont use it anymore
				int comp_err = 0;
				const char *zlib_buf = serv->mZLib->Compress(send_buf, flush_size, calc_size, comp_err, serv->mC.zlib_compress_level);

				if (calc_size && zlib_buf) { // compression successful
					if (!AppendSend(zlib_buf, calc_size)) { // add compressed data to final send buffer
						CloseNow();
						return -1;
					}

					serv->mProtoSaved[0] += flush_size - calc_size; // add difference to saved upload statistics

				} else { // compression is larger than initial data or something failed
					if (!AppendSend(send_buf, flush_size)) { // add uncompressed data to final send buffer
						CloseNow();
						return -1;
					}

					if (calc_size) {
						if (Log(5))
							LogStream() << "Compressed ZLib data is larger, fall back: " << calc_size << " vs " << flush_size << endl;
					} else {
						if (comp_err > -100) { // note: special message
							if (Log(0)) // todo: see if this happens too often, we dont want to flood in logs eigther
								LogStream() << "Reallocation of ZLib buffer failed, fall back: " << comp_err << endl;
						} else {
							LogStream() << "Failed compressing data with ZLib, fall back: " << comp_err << endl;
						}
					}
				}

				mBufFlush.clear(); // clean up flush buffer in both cases
				ShrinkStringToFit(mBufFlush);

			} else if (Log(1)) { // client will fail to decompress when pipe is missing, note: data stays in flush buffer and is compressed on later flush when it ends with pipe
				LogStream() << "Missing ending pipe in compress data of " << flush_size << " bytes, ending: " << mBufFlush.substr((flush_size > 100) ? (flush_size - 100) : 0) << endl; // note: only tail, where pipe is missing
			}

		} else { // compression is disabled or data too short for good result
			if (!AppendSend(send_buf, flush_size)) { // add uncompressed data to final send buffer
				CloseNow();
				return -1;
			}

			mBufFlush.clear(); // clean up flush buffer
			ShrinkStringToFit(mBufFlush);
		}
	}

	send_buf = mBufSend.data() + mSendPos; // pointer to unsent part of send buffer
	buf_size = mBufSend.size() - mSendPos; // note: encrypted data is bigger, and data that was not flushed is not in send buffer at all

	/*
	if (!send_buf)
		return 0;
	*/

	calc_size = buf_size; // we dont use it anymore, make copy of send buffer size because send method will change it

	const int send_res = SendAll(send_buf, calc_size); // try to send as much data as possible
	const int send_err = ((send_res == -1) ? SockError() : 0); // note: take error before anything else can change it
	const bool send_again = ((send_res == -1) && SockTryAgain());

	if (calc_size) // note: bytes that really went to socket, after compression and encryption
		OnSent(calc_size);

	if (send_res == -1) {
		if (Log(6) && serv && serv->mNetOutLog && serv->mNetOutLog.is_open())
			serv->mNetOutLog << '[' << AddrIP() << "] Failed sending all data, sent " << calc_size << " of " << buf_size << " bytes, " << (buf_size - calc_size) << " bytes left, " << send_err << '=' << SockErrorText(send_err) << ": " << mBufSend.substr(mSendPos + calc_size, 100) << endl; // note: only start of unsent data, send position is moved below

		if (!send_again) { // analyse the error if any
			if (Log(2))
				LogStream() << "Error during writing, closing: " << send_err << endl;

			CloseNow();
			return -1;
		}

		if (calc_size > 0) { // some data was sent, update the buffer
			if (serv)
				mTimeLastIOAction = serv->mTime;
			else
				mTimeLastIOAction.Get();

			mSendPos += calc_size; // note: sent data stays in buffer, only position moves

			if (mSendPos > (mBufSend.size() >> 1)) { // more than half is sent, cut it off with a copy that really reduces memory, all cuts together copy at most size of buffer
				StrCutLeft(mBufSend, mSendPos);
				mSendPos = 0;
			}

			buf_size -= calc_size;

		} else if (bool(mCloseAfter) && (mCloseAfter < (serv ? serv->mTime : cTime()))) { // note: nothing could be sent and time of close nice is over, until then it is tried again, so kick or redirect message behind slow data still goes out
			CloseNow();
		}

		if (serv && ok) { // buffer overfill protection, only on registered connections
			serv->mConnChooser.OptIn(this, eCC_OUTPUT); // choose the connection to send the rest of data as soon as possible

			if (!InputParked() && (buf_size < serv->mC.max_unblock_size)) { // if buffer size is smaller than unblock size, allow read operation on the connection, note: input stays blocked while waiting for host name
				serv->mConnChooser.OptIn(this, eCC_INPUT);

				if (Log(5)) {
					if (serv->mNetOutLog && serv->mNetOutLog.is_open())
						serv->mNetOutLog << "Unblocking read operation on socket: " << buf_size << " of " << serv->mC.max_unblock_size << endl;

					LogStream() << "Unblocking input: " << buf_size << " of " << serv->mC.max_unblock_size << endl;
				}
			} else if (buf_size >= serv->mC.max_outfill_size) { // if buffer is bigger than maximum send size, block read operation
				serv->mConnChooser.OptOut(this, eCC_INPUT);

				if (Log(5)) {
					if (serv->mNetOutLog && serv->mNetOutLog.is_open())
						serv->mNetOutLog << "Blocking read operation on socket: " << buf_size << " of " << serv->mC.max_outfill_size << endl;

					LogStream() << "Blocking input: " << buf_size << " of " << serv->mC.max_outfill_size << endl;
				}
			}
		}
	} else { // all data was sent
		mBufSend.clear(); // clean up send buffer
		ShrinkStringToFit(mBufSend);
		mSendPos = 0;
		mBigWrite = 0; // note: limit is normal again

		if (bool(mCloseAfter)) // close nice the connection
			CloseNow();

		if (serv && ok) { // unregister connection for write operation
			serv->mConnChooser.OptOut(this, eCC_OUTPUT);

			if (Log(5))
				LogStream() << "Blocking output" << endl;

			if (!InputParked()) // note: input can be blocked by full buffer above, it is allowed again when all data is sent, chooser does nothing when it is already set
				serv->mConnChooser.OptIn(this, eCC_INPUT);
		}

		if (serv)
			mTimeLastIOAction = serv->mTime;
		else
			mTimeLastIOAction.Get();

		//OnFlushDone(); // report that flush is done
	}

	return calc_size;
}

int cAsyncConn::OnCloseNice(void)
{
	return 0;
}

cMessageParser* cAsyncConn::CreateParser()
{
	if (this->mxProtocol)
		return this->mxProtocol->CreateParser();
	else
		return NULL;
}

void cAsyncConn::DeleteParser(cMessageParser *OldParser)
{
	if (this->mxProtocol) {
		this->mxProtocol->DeleteParser(OldParser);
	} else {
		delete OldParser;
		OldParser = NULL;
	}
}

string* cAsyncConn::FactoryString()
{
	if (mpMsgParser == NULL)
		mpMsgParser = this->CreateParser();

	if (mpMsgParser == NULL)
		return NULL;

	mpMsgParser->ReInit();
	return &(mpMsgParser->GetStr());
}

bool cAsyncConn::SetSecConn(const string &addr, string &vers)
{
	if (mTLSVer.size() || vers.empty() || (vers.size() > 3))
		return false;

	unsigned long num = 0;

	if (!cBanList::Ip2Num(addr, num, false)) // validate ip
		return false;

	mNumIP = num;
	mAddrIP = addr;
	mIP = inet_addr(addr.c_str());
	mAddrHost.clear(); // note: host name is looked up when connection is started

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

	if (!cBanList::Ip2Num(addr, num, false)) // validate ip
		return false;

	if (mNumIP == num) // same ip, valid
		return true;

	mNumIP = num;
	mAddrIP = addr;
	mIP = inet_addr(addr.c_str());
	mAddrHost.clear(); // note: old host belongs to old ip

	if (mxServer) {
		nVerliHub::cServerDC *serv = (nVerliHub::cServerDC*)mxServer;

		if (serv) // send userip to operators
			serv->ShowUserIP(this);

		if (mxServer->mUseDNS && !mxServer->LookupHost(this, false)) // note: lookup on this thread when workers are not available
			DNSLookup();
	}

	return true;
}

bool cAsyncConn::DNSLookup()
{
	if (mAddrHost.size())
		return true;

	mAddrHost = cAsyncSocketServer::ReverseLookup(mAddrIP); // note: blocks, used only when workers are not available
	return mAddrHost.size();
}

unsigned long cAsyncConn::DNSResolveHost(const string &host)
{
	struct addrinfo hints, *res = NULL;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	unsigned long addr = 0;

	if ((getaddrinfo(host.c_str(), NULL, &hints, &res) == 0) && res) { // note: thread safe, unlike gethostbyname
		addr = ((struct sockaddr_in*)res->ai_addr)->sin_addr.s_addr;
		freeaddrinfo(res);
	}

	return addr;
}

bool cAsyncConn::DNSResolveReverse(const string &ip, string &host)
{
	host = cAsyncSocketServer::ReverseLookup(ip); // note: blocks
	return host.size();
}

/*
string cAsyncConn::IPAsString(unsigned long addr) // todo: pavel talked about this, use it instead of AddrToNumber?
{
	struct in_addr in;
	in.s_addr = addr;
	char ip[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, (const void*) &in, ip, INET_ADDRSTRLEN);
	return string(ip, INET_ADDRSTRLEN);
}
*/

cAsyncConn* cConnFactory::CreateConn(tSocket sd)
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
