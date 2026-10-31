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

#ifndef CASYNCCONN_H
#define CASYNCCONN_H

// buffer sizes
#define MAX_MESS_SIZE			524288	// 0.50 mb, maximum size of input buffer
#define MAX_SEND_SIZE			1048576	// 1.00 mb, maximum size of output buffer, is now configurable with max_outbuf_size
#define MAX_SEND_FILL_SIZE		786432	// 0.75 mb, on this level incoming data is blocked, in order to finish sending output buffer, is now configurable with max_outfill_size
#define MAX_SEND_UNBLOCK_SIZE	524288	// 0.50 mb, under this level incoming data is unblocked again, is now configurable with max_unblock_size

#include "cobj.h"
#include "ctime.h"
#include "cconnbase.h"
#include "cprotocol.h"
#include "ctlssession.h"
#include "csocketapi.h"


#include <string>
#include <cstring>
#include <list>
#include <vector>
#include <atomic>
#include <memory>

using namespace std;

namespace nVerliHub {
	namespace nEnums {

		/**
		 * Identify the type of socket connection
		 */
		enum tConnType {
			/// TCP connection is listening
			eCT_LISTEN,
			/// TCP client connection
			eCT_CLIENT,
			/// UDP client connection
			//eCT_CLIENTUDP,
			/// TCP server connection
			eCT_SERVER//,
			/// UDP server connection
			//eCT_SERVERUDP
		};

		/**
		 * Identify the status when reading a line in the buffer.
		 */
		enum tLineStatus {
			/// No line has been read from the buffer.
			AC_LS_NO_LINE,
			/// Line is read partially.
			AC_LS_PARTLY,
			/// Line is read.
			AC_LS_LINE_DONE,
			/// An error occurred while reading a line from the buffer.
			AC_LS_ERROR
		};
	};

 	namespace nProtocol {
 		class cProtocol;
 		class cMessageParser;
 	};

	namespace nSocket {
 		class cAsyncSocketServer;
 		class cAsyncConn;

		using namespace nEnums;
		using namespace nUtils;

		/// @addtogroup Core
		/// @{
		/**
		 * @brief Connection factory to create and delete a connection.
		 * This class is used by cAsyncConn and cServerDC to create and delete
		 * user connection.
		 * @author Daniel Muller
		 */
		class cConnFactory
		{
			public:
				/**
				 * Class constructor.
				 * @param protocol Pointer to an instance of cProtocol interface.
				 */
				cConnFactory(nProtocol::cProtocol *protocol):
					mProtocol(protocol)
				{}

				/**
				 * Class destructor.
				 */
				virtual ~cConnFactory(){};

				/**
				 * Create a new connection for the given socket.
				 * @param sd Socket identifier of the connection.
				 * @return A new connection object that is an instance of cAsyncConn class.
				 */
				virtual cAsyncConn * CreateConn(nSocket::tSocket sd=0);

				/**
				 * Delete a connection.
				 * @param conn A pointer to the connection to delete.
				 */
				virtual void DeleteConn(cAsyncConn * &conn);

				/// Pointer to an instance of cProtocol interface.
				nProtocol::cProtocol *mProtocol;
		};


		/**
		 * @brief Network connection class for asynchronous or non-blocking connection.
		 * This is the base class for an user connection.
		 * @author Daniel Muller
		 * @author Janos Horvath (UDP support)
		 */
		class cAsyncConn: public cConnBase, public cObj
		{
			public:
				/// Define a list of connections.
				typedef list<cAsyncConn*> tConnList;

				/// Define an iterator to a list of connections.
				typedef tConnList::iterator tCLIt;

				/**
				 * Class constructor.
				 * @param sd Socket identifier of the connection.
				 * @param s Pointer to cAsyncSocketServer instance.
				 * @param ct The type of connection. See tConnType for more information.
				 * @see tConnType
				 */
				cAsyncConn(tSocket sd = 0, cAsyncSocketServer *s = NULL, tConnType ct = nEnums::eCT_CLIENT);

				/**
				 * Class constructor.
				 * @param host The hostname of the connection
				 * @param port The port of the connection.
				 * @param udp If the connection is UDP socket.
				 */
				cAsyncConn(const string & host, int port/*, bool udp=false*/);

				/**
				 * Class destructor.
				 */
				virtual ~cAsyncConn();

				/**
				 * Create a new connection.
				 * @see AcceptSock()
				 */
				virtual cAsyncConn* Accept(const unsigned int sleep, const unsigned int tries);

