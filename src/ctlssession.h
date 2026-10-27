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

	Please see https://www.gnu.org/licenses/ for a copy
	of the GNU General Public License.
*/

#ifndef NSOCKETCTLSSESSION_H
#define NSOCKETCTLSSESSION_H

#ifdef HAVE_CONFIG_H
	#include <config.h>
#endif

#ifdef USE_TLS

#include <string>

struct ssl_st; // note: openssl types, so this header does not pull openssl into every file
struct ssl_ctx_st;
struct bio_st;

namespace nVerliHub {
	namespace nSocket {

/*
	server tls context, certificate, key and protocol settings shared by all connections
	note: replacing it does not affect running sessions, each session holds its own reference
*/

class cTLSContext
{
	public:
		cTLSContext() = default;
		~cTLSContext();

		cTLSContext(const cTLSContext&) = delete;
		cTLSContext& operator=(const cTLSContext&) = delete;

		// min_ver is minor version: 0 = 1.0, 1 = 1.1, 2 = 1.2, 3 = 1.3, on error err describes it and old context stays
		bool Load(const std::string &cert, const std::string &key, unsigned int min_ver, std::string &err);
		void Clear();

		bool IsLoaded() const
		{
			return (mCtx != NULL);
		}

		ssl_ctx_st *Get() const
		{
			return mCtx;
		}

		// keyprint of loaded certificate as SHA256/<base32>, same as nmdcs url uses with kp parameter
		std::string Keyprint() const;

		enum tCertState
		{
			eCS_VALID,
			eCS_MISSING,
			eCS_EXPIRED,
			eCS_INVALID // note: file exists but can not be read, it is never overwritten
		};

		// state of certificate file, info is expiry date or error
		static tCertState CertState(const std::string &cert, std::string &info);

		/*
			create self signed ecdsa p-256 certificate and its key, old files are replaced only when both are written
			host is comma separated list of host names and ip addresses
		*/
		static bool Generate(const std::string &cert, const std::string &key, const std::string &org, const std::string &mail, const std::string &host, unsigned int days, std::string &err);

	private:
		ssl_ctx_st *mCtx = NULL;
};

/*
	tls session of single connection, it never touches socket
	encrypted data received from socket is given to Feed() and encrypted data to send is returned to caller
	so all socket options, connection chooser and send buffers stay as they are
*/

class cTLSSession
{
	public:
		explicit cTLSSession(ssl_ctx_st *ctx); // note: server side
		~cTLSSession();

		cTLSSession(const cTLSSession&) = delete;
		cTLSSession& operator=(const cTLSSession&) = delete;

		// false when session could not be created
		bool IsValid() const
		{
			return (mSSL != NULL);
		}

		/*
			give encrypted data received from socket
			decrypted data is appended to plain, encrypted data that must be sent back is appended to out, for example handshake
			returns false on fatal error, err describes it, out can still contain alert that should be sent before closing
		*/
		bool Feed(const char *data, size_t len, std::string &plain, std::string &out, std::string &err);

		/*
			encrypt data to send, encrypted data is appended to out
			before handshake is done data is kept and encrypted when handshake is done, that output comes from Feed()
		*/
		bool Encrypt(const char *data, size_t len, std::string &out, std::string &err);

		// close notify for peer, appended to out
		void Shutdown(std::string &out);

		// handshake is done
		bool IsReady() const
		{
			return mReady;
		}

		// peer sent close notify
		bool IsClosed() const
		{
			return mClosed;
		}

		// negotiated version as "1.3", "1.2", "1.1" or "1.0", empty before handshake is done
		std::string Version() const;

		// largest amount of decrypted data that one Feed() can produce more than encrypted data given to it
		static const size_t sMaxExtra = 16384 + 1024;

	private:
		void Drain(std::string &out);
		bool ReadAll(std::string &plain, std::string &err);

		ssl_st *mSSL = NULL;
		bio_st *mRead = NULL; // note: encrypted data from socket goes here
		bio_st *mWrite = NULL; // note: encrypted data for socket comes from here
		std::string mPending; // note: data that was given to Encrypt() before handshake was done
		bool mReady = false;
		bool mClosed = false;
};

	}; // namespace nSocket
}; // namespace nVerliHub

#endif // USE_TLS
#endif
