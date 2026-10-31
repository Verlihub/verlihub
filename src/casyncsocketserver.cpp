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

#include "casyncsocketserver.h"
#include "csocketapi.h"
#include "stringutils.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include <cerrno>
#include <cstring>
#include <chrono>
#include <sstream>
#include <sys/stat.h>

using namespace std;

namespace nVerliHub {
	using namespace nUtils;
	using namespace nEnums;
	using namespace nSocket;

	namespace nSocket {

	//bool nSocket::cAsyncSocketServer::WSinitialized = false;

	static constexpr size_t sWorkerMaxJobs = 10000; // note: posting fails when this many jobs are waiting
	static constexpr size_t sWorkerMaxResults = 500; // note: maximum results processed in one main loop step
	static constexpr long long sHostWaitMax = 5000; // note: milliseconds to wait for host name before connection is started without it
	static constexpr long long sTLSHandshakeMax = 10000; // note: milliseconds for tls handshake, connection is closed when it takes longer
	static constexpr unsigned int sTLSCertDays = 5 * 365; // note: validity of created certificate

	static long long SteadyMS() // note: start wait deadlines, steady clock does not jump when clock time is changed, for example by time sync
	{
		return (long long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
	}

cAsyncSocketServer::cAsyncSocketServer(string CfgBase, int port):
	cObj("cAsyncSocketServer"),
	mConfBaseDir(CfgBase),
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
	mWorkerThreads(2),
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
	/*
	#ifdef _WIN32
	WSACleanup();
	#endif
	*/

	StopWorkers(); // note: in case main loop was never run or did not stop normally

	vhLog(1) << "Allocated objects: " << cObj::GetCount() << endl;
	vhLog(1) << "Unclosed sockets: " << cAsyncConn::sSocketCounter << endl;
}

int cAsyncSocketServer::run()
{
	mT.stop = cTime(0, 0);
	mbRun = true;
	vhLog(1) << "Main loop start" << endl;

	StartWorkers();
	mTime.Get();
	mFrequency.Reset(mTime); // note: counter is made with hub, loading of database, plugins and scripts takes time, it must not count as slow main loop

	while (mbRun) {
		mTime.Get(); // note: always current time, dont modify this container anywhere
		TimeStep();

		if (mTime >= (mT.main + timer_serv_period)) {
			mT.main = mTime;
			OnTimerBase(mTime);
		}

		TimerSlices(); // note: every step, connection timer handles only connections whose part of period is due

		CheckStartWaits(); // note: every step, tls detection waits only milliseconds

		StepWait(mStepDelay);
		mFrequency.Insert(mTime);

		if (mT.stop.Sec() && (mTime >= mT.stop))
			mbRun = false;
	}

	StopWorkers(); // note: unprocessed results are dropped
	vhLog(1) << "Main loop stop with code " << mRunResult << endl;
	return mRunResult;
}

void cAsyncSocketServer::StepWait(unsigned int msec)
{
	/*
		sleep between main loop steps, but wake up at once when workers have results
		note: we dont wait in connection poll instead, poll goes through all sockets, so step delay is what keeps number of those calls low
		note: after results are processed we continue sleeping until step delay is over, so connections are still handled in batches
	*/

	if (!mWakeup.IsOpen()) {
		::usleep(msec * 1000);
		return;
	}

	const auto until = chrono::steady_clock::now() + chrono::milliseconds(msec);

	while (true) {
		const auto left = chrono::duration_cast<chrono::milliseconds>(until - chrono::steady_clock::now()).count();

		if (left <= 0)
			break;

		if (mWakeup.IsBroken()) { // note: broken socket gives events all the time, so sleep rest of step and handle results once
			if (mWakeup.BrokenNews())
				vhLog(0) << "Wakeup socket is broken, worker results are handled once per main loop step" << endl;

			::usleep(left * 1000);
			mTime.Get();
			RunPoolResults();
			break;
		}

		struct pollfd pfd;
		pfd.fd = (tSocket)mWakeup;
		pfd.events = POLLIN;
		pfd.revents = 0;
		const int res = SockPoll(&pfd, 1, (int)left);

		if (res < 0) {
			if (SockInterrupted()) // note: signal, continue sleeping
				continue;

			::usleep(left * 1000); // note: should not happen, sleep like before
			break;
		}

		if (res > 0) {
			if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL))
				mWakeup.SetBroken();

			mTime.Get(); // note: results should see current time, not time when step started
			RunPoolResults(); // note: clear also finds broken socket, then next round sleeps
		}
	}
}