				/**
				 * Return the hostname.
				 * @return The hostname.
				 */
				const string& AddrHost()
				{
					return mAddrHost;
				}

				/// Set host name that was looked up on other thread.
				void SetAddrHost(const string &host)
				{
					mAddrHost = host;
				}

				/**
				 * Return the IP address.
				 * @return The IP address.
				 */
				const string& AddrIP()
				{
					return mAddrIP;
				}

				unsigned long AddrToNumber() const // numerical ip address
				{
					return mNumIP;
				}

				/**
				 * Return the port.
				 * @return The port.
				 */
				unsigned int AddrPort() const
				{
					return mAddrPort;
				}

				// return server address and port that user is connected to

				unsigned int GetServPort() const
				{
					return mServPort;
				}

				const string& GetServAddr()
				{
					return mServAddr;
				}

				/**
				 * Return if the buffer that contains stock data is empty.
				 * @return True if buffer is empty.
				 */
				bool BufferEmpty()
				{
					 return mBufEnd == mBufReadPos;
				}

				/*
					returns buffer sizes
				*/
				size_t GetBufferSize() const // note: only data that is not sent yet
				{
					return mBufSend.size() - mSendPos;
				}

				size_t GetBufferCapacity() const
				{
					return mBufSend.capacity();
				}

				size_t GetFlushSize() const
				{
					return mBufFlush.size();
				}

				size_t GetFlushCapacity() const
				{
					return mBufFlush.capacity();
				}

				/**
				* Reset the status of the line and the delimiter to default value (new line).
				*/
				void ClearLine();


				/**
				 * Close the connection.
				 */
				void Close();

				/**
				* Close the connection after given milliseconds.
				* @param msec Number in millisecond to wait before closing the connection.
				*/
				void CloseNice(int msec = 0);

				/**
				 * Close immediatly the connection.
				 * Use CloseNice() not to close it immediately.
				 * @see CloseNice()
				 */
				void CloseNow();

				/**
				 * Connect to given host and port
				 * @param host The hostname.
				 * @param port The port.
				 * @return -1 in case of errors.
				 */
				int Connect(const string &hostname, int port);

				/**
				 * Create a new message parser instance.
				 * This method is called by FactoryString() when creating
				 * a new string.
				 * @return An pointer to an instance of message parser.
				 */
				virtual nProtocol::cMessageParser *CreateParser();

				/**
				 * Delete a message parser instance.
				 */
				virtual void DeleteParser(nProtocol::cMessageParser *);

				/**
				 * Return the hostname for the IP address of the connection.
				 * The hostname is stored in mAddrHost attribute and mIp attribute is used.
				 *
				 * Please note that DNS lookup may be extremely slow when dealing with a large amount of connections.
				 * @return True on success or false on failure.
				 */
				bool DNSLookup();

				/**
				 * Return the hostname for the given IP address.
				 *
				 * Please note that DNS lookup may be extremely slow when dealing with a large amount of connections.
				 * @param ip The IP address to resolve.
				 * @param host A string that contains the result.
				 * @return True on success or false on failure.
				 */
				static bool DNSResolveReverse(const string &ip, string &host);

				/**
				 * Resolve the given host to an IP address.
				 * @param host The hostname to resolve.
				 * @return The IP address in network byte order.
				 */
				static unsigned long DNSResolveHost(const string &host);

				/**
				 * Create a new string that may be used to store the result
				 * of ReadLineLocal() call.
				 * This method will also create a new instance of the message
				 * parser if it does not exist and re-initialize its state.
				 * @return A pointer to an allocated string object.
				 */
				virtual string * FactoryString();

				/**
				 * Flush the buffer and send the output data.
				 */
				void Flush();

				/**
				 * Return the connection type object.
				 * @return The connection object.
				 */
				//virtual const tConnType& getType();

				/**
				 * Return the connection type object.
				 * @return The connection object.
				 */
				tConnType GetType();

				/**
				 * Return the IP address as binary data in network byte order.
				 * @return The IP address in network byte order.
				 */
				/*
				unsigned long GetSockAddress() const
				{
					return mAddrIN.sin_addr.s_addr;
				}
				*/

				/**
				* Return the pointer to current read line.
				* @return Pointer to a string.
				*/
				string * GetLine();

