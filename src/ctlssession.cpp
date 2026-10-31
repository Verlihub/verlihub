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

#include "ctlssession.h"

#ifdef USE_TLS

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/x509v3.h>
#include <openssl/evp.h>
#include <openssl/bn.h>
#include <cstdio>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#ifdef _WIN32
	#include <ws2tcpip.h> // inet_pton
	#include <windows.h> // MoveFileExA
#else
	#include <arpa/inet.h> // inet_pton
#endif

#ifndef O_BINARY
	#define O_BINARY 0 // note: windows opens files in text mode by default, that changes new lines
#endif

using namespace std;

namespace nVerliHub {
	namespace nSocket {

static string Errors(int code) // note: also clears openssl error queue
{
	string res;
	char buf[256];
	unsigned long err;

	while ((err = ERR_get_error())) {
		ERR_error_string_n(err, buf, sizeof(buf));

		if (res.size())
			res.append("; ");

		res.append(buf);
	}

	if (res.empty()) {
		if (code == SSL_ERROR_SYSCALL)
			res = "Unexpected end of TLS stream";
		else if (code)
			res = "TLS error " + to_string(code);
		else
			res = "Unknown TLS error";
	}

	return res;
}

/*
	alpn, client that offers nmdcs gets it, client that offers something else or nothing continues without alpn
*/

static int SelectALPN(SSL *ssl, const unsigned char **out, unsigned char *outlen, const unsigned char *in, unsigned int inlen, void *arg)
{
	(void)ssl;
	(void)arg;
	static const unsigned char prot[] = {5, 'n', 'm', 'd', 'c', 's'};
	unsigned char *sel = NULL;

	if (SSL_select_next_proto(&sel, outlen, prot, sizeof(prot), in, inlen) == OPENSSL_NPN_NEGOTIATED) {
		*out = sel;
		return SSL_TLSEXT_ERR_OK;
	}

	return SSL_TLSEXT_ERR_NOACK; // note: dont refuse old clients
}

/*
	context
*/

cTLSContext::~cTLSContext()
{
	Clear();
}

void cTLSContext::Clear()
{
	if (mCtx) {
		SSL_CTX_free(mCtx);
		mCtx = NULL;
	}
}

bool cTLSContext::Load(const string &cert, const string &key, unsigned int min_ver, string &err)
{
	ERR_clear_error();
	SSL_CTX *ctx = SSL_CTX_new(TLS_server_method());

	if (!ctx) {
		err = "Failed to create TLS context";
		return false;
	}

	int ver = TLS1_2_VERSION;

	switch (min_ver) {
		case 0:
			ver = TLS1_VERSION;
			break;
		case 1:
			ver = TLS1_1_VERSION;
			break;
		case 2:
			ver = TLS1_2_VERSION;
			break;
		default:
#ifdef TLS1_3_VERSION
			ver = TLS1_3_VERSION;
#endif
			break;
	}

	SSL_CTX_set_min_proto_version(ctx, ver);

	if (ver < TLS1_2_VERSION) { // note: old versions sign handshake with sha1 and md5, openssl 3 and many distributions refuse that above security level 0
		SSL_CTX_set_security_level(ctx, 0);
		SSL_CTX_set_cipher_list(ctx, "DEFAULT:@SECLEVEL=0");
	}
	SSL_CTX_set_options(ctx, SSL_OP_NO_COMPRESSION | SSL_OP_CIPHER_SERVER_PREFERENCE); // note: data is already compressed with zlib when client wants it
#ifdef SSL_OP_NO_RENEGOTIATION
	SSL_CTX_set_options(ctx, SSL_OP_NO_RENEGOTIATION);
#endif
	SSL_CTX_set_mode(ctx, SSL_MODE_RELEASE_BUFFERS); // note: idle connection does not keep read and write buffers
	SSL_CTX_set_alpn_select_cb(ctx, SelectALPN, NULL);

	if (SSL_CTX_use_certificate_chain_file(ctx, cert.c_str()) != 1) {
		err = "Failed to load certificate " + cert + ": " + Errors(0);
		SSL_CTX_free(ctx);
		return false;
	}

	if (SSL_CTX_use_PrivateKey_file(ctx, key.c_str(), SSL_FILETYPE_PEM) != 1) {
		err = "Failed to load key " + key + ": " + Errors(0);
		SSL_CTX_free(ctx);
		return false;
	}

	if (SSL_CTX_check_private_key(ctx) != 1) {
		err = "Key does not match certificate: " + Errors(0);
		SSL_CTX_free(ctx);
		return false;
	}

	Clear(); // note: running sessions keep their own reference to old context
	mCtx = ctx;
	return true;
}

string cTLSContext::Keyprint() const
{
	if (!mCtx)
		return "";

	X509 *cert = SSL_CTX_get0_certificate(mCtx);
	unsigned char md[EVP_MAX_MD_SIZE];
	unsigned int len = 0;

	if (!cert || (X509_digest(cert, EVP_sha256(), md, &len) != 1))
		return "";

	static const char abc[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567"; // note: base32 without padding
	string res("SHA256/");
	unsigned int buf = 0, bits = 0;

	for (unsigned int i = 0; i < len; i++) {
		buf = (buf << 8) | md[i];
		bits += 8;

		while (bits >= 5) {
			res.append(1, abc[(buf >> (bits - 5)) & 31]);
			bits -= 5;
		}
	}

	if (bits)
		res.append(1, abc[(buf << (5 - bits)) & 31]);

	return res;
}

cTLSContext::tCertState cTLSContext::CertState(const string &cert, string &info)
{
	struct stat st;

	if (stat(cert.c_str(), &st) != 0) {
		info = "File not found";
		return eCS_MISSING;
	}

	FILE *file = fopen(cert.c_str(), "r");

	if (!file) {
		info = "Failed to open file";
		return eCS_INVALID;
	}

	ERR_clear_error();
	X509 *x509 = PEM_read_X509(file, NULL, NULL, NULL);
	fclose(file);

	if (!x509) {
		info = "Failed to read certificate: " + Errors(0);
		return eCS_INVALID;
	}

	BIO *mem = BIO_new(BIO_s_mem());

	if (mem) { // note: expiry date for log
		ASN1_TIME_print(mem, X509_get0_notAfter(x509));
		char *data = NULL;
		const long len = BIO_get_mem_data(mem, &data);
		info.assign(data, len);
		BIO_free(mem);
	}

	const bool expired = (X509_cmp_current_time(X509_get0_notAfter(x509)) < 0);
	X509_free(x509);
	return (expired ? eCS_EXPIRED : eCS_VALID);
}

static bool AddExt(X509 *cert, int nid, const string &value)
{
	X509V3_CTX ctx;
	X509V3_set_ctx_nodb(&ctx);
	X509V3_set_ctx(&ctx, cert, cert, NULL, NULL, 0);
	string copy(value); // note: older openssl takes non const value
	X509_EXTENSION *ext = X509V3_EXT_conf_nid(NULL, &ctx, nid, &copy[0]);

	if (!ext)
		return false;

	const bool res = (X509_add_ext(cert, ext, -1) == 1);
	X509_EXTENSION_free(ext);
	return res;
}

static int MoveOver(const string &from, const string &to) // note: like rename, also on windows where rename does not replace existing file
{
#ifdef _WIN32
	return (MoveFileExA(from.c_str(), to.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) ? 0 : -1);
#else
	return rename(from.c_str(), to.c_str());
#endif
}

static bool WriteFile(const string &path, mode_t mode, const string &data, string &err)
{
	const int fd = open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, mode); // note: key must not be readable by others

	if (fd < 0) {
		err = "Failed to open " + path + " for writing";
		return false;
	}

	size_t done = 0;

	while (done < data.size()) {
		const ssize_t res = write(fd, data.data() + done, data.size() - done);

		if (res <= 0) {
			close(fd);
			err = "Failed to write " + path;
			return false;
		}

		done += res;
	}

	if (close(fd) != 0) {
		err = "Failed to close " + path;
		return false;
	}

	return true;
}

bool cTLSContext::Generate(const string &cert, const string &key, const string &org, const string &mail, const string &host, unsigned int days, string &err)
{
	if (cert.empty() || key.empty()) {
		err = "Missing certificate or key file name";
		return false;
	}

	ERR_clear_error();
	EVP_PKEY *pkey = NULL;
	EVP_PKEY_CTX *kctx = EVP_PKEY_CTX_new_id(EVP_PKEY_EC, NULL); // note: works with openssl 1.1 and 3

	if (!kctx || (EVP_PKEY_keygen_init(kctx) != 1) || (EVP_PKEY_CTX_set_ec_paramgen_curve_nid(kctx, NID_X9_62_prime256v1) != 1) || (EVP_PKEY_CTX_set_ec_param_enc(kctx, OPENSSL_EC_NAMED_CURVE) != 1) || (EVP_PKEY_keygen(kctx, &pkey) != 1)) {
		err = "Failed to generate key: " + Errors(0);

		if (kctx)
			EVP_PKEY_CTX_free(kctx);

		return false;
	}

	EVP_PKEY_CTX_free(kctx);
	X509 *x509 = X509_new();
	BIGNUM *serial = BN_new();
	bool good = (x509 && serial);

	if (good) // note: random 128 bit serial
		good = (BN_rand(serial, 127, BN_RAND_TOP_ANY, BN_RAND_BOTTOM_ANY) == 1) && BN_to_ASN1_INTEGER(serial, X509_get_serialNumber(x509));

	if (good)
		good = (X509_set_version(x509, 2) == 1) && X509_gmtime_adj(X509_getm_notBefore(x509), -3600) && X509_time_adj_ex(X509_getm_notAfter(x509), days, 0, NULL) && (X509_set_pubkey(x509, pkey) == 1); // note: one hour back for clocks that are a bit behind

	if (good) { // note: organization is also common name
		const string name = (org.size() ? org : "Verlihub");
		X509_NAME *subj = X509_get_subject_name(x509);
		good = (X509_NAME_add_entry_by_txt(subj, "CN", MBSTRING_UTF8, (const unsigned char*)name.c_str(), -1, -1, 0) == 1) && (X509_NAME_add_entry_by_txt(subj, "O", MBSTRING_UTF8, (const unsigned char*)name.c_str(), -1, -1, 0) == 1) && (X509_set_issuer_name(x509, subj) == 1);
	}

	string alt; // note: host names and addresses, and mail
	size_t pos = 0;

	while (good && (pos <= host.size())) {
		size_t end = host.find(',', pos);

		if (end == host.npos)
			end = host.size();

		string part = host.substr(pos, end - pos);
		part.erase(0, part.find_first_not_of(' '));
		part.erase(part.find_last_not_of(' ') + 1);

		if (part.size()) {
			unsigned char buf[16];
			const bool ip = (inet_pton(AF_INET, part.c_str(), buf) == 1) || (inet_pton(AF_INET6, part.c_str(), buf) == 1);
			alt.append(alt.size() ? "," : "").append(ip ? "IP:" : "DNS:").append(part);
		}

		pos = end + 1;
	}

	if (good && mail.size())
		alt.append(alt.size() ? "," : "").append("email:").append(mail);

	if (good)
		good = AddExt(x509, NID_basic_constraints, "critical,CA:FALSE") && AddExt(x509, NID_key_usage, "critical,digitalSignature") && AddExt(x509, NID_ext_key_usage, "serverAuth") && (alt.empty() || AddExt(x509, NID_subject_alt_name, alt));

	if (good)
		good = (X509_sign(x509, pkey, EVP_sha256()) > 0);

	string cert_pem, key_pem;

	if (good) {
		BIO *cbio = BIO_new(BIO_s_mem()), *kbio = BIO_new(BIO_s_mem());
		good = (cbio && kbio && (PEM_write_bio_X509(cbio, x509) == 1) && (PEM_write_bio_PrivateKey(kbio, pkey, NULL, NULL, 0, NULL, NULL) == 1)); // note: key is written as pkcs8

		if (good) {
			char *data = NULL;
			long len = BIO_get_mem_data(cbio, &data);
			cert_pem.assign(data, len);
			len = BIO_get_mem_data(kbio, &data);
			key_pem.assign(data, len);
		}

		if (cbio)
			BIO_free(cbio);

		if (kbio)
			BIO_free(kbio);
	}

	if (!good && err.empty())
		err = "Failed to create certificate: " + Errors(0);

	if (serial)
		BN_free(serial);

	if (x509)
		X509_free(x509);

	EVP_PKEY_free(pkey);

	if (!good)
		return false;

	const string cert_tmp = cert + ".tmp", key_tmp = key + ".tmp"; // note: old files stay when writing fails

	if (!WriteFile(key_tmp, 0600, key_pem, err) || !WriteFile(cert_tmp, 0644, cert_pem, err)) {
		unlink(key_tmp.c_str());
		unlink(cert_tmp.c_str());
		return false;
	}

	if ((MoveOver(key_tmp, key) != 0) || (MoveOver(cert_tmp, cert) != 0)) { // note: old file is replaced at once
		err = "Failed to replace " + cert + " or " + key;
		unlink(key_tmp.c_str());
		unlink(cert_tmp.c_str());
		return false;
	}

	return true;
}

/*
	session
*/

cTLSSession::cTLSSession(ssl_ctx_st *ctx)
{
	if (!ctx)
		return;

	mSSL = SSL_new(ctx);

	if (!mSSL)
		return;

	mRead = BIO_new(BIO_s_mem());
	mWrite = BIO_new(BIO_s_mem());

	if (!mRead || !mWrite) {
		if (mRead)
			BIO_free(mRead);

		if (mWrite)
			BIO_free(mWrite);

		mRead = mWrite = NULL;
		SSL_free(mSSL);
		mSSL = NULL;
		return;
	}

	BIO_set_mem_eof_return(mRead, -1); // note: empty read buffer means wait for more data, not end of stream
	SSL_set_bio(mSSL, mRead, mWrite); // note: ssl owns both now
	SSL_set_accept_state(mSSL);
}

cTLSSession::~cTLSSession()
{
	if (mSSL)
		SSL_free(mSSL); // note: frees both bios
}

void cTLSSession::Drain(string &out)
{
	char buf[16384];
	int len;

	while ((len = BIO_read(mWrite, buf, sizeof(buf))) > 0)
		out.append(buf, len);
}

bool cTLSSession::ReadAll(string &plain, string &err)
{
	char buf[16384];

	while (true) {
		const int len = SSL_read(mSSL, buf, sizeof(buf));

		if (len > 0) {
			plain.append(buf, len);
			continue;
		}

		const int code = SSL_get_error(mSSL, len);

		if ((code == SSL_ERROR_WANT_READ) || (code == SSL_ERROR_WANT_WRITE)) // note: need more data
			return true;

		if (code == SSL_ERROR_ZERO_RETURN) { // note: peer sent close notify
			mClosed = true;
			return true;
		}

		err = Errors(code);
		return false;
	}
}

bool cTLSSession::Feed(const char *data, size_t len, string &plain, string &out, string &err)
{
	if (!mSSL) {
		err = "No TLS session";
		return false;
	}

	ERR_clear_error();

	if (len && (BIO_write(mRead, data, len) != (int)len)) { // note: memory bio takes everything
		err = "Failed to buffer TLS data";
		return false;
	}

	if (!mReady) {
		const int res = SSL_do_handshake(mSSL);

		if (res == 1) {
			mReady = true;

			if (mPending.size()) { // note: data that was waiting for handshake
				string pending;
				pending.swap(mPending);

				if (!Encrypt(pending.data(), pending.size(), out, err)) {
					Drain(out);
					return false;
				}
			}

		} else {
			const int code = SSL_get_error(mSSL, res);

			if ((code != SSL_ERROR_WANT_READ) && (code != SSL_ERROR_WANT_WRITE)) {
				err = Errors(code);
				Drain(out); // note: alert for peer
				return false;
			}
		}
	}

	if (mReady && !ReadAll(plain, err)) {
		Drain(out);
		return false;
	}

	Drain(out); // note: handshake messages, session tickets, alerts
	return true;
}

bool cTLSSession::Encrypt(const char *data, size_t len, string &out, string &err)
{
	if (!mSSL) {
		err = "No TLS session";
		return false;
	}

	if (!len)
		return true;

	if (!mReady) {
		mPending.append(data, len);
		return true;
	}

	ERR_clear_error();
	size_t done = 0;

	while (done < len) { // note: memory bio never blocks, but write is limited to int size
		const size_t part = ((len - done) > 1048576) ? 1048576 : (len - done);
		const int res = SSL_write(mSSL, data + done, (int)part);

		if (res <= 0) {
			err = Errors(SSL_get_error(mSSL, res));
			Drain(out);
			return false;
		}

		done += res;
	}

	Drain(out);
	return true;
}

void cTLSSession::Shutdown(string &out)
{
	if (!mSSL || !mReady)
		return;

	ERR_clear_error();
	SSL_shutdown(mSSL); // note: we dont wait for close notify of peer
	Drain(out);
}

string cTLSSession::Version() const
{
	if (!mSSL || !mReady)
		return "";

	switch (SSL_version(mSSL)) {
#ifdef TLS1_3_VERSION
		case TLS1_3_VERSION:
			return "1.3";
#endif
		case TLS1_2_VERSION:
			return "1.2";
		case TLS1_1_VERSION:
			return "1.1";
		case TLS1_VERSION:
			return "1.0";
		default:
			return "";
	}
}

	}; // namespace nSocket
}; // namespace nVerliHub

#endif // USE_TLS