void cAsyncSocketServer::RunPoolResults()
{
	mWakeup.Clear();
	bool more = false;

	for (size_t i = 0; i < mPools.size(); i++) { // note: result is allowed to start new pool, dont use iterator
		mPools[i]->RunResults(sWorkerMaxResults);

		if (mPools[i]->ResultsQueued())
			more = true;
	}

	if (more) // note: too many results, continue in next step
		mWakeup.Signal();
}

bool cAsyncSocketServer::StartWorkers()
{
	if (!mWakeup.IsOpen()) {
		if (!mWakeup.Open()) {
			const int err = SockError();
			vhLog(0) << "Failed to create wakeup pipe, worker threads are disabled: " << err << " = " << SockErrorText(err) << endl;
			return false;
		}

		mConnChooser.AddConn(&mWakeup);
		mConnChooser.cConnChoose::OptIn(&mWakeup, tChEvent(eCC_INPUT | eCC_ERROR));
	}

	if (!StartPool(mWorkers, mWorkerThreads, sWorkerMaxJobs)) {
		vhLog(0) << "Failed to start worker threads, they are disabled" << endl;
		return false;
	}

	vhLog(1) << "Started worker threads: " << mWorkers.Threads() << endl;
	return true;
}

bool cAsyncSocketServer::StartPool(nThread::cWorkerPool &pool, unsigned int threads, size_t max_jobs)
{
	if (pool.IsRunning())
		return true;

	if (!mWakeup.IsOpen()) // note: main loop is not running
		return false;

	pool.SetNotify([this] { mWakeup.Signal(); });

	if (!pool.Start(threads, max_jobs))
		return false;

	if (find(mPools.begin(), mPools.end(), &pool) == mPools.end())
		mPools.push_back(&pool);

	return true;
}

