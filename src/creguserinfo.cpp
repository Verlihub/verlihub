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
	mEnabled(1),
	mPwdChange(false)
{}

cRegUserInfo::~cRegUserInfo(){}

bool cRegUserInfo::PWVerify(const string &pass)
{
	string crypted_p;
	unsigned char buf[MD5_DIGEST_LENGTH + 1];
	bool Result = false;
	string res; // used for hexadecimal conversion
	char hex[32];

	switch (mPWCrypt) {
		case eCRYPT_ENCRYPT:
			crypted_p = crypt(pass.c_str(), mPasswd.c_str());
			Result = crypted_p == mPasswd;
			break;
		case eCRYPT_MD5:
			MD5((const unsigned char*)pass.data(), pass.length(), buf);
			buf[MD5_DIGEST_LENGTH] = 0;

			for (int i = 0; i < 16; i++) { // convert to hexadecimal
				sprintf(hex, "%02x", buf[i]);
				res.append(hex);
			}

			Result = mPasswd == res; // string((const char*)buf)
			break;
		case eCRYPT_NONE:
			Result = pass == mPasswd;
			break;
	}

	return Result;
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
	string salt;
	mPwdChange = !str.size();

	if (str.size()) {
		static const char *saltchars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmlopqrstuvwxyz0123456789./";
		static const int saltcharsnum = strlen(saltchars);
		unsigned char charsalt[2] = {((char*)&str)[0], ((char*)&str)[1]};
		unsigned char buf[MD5_DIGEST_LENGTH + 1];
		string res; // used for hexadecimal conversion
		char hex[32];

		switch (crypt_method) {
			case eCRYPT_ENCRYPT:
				charsalt[0] = saltchars[charsalt[0] % saltcharsnum];
				charsalt[1] = saltchars[charsalt[1] % saltcharsnum];
				salt.assign((char*)charsalt, 2);
				mPasswd = crypt(str.c_str(), salt.c_str());
				mPWCrypt = eCRYPT_ENCRYPT;
				break;
			case eCRYPT_MD5:
				MD5((const unsigned char*)str.c_str(), str.size(), buf);
				buf[MD5_DIGEST_LENGTH] = 0;

				for (int i = 0; i < 16; i++) { // convert to hexadecimal
					sprintf(hex, "%02x", buf[i]);
					res.append(hex);
				}

				mPasswd = res; // string((char*)buf)
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
