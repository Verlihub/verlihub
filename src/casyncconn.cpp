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

#include <ostream>
#include "casyncsocketserver.h"
#include "cserverdc.h"

#if defined _WIN32
	#include <Winsock2.h>
	#define ECONNRESET WSAECONNRESET
	#define ETIMEDOUT WSAETIMEDOUT
	#define EHOSTUNREACH WSAEHOSTUNREACH
	#define socklen_t int
	#define sockoptval_t char
#endif

#if HAVE_ERRNO_H
	#include <errno.h>
#endif

#include "casyncconn.h"
#include "cprotocol.h"

#if !defined _WIN32
	#include <arpa/inet.h>
	#include <netinet/in.h> /* for sockaddr_in */
	#include <sys/socket.h> /* for AF_INET */
	#include <netdb.h> /* for gethostbyaddr */
#endif

#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include "ctime.h"
#include "stringutils.h"

#if ! defined _WIN32
	#define sockoptval_t int
	inline int closesocket(int s)
	{
		return ::close(s);
	}
#endif

#ifndef MSG_NOSIGNAL
	#define MSG_NOSIGNAL 0
#endif

using namespace std;

namespace nVerliHub {
	using namespace nUtils;
	using namespace nEnums;

	namespace nSocket {

char *cAsyncConn::msBuffer = new char[MAX_MESS_SIZE + 1];
unsigned long cAsyncConn::sSocketCounter = 0;

cAsyncConn::cAsyncConn(int desc, cAsyncSocketServer *s, tConnType ct): // connection to hub
	cObj("cAsyncConn"),
	mZLibFlag(false),
	//mIterator(0),
	ok(desc > 0),
	mWritable(true),
	mSockDesc(desc),
	mxServer(s),
	mxMyFactory(NULL),
	mxAcceptingFactory(NULL),
	mxProtocol(NULL),
	mpMsgParser(NULL),
	mAddrPort(0),
	mServPort(0),
	mType(ct)
{
	mMaxBuffer = MAX_SEND_SIZE;

	if (mxServer) {
		nVerliHub::cServerDC *serv = (nVerliHub::cServerDC*)mxServer;

		if (serv)
			mMaxBuffer = serv->mC.max_outbuf_size;
	}

	struct sockaddr saddr;
	struct sockaddr_in *addr_in;
	socklen_t addr_size = sizeof(saddr);
	mIP = 0;
	ClearLine();
	mBufEnd = mBufReadPos = 0;

	if (mSockDesc) {
		if (0 > getpeername(mSockDesc, &saddr, &addr_size)) {
			if (Log(2))
				LogStream() << "Error getting peer name, closing" << endl;

			CloseNow();
		}

		addr_in = (struct sockaddr_in*)&saddr;
		mIP = addr_in->sin_addr.s_addr; // copy ip
		mAddrIP = inet_ntoa(addr_in->sin_addr); // ip address

		if (mxServer && mxServer->mUseDNS) // host name
			DNSLookup();

		mAddrPort = ntohs(addr_in->sin_port); // port number

		if (getsockname(mSockDesc, &saddr, &addr_size) == 0) { // get server address and port that user is connected to
			addr_in = (struct sockaddr_in*)&saddr;
			mServAddr = inet_ntoa(addr_in->sin_addr);
			mServPort = ntohs(addr_in->sin_port);
		} else if (Log(2)) {
			LogStream() << "Error getting socket name" << endl;
		}
	}

	memset(&mCloseAfter, 0, sizeof(mCloseAfter));
}

// connect to given host or ip on port
cAsyncConn::cAsyncConn(const string &host, int port, bool udp):
	cObj("cAsyncConn"),
	//mIterator(0),
	ok(false),
	mWritable(true),
#if !defined _WIN32
	mSockDesc(-1),
#else
	mSockDesc(0),
#endif
	mxServer(NULL),
	mxMyFactory(NULL),
	mxAcceptingFactory(NULL),
	mxProtocol(NULL),
	mpMsgParser(NULL),
	mAddrPort(port),
	mType(eCT_SERVER),
	mBufEnd(0),
	mBufReadPos(0),
	mCloseAfter(0, 0)
{
	ClearLine();

	if (udp) {
		mType = eCT_SERVERUDP;
		SetupUDP(host, port);
	} else {
		Connect(host, port);
	}
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
	if(mSockDesc <= 0)
		return;
	mWritable = false;
	ok = false;
	if(mxServer)
		mxServer->OnConnClose(this);
	TEMP_FAILURE_RETRY(closesocket(mSockDesc));
	if(errno != EINTR) {
		sSocketCounter --;
		if (Log(3))
			LogStream() << "Closing socket " << mSockDesc << endl;
	}
	else if(ErrLog(1))
		LogStream() << "Socket not closed" << endl;
	mSockDesc = 0;
}

void cAsyncConn::Flush()
{
	string empty;

	if (GetFlushSize() || GetBufferSize()) // write buffers
		Write(empty, true);
}

int cAsyncConn::ReadLineLocal()
{
	if (!mxLine)
		throw "ReadLine with null line pointer";

	char *pos, *buf;
	buf = msBuffer + mBufReadPos;
	int len;
	len = mBufEnd - mBufReadPos;

	if (NULL == (pos = (char*)memchr(buf, mSeparator, len))) {
		if (mxLine->size() + len > mLineSizeMax) {
			CloseNow();
			return 0;
		}

		mxLine->reserve(len);
		mxLine->append((char*)buf, len);
		mBufEnd = 0;
		mBufReadPos = 0;
		return len;
	}

	len = pos - buf;
	mxLine->reserve(len);
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
	OnCloseNice();
	mWritable = false;

	if ((msec <= 0) || (!GetFlushSize() && !GetBufferSize())) {
		CloseNow();
		return;
	}

	mCloseAfter.Get();
	mCloseAfter += msec;
}

void cAsyncConn::CloseNow()
{
	mWritable = false;
	ok = false;
	if(mxServer) {
		mxServer->mConnChooser.OptOut((cConnBase*)this, eCC_ALL);
		mxServer->mConnChooser.OptIn((cConnBase*)this, eCC_CLOSE);
	}
}

int cAsyncConn::ReadAll()
{
	int buf_len = 0 , i=0, addr_len = sizeof(struct sockaddr);
	mBufReadPos = 0;
	mBufEnd = 0;
	bool udp = (this->GetType() == eCT_CLIENTUDP);

	if(!ok || !mWritable)
		return -1;

	if(!udp) {
		while(
			((buf_len = recv(mSockDesc, msBuffer, MAX_MESS_SIZE, 0)) == -1) &&
			((errno == EAGAIN) || (errno == EINTR))
			&& (i++ <= 100)
		)	{
#if ! defined _WIN32
	    ::usleep(5);
#endif
		}
	} else {
		while(
			((buf_len = recvfrom(mSockDesc, msBuffer, MAX_MESS_SIZE, 0, (struct sockaddr *)&mAddrIN, (socklen_t *)&addr_len)) == -1) &&
			(i++ <= 100)
		) {
#if ! defined _WIN32
			::usleep(5);
#endif
		}
	}

	if(buf_len <= 0) {
		if(!udp) {
			if(buf_len == 0) {
				/* Connection closed - hung up*/
				if(Log(2))
					LogStream() << "User hung up.." << endl;
				CloseNow();
				return -1;
			} else {
				if(Log(2))
					LogStream() << "Read IO Error: " << errno << " : " << strerror(errno) << endl;
				switch(errno) {
					case ECONNRESET:/* connection reset by peer */ break;
					case ETIMEDOUT:/* connection timed out */break;
					case EHOSTUNREACH:/* No route to host */ break;
					default:break;
				} // end switch
			}
			CloseNow();
			return -1;
		}
	} else {
		// Received data
		mBufEnd = buf_len;
		mBufReadPos = 0;
		// End string
		msBuffer[mBufEnd] = '\0';
		mTimeLastIOAction.Get();
	}
	return buf_len;
}

int cAsyncConn::SendAll(const char *buf, size_t &len)
{
	size_t total = 0;        /* how many bytes we've sent */
	size_t bytesleft = len; /* how many we have left to send */
	int n = 0;
	int repetitions=0;
	bool udp = (this->GetType() == eCT_SERVERUDP);

#ifndef QUICK_SEND
	while(total < len) {
		try {
			if(!udp) {
#if ! defined _WIN32
				n = send(mSockDesc, buf + total, bytesleft, MSG_NOSIGNAL|MSG_DONTWAIT);
#else
				int RetryCount = 0;
				do {
					if ((n = send(mSockDesc, buf + total, (int)bytesleft, 0)) != SOCKET_ERROR)
						break;
					if (WSAGetLastError() == WSAEWOULDBLOCK)
					{
					  if(ErrLog(3)) LogStream() << "cAsynConn Warning.. Resource unavailable...Retrying.. " << ++RetryCount <<endl;
							::Sleep(50);
					}
				}
				while (WSAGetLastError() == WSAEWOULDBLOCK);
#endif
			} else {
				n = sendto(mSockDesc, buf + total, bytesleft, 0, (struct sockaddr *)&mAddrIN, sizeof(struct sockaddr));
			}
    		} catch(...) {
			if(ErrLog(2))
				LogStream() << "exception in SendAll(buf," << len
					<< ") total=" << total
					<< " left=" << bytesleft
					<< " rep=" << repetitions
					<< " n=" << n << endl;
			return -1;
		}
		repetitions++;
		if(n == -1)
			break;
		total += n;
		bytesleft -= n;
	}
#else
	if(!udp) {
		n = send(mSockDesc, buf + total, bytesleft, 0);
	}
	else
		n = sendto(mSockDesc, buf + total, bytesleft, 0, (struct sockaddr *)&mAddrIN, sizeof(struct sockaddr));
	total = n;
#endif
	len = total; /* return number actually sent here */
	return n == -1?-1:0; /* return -1 on failure, 0 on success */
}

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

int cAsyncConn::Connect(const string &host, int port)
{
	struct sockaddr_in dest_addr;
	mSockDesc = CreateSock();

	if(mSockDesc == INVALID_SOCKET) {
		vhErr(1) << "Error getting socket." << endl;
		ok = false;
		return -1;
	}
	cTime timeout(5.0);
	SetSockOpt(SO_RCVTIMEO, &timeout, sizeof(timeval));
	SetSockOpt(SO_SNDTIMEO, &timeout, sizeof(timeval));

	struct hostent *he = gethostbyname(host.c_str());
	if(he != NULL) {
		dest_addr.sin_family = AF_INET;
		dest_addr.sin_port = htons(port);
		dest_addr.sin_addr.s_addr = *(unsigned*)(he->h_addr_list[0]);//inet_addr(host.c_str());
		memset(&(dest_addr.sin_zero), '\0', 8);

		int s = connect(mSockDesc, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr));
		if(s==-1) {
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
#ifndef _WIN32
	return setsockopt(this->mSockDesc, SOL_SOCKET, optname, optval , optlen);
#else
	return 0;
#endif
}

int cAsyncConn::GetSockOpt(int optname, void *optval, int &optlen)
{
	int result = 0;
#ifndef _WIN32
	socklen_t _optlen;
	result = getsockopt(this->mSockDesc, SOL_SOCKET, optname, optval , &_optlen);
#endif
	return result;
}

tSocket cAsyncConn::CreateSock(bool udp)
{
	tSocket sock;
	sockoptval_t yes = 1;

	if(!udp) {
		/* Create tcp socket */
		if((sock = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
			return INVALID_SOCKET;

		/* Fix the address already in use error */
		if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(sockoptval_t)) == INVALID_SOCKET) {
			closesocket(sock);
			return INVALID_SOCKET;
		}
	} else {
		/* Create udp socket */
		if((sock = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET)
			return INVALID_SOCKET;
	}

	sSocketCounter ++;
	if(Log(3))
		LogStream() << "New socket " << sock << endl;

	return sock;
}

int cAsyncConn::BindSocket(int sock, int port, const char *ia)
{
	if(sock < 0)
		return -1;
	memset(&mAddrIN, 0, sizeof(struct sockaddr_in));
	mAddrIN.sin_family = AF_INET;
	mAddrIN.sin_addr.s_addr = INADDR_ANY; // default listen address
	if(ia)
#if !defined _WIN32
		inet_aton(ia, &mAddrIN.sin_addr); // override it
#else
		mAddrIN.sin_addr.s_addr = inet_addr(ia);
#endif
	mAddrIN.sin_port = htons(port);
	memset(&(mAddrIN.sin_zero), '\0', 8);


	/* Bind socket to port */
	if(bind(sock, (struct sockaddr *)&mAddrIN, sizeof(mAddrIN)) == -1) {
		return -1;
	}
	return sock;
}

int cAsyncConn::ListenSock(int sock)
{
	if(sock < 0)
		return -1;
	if(listen(sock, 100) == -1) {
		vhErr(0) << "Error listening" << endl;
		return -1;
	}
	return sock;
}

tSocket cAsyncConn::NonBlockSock(int sock)
{
	if(sock < 0)
		return -1;
#if !defined _WIN32
	int flags;
	if((flags = fcntl(sock, F_GETFL, 0)) < 0)
		return INVALID_SOCKET;
	if( fcntl(sock, F_SETFL, flags | O_NONBLOCK) < 0 )
		return INVALID_SOCKET;
#else
	unsigned long one = 1;
	if(SOCKET_ERROR == ioctlsocket(sock, FIONBIO, &one))
		return INVALID_SOCKET;
#endif
	return sock;
}

int cAsyncConn::ListenOnPort(int port, const char *address, bool udp)
{
	if(mSockDesc)
		return -1;
	mSockDesc = CreateSock(udp);
	mSockDesc = BindSocket(mSockDesc,port,address);
	if(!udp) {
	    mSockDesc = ListenSock(mSockDesc);
	    mSockDesc = NonBlockSock(mSockDesc);
	}
	ok = mSockDesc > 0;
	return mSockDesc;
}

tSocket cAsyncConn::AcceptSock()
{
	socklen_t namelen;
	sockoptval_t yes = 1;
	int i=0;
	#if ! defined _WIN32
	struct sockaddr_in client;
	#else
	struct sockaddr client;
	#endif


	/* Get a socket for the connected user.  */
	namelen = sizeof(client);
	memset(&client, 0, namelen);

	#if ! defined _WIN32
		tSocket socknum = ::accept(mSockDesc, (struct sockaddr *)&client, &namelen);
	#else
		tSocket socknum = accept(mSockDesc, (struct sockaddr *)&client, &namelen);
	#endif

	while(( socknum == INVALID_SOCKET) && ((errno == EAGAIN) || (errno == EINTR)) && (i++ < 10)) {
		#if ! defined _WIN32
		socknum = ::accept(mSockDesc, (struct sockaddr *)&client, (socklen_t*)&namelen);
		#else
   		socknum = accept(mSockDesc, (struct sockaddr *)&client, &namelen);
		#endif

		#if ! defined _WIN32
		::usleep(50);
		#else
		::Sleep(1);
		#endif
	}

	if(socknum == INVALID_SOCKET) {
		#ifdef _WIN32
		vhErr(1) << WSAGetLastError() << "  " << sizeof(fd_set) << endl;
		#endif
		return INVALID_SOCKET;
	}
	if(Log(3))
		LogStream() << "Accepted Socket " << socknum << endl;
	sSocketCounter++;

#ifndef _WIN32
	if(setsockopt(socknum, SOL_SOCKET, SO_KEEPALIVE, &yes, sizeof(int)) == SOCKET_ERROR) {
		TEMP_FAILURE_RETRY(closesocket(socknum));
		if(errno != EINTR) {
			sSocketCounter --;
			if (Log(3))
				LogStream() << "Closing socket " << socknum << endl;
		} else if(ErrLog(1))
			LogStream() << "Socket not closed" << endl;
		return INVALID_SOCKET;
	}
#endif
	if((socknum = NonBlockSock(socknum)) == INVALID_SOCKET)
		return INVALID_SOCKET;

	return socknum;
}

cConnFactory * cAsyncConn::GetAcceptingFactory()
{
	if(this->mxAcceptingFactory)
		return mxAcceptingFactory;
	if(mxServer && mxServer->mFactory)
		return mxServer->mFactory;
	return NULL;
}

cAsyncConn * cAsyncConn::Accept()
{
	tSocket sd;
	cConnFactory *AcceptingFactory = NULL;
	cAsyncConn *new_conn = NULL;

	sd = AcceptSock();
	if(sd == INVALID_SOCKET)
		return NULL;
	mTimeLastIOAction.Get();

	AcceptingFactory = this->GetAcceptingFactory();
	if (AcceptingFactory != NULL)
		new_conn = AcceptingFactory->CreateConn(sd);
	if(!new_conn)
		throw "can't create connection";

	return new_conn;
}

const tConnType& cAsyncConn::getType()
{
	return mType;
}

tConnType cAsyncConn::GetType()
{
	return mType;
}

int cAsyncConn::OnTimerBase(cTime &now)
{

	if(bool(mCloseAfter) && (mCloseAfter > now)) {
		CloseNow();
		return 0;
	}
	Flush();
	OnTimer(now);
	return 0;
}

int cAsyncConn::OnTimer(cTime &now)
{
	return 0;
}

void cAsyncConn::OnFlushDone()
{}

int cAsyncConn::Write(const string &data, bool flush)
{
	size_t flush_size = GetFlushSize(), buf_size = GetBufferSize(), data_size = data.size();
	size_t calc_size = flush_size + buf_size + data_size;

	if (calc_size > mMaxBuffer) { // disconnect user who is receiving too slow and his buffer is overfilled, we cant waste memory forever
		if (Log(2))
			LogStream() << "Output buffer is too big, closing: " << flush_size << " + " << buf_size << " + " << data_size << " = " << calc_size << " of " << mMaxBuffer << endl;

		CloseNow();
		return -1;
	}

	if (data_size) { // we have something new to append
		mBufFlush.reserve(mBufFlush.size() + data_size);
		mBufFlush.append(data.data(), data_size);
		flush_size += data_size;
	}

	buf_size += flush_size;
	flush = (flush || (buf_size > (mMaxBuffer >> 1))); // force flush if required

	if (!buf_size || !flush) // nothing to send or send it later
		return 0;

	const char *send_buf = mBufFlush.data(); // pointer to flush buffer
	nVerliHub::cServerDC *serv = NULL;

	if (mxServer)
		serv = (nVerliHub::cServerDC*)mxServer;
	else if (Log(5))
		LogStream() << "Server not available for write operations" << endl;

	if (flush_size && send_buf) { // check if there is something to flush, else send old remaining data
		if (mZLibFlag && serv && !serv->mC.disable_zlib && (flush_size >= serv->mC.zlib_min_len)) { // compress data only when flushing or we will destroy everything, only if minimum length is reached
			if (send_buf[flush_size - 1] == '|') {
				calc_size = 0; // we dont use it anymore
				int comp_err = 0;
				const char *zlib_buf = serv->mZLib->Compress(send_buf, flush_size, calc_size, comp_err, serv->mC.zlib_compress_level);

				if (calc_size && zlib_buf) { // compression successful
					buf_size -= flush_size; // recalculate final send buffer size
					buf_size += calc_size;
					mBufSend.reserve(mBufSend.size() + calc_size);
					mBufSend.append(zlib_buf, calc_size); // add compressed data to final send buffer
					serv->mProtoSaved[0] += flush_size - calc_size; // add difference to saved upload statistics
				} else { // compression is larger than initial data or something failed
					mBufSend.reserve(mBufSend.size() + flush_size);
					mBufSend.append(send_buf, flush_size); // add uncompressed data to final send buffer

					if (Log(5)) {
						if (calc_size) {
							LogStream() << "Compressed ZLib data is larger, fall back: " << calc_size << " vs " << flush_size << endl;
						} else {
							switch (comp_err) {
								case -90:
									LogStream() << "Reallocation of ZLib buffer failed, fall back: " << comp_err << endl;
									break;

								default:
									LogStream() << "Failed compressing data with ZLib, fall back: " << comp_err << endl;
									break;
							}
						}
					}
				}

				mBufFlush.erase(0, flush_size); // clean up flush buffer in both cases
				ShrinkStringToFit(mBufFlush);
			} else if (Log(1)) { // client will fail to decompress when pipe is missing, this happens when we are flushing incomplete data, todo: not sure if wait or do something already here
				LogStream() << "Missing ending pipe in compress data: " << mBufFlush << endl; // todo: log only tail of data, dont fill logs
			}
		} else { // compression is disabled or data too short for good result
			mBufSend.reserve(mBufSend.size() + flush_size);
			mBufSend.append(send_buf, flush_size); // add uncompressed data to final send buffer
			mBufFlush.erase(0, flush_size); // clean up flush buffer
			ShrinkStringToFit(mBufFlush);
		}
	}

	send_buf = mBufSend.data(); // pointer to send buffer

	if (!send_buf)
		return 0;

	calc_size = buf_size; // we dont use it anymore, make copy of send buffer size because send method will change it

	if (SendAll(send_buf, calc_size) == -1) { // try to send as much data as possible
		if (Log(6) && serv && serv->mNetOutLog && serv->mNetOutLog.is_open())
			serv->mNetOutLog << '[' << AddrIP() << "] Failed sending all data, " << calc_size << " of " << buf_size << ", " << errno << '=' << strerror(errno) << ": " << mBufSend << endl; // todo: log only part of data, dont fill logs

		if ((errno != EAGAIN) && (errno != EINTR)) { // analyse the error if any
			if (Log(2))
				LogStream() << "Error during writing, closing: " << errno << endl;

			CloseNow();
			return -1;
		}

		if (calc_size > 0) { // some data was sent, update the buffer
			mTimeLastIOAction.Get();
			StrCutLeft(mBufSend, calc_size); // this is supposed to actually reduce the size of buffer, it does a copy so it is slower but memory usage is important
			buf_size -= calc_size;
		} else if (bool(mCloseAfter)) { // we must close nice the connection
			CloseNow();
		}

		if (mxServer && ok) { // buffer overfill protection, only on registered connections
			mxServer->mConnChooser.OptIn(this, eCC_OUTPUT); // choose the connection to send the rest of data as soon as possible

			if (buf_size < (serv ? serv->mC.max_unblock_size : MAX_SEND_UNBLOCK_SIZE)) { // if buffer size is smaller than unblock size, allow read operation on the connection
				mxServer->mConnChooser.OptIn(this, eCC_INPUT);

				if (Log(5)) {
					if (serv && serv->mNetOutLog && serv->mNetOutLog.is_open())
						serv->mNetOutLog << "Unblocking read operation on socket: " << buf_size << " of " << serv->mC.max_unblock_size << endl;

					LogStream() << "Unblocking input: " << buf_size << " of " << (serv ? serv->mC.max_unblock_size : MAX_SEND_UNBLOCK_SIZE) << endl;
				}
			} else if (buf_size >= (serv ? serv->mC.max_outfill_size : MAX_SEND_FILL_SIZE)) { // if buffer is bigger than maximum send size, block read operation
				mxServer->mConnChooser.OptOut(this, eCC_INPUT);

				if (Log(5)) {
					if (serv && serv->mNetOutLog && serv->mNetOutLog.is_open())
						serv->mNetOutLog << "Blocking read operation on socket: " << buf_size << " of " << serv->mC.max_outfill_size << endl;

					LogStream() << "Blocking input: " << buf_size << " of " << (serv ? serv->mC.max_outfill_size : MAX_SEND_FILL_SIZE) << endl;
				}
			}
		}
	} else { // all data was sent
		mBufSend.erase(0, buf_size); // clean up send buffer
		ShrinkStringToFit(mBufSend);

		if (bool(mCloseAfter)) // close nice the connection
			CloseNow();

		if (mxServer && ok) { // unregister connection for write operation
			mxServer->mConnChooser.OptOut(this, eCC_OUTPUT);

			if (Log(5))
				LogStream() << "Blocking output" << endl;
		}

		mTimeLastIOAction.Get();
		OnFlushDone(); // report that flush is done
	}

	return calc_size;
}

int cAsyncConn::OnCloseNice(void)
{
	return 0;
}

cMessageParser *cAsyncConn::CreateParser()
{
	if (this->mxProtocol != NULL)
		return this->mxProtocol->CreateParser();
	else return NULL;
}

void cAsyncConn::DeleteParser(cMessageParser *OldParser)
{
	if (this->mxProtocol != NULL)
		this->mxProtocol->DeleteParser(OldParser);
	else
		delete OldParser;
}

string * cAsyncConn::FactoryString()
{
	if(mpMsgParser == NULL)
		mpMsgParser = this->CreateParser();
	if(mpMsgParser == NULL)
		return NULL;
	mpMsgParser->ReInit();
	return &(mpMsgParser->GetStr());
}


bool cAsyncConn::DNSLookup()
{
	struct hostent *hp;
	if(mAddrHost.size())
	    	return true;
	if((hp=gethostbyaddr((char *)&mIP,sizeof(mIP),AF_INET)))
		mAddrHost = hp->h_name;
	return (hp != NULL);
}

unsigned long cAsyncConn::DNSResolveHost(const string &host)
{
	struct sockaddr_in AddrIN;
	memset(&AddrIN, 0, sizeof(sockaddr_in));
	struct hostent *he = gethostbyname(host.c_str());
	if(he != NULL) {
		AddrIN.sin_addr = *((struct in_addr *)he->h_addr);
	}
	return AddrIN.sin_addr.s_addr;
}

bool cAsyncConn::DNSResolveReverse(const string &ip, string &host)
{
	struct hostent *hp;
	struct in_addr addr;
#ifndef _WIN32
	if(!inet_aton(ip.c_str(), &addr))
		return false;
#else
	addr.s_addr = inet_addr(ip.c_str());
#endif
	if((hp = gethostbyaddr((char *)&addr,sizeof(addr),AF_INET)))
		host=hp->h_name;
	return hp != NULL;
}

/*
string cAsyncConn::IPAsString(unsigned long addr)
{
	struct in_addr in;
	in.s_addr = addr;
	char ip[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, (const void*) &in, ip, INET_ADDRSTRLEN);
	return string(ip, INET_ADDRSTRLEN);
}
*/

cAsyncConn * cConnFactory::CreateConn(tSocket sd)
{
	cAsyncConn *conn = new cAsyncConn(sd);
	conn->mxMyFactory = this;
	return conn;
}

void cConnFactory::DeleteConn(cAsyncConn * &conn)
{
	conn->Close();
	delete conn;
	conn = NULL;
}
	}; // namespace nSocket
}; // namespace nVerliHub