void cAsyncSocketServer::StopWorkers()
{
	for (auto pool: mPools) // note: first ask all pools to stop and abort their running work, so they finish at same time
		pool->RequestStop();

	for (auto pool: mPools) // note: then wait for threads, nothing can signal wakeup after this
		pool->Stop();

	mPools.clear();

	if (mWakeup.IsOpen()) {
		mConnChooser.DelConn(&mWakeup);
		mWakeup.Close();
	}
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
	tCLIt it;

	for (it = mConnList.begin(); it != mConnList.end(); ++it) { // client connections
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

	for (it = mListList.begin(); it != mListList.end(); ++it) { // listening connections
		if (*it) {
			mConnChooser.DelConn(*it);
			delete (*it);
			(*it) = NULL;
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

	new_conn->mTimerSlot = mTimerSlotNext;
	mTimerSlotNext = ((mTimerSlotNext + 1) % sTimerSlices);
	mConnChooser.AddConn(new_conn);
	mConnChooser.cConnChoose::OptIn((cConnBase*)new_conn, tChEvent(eCC_INPUT | eCC_ERROR));
	tCLIt it = mConnList.insert(mConnList.begin(), new_conn);
	new_conn->mIterator = it;

	if (ExternalTLS() && (new_conn->AddrIP() == mTLSAddr)) // external tls proxy, wait for myip command
		return;

	if (0 > StartConn(new_conn))
		delConnection(new_conn);
}

int cAsyncSocketServer::StartConn(cAsyncConn *conn)
{
	// note: connection is started when host name is known and tls detection is done

#ifdef USE_TLS
	if (NativeTLS() && conn->mTLSVer.empty()) { // note: client that wants tls sends data at once, plain client waits for $Lock
		conn->mStartWait |= cAsyncConn::eSW_DETECT;
		AddStartWait(mTLSDetectList, conn, SteadyMS() + mTLSWait);
	}
#endif

	if (mUseDNS && conn->AddrHost().empty() && !LookupHost(conn, true))
		conn->DNSLookup(); // note: workers are not available, do it on this thread like before

	if (conn->mStartWait) {
		UpdateStartInput(conn);
		return 0;
	}

	return OnNewConn(conn);
}

bool cAsyncSocketServer::LookupHost(cAsyncConn *conn, bool park)
{
	if (!conn || !conn->ok)
		return false;

	const tSocket sock = (tSocket)(*conn);
	const unsigned long long id = conn->mConnID;
	const string addr = conn->AddrIP();

	const bool posted = mWorkers.Post([this, sock, id, addr]() -> nThread::cWorkerPool::tResult { // note: runs on worker thread
		const string host = ReverseLookup(addr);
		return [this, sock, id, addr, host] { OnHostResult(sock, id, addr, host); }; // note: runs on main thread
	});

	if (!posted)
		return false;

	if (park) {
		conn->mStartWait |= cAsyncConn::eSW_HOST;
		AddStartWait(mHostWaitList, conn, SteadyMS() + sHostWaitMax);
		UpdateStartInput(conn);
	}

	return true;
}

string cAsyncSocketServer::ReverseLookup(const string &addr)
{
	string host;
	struct sockaddr_in sa;
	memset(&sa, 0, sizeof(sa));
	sa.sin_family = AF_INET;

	if (inet_pton(AF_INET, addr.c_str(), &sa.sin_addr) == 1) {
		char name[NI_MAXHOST];

		if (getnameinfo((struct sockaddr*)&sa, sizeof(sa), name, sizeof(name), NULL, 0, NI_NAMEREQD) == 0) // note: thread safe, unlike gethostbyaddr
			host = name;
	}

	return host;
}

bool cAsyncSocketServer::LookupHosts(const vector<string> &addrs, tHostsDone done)
{
	if (addrs.empty() || !done)
		return false;

	return mWorkers.Post([addrs, done]() -> nThread::cWorkerPool::tResult { // note: runs on worker thread
		vector<string> hosts;
		hosts.reserve(addrs.size());

		for (const auto &addr: addrs)
			hosts.push_back(ReverseLookup(addr));

		return [done, hosts] { done(hosts); }; // note: runs on main thread
	});
}

cAsyncConn* cAsyncSocketServer::FindConn(tSocket sock, unsigned long long id)
{
	cConnBase *base = mConnChooser[sock];

	if (!base || (base == &mWakeup))
		return NULL;

	cAsyncConn *conn = (cAsyncConn*)base; // note: everything else in chooser is connection
	return ((conn->mConnID == id) ? conn : NULL);
}

void cAsyncSocketServer::OnHostResult(tSocket sock, unsigned long long id, const string &addr, const string &host)
{
	cAsyncConn *conn = FindConn(sock, id);

	if (!conn || (conn->AddrIP() != addr)) // note: connection is gone or ip was changed meanwhile, then newer lookup is on its way
		return;

	if (host.size())
		conn->SetAddrHost(host);

	if (conn->mStartWait & cAsyncConn::eSW_HOST)
		StartWaitDone(conn, cAsyncConn::eSW_HOST);
}

void cAsyncSocketServer::StartWaitDone(cAsyncConn *conn, unsigned int flag)
{
	conn->mStartWait &= ~flag;

	if (!conn->ok) // note: connection is closing, it will be deleted in main loop
		return;

	UpdateStartInput(conn);

	if (conn->mStartWait) // note: still waiting for something else
		return;

	if (0 > OnNewConn(conn)) {
		conn->CloseNow(); // note: dont delete here, we might be in the middle of connection loop
		return;
	}

	if (conn->HasHeldInput()) { // note: data that came before start, also data of current read is put behind it, so order stays
		conn->HoldInput();

		while (conn->ok && !conn->mStartWait && conn->TakeHeldInput())
			ParseInput(conn);
	}
}

void cAsyncSocketServer::UpdateStartInput(cAsyncConn *conn)
{
	if (conn->InputParked())
		mConnChooser.cConnChoose::OptOut((cConnBase*)conn, eCC_INPUT);
	else
		mConnChooser.cConnChoose::OptIn((cConnBase*)conn, eCC_INPUT);
}

void cAsyncSocketServer::AddStartWait(deque<sStartWait> &list, cAsyncConn *conn, long long until)
{
	list.push_back({(tSocket)(*conn), conn->mConnID, until}); // note: same wait time for everyone in list, so it stays in order of time
}

void cAsyncSocketServer::CheckStartWait(deque<sStartWait> &list, unsigned int flag, long long now, const function<void(cAsyncConn*)> &expired)
{
	while (list.size()) {
		const sStartWait item = list.front();
		cAsyncConn *conn = FindConn(item.mSock, item.mID);
		const bool waiting = (conn && (conn->mStartWait & flag));

		if (waiting && (item.mUntil > now)) // note: rest of list is not expired either
			break;

		list.pop_front(); // note: expired, or connection is done waiting or gone

		if (waiting)
			expired(conn);
	}
}

void cAsyncSocketServer::CheckStartWaits()
{
	const long long now = SteadyMS();

	CheckStartWait(mHostWaitList, cAsyncConn::eSW_HOST, now, [this](cAsyncConn *conn) { // note: start without host name, late result will still set it
		if (conn->Log(2))
			conn->LogStream() << "Host name lookup timed out" << endl;

		StartWaitDone(conn, cAsyncConn::eSW_HOST);
	});

#ifdef USE_TLS
	CheckStartWait(mTLSDetectList, cAsyncConn::eSW_DETECT, now, [this](cAsyncConn *conn) { // note: client did not start tls, it waits for $Lock
		TLSDetected(conn, false);
	});

	CheckStartWait(mTLSHandshakeList, cAsyncConn::eSW_HANDSHAKE, now, [](cAsyncConn *conn) {
		if (conn->Log(2))
			conn->LogStream() << "TLS handshake timed out" << endl;

		conn->CloseNow();
	});
#endif
}

#ifdef USE_TLS

void cAsyncSocketServer::TLSDetected(cAsyncConn *conn, bool tls)
{
	if (tls) {
		conn->mStartWait = ((conn->mStartWait & ~cAsyncConn::eSW_DETECT) | cAsyncConn::eSW_HANDSHAKE);
		AddStartWait(mTLSHandshakeList, conn, SteadyMS() + sTLSHandshakeMax);
		UpdateStartInput(conn);
		return;
	}

	conn->mTLSVer = "0.0"; // note: same as external proxy reports for plain connection
	StartWaitDone(conn, cAsyncConn::eSW_DETECT);
}

void cAsyncSocketServer::TLSReady(cAsyncConn *conn)
{
	if (conn->mStartWait & cAsyncConn::eSW_HANDSHAKE)
		StartWaitDone(conn, cAsyncConn::eSW_HANDSHAKE);
}

bool cAsyncSocketServer::PostTLS(cAsyncConn *conn, const shared_ptr<nSocket::cTLSSession> &session, const string &data)
{
	if (!StartPool(mTLSWorkers, 0, sWorkerMaxJobs)) // note: all cpu cores, handshake is cpu work
		return false;

	const tSocket sock = (tSocket)(*conn);
	const unsigned long long id = conn->mConnID;

	return mTLSWorkers.Post([this, session, sock, id, data]() -> nThread::cWorkerPool::tResult { // note: runs on worker thread, main thread does not touch session meanwhile
		string plain, out, err;
		const bool good = session->Feed(data.data(), data.size(), plain, out, err);

		return [this, sock, id, good, plain, out, err] { // note: runs on main thread
			cAsyncConn *found = FindConn(sock, id);

			if (found) // note: else connection is gone, session is freed together with job
				found->TLSResult(good, plain, out, err);
		};
	});
}

#endif

void cAsyncSocketServer::delConnection(cAsyncConn *old_conn)
{
	if (!old_conn)
		throw "cAsyncSocketServer::delConnection null pointer";

	if (old_conn->GetType() == eCT_LISTEN) { // note: listening socket is not in connection list, its iterator is not set, and hub must keep accepting users
		vhErr(1) << "Error on listening socket, it stays open: " << old_conn << endl;
		return;
	}

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

	old_conn->ok = false; // note: plugins still get close below, what they send to this connection must not register its socket in chooser again
	old_conn->mWritable = false;
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

	if (conn->BufferEmpty()) // note: tls data was received, but nothing was decrypted yet, for example during handshake
		return 1;

	const int res = ParseInput(conn);

	if ((res <= 0) && conn->HasHeldInput()) // note: data is kept for later, see StartWaitDone(), main loop deletes connection when nothing was read
		return 1;

	return res;
}

int cAsyncSocketServer::ParseInput(cAsyncConn *conn)
{
	cAsyncConn *was = mNowTreating;
	mNowTreating = conn; // note: delConnection only marks connection, so it is not deleted while we are in it
	int just_read = 0;

	while (conn->ok && conn->mWritable) {
		if (conn->mStartWait) { // note: connection is not started yet, for example host name is looked up or $MyIP of external proxy started it again, data is parsed after OnNewConn, see StartWaitDone()
			conn->HoldInput();
			break;
		}

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

	mNowTreating = was;

	if ((was != conn) && !conn->ok) // note: called outside of connection loop, for example with data from worker, make sure loop deletes it
		conn->CloseNow();

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
	return 0;
}

bool cAsyncSocketServer::DueSlices(long long now, int period, long long &last, unsigned int &from, unsigned int &count)
{
	const long long slice = ((((period > 1) ? period : 1) * 1000LL) / sTimerSlices); // note: period is seconds, at least one
	const long long cur = (now / slice);

	if ((last < 0) || (cur < last)) { // note: first step, or period was changed to longer one, start with next part
		last = cur;
		return false;
	}

	if (cur == last)
		return false;

	from = (unsigned int)((last + 1) % sTimerSlices);
	count = (((cur - last) >= sTimerSlices) ? sTimerSlices : (unsigned int)(cur - last)); // note: slow step can make several parts due, at most whole period
	last = cur;
	return true;
}

void cAsyncSocketServer::TimerSlices()
{
	const long long now = SteadyMS();
	unsigned int from = 0, count = 0;

	if (DueSlices(now, timer_conn_period, mConnSliceLast, from, count)) { // connection timer
		for (tCLIt it = mConnList.begin(); it != mConnList.end(); ++it) {
			if ((*it) && (*it)->ok && InTimerSlice((*it)->mTimerSlot, from, count))
				(*it)->OnTimerBase(mTime);
		}
	}
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

	#if USE_EPOLL
		cConnEpoll::iterator it;
	#elif !USE_SELECT
		cConnChoose::iterator it;
	#else
		cConnSelect::iterator it;
	#endif

	cConnChoose::sChooseRes res;

	for (it = mConnChooser.begin(); it != mConnChooser.end();) {
		res = (*it);
		++it;

		if (res.mConn == &mWakeup) { // note: not a real connection, workers have results for us
			RunPoolResults();
			continue;
		}

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

bool cAsyncSocketServer::Listen(int OnPort/*, bool UDP*/)
{
	//if (!UDP)
		cAsyncConn *ListenSock = new cAsyncConn(0, this, eCT_LISTEN);
	/*
	else
		cAsyncConn *ListenSock = new cAsyncConn(0, this, eCT_CLIENTUDP);
	*/

	if (this->ListenWithConn(ListenSock, OnPort/*, UDP*/))
		return true;

	delete ListenSock;
	ListenSock = NULL;
	return false;
}

bool cAsyncSocketServer::StartListening(int OverrideDefaultPort)
{
	if (OverrideDefaultPort && !mPort)
		mPort = OverrideDefaultPort;

	if (mPort && !OverrideDefaultPort)
		OverrideDefaultPort = mPort;

#ifdef USE_TLS
	CheckTLS(true); // note: hub also works without tls
#endif

	return this->Listen(OverrideDefaultPort/*, false*/);
}

#ifdef USE_TLS

static time_t FileTime(const string &path)
{
	struct stat st;
	return ((stat(path.c_str(), &st) == 0) ? st.st_mtime : 0);
}

bool cAsyncSocketServer::CheckTLS(bool force, string *report)
{
	ostringstream res;

	auto done = [&](bool good, int level) {
		if (res.str().size() && Log(level))
			LogStream() << res.str() << endl;

		if (report)
			*report = res.str();

		return good;
	};

	if (!mTLSEnable) {
		if (mTLSContext.IsLoaded() || force) {
			mTLSContext.Clear();
			mTLSCertTime = 0;
			mTLSKeyTime = 0;
			res << "TLS is disabled with tls_enable";
		}

		return done(false, 1);
	}

	string cert(mTLSCert), key(mTLSKey), info, err;

	if (cert.size() && !HasPathSeparator(cert)) // note: file name only, look in config directory
		cert = mConfBaseDir + "/" + cert;

	if (key.size() && !HasPathSeparator(key))
		key = mConfBaseDir + "/" + key;

	cTLSContext::tCertState state = cTLSContext::CertState(cert, info);

	if (state == cTLSContext::eCS_INVALID) { // note: never overwrite certificate that we can not read, it might be from certificate authority
		res << "TLS certificate is not usable, " << (mTLSContext.IsLoaded() ? "old one stays" : "TLS is disabled") << ": " << cert << ": " << info;
		return done(false, 0);
	}

	if ((state != cTLSContext::eCS_VALID) || !FileTime(key)) { // note: missing or expired certificate, or missing key
		if (Log(0))
			LogStream() << "Creating new TLS certificate, " << ((state == cTLSContext::eCS_EXPIRED) ? ("old one expired: " + info) : string("certificate or key not found")) << ": " << cert << endl;

		if (!cTLSContext::Generate(cert, key, mTLSOrg, mTLSMail, mTLSHost, sTLSCertDays, err)) {
			res << "Failed creating TLS certificate, " << (mTLSContext.IsLoaded() ? "old one stays" : "TLS is disabled") << ": " << err;
			return done(false, 0);
		}

		state = cTLSContext::CertState(cert, info);
		force = true;
	}

	const time_t cert_time = FileTime(cert), key_time = FileTime(key);

	if (!force && mTLSContext.IsLoaded() && (cert_time == mTLSCertTime) && (key_time == mTLSKeyTime)) // note: nothing changed
		return done(true, 0);

	if (!mTLSContext.Load(cert, key, mTLSVer, err)) {
		res << "Failed loading TLS certificate, " << (mTLSContext.IsLoaded() ? "old one stays" : "TLS is disabled") << ": " << err;
		return done(false, 0);
	}

	mTLSCertTime = cert_time;
	mTLSKeyTime = key_time;
	res << "TLS certificate loaded: " << cert << ", valid until " << info << ", keyprint " << mTLSContext.Keyprint();
	return done(true, 0);
}

#endif

bool cAsyncSocketServer::ExternalTLS() const
{
	return (mTLSAddr.size() && (mTLSForce || (mTLSAddr == mAddr))); // note: hub listens on same address as proxy, or it is forced
}

bool cAsyncSocketServer::NativeTLS() const
{
#ifdef USE_TLS
	return (mTLSContext.IsLoaded() && mTLSWait); // note: without wait time tls can not be detected, same as it was with proxy
#else
	return false;
#endif
}

bool cAsyncSocketServer::HasTLS() const
{
	return (ExternalTLS() || NativeTLS());
}

bool cAsyncSocketServer::ListenWithConn(cAsyncConn *ListenSock, int OnPort/*, bool UDP*/)
{
	if (!ListenSock)
		return false;

	string addr = mAddr;

	if (!ListenSock->ListenOnPort(OnPort, addr.c_str(), mAcceptNum/*, UDP*/)) {
		if (Log(0)) {
			LogStream() << "Can't listen on " << addr << ':' << OnPort << (/*UDP ? " UDP":*/" TCP") << endl;
			LogStream() << "Please make sure the port is open and not already used by another process" << endl;
			LogStream() << "Remember that hub must be started using root when port number is below 1024" << endl;
		}

		return false;
	}

	this->mConnChooser.AddConn(ListenSock);
	this->mConnChooser.cConnChoose::OptIn((cConnBase*)ListenSock, tChEvent(eCC_INPUT | eCC_ERROR));
	mListList.push_back(ListenSock); // add to listening connections

	if (Log(0))
		LogStream() << "Listening for connections on " << addr << ':' << OnPort << (/*UDP?" UDP":*/" TCP") << endl;

	return true;
}

	}; // namespace nSocket
}; // namespace nVerliHub
