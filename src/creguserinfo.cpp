/*
	Copyright (C) 2003-2005 Daniel Muller, dan at verliba dot cz
	Copyright (C) 2006-2017 Verlihub Team, info at verlihub dot net

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
// for the crypt
// broken build on NetBSD
#ifndef HAVE_NETBSD
#define _XOPEN_SOURCE
#endif
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <openssl/md5.h>
#include "creguserinfo.h"
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

bool cRegUserInfo::PWVerify(const string &pass)
{
	if (!mPasswd.size() || !pass.size()) // check both password lengths
		return false;

	string crypt_buf;
	unsigned char md5_buf[MD5_DIGEST_LENGTH + 1];
	char md5_hex[33];
	bool result = false;

	switch (mPWCrypt) {
		case eCRYPT_ENCRYPT:
			crypt_buf = crypt(pass.c_str(), mPasswd.c_str());
			result = crypt_buf == mPasswd;
			break;
		case eCRYPT_MD5:
			MD5((const unsigned char*)pass.c_str(), pass.size(), md5_buf);
			md5_buf[MD5_DIGEST_LENGTH] = 0;

			for (int i = 0; i < MD5_DIGEST_LENGTH; i++) { // convert to hexadecimal
				sprintf(md5_hex + (i * 2), "%02x", md5_buf[i]);
			}

			md5_hex[32] = 0;
			result = mPasswd == string(md5_hex);
			break;
		case eCRYPT_NONE:
			result = pass == mPasswd;
			break;
	}

	return result;
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
	os << " [*] " << autosprintf(_("Registered on: %s [%s]"), (ui.mRegDate ? cTime(ui.mRegDate, 0).AsDate().AsString().c_str() : _("Unknown")), (ui.mRegOp.size() ? ui.mRegOp.c_str() : _("Not set"))) << "\r\n";
	os << " [*] " << autosprintf(_("Last login: %s [%d] [%s]"), (ui.mLoginLast ? cTime(ui.mLoginLast, 0).AsDate().AsString().c_str() : _("Never")), ui.mLoginCount, (ui.mLoginIP.size() ? ui.mLoginIP.c_str() : _("Not set"))) << "\r\n";
	os << " [*] " << autosprintf(_("Last error: %s [%d] [%s]"), (ui.mErrorLast ? cTime(ui.mErrorLast, 0).AsDate().AsString().c_str() : _("Never")), ui.mErrorCount, (ui.mErrorIP.size() ? ui.mErrorIP.c_str() : _("Not set"))) << "\r\n";
	os << " [*] " << autosprintf(_("Password set: %s"), ((ui.mPwdChange || ui.mPasswd.empty()) ? _("No") : _("Yes"))) << "\r\n";
	os << " [*] " << autosprintf(_("Enabled: %s"), (ui.mEnabled ? _("Yes") : _("No"))) << "\r\n";
	os << " [*] " << autosprintf(_("Protected: %s"), (ui.mClassProtect ? _("Yes") : _("No"))) << "\r\n";
	os << " [*] " << autosprintf(_("Hide kicks: %s"), (ui.mHideKick ? _("Yes") : _("No"))) << "\r\n";
	os << " [*] " << autosprintf(_("Hide key: %s"), (ui.mHideKeys ? _("Yes") : _("No"))) << "\r\n";
	os << " [*] " << autosprintf(_("Show key: %s"), (ui.mShowKeys ? _("Yes") : _("No"))) << "\r\n";
	os << " [*] " << autosprintf(_("Hide share: %s"), (ui.mHideShare ? _("Yes") : _("No"))) << "\r\n";
	os << " [*] " << autosprintf(_("Hide messages: %s"), (ui.mHideCtmMsg ? _("Yes") : _("No"))) << "\r\n";
	os << " [*] " << autosprintf(_("Authorization IP: %s"), (ui.mAuthIP.size() ? ui.mAuthIP.c_str() : _("Not set"))) << "\r\n";
	os << " [*] " << autosprintf(_("Alternate IP: %s"), (ui.mAlternateIP.size() ? ui.mAlternateIP.c_str() : _("Not set"))) << "\r\n";
	os << " [*] " << autosprintf(_("User note: %s"), (ui.mNoteUsr.size() ? ui.mNoteUsr.c_str() : _("Not set"))) << "\r\n";

	if (ui.mClass >= eUC_OPERATOR)
		os << " [*] " << autosprintf(_("Operator note: %s"), (ui.mNoteOp.size() ? ui.mNoteOp.c_str() : _("Not set"))) << "\r\n";

	return os;
}

string& cRegUserInfo::GetNick()
{
	return mNick;
}

void cRegUserInfo::SetPass(string str, tCryptMethods crypt_method)
{
	mPwdChange = !str.size();

	if (str.size()) {
		string salt;
		static const char *saltchars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmlopqrstuvwxyz0123456789./";
		static const int saltcharsnum = strlen(saltchars);
		unsigned char charsalt[2] = {(unsigned char)((char*)&str)[0], (unsigned char)((char*)&str)[1]};
		unsigned char md5_buf[MD5_DIGEST_LENGTH + 1];
		char md5_hex[33];

		switch (crypt_method) {
			case eCRYPT_ENCRYPT:
				charsalt[0] = saltchars[charsalt[0] % saltcharsnum];
				charsalt[1] = saltchars[charsalt[1] % saltcharsnum];
				salt.assign((char*)charsalt, 2);
				mPasswd = crypt(str.c_str(), salt.c_str());
				mPWCrypt = eCRYPT_ENCRYPT;
				break;
			case eCRYPT_MD5:
				MD5((const unsigned char*)str.c_str(), str.size(), md5_buf);
				md5_buf[MD5_DIGEST_LENGTH] = 0;

				for (int i = 0; i < MD5_DIGEST_LENGTH; i++) { // convert to hexadecimal
					sprintf(md5_hex + (i * 2), "%02x", md5_buf[i]);
				}

				md5_hex[32] = 0;
				mPasswd = string(md5_hex);
				mPWCrypt = eCRYPT_MD5;
				break;
			case eCRYPT_NONE:
				mPasswd = str;
				mPWCrypt = eCRYPT_NONE;
				break;
		}
	} else
		mPasswd = str;
}

	}; // namespace nTables
}; // namespace nVerliHub
