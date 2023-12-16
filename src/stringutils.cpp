/*
	Copyright (C) 2003-2005 Daniel Muller, dan at verliba dot cz
	Copyright (C) 2006-2024 Verlihub Team, info at verlihub dot net

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

#include "stringutils.h"
#include "i18n.h"

#ifdef HAVE_CONFIG_H
	#include <config.h>
#endif

#include <iostream>
#include <fstream>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include <ctype.h>

/*
#if OPENSSL_VERSION_NUMBER < 0x10100000L
	#include <openssl/md5.h>
#else
*/
	#include <openssl/evp.h>
//#endif

/*
#ifdef _WIN32
	#include <windows.h>
#else
*/
	#include <unistd.h>
//#endif

#ifdef HAVE_BSD
	#include <sys/syslimits.h>
#endif

namespace nVerliHub {
	namespace nUtils {

int StrCompare(const string &str1, int start, int count, const string &str2)
{
	return str1.compare(start, count, str2);
}

int StrCompare(const string &str1, int start, int count, const char *str2)
{
	return str1.compare(start, count, str2);
}

string toLower(const string &str, bool cyr)
{
	string result = str;
	transform(str.begin(), str.end(), result.begin(), ::tolower);

	if (cyr) { // cyrillic alphabet
		unsigned int b;

		for (unsigned int i = 0; i < result.size(); ++i) {
			b = (unsigned int)((unsigned char)result[i]);

			if (b == 168)
				result[i] = (unsigned char)(b + 16);
			else if ((b >= 192) && (b <= 223))
				result[i] = (unsigned char)(b + 32);
			else
				result[i] = (unsigned char)(b);
		}
	}

	return result;
}

string toUpper(const string &str, bool cyr)
{
	string result = str;
	transform(str.begin(), str.end(), result.begin(), ::toupper);

	if (cyr) { // cyrillic alphabet
		unsigned int b;

		for (unsigned int i = 0; i < result.size(); ++i) {
			b = (unsigned int)((unsigned char)result[i]);

			if (b == 184)
				result[i] = (unsigned char)(b - 16);
			else if ((b >= 224) && (b <= 255))
				result[i] = (unsigned char)(b - 32);
			else
				result[i] = (unsigned char)(b);
		}
	}

	return result;
}

void ShrinkStringToFit(string &str)
{
	if (str.capacity() != str.size())
		std::string(str.data(), str.size()).swap(str);
}

void StrCutLeft(string &str, size_t cut)
{
	if(cut > str.length()) cut = str.length();
	std::string(str, cut, str.size() - cut).swap(str);
}

void StrCutLeft(const string &str1, string &str2, size_t cut)
{
	if(cut > str1.size()) cut = str1.size();
	std::string(str1, cut, str1.size() - cut).swap(str2);
}

bool LoadFileInString(const string &FileName, string &dest)
{
	string buf;
	bool AddLine = false;
	ifstream is(FileName.c_str());

	if(!is.is_open()) return false;
	while(!is.eof() && is.good())
	{
		getline(is, buf);
		if (AddLine) dest += "\r\n";
		else AddLine = true;
		dest+= buf;
	}
	is.close();
	return true;
}

void ExpandPath(string &Path)
{
	if (Path.substr(0, 2) == "./") {
		string tmp = Path;

		/*
		#ifdef _WIN32
			char *cPath = new char[35];
			int size = GetCurrentDirectory(35, cPath);

			if (!size) {
				delete[] cPath;
				return;
			} else if (size > 35) {
				delete[] cPath;
				cPath = new char[size];
				GetCurrentDirectory(35, cPath);
			}

			Path = string(cPath);
			delete[] cPath;
		#elif defined HAVE_BSD
		*/
		#if defined HAVE_BSD
			char *cPath = getcwd(NULL, PATH_MAX);
			Path = cPath;
			free(cPath);
		#elif defined HAVE_APPLE
			char *cPath = getcwd(NULL, PATH_MAX);
			Path = cPath;
			free(cPath);
		#else
			char *cPath = get_current_dir_name();
			Path = cPath;
			free(cPath);
		#endif

		Path += '/' + tmp.substr(2, tmp.length());
	}

	size_t pos;

	//#if ! defined _WIN32
		pos = Path.find('~');

		if (pos != Path.npos) {
			Path.replace(pos, 2, getenv("HOME"));
		}
	//#endif

	pos = Path.find("../"); // todo: doesnt work on windows

	while (pos != Path.npos) {
		Path.replace(pos, 3, "");
		pos = Path.find("../", pos);
	}

	int len = Path.length();

	if (Path.substr(len - 1, len) != "/")
		Path.append(1, '/');
}

void GetPath(const string &FileName, string &Path, string &File)
{
	Path = FileName;
	size_t i = FileName.rfind('/');
	if(i != string::npos)
		Path = FileName.substr(0, i+1);
	File = FileName.substr(i+1);
}

void FilterPath(string &Path)
{
	size_t pos = Path.find("../");
	while (pos != Path.npos) {
		Path.replace(pos, 3, "");
		pos = Path.find("../", pos);
	}
}

/*!
    \fn ReplaceVarInString(const string&,const string &varname,string &dest, const string& by)
 */
void ReplaceVarInString(const string&src ,const string &varname, string &dest, const string& by)
{
	string searchvar("%[");
	searchvar+=varname;
	searchvar+=']';
	dest = src;
	size_t pos = dest.find(searchvar);
	while (pos != dest.npos)
	{
		dest.replace(pos, searchvar.size(), by);
		pos = dest.find(searchvar, pos);
	}
}


/*!
    \fn ReplaceVarInString(const string&,const string &varname,string &dest, int by)
 */
void ReplaceVarInString(const string &src,const string &varname,string &dest, int by)
{
	ostringstream os;
	os << by;
	ReplaceVarInString(src, varname, dest, os.str());
}


/*!
    \fn ReplaceVarInString(const string&,const string &varname,string &dest, double by)
 */
void ReplaceVarInString(const string&src,const string &varname,string &dest, double by)
{
	ostringstream os;
	os << by;
	ReplaceVarInString(src, varname, dest, os.str());
}


/*!
    \fn ReplaceVarInString(const string&,const string &varname,string &dest, long by)
 */
void ReplaceVarInString(const string &src,const string &varname,string &dest, long by)
{
	ostringstream os;
	os << by;
	ReplaceVarInString(src, varname, dest, os.str());
}

/*!
    \fn ReplaceVarInString(const string&,const string &varname,string &dest, __int64 by)
 */
void ReplaceVarInString(const string &src,const string &varname,string &dest, __int64 by)
{
	ReplaceVarInString(src, varname, dest, StringFrom(by));
}

string convertByte(__int64 byte, bool UnitSec)
{
	static const char *byteUnit[] = { _("B"), _("KB"), _("MB"), _("GB"), _("TB"), _("PB"), _("EB"), _("ZB"), _("YB") };
	static const char *byteSecUnit[] = { _("B/s"), _("KB/s"), _("MB/s"), _("GB/s"), _("TB/s"), _("PB/s"), _("EB/s"), _("ZB/s"), _("YB/s") };
	unsigned int unit;
	double long lByte = byte;

	if (lByte < 1024) {
		unit = 0;
	} else {
		for (unit = 0; lByte >= 1024; unit++)
			lByte /= 1024;
	}

	ostringstream os(ostringstream::out);
	os.precision(2);
	os << fixed << lByte << ' ';

	if (UnitSec) {
		if (unit >= (sizeof(byteSecUnit) / sizeof(char*)))
			os << "X/s";
		else
			os << byteSecUnit[unit];
	} else {
		if (unit >= (sizeof(byteUnit) / sizeof(char*)))
			os << 'X';
		else
			os << byteUnit[unit];
	}

	return os.str();
}

string StringFrom(__int64 const &val)
{
	char buf[32];

	/*
	#ifdef _WIN32
		sprintf(buf, "%I64d", val);
	#else
	*/
		sprintf(buf, "%lld", val);
	//#endif

	return buf;
}

__int64 StringAsLL(const string &str)
{
	/*
	#ifdef _WIN32
		__int64 result;
		sscanf(str.c_str(), "%I64d", &result);
		return result;
	#else
	*/
		return strtoll(str.c_str(), NULL, 10);
	//#endif
}

bool IsNumber(const char *num)
{
	if (!num || (num[0] == '\0'))
		return false;

	for (unsigned int pos = 0; num[pos]; pos++) {
		if ((pos == 0) && (num[pos] == '-')) { // allow negative numbers
			// pass
		} else if (!isdigit(num[pos])) {
			return false;
		}
	}

	return true;
}

unsigned int CountLines(const string &str)
{
	unsigned int lines = 1;
	size_t slen = str.size();

	if (slen) {
		for (unsigned int pos = 0; pos < slen; pos++) {
			if (str[pos] == '\n')
				lines++;
		}
	}

	return lines;
}

bool LimitLines(const string &str, int max)
{
	int lines = 1;

	for (unsigned int pos = 0; pos < str.size(); pos++) {
		if (str[pos] == '\n') {
			lines++;

			if (lines > max) {
				return false;
			}
		}
	}

	return true;
}

string StrByteList(const string &data, const string &sep)
{
	string res;

	for (unsigned i = 0; i < data.size(); ++i) {
		if (i > 0)
			res.append(sep);

		res.append(StringFrom(int(data[i])));
	}

	return res;
}

string StrToMD5ToHex(const string &str)
{
	size_t len = str.size();

	if (!len)
		return string();

	unsigned char *buf = (unsigned char*)str.c_str();
	unsigned char md5[16]; // MD5_DIGEST_LENGTH

/*
#if OPENSSL_VERSION_NUMBER < 0x10100000L
	MD5_CTX ctx;
	MD5_Init(&ctx);
	MD5_Update(&ctx, buf, len);
	MD5_Final(md5, &ctx);
#else
*/
	EVP_MD_CTX *ctx = EVP_MD_CTX_new();
	EVP_DigestInit_ex(ctx, EVP_md5(), NULL);
	EVP_DigestUpdate(ctx, buf, len);
	EVP_DigestFinal_ex(ctx, md5, NULL);
	EVP_MD_CTX_free(ctx);
//#endif

	static const char arr[] = "0123456789abcdef";
	string out;

	for (int pos = 0; pos < 16; pos++) {
		unsigned char chr = md5[pos];
		char dec[3];
		dec[0] = arr[chr >> 4];
		dec[1] = arr[chr & 0xf];
		dec[2] = 0;
		out.append(dec);
	}

	return out;
}

string StrToHex(const string &str)
{
	static const char hex[] = "0123456789abcdef";
	string out;

	for (unsigned char chr: str) {
		out.append(1, '%');
		out.append(1, hex[chr >> 4]);
		out.append(1, hex[chr & 15]);
	}

	return out;
}

	}; // namespace nUtils
}; // namespace nVerliHub