				/**
				 * Return the value for the given option of the socket.
				 * This method does nothing on Windows OS.
				 * @param optname The name of the option.
				 * @param optval Buffer where value is stored.
				 * @param optlen The length of the buffer.
				 * @return A negative number if value is not retrived.
				 * @see getsockopt
				 */
				//int GetSockOpt(int optname, void *optval, int &optlen);

				/**
				 * Convert a given IP address in network byte order to a string.
				 * @param addr IP address in network byte order.
				 * @return The IP address.
				 */
				//static string IPAsString(unsigned long addr);

				/**
				* Return the status of the line.
				* @return The line status.
				*/
				int LineStatus()
				{
					return meLineStatus;
				}

				/**
				 * Create a new connection and listen on a given port.
				 * If TCP connection is created, then it is non-blocking.
				 * @param port The port.
				 * @param address The address.
				 * @param udp True if it is an UDP connection.
				 * @return True or False if the connection already exists.
				 */
				bool ListenOnPort(int port, const char *ia, const unsigned int blog/*, bool udp = false*/);

				/**
				 * Event handler function called when write buffer gets empty.
				 */
				//virtual void OnFlushDone();

				/**
				 * Event handler function called every period of time by OnTimerBase().
				 * The frequency of this method is called depends on
				 * the value of timer_serv_period config variable.
				 * @param now The current time.
				 * @return Always zero.
				 * @see OnTimerBase()
				 */
				virtual int OnTimer(const cTime &now);

				/// Called with number of bytes that were received from socket or sent to it, this is real network traffic, with tls and zlib.
				virtual void OnReceived(size_t)
				{}

				virtual void OnSent(size_t)
				{}

				/**
				 * This event is trigger every N seconds by cAsyncSocketServer::OnTimerBase()
				 * that handles all socket connections to the server.
				 * @param now The current time.
				 * @return Always zero.
				 */
				int OnTimerBase(const cTime &now);

				/**
				 * Read all available data from the socket and store them in the buffer.
				 * @see ReadLineLocal()
				 * @return Number of read bytes
				 */
				int ReadAll(const unsigned int tries, const unsigned int sleep);

				/**
				 * Read a line and store it in the internal line buffer.
				 * Use GetLine() to return the content of the read line.
				 * @see GetLine()
				 * @return Number of read bytes.
				 */
				int ReadLineLocal();

				/**
				 * Send data using UDP connection.
				 * @param host The hostname.
				 * @param port The destination port.
				 * @param data The data to send.
				 * @return A negative number of failure.
				 */
				//static int SendUDPMsg(const string &host, int port, const string &data);

				/**
				* Set a pointer where to store the line to read and the delimiter.
				* @param strp Pointer to an allocated memory area where to store the line to read.
				* @param delimiter New delimiter.
				* @param max Length of allocated memory and so of the line.
				*/
				void SetLineToRead(string *,char , int max=-1);

				/**
				 * Set the value for the given option on the socket.
				 * This method does nothing on Windows OS.
				 * @param optname The name of the option.
				 * @param optval Buffer where the value is stored.
				 * @param optlen The length of the buffer.
				 * @return A negative number if value is not set.
				 * @see setsockopt
				 */
				int SetSockOpt(int optname, const void *optval, int optlen);

				/**
				 * Setup an UDP socket.
				 * @param host The hostname.
				 * @param port The port.
				 * @return Zero on success or -1 on failure.
				 */
				//int SetupUDP(const string &, int);

				/**
				 * Return the socket identification or descriptor.
				 * @return The socket descriptor.
				 */
				virtual operator tSocket() const
				{
					return mSockDesc;
				}


				/**
				 * Write the given data into the output buffer.
				 * The content of the buffer may be flushed manually or when the buffer
				 * overflows its maximum capacity.
				 * @param data Data to be written.
				 * @param Flush True if the buffer must be flushed.
				 * @return Number of sent bytes. Zero in case buffer is not flushed or negative number in case of errors.
				 */
				int Write(const string &data, bool flush, bool pipe = false); // note: pipe adds | after data in buffer, without changing data

				// states that client supports zlib compression
				bool mZLibFlag;

				// tls version, "0.0" when external proxy says connection is not encrypted, empty when unknown
				string mTLSVer;

			#ifdef USE_TLS
				/// Start tls on this connection, data that is read and written after this is decrypted and encrypted.
				bool StartTLS(ssl_ctx_st *ctx);

				bool IsTLS() const
				{
					return (mTLS != nullptr);
				}

				/// Called when tls handshake is done, mTLSVer is already set, by default it lets server start connection.
				virtual void OnTLSReady();

