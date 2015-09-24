/*
	Copyright (C) 2003-2005 Daniel Muller, dan at verliba dot cz
	Copyright (C) 2006-2015 Verlihub Team, info at verlihub dot net

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

using namespace std;

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
#include <iomanip>
#include <openssl/md5.h>
#include "creguserinfo.h"
#include "ctime.h"
#include "i18n.h"

#define PADDING 25

namespace nVerliHub {
	using namespace nUtils;
	namespace nTables {

cRegUserInfo::cRegUserInfo():
	mPWCrypt(eCRYPT_NONE),
	mClass(0),
	mClassProtect(0),
	mClassHideKick(0),
	mHideKick(false),
	mHideKeys(false),
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

ostream & operator << (ostream &os, cRegUserInfo &ui)
{
	static const char *ClassName[] = {_("Guest"), _("Registered"), _("VIP"), _("Operator"), _("Cheef"), _("Administrator"), _("6-err"), _("7-err"), _("8-err"), _("9-err"), _("Master")};
	os << " [*] " << setw(PADDING) << setiosflags(ios::left) << _("Class") << ((ui.mClass == -1) ? _("Pinger") : (ClassName[ui.mClass] ? ClassName[ui.mClass] : _("Invalid"))) << " [" << ui.mClass << "]\r\n";
	os << " [*] " << setw(PADDING) << setiosflags(ios::left) << _("Last login") << cTime(ui.mLoginLast,0).AsDate() << " " << _("from") << " " << ui.mLoginIP << "\r\n";
	os << " [*] " << setw(PADDING) << setiosflags(ios::left) << _("Registered since");

	if (ui.mRegDate)
		os << cTime(ui.mRegDate, 0).AsDate() << " " << autosprintf(_("by %s"),ui.mRegOp.c_str());
	else
		os <<  _("No information");

	os << "\r\n";
	os << " [*] " << setw(PADDING) << setiosflags(ios::left) << _("Last error");

	if (ui.mErrorLast)
		os << cTime(ui.mErrorLast).AsDate() << " " << _("from") << " " << ui.mErrorIP;
	else
		os <<  _("No information");

	os << "\r\n";
	os << " [*] " << setw(PADDING) << setiosflags(ios::left) << _("Login errors/total") << ui.mErrorCount << "/" << ui.mLoginCount << "\r\n";
	os << " [*] " << setw(PADDING) << setiosflags(ios::left) << _("Password set") << ((ui.mPasswd.size() != 0) ? _("Yes") : _("No")) << "\r\n";
	os << " [*] " << setw(PADDING) << setiosflags(ios::left) << _("Account enabled") << ((ui.mEnabled != 0) ? _("Yes") : _("No")) << "\r\n";
	os << " [*] " << setw(PADDING) << setiosflags(ios::left) << _("Protected") << ((ui.mClassProtect != 0) ? _("Yes") : _("No")) << "\r\n";
	os << " [*] " << setw(PADDING) << setiosflags(ios::left) << _("Hidden kicks") << ((ui.mHideKick != 0) ? _("Yes") : _("No")) << "\r\n";
	os << " [*] " << setw(PADDING) << setiosflags(ios::left) << _("Hidden key") << ((ui.mHideKeys != 0) ? _("Yes") : _("No")) << "\r\n";
	os << " [*] " << setw(PADDING) << setiosflags(ios::left) << _("Hidden share") << ((ui.mHideShare != 0) ? _("Yes") : _("No")) << "\r\n";
	os << " [*] " << setw(PADDING) << setiosflags(ios::left) << _("Hidden share messages") << ((ui.mHideCtmMsg != 0) ? _("Yes") : _("No")) << "\r\n";
	os << " [*] " << setw(PADDING) << setiosflags(ios::left) << _("Authorization IP") << (ui.mAuthIP.empty() ? "--" : ui.mAuthIP) << "\r\n";
	os << " [*] " << setw(PADDING) << setiosflags(ios::left) << _("Alternate IP") << (ui.mAlternateIP.empty() ? "--" : ui.mAlternateIP);
	return os;
}

string & cRegUserInfo::GetNick(){
	return mNick;
}

/*!
    \fn ::nTables::cRegUserInfo::SetPass(const string &)
 */
void cRegUserInfo::SetPass(string str, int crypt_method)
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
