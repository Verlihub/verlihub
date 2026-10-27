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

#ifndef CASYNCSOCKETSERVER_H
#define CASYNCSOCKETSERVER_H

#include "cconnchoose.h"

#if USE_EPOLL
	#include "cconnepoll.h"
#elif USE_SELECT
	#include "cconnselect.h"
#else
	#include "cconnpoll.h"
#endif

#include "ctimeout.h"
#include "cobj.h"
#include "casyncconn.h"
#include "cmeanfrequency.h"
#include "cworkerpool.h"
#include "cwakeup.h"

#include <list>
#include <deque>
#include <vector>
#include <functional>

/*
#ifdef USE_SSL_CONNECTS
	#include <openssl/ssl.h>
	#include <openssl/err.h>
#endif
*/

using namespace std;

namespace nVerliHub {
	using namespace nUtils;

	namespace nSocket {
		/// @addtogroup Core
		/// @{
		/**
		* General asynchronous (non-blocking) socket server with some basic functionality.
		*
		* @author Daniel Muller
		*/
		class cAsyncSocketServer : public cObj
		{
			public:
				friend class nVerliHub::nSocket::cAsyncConn;

				/// Define a list of connections.
				typedef list<cAsyncConn*> tConnList;

				/// Define an iterator to a list of connections.
				typedef tConnList::iterator tCLIt;

				// base directory
				string mConfBaseDir;

				/**
				* Class constructor.
				* Create a server and start listening on given port.
				* @param port The port to listen on.
				*/
				cAsyncSocketServer(string CfgBase = ".", int port = 0);

				/**
				* Class destructor.
				*/
				virtual ~cAsyncSocketServer();

				/**
				* Stop main process loop and delete all connections.
				*/
				void close();

				/**
				* Return the port on which the server is listening on.
				* @return The port.
				*/
				//virtual unsigned int getPort() const;

				/**
				 * Create a new connection and handle it.
				 * @param OnPort The port to listen on.
				 * @param UDP True if it is an UDP connection.
				 * @return The new connection.
				 */
				virtual bool Listen(int OnPort/*, bool UDP = false*/);

				/**
				 * Handle the given connection and listen on the given port.
				 * @param connection The connection to listen on.
				 * @param OnPort The port to listen on.
				 * @param UDP True if it is an UDP connection.
				 * @return True or False.
				 */
				virtual bool ListenWithConn(cAsyncConn *connection, int OnPort/*, bool UDP=false*/);

				/**
				* This event is triggered when a connection is closed.
				* @param conn The closed connection closed.
				*/
				void OnConnClose(cAsyncConn*);

				/**
				* Event handler function called every period of time by OnTimerBase().
				* The frequency of this method is called depends on
				* the value of timer_serv_period config variable.
				* @param now The current time.
				* @return Always zero.
				* @see OnTimerBase()
				*/
				virtual int OnTimer(const cTime &now);

				/**
				* This event is trigger every N seconds.
				* This method is responsable of calling cAsyncConn::OnTimerBase()
				* for every connections the server is handling.
				* @param now Current time.
				* @return Always zero.
				*/
				int OnTimerBase(const cTime &now);

			/*
			#ifdef USE_SSL_CONNECTS
				// ssl context
				SSL_CTX* mSSLCont;
			#endif
			*/

				/**
				* Start the main loop.
				* This ìs the method that calls OnTimerBase() and TimeStep()
				* Run it until it is stopped or paused.
				* @see TimeStep()
				* @see OnTimerBase()
				* @return The error code.
				*/
				int run();

				/**
				 *
				 * Set the port to listen on
				 * @param newPort The new port.
				 */
				//virtual void setPort(const unsigned int newPort);

				/**
				 * Start the socket to listen on.
				 * @param OverrideDefaultPort Use this port instead of mPort.
				 * @return false on failure.
				 */
				virtual bool StartListening(int OverrideDefaultPort = 0);

				/*
					set stop code and exit main loop using delay
					or reset stop code and delay
				*/
				void stop(int code, int delay = 0);

				/**
				* This method handles all connections and
				* iterate over them with cConnChoose instance.
				* Handle new incoming connection, take care of existing ones
				* and close those connections that are not actived anymore.
				*/
				void TimeStep();

				// address to listen on
				string mAddr;

				// main and extra ports to listen on
				unsigned int mPort;
				string mExtra;

				/// Connection period to call event time method cAsyncConn::OnTimerBase().
				/// This value controls how often cAsyncConn::OnTimerBase() is called.
				int timer_conn_period;

				/// Server period to call event time method OnTimerBase().
				/// This value controls how often OnTimerBase() is called.
				int timer_serv_period;

				/// Delay in milliseconds to wait in main loop for every iteration.
				unsigned int mStepDelay;
				unsigned int mNoConnDelay;
				unsigned int mNoReadTry;
				unsigned int mNoReadDelay;
				unsigned int mChooseTimeOut;
				unsigned int mAcceptNum;
				unsigned int mAcceptTry;

				/// Maximum size of the buffer for cAsyncConn::SetLineToRead() method.
				unsigned long mMaxLineLength;

				string mTLSAddr;
				unsigned int mTLSPort;
				unsigned int mTLSWait;
				string mTLSCert;
				string mTLSKey;
				string mTLSOrg;
				string mTLSMail;
				string mTLSHost;
				unsigned int mTLSVer;
				unsigned int mTLSBuf;
				bool mTLSForce;
				bool mTLSLog;

				/// Use reverse DNS lookup feature when there is a new connection.
				int mUseDNS;

				/// Number of worker threads for blocking or heavy work, 0 = number of cpu cores.
				unsigned int mWorkerThreads;

