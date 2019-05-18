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

#define USE_TLS_PROXY

#include "casyncsocketserver.h"
#ifdef USE_TLS_PROXY
#include "dcproxy.h"
#endif

/*
#if defined _WIN32
#  include <Winsock2.h>
#else
*/
#include <sys/socket.h>
//#endif

#include <unistd.h>
#include <stdio.h>
#include <algorithm>

using namespace std;

namespace nVerliHub {
	using namespace nUtils;
	using namespace nEnums;
	using namespace nSocket;
	namespace nSocket {

	//bool nSocket::cAsyncSocketServer::WSinitialized = false;

cAsyncSocketServer::cAsyncSocketServer(int port):
	cObj("cAsyncSocketServer"),
	mAddr("0.0.0.0"),
	mPort(port),
	timer_conn_period(4),
	timer_serv_period(1),
	mStepDelay(0),
	mNoConnDelay(0),
	mNoReadTry(0),
	mNoReadDelay(0),
	mChooseTimeOut(0),
	mAcceptNum(0),
	mAcceptTry(0),
	mMaxLineLength(0),
	mUseDNS(0),
	mFrequency(mTime, 90.0, 20),
	mbRun(false),
	mFactory(NULL),
	mRunResult(0),
	mNowTreating(NULL)
{
	/*
	#ifdef _WIN32
	if(!this->WSinitialized) {

		WORD wVersionRequested;
		WSADATA wsaData;
		int err;

		wVersionRequested = MAKEWORD(2, 2);

		err = WSAStartup(wVersionRequested, &wsaData);
		if(err != 0) {
			// Tell the user that we could not find a usable
			// WinSock DLL.
			return;
		}

		// Confirm that the WinSock DLL supports 2.2.
		// Note that if the DLL supports versions greater
		// than 2.2 in addition to 2.2, it will still return
		// 2.2 in wVersion since that is the version we
		// requested.
		if(LOBYTE( wsaData.wVersion ) != 2 ||  HIBYTE( wsaData.wVersion ) != 2) {
			// Tell the user that we could not find a usable WinSock DLL
			WSACleanup();
			return;
		}

		// The WinSock DLL is acceptable. Proceed.
		this->WSinitialized = true;
	}
	#endif
	*/
}

cAsyncSocketServer::~cAsyncSocketServer()
{
	close();
	/*
	#ifdef _WIN32
	WSACleanup();
	#endif
	*/
	vhLog(2) << "Allocated objects: " << cObj::GetCount() << endl;
	vhLog(2) << "Unclosed sockets: " << cAsyncConn::sSocketCounter << endl;
}

int cAsyncSocketServer::run()
{
	mT.stop = cTime(0, 0);
	mbRun = true;
	vhLog(1) << "Main loop start" << endl;

	while (mbRun) {
		mTime.Get(); // note: always current time, dont modify this container anywhere
		TimeStep();

		if (mTime >= (mT.main + timer_serv_period)) {
			mT.main = mTime;
			OnTimerBase(mTime);
		}

		//#if !defined _WIN32
			::usleep(mStepDelay * 1000);
		/*
		#else
			::Sleep(mStepDelay);
		#endif
		*/

		mFrequency.Insert(mTime);

		if (mT.stop.Sec() && (mTime >= mT.stop))
			mbRun = false;
	}

	vhLog(1) << "Main loop stop with code " << mRunResult << endl;

	return mRunResult;
}

void cAsyncSocketServer::stop(int code, int delay)
{
	if (delay == -1)
		mT.stop = cTime(0, 0);
	else
		mT.stop = (mTime + (int)delay);

	mRunResult = code;
}

void cAsyncSocketServer::close()
{
	mbRun = false;

	for (tCLIt it = mConnList.begin(); it != mConnList.end(); ++it) {
		if (*it) {
			mConnChooser.DelConn(*it);

			if (mFactory) {
				mFactory->DeleteConn(*it);
			} else {
				delete (*it);
				(*it) = NULL;
			}
		}
	}
}

/*
unsigned int cAsyncSocketServer::getPort() const
{
	return mPort;
}

void cAsyncSocketServer::setPort(unsigned int _newVal)
{
	mPort = _newVal;
}
*/

void cAsyncSocketServer::addConnection(cAsyncConn *new_conn)
{

	if (!new_conn)
		throw "cAsyncSocketServer::addConnection null pointer";

	if (!new_conn->ok) {
		if (new_conn->Log(3))
			new_conn->LogStream() << "Access refused: " << new_conn->AddrIP() << endl;

		new_conn->mxMyFactory->DeleteConn(new_conn);
		return;
	}

	mConnChooser.AddConn(new_conn);
	mConnChooser.cConnChoose::OptIn((cConnBase*)new_conn, tChEvent(eCC_INPUT | eCC_ERROR));
	tCLIt it = mConnList.insert(mConnList.begin(), new_conn);
	new_conn->mIterator = it;

	if (0 > OnNewConn(new_conn))
		delConnection(new_conn);
}

void cAsyncSocketServer::delConnection(cAsyncConn *old_conn)
{
	if (!old_conn)
		throw "cAsyncSocketServer::delConnection null pointer";

	if (mNowTreating == old_conn) {
		old_conn->ok = false;
		return;
	}

	bool badit = false;
	tCLIt it = old_conn->mIterator;

	/*
		check iterator before creating a pointer to it if we dont want a segmentation fault
		this sometimes happens with larger amount of users
		todo: does this leave any memory leaks?
	*/

	if (it == mConnList.end()) {
		vhErr(1) << "Invalid iterator for connection: " << old_conn << endl;
		badit = true;
		//throw "Deleting connection without iterator";
	}

	if (!badit) {
		cAsyncConn *found = (*it);

		if (found != old_conn) {
			vhErr(1) << "Connection not found: " << old_conn << endl;
			throw "Deleting non existing connection";
		}
	}

	mConnChooser.DelConn(old_conn);

	if (!badit)
		mConnList.erase(it);

	old_conn->mIterator = mConnList.end();

	if (old_conn->mxMyFactory) {
		old_conn->mxMyFactory->DeleteConn(old_conn);
	} else {
		delete old_conn;
		old_conn = NULL;
	}
}

int cAsyncSocketServer::input(cAsyncConn *conn)
{
	if (conn->ReadAll(mNoReadTry, mNoReadDelay) <= 0) // read all data available into a buffer
		return 0;

	int just_read = 0;

	while (conn->ok && conn->mWritable) {
		if (conn->LineStatus() == AC_LS_NO_LINE) // create new line if necessary
			conn->SetLineToRead(FactoryString(conn), '|', mMaxLineLength);

		just_read += conn->ReadLineLocal(); // read data into it from the buffer

		if (conn->LineStatus() == AC_LS_LINE_DONE) {
			OnNewMessage(conn, conn->GetLine());
			conn->ClearLine(); // connection may be closed after this
		}

		if (conn->BufferEmpty())
			break;
	}

	return just_read;
}

int cAsyncSocketServer::output(cAsyncConn *conn)
{
	conn->Flush();
	return 0;
}

void cAsyncSocketServer::OnNewMessage(cAsyncConn *, string *str)
{
	delete str;
	str = NULL;
}

string* cAsyncSocketServer::FactoryString(cAsyncConn *)
{
	return new string;
}

void cAsyncSocketServer::OnConnClose(cAsyncConn *conn)
{
	if (!conn)
		return;

	mConnChooser.DelConn(conn);
}

int cAsyncSocketServer::OnNewConn(cAsyncConn *conn)
{
	if (!conn)
		return -1;

	return 0;
}

int cAsyncSocketServer::OnTimerBase(const cTime &now)
{
	OnTimer(now);

	if ((mT.conn + timer_conn_period) <= now) {
		mT.conn = now;
		tCLIt it;

		for (it = mConnList.begin(); it != mConnList.end(); ++it) {
			if ((*it) && (*it)->ok)
				(*it)->OnTimerBase(now);
		}
	}

	return 0;
}

int cAsyncSocketServer::OnTimer(const cTime &now)
{
	return 0;
}

void cAsyncSocketServer::TimeStep()
{
	cTime tmout(0, ((mChooseTimeOut * 1000) + 1));

	if (!mConnChooser.Choose(tmout)) {
		//#if !defined _WIN32
			::usleep(mNoConnDelay);
		/*
		#else
			::Sleep(0);
		#endif
		*/

		return;
	}

	#if !USE_SELECT
		cConnChoose::iterator it;
	#else
		cConnSelect::iterator it;
	#endif

	cConnChoose::sChooseRes res;

	for (it = mConnChooser.begin(); it != mConnChooser.end();) {
		res = (*it);
		++it;
		mNowTreating = (cAsyncConn*)res.mConn;

		if (!mNowTreating)
			continue;

		cAsyncConn *conn = mNowTreating;
		const int activity = res.mRevent;
		bool &OK = conn->ok;

		if (OK && (activity & eCC_INPUT) && (conn->GetType() == eCT_LISTEN)) { // some connections may have been disabled during this loop so skip them
			unsigned int i = 0;
			cAsyncConn *new_conn = NULL; // accept incoming connection

			do {
				new_conn = conn->Accept(mNoConnDelay, mAcceptTry);

				if (new_conn)
					addConnection(new_conn);

				i++;
			} while (new_conn && (i <= mAcceptNum));

			/*
			#ifdef _WIN32
				vhLog(1) << "num connections" << mConnChooser.mConnList.size() << endl;
			#endif
			*/
		}

		if (OK && (activity & eCC_INPUT) && /*(*/(conn->GetType() == eCT_CLIENT)/* || (conn->GetType() == eCT_CLIENTUDP))*/) { // data to be read or data in buffer
			if (input(conn) <= 0)
				OK = false;
		}

		if (OK && (activity & eCC_OUTPUT)) // note: in sockbuf::write is a bug, missing buf increment, it will block until whole buffer is sent
			output(conn);

		mNowTreating = NULL;

		if (!OK || (activity & (eCC_ERROR | eCC_CLOSE)))
			delConnection(conn);
	}
}

cAsyncConn* cAsyncSocketServer::Listen(int OnPort/*, bool UDP*/)
{
	//if(!UDP)
		cAsyncConn *ListenSock = new cAsyncConn(0, this, eCT_LISTEN);
	/*
	else
		cAsyncConn *ListenSock = new cAsyncConn(0, this, eCT_CLIENTUDP);
	*/

	if (this->ListenWithConn(ListenSock, OnPort/*, UDP*/)) {
		return ListenSock;
	} else {
		delete ListenSock;
		ListenSock = NULL;
		return NULL;
	}
}

int cAsyncSocketServer::StartListening(int OverrideDefaultPort)
{
	if (OverrideDefaultPort && !mPort)
		mPort = OverrideDefaultPort;

	if (mPort && !OverrideDefaultPort)
		OverrideDefaultPort = mPort;

#ifdef USE_TLS_PROXY
	const int hubPort = 4112;
	int proxyPort = mPort;
	mPort = hubPort;
	OverrideDefaultPort = hubPort;

	stringstream hubAddrS;
	hubAddrS << "127.0.0.1:" << hubPort;
	string hubAddr = hubAddrS.str();

	stringstream proxyAddrsS;
	proxyAddrsS << "0.0.0.0:" << proxyPort;
	string proxyAddrs = proxyAddrsS.str();

	DCProxyConfig *proxyConf = NewDCProxyConfig();
	proxyConf->HubAddr = hubAddr.c_str();
	proxyConf->HubNetwork = "tcp4"; // TODO: unix
	proxyConf->Hosts = proxyAddrs.c_str(); // TODO: multiple addresses
	proxyConf->Wait   = 600; // ms
	proxyConf->Buffer = 10;  // KB

	LogStream() << "Starting TLS proxy " << proxyAddrs << " -> " << hubAddr << endl;

	if (!DCProxyStart(proxyConf)) {
		char *err = DCLastError();
		if (err && Log(0)) {
			LogStream() << "Error starting TLS proxy: " << err << endl;
			delete err;
		}
		delete proxyConf;
		return -1;
	}
	delete proxyConf;

	if (this->Listen(hubPort/*, false*/))
		return 0;
#else
	if (this->Listen(OverrideDefaultPort/*, false*/))
		return 0;
#endif

	return -1;
}

cAsyncConn* cAsyncSocketServer::ListenWithConn(cAsyncConn *ListenSock, int OnPort/*, bool UDP*/)
{
	if (ListenSock) {
		if (ListenSock->ListenOnPort(OnPort, mAddr.c_str(), mAcceptNum/*, UDP*/) < 0) {
			if (Log(0)) {
				LogStream() << "Cannot listen on " << mAddr << ':' << OnPort << (/*UDP ? " UDP":*/" TCP") << endl;
				LogStream() << "Please make sure the port is open and not already used by another process" << endl;
			}

			throw "Unable to listen";
			return NULL;
		}

		this->mConnChooser.AddConn(ListenSock);
		this->mConnChooser.cConnChoose::OptIn((cConnBase*)ListenSock, tChEvent(eCC_INPUT | eCC_ERROR));

		if (Log(0))
			LogStream() << "Listening for connections on " << mAddr << ':' << OnPort << (/*UDP?" UDP":*/" TCP") << endl;

		return ListenSock;
	}

	return NULL;
}

/*
bool cAsyncSocketServer::StopListenConn(cAsyncConn *connection)
{
	if (connection) {
		this->mConnChooser.DelConn(connection);
		return true;
	}

	return false;
}
*/

	}; // namespace nSocket
}; // namespace nVerliHub