				/// Result of handshake that was done on worker thread, called by server on main thread.
				void TLSResult(bool good, const string &plain, const string &out, const string &err);
			#endif
				bool SetSecConn(const string &addr, string &vers);
				bool SetUserIP(const string &addr);

				/// Timer of last IO operation.
				cTime mTimeLastIOAction;

				/// Connection iterator.
				tCLIt mIterator;

				unsigned int mTimerSlot = 0; // note: part of connection timer period when this connection gets its timer, set when connection is added to server

				/// Indicate if the connection is still valid.
				bool ok;

				/// Indicate if we can write data to the connection.
				bool mWritable;

				/// Pointer to an instance of cAsyncSocketServer.
				cAsyncSocketServer *mxServer;

				/// Pointer to an instance of connection factory.
				cConnFactory * mxMyFactory;

				/// Pointer to an instance of the protocol handler.
				nProtocol::cProtocol * mxProtocol;

				/// Pointer to an instance of the message parser.
				nProtocol::cMessageParser *mpMsgParser;

				/// Number of connections whit a socket.
				static unsigned long sSocketCounter;

				/// Last given connection id, atomic so connection can be safely created on any thread.
				static std::atomic<unsigned long long> sConnIDCounter;

				/// Unique connection id, never reused, unlike socket descriptor.
				/// Use it to find connection again when result of work on other thread comes back.
				const unsigned long long mConnID = ++sConnIDCounter;

				/// What connection waits for before it is started, see tStartWait, connection is started when nothing is left.
				enum tStartWait
				{
					eSW_HOST = 1 << 0, // host name lookup
					eSW_DETECT = 1 << 1, // first data, to see if client starts tls
					eSW_HANDSHAKE = 1 << 2 // tls handshake
				};

				unsigned int mStartWait = 0;

				/// Input is not read while connection waits only for host name, tls detection and handshake need input.
				bool InputParked() const
				{
					return ((mStartWait & eSW_HOST) && !(mStartWait & (eSW_DETECT | eSW_HANDSHAKE)));
				}

				/*
					protocol data that came before connection was started, for example plain client that did not wait for $Lock while host name is looked up
					note: it is not parsed before OnNewConn, there zone is set and bans and scripts can refuse connection, read buffer is shared by all connections, so data is kept here
				*/
				void HoldInput()
				{
					if (mBufEnd > mBufReadPos)
						mStartHold.append(msBuffer.data() + mBufReadPos, mBufEnd - mBufReadPos);

					mBufEnd = 0;
					mBufReadPos = 0;
				}

				bool HasHeldInput() const
				{
					return !mStartHold.empty();
				}

				// note: data kept outside of normal buffers, before start and around tls handshake, for buffer information
				size_t GetWaitSize() const
				{
				#ifdef USE_TLS
					return mStartHold.size() + mTLSQueue.size() + mTLSHold.size();
				#else
					return mStartHold.size();
				#endif
				}

				// next part of held data into read buffer, returns false when nothing is left
				bool TakeHeldInput()
				{
					if (mStartHold.empty())
						return false;

					const size_t len = ((mStartHold.size() < MAX_MESS_SIZE) ? mStartHold.size() : size_t(MAX_MESS_SIZE));
					memcpy(msBuffer.data(), mStartHold.data(), len);
					mStartHold.erase(0, len);
					mBufReadPos = 0;
					mBufEnd = (unsigned int)len;
					msBuffer[mBufEnd] = '\0';
					return true;
				}

			protected:
				/// Socket descriptor.
				tSocket mSockDesc;

				/*
					buffers that contain outgoing protocol data to send
					if data is not sent at once, rest is stored in send buffer
					flush buffer is used to store data for compression on flush
					we dont want to mix compressed and uncompressed buffers
					else we are going to recompress already compressed unsent data
				*/
				string mBufSend, mBufFlush;
				size_t mSendPos = 0; // note: bytes at start of mBufSend that are already sent, they are cut off when more than half of buffer is sent

				/// Line separator character.
				/// Delimiter is used to split lines in the buffer and the default one is new line.
				char mSeparator;

				/// Read bytes from the buffer after ReadLocalLine() call.
				/// This represents the length of the line.
				int mLineSize;

				/// Host name of the connection.
				string mAddrHost;

				/// IP address of the connection.
				string mAddrIP;

				/// Integer that represents the numeric value of the IP address of the connection.
				unsigned long mIP;
				unsigned long mNumIP;