				/// Worker threads, their results are processed in main loop.
				/// Posting fails when workers are not running, caller must then do the work itself.
				nThread::cWorkerPool mWorkers;

				/// Start new connection, when dns_lookup is enabled this happens only after host name is known.
				/// @return Same as OnNewConn(), negative value means connection must be closed.
				int StartConn(cAsyncConn *conn);

				/// Look up host name on worker thread, when park is true connection is not started and its input is not read until result comes back.
				/// @return False when lookup could not be queued, caller must then do it by itself.
				bool LookupHost(cAsyncConn *conn, bool park);

				/// Find connection by socket and id, returns null when connection is gone or socket is reused by other connection.
				cAsyncConn* FindConn(tSocket sock, unsigned long long id);

				/// Host names in same order as addresses, empty string when not found.
				using tHostsDone = std::function<void(const vector<string> &hosts)>;

				/// Look up host names on worker thread, done is called on main thread.
				/// @return False when lookup could not be queued, caller must then use ReverseLookup() by itself.
				bool LookupHosts(const vector<string> &addrs, tHostsDone done);

				/// Reverse lookup of ip address, blocks until resolver answers, thread safe.
				static string ReverseLookup(const string &addr);

				/// Start extra worker pool for slow work that must not hold up jobs in mWorkers, its results are also processed in main loop.
				/// Works only while main loop is running, pool is stopped together with mWorkers.
				bool StartPool(nThread::cWorkerPool &pool, unsigned int threads, size_t max_jobs = 0);

				/// The current time.
				cTimePrint mTime;

				/// Measure the frequency of the server.
				nUtils::cMeanFrequency<unsigned ,21> mFrequency;

				unsigned int GetConnListSize() const
				{
					return mConnList.size();
				}

				unsigned int GetConnChooserSize() const
				{
					return mConnChooser.mConnList.size();
				}

			#if defined(USE_TLS_PROXY) || defined(USE_FEARTLS_PROXY) // tls proxy
				void StopProxy(int code);
			#endif

		protected:

			/// Indicate if the main loop is running.
			bool mbRun;

			/// List of connections that the server is handling.
			/// The list contains pointers to cAsyncConn instance.
			tConnList mConnList;
			tConnList mListList; // listening connections

			#if USE_EPOLL
				/// Connection chooser for epoll.
				cConnEpoll mConnChooser;
			#elif !USE_SELECT
				/// Connection chooser for poll.
				cConnPoll mConnChooser;
			#else
				/// Connection chooser for select.
				cConnSelect mConnChooser;
			#endif

			/// True if Windows sockets is initialized.
			//static bool WSinitialized;

			/// Pointer to connection factory instance.
			cConnFactory *mFactory;

			/**
			* Add the connection to the server so it can be processed.
			* @param conn The connection to add.
			*/
			virtual void addConnection(cAsyncConn *);

			/**
			* Return true if the server accepts new incoming connections.
			* @return True if the server accepts a new connection or false otherwise.
			*/
			/*
			virtual bool AllowNewConn()
			{
				return true;
			};
			*/

			/**
			* Remove the connection from the server.
			* The pointer to the connection will be deleted and not valid anymore.
			* @param conn The connection to remove.
			*/
			void delConnection(cAsyncConn *);

			/**
			* Create a new string buffer for the given connection.
			* @param conn The connection.
			* @return Pointer to the new buffer.
			*/
			virtual string * FactoryString(cAsyncConn *conn);

			/**
			* Perform input operation on the given connection.
			* This method will read all data from the connection
			* and return the number of read bytes or a negative number if an error occurred.
			* @param conn The connection.
			* @return Number of read bytes.
			*/
			virtual int input(cAsyncConn *conn);

			/**
			* This event is triggered when there is a new incoming message
			* for a connection.
			* @param conn The connection that has new incoming message.
			* @param message Pointer to the new message.
			*/
			virtual void OnNewMessage(cAsyncConn *, string *);

			/**
			* Perform output operation on the given connection.
			* This method will send all data from the connection
			* and return the number of sent bytes or a negative number if an error occurred.
			* @param conn The connection.
			* @return Number of sent bytes.
			*/
			int output(cAsyncConn * conn);

			/**
			* This event is triggered when there is a new incoming connection.
			* @param conn The new connection.
			* @return Zero if connection is accepted or a negative number otherwise.
			*/
			virtual int OnNewConn(cAsyncConn*);

			struct sTimers // structure that contains timers
			{
				cTime main; // server time of the last call of OnTimerBase
				cTime conn; // connection time of the last call of cAsyncConn::OnTimerBase
				cTime stop; // stop delay
			};

			sTimers mT; // timer structure
			int mRunResult; // stop code

			/// Stop all worker pools, derived class must call this in its destructor when it owns pools.
			void StopWorkers();
		private:
			/// Pointer to the connection that server is currently handling
			cAsyncConn * mNowTreating;

			/// Wakes up main loop when workers have results, note: workers must be stopped before this is closed.
			cWakeup mWakeup;

			bool StartWorkers();
			void StepWait(unsigned int msec);
			void RunPoolResults();

			/// All running pools whose results are processed in main loop, mWorkers included.
			std::vector<nThread::cWorkerPool*> mPools;

			/// Connections waiting for host name, in order of start time.
			struct sHostWait
			{
				tSocket mSock;
				unsigned long long mID;
				long mStart;
			};

			std::deque<sHostWait> mHostWaitList;

			void OnHostResult(tSocket sock, unsigned long long id, const string &addr, const string &host);
			void HostWaitDone(cAsyncConn *conn);
			void CheckHostWait(const cTime &now);
		};
		/// @}

	}; // namespace nSocket
}; // namespace nVerliHub

#endif
