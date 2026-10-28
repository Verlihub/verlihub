/*
	Copyright (C) 2003-2005 Daniel Muller, dan at verliba dot cz
	Copyright (C) 2006-2025 Verlihub Team, info at verlihub dot net

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

// for the crypt, broken build on NetBSD
#ifndef HAVE_NETBSD
	#define _XOPEN_SOURCE
#endif

#include <unistd.h>
#include <string.h>
#include <stdio.h>

#ifdef _WIN32 // note: windows has no crypt, openssl gives same classic des crypt
	#define OPENSSL_SUPPRESS_DEPRECATED // note: des is deprecated since openssl 3.0, but it is still there
	#include <openssl/des.h>
#endif

#include <openssl/rand.h>
#include <random>

#include "creguserinfo.h"
#include "stringutils.h"
#include "cuser.h"
#include "ctime.h"
#include "i18n.h"

using namespace std;

namespace nVerliHub {
	using namespace nUtils;
	namespace nTables {

cRegUserInfo::cRegUserInfo():
	mPWCrypt(eCRYPT_NONE),
	mClass(eUC_NORMUSER),
	mClassProtect(eUC_NORMUSER),
	mClassHideKick(eUC_NORMUSER),
	mHideKick(false),
	mHideKeys(false),
	mShowKeys(false),
	mHideShare(false),
	mHideChat(false),
	mHideCtmMsg(false),
	mRegDate(0),
	mLoginCount(0),
	mErrorCount(0),
	mLoginLast(0),
	mLogoutLast(0),
	mErrorLast(0),
	mPwdChange(false),
	mEnabled(true)
{}

cRegUserInfo::~cRegUserInfo()
{}

static string CryptPass(const string &pass, const string &salt)
{
#ifdef _WIN32
	char buf[14]; // note: 13 chars and null
	const char *res = DES_fcrypt(pass.c_str(), salt.c_str(), buf); // note: only classic des, not $1$, $5$ or $6$ like system crypt
#else
	const char *res = crypt(pass.c_str(), salt.c_str());
#endif

	return (res ? res : ""); // note: crypt can return null on unsupported salt
}

bool cRegUserInfo::PWVerify(const string &pass)
{
	if (!mPasswd.size() || !pass.size()) // check both password lengths
		return false;

	string comp;

	switch (mPWCrypt) {
		case eCRYPT_ENCRYPT:
			comp = CryptPass(pass, mPasswd);
			break;

		case eCRYPT_MD5:
			comp = StrToMD5ToHex(pass);
			break;

		case eCRYPT_NONE:
			comp = pass;
			break;
	}

	return comp == mPasswd;
}

istream & operator >> (istream &is, cRegUserInfo &ui)
{
	int i;
	is >> ui.GetNick() >> i >> ui.mPasswd >> ui.mClass;
	ui.mPWCrypt = i;
	return is;
}

ostream& operator << (ostream &os, cRegUserInfo &ui)
{
	os << "\r\n [*] " << autosprintf(_("Class: %d"), ui.mClass) << "\r\n";
	os << " [*] " << autosprintf(_("Registered on: %s [%s]"), (ui.mRegDate ? cTimePrint(ui.mRegDate, 0).AsDate().AsString().c_str() : _("Unknown")), (ui.mRegOp.size() ? ui.mRegOp.c_str() : _("Not set"))) << "\r\n";
	os << " [*] " << autosprintf(_("Last login: %s [%d] [%s]"), (ui.mLoginLast ? cTimePrint(ui.mLoginLast, 0).AsDate().AsString().c_str() : _("Never")), ui.mLoginCount, (ui.mLoginIP.size() ? ui.mLoginIP.c_str() : _("Not set"))) << "\r\n";
	os << " [*] " << autosprintf(_("Last error: %s [%d] [%s]"), (ui.mErrorLast ? cTimePrint(ui.mErrorLast, 0).AsDate().AsString().c_str() : _("Never")), ui.mErrorCount, (ui.mErrorIP.size() ? ui.mErrorIP.c_str() : _("Not set"))) << "\r\n";
	os << " [*] " << autosprintf(_("Password set: %s"), ((ui.mPwdChange || ui.mPasswd.empty()) ? _("No") : _("Yes"))) << "\r\n";
	os << " [*] " << autosprintf(_("Enabled: %s"), (ui.mEnabled ? _("Yes") : _("No"))) << "\r\n";
	os << " [*] " << autosprintf(_("Protected: %s"), (ui.mClassProtect ? _("Yes") : _("No"))) << "\r\n";
	os << " [*] " << autosprintf(_("Hide kicks: %s"), (ui.mHideKick ? _("Yes") : _("No"))) << "\r\n";
	os << " [*] " << autosprintf(_("Hide key: %s"), (ui.mHideKeys ? _("Yes") : _("No"))) << "\r\n";
	os << " [*] " << autosprintf(_("Show key: %s"), (ui.mShowKeys ? _("Yes") : _("No"))) << "\r\n";
	os << " [*] " << autosprintf(_("Hide share: %s"), (ui.mHideShare ? _("Yes") : _("No"))) << "\r\n";
	os << " [*] " << autosprintf(_("Hide chat: %s"), (ui.mHideChat ? _("Yes") : _("No"))) << "\r\n";
	os << " [*] " << autosprintf(_("Hide messages: %s"), (ui.mHideCtmMsg ? _("Yes") : _("No"))) << "\r\n";
	os << " [*] " << autosprintf(_("Authorization IP: %s"), (ui.mAuthIP.size() ? ui.mAuthIP.c_str() : _("Not set"))) << "\r\n";
	os << " [*] " << autosprintf(_("Alternate IP: %s"), (ui.mAlternateIP.size() ? ui.mAlternateIP.c_str() : _("Not set"))) << "\r\n";
	os << " [*] " << autosprintf(_("Fake IP: %s"), (ui.mFakeIP.size() ? ui.mFakeIP.c_str() : _("Not set"))) << "\r\n";
	//os << " [*] " << autosprintf(_("Language: %s"), (ui.mLang.size() ? ui.mLang.c_str() : _("Not set"))) << "\r\n";
	os << " [*] " << autosprintf(_("User note: %s"), (ui.mNoteUsr.size() ? ui.mNoteUsr.c_str() : _("Not set"))) << "\r\n";

	if (ui.mClass >= eUC_OPERATOR)
		os << " [*] " << autosprintf(_("Operator note: %s"), (ui.mNoteOp.size() ? ui.mNoteOp.c_str() : _("Not set"))) << "\r\n";

	return os;
}

string& cRegUserInfo::GetNick()
{
	return mNick;
}

void cRegUserInfo::SetPass(const string &str, tCryptMethods crypt_method)
{
	string pass(str);
	mPwdChange = pass.empty();

	if (mPwdChange) {
		mPasswd = pass;
		return;
	}

	static const char arr[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789./"; // 64
	unsigned char dec[2];
	string out;

	switch (crypt_method) {
		case eCRYPT_ENCRYPT:
			if (RAND_bytes(dec, 2) != 1) { // note: random salt, old hashes still verify because each hash keeps its own salt
				std::random_device rnd;
				dec[0] = (unsigned char)rnd();
				dec[1] = (unsigned char)rnd();
			}

			out.append(1, arr[dec[0] % 64]);
			out.append(1, arr[dec[1] % 64]);
			mPasswd = CryptPass(pass, out);
			mPWCrypt = eCRYPT_ENCRYPT;
			break;

		case eCRYPT_MD5:
			mPasswd = StrToMD5ToHex(pass);
			mPWCrypt = eCRYPT_MD5;
			break;

		case eCRYPT_NONE:
			mPasswd = pass;
			mPWCrypt = eCRYPT_NONE;
			break;
	}
}

	}; // namespace nTables
}; // namespace nVerliHub