				/// Port of the connection.
				unsigned int mAddrPort;

				// server address and port that user is connected to
				string mServAddr;
				unsigned int mServPort;

				/// The maximum size of the buffer that contains stock data.
				/// @see msBuffer
				unsigned long mMaxBuffer;
				size_t mBigWrite = 0; // note: size of single message bigger than mMaxBuffer that came to empty buffer, for example nick list of big hub, limit is raised by it until all data is sent

				/// The maximum size of the buffer that contains the read line.
				/// This value can be changed with SetLineToRead() call and by
				/// allocating a new memory area.
				/// @see SetLineToRead()
				unsigned mLineSizeMax;

				/// Structure for Internet address.
				struct sockaddr_in mAddrIN;

				/// The type of the connection.
				tConnType mType;

				/**
				 * Validate and accept a new socket connection.
				 * @return A negative value indicates that the connection is not accepted.
				 */
				tSocket AcceptSock(const unsigned int sleep, const unsigned int tries);

				/**
				 * Bind the given socket to an address and port.
				 * @param sock The identifier of the socket to bind.
				 * @param port The port.
				 * @param add The address.
				 */
				tSocket BindSocket(tSocket sock, int port, const char *addr);

				/**
				 * Create a new socket connection.
				 * @param udp True if the connection is UDP one. Default to TCP.
				 * @return A negative value indicates that the connection is not created.
				 */
				tSocket CreateSock(/*bool udp=false*/);

				/**
				 * Return a pointer to an instance of connection factory
				 * that is capable of accepting new connection.
				 * @return A pointer to cConnFactory instance.
				 */
				virtual cConnFactory * GetAcceptingFactory();

				/**
				 * Listen on the given socket for new connection.
				 * The maximum length of the queue of pending connection is 100.
				 * @param sock The identifier of the socket to listen on.
				 * @return A negative value if the operation failed or the identifier of the socket.
				 */
				tSocket ListenSock(tSocket sock, const unsigned int blog);

				/**
				 * Set socket in non-blocking mode.
				 * @param sock The identifier of the socket.
				 * @return A negative value if the operation failed or the identifier of the socket.
				 */
				tSocket NonBlockSock(tSocket sock);

				/**
				 * Event handler function called before the connection is closed.
				 * @return Always zero.
				 */
				virtual int OnCloseNice(void);

				/**
				* Send len bytes in the buffer.
				* See http://www.ecst.csuchico.edu/~beej/guide/net/html/.
				* @param buf Buffer to send
				* @param len Number of bytes to send
				* @return Number of sent bytes
				*/
				int SendAll(const char *buf, size_t &len);

				// add data to final send buffer, encrypt it when connection uses tls
				bool AppendSend(const char *data, size_t len);

				string mStartHold; // note: see HoldInput()

			#ifdef USE_TLS
				std::shared_ptr<cTLSSession> mTLS; // note: tls session, null when connection is not encrypted, worker that does handshake holds it too

				/*
					while worker thread does handshake, this thread does not touch session
					received data waits in queue and data to send waits in hold, both are handled when result is back
				*/
				bool mTLSBusy = false;
				string mTLSQueue;
				string mTLSHold;

				// decrypt received data in place of shared read buffer
				int TLSRead(int len);

				// handle result of feeding session, decrypted data is put into read buffer, false on error when connection is closed
				bool TLSFed(bool was_ready, bool good, const string &plain, const string &out, const string &err);

				// feed data that came while worker had session
				void TLSFeedQueue();
			#endif
			private:
				/// Pointer to a line in the buffer.
				/// The string is stored by ReadLineLocal() call and then fetched with GetLine() call.
				/// Buffer is split in lines depending on the delimiter stored in mSeparator.
				/// @see GetLine()
				/// @see ReadLineLocal()
				/// @see msBuffer
				string *mxLine;

				/// The status of the read line from the buffer.
				/// @see tLineStatus
				nEnums::tLineStatus meLineStatus;

				static vector<char> msBuffer; // note: this buffer is shared between all connections
				unsigned int mBufEnd;

				// Read buffer position
				/// Current position in the buffer.
				/// The position is pushed forward when a new line is read from the buffer.
				/// @see ReadLineLocal()
				unsigned int mBufReadPos;

				/// The time when the connection has been closed.
				cTime mCloseAfter;
		};
		/// @}
	}; // namespace nSocket
}; // namespace nVerliHub
#endif
