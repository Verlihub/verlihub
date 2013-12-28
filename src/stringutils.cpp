/*
	Copyright (C) 2003-2005 Daniel Muller, dan at verliba dot cz
	Copyright (C) 2006-2014 Verlihub Project, devs at verlihub-project dot org

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
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace nVerliHub {
	namespace nUtils {

int StrCompare(const string &str1, int start, int count, const string &str2)
{
	return str1.compare(start,count,str2);
}

string toLower(const string &str)
{
	string result = str;
	transform(str.begin(), str.end(), result.begin(), ::tolower);
	return result;
}

string toUpper(const string &str)
{
	string result = str;
	transform(str.begin(), str.end(), result.begin(), ::toupper);
	return result;
}

void ShrinkStringToFit(string &str)
{
	std::string(str.data(), str.size()).swap(str);
}

void StrCutLeft(string &str, size_t cut)
{
	string tmp;
	if(cut > str.length()) cut = str.length();
	std::string(str, cut, str.size() - cut).swap(str);
}

void StrCutLeft(const string &str1, string &str2, size_t cut)
{
	string tmp;
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

if(Path.substr(0,2) == "./") {
		string tmp = Path;
#ifdef _WIN32
		char * cPath = new char[35];
		int size = GetCurrentDirectory(35, cPath);
		if(!size) {
			delete cPath;
			return;
		}
		else if(size > 35) {
			delete cPath;
			cPath = new char[size];
			GetCurrentDirectory(35, cPath);
		}
		Path = string(cPath);
		delete cPath;
#elif defined HAVE_FREEBSD || HAVE_OPENBSD
	Path = getcwd(NULL, PATH_MAX);
#else
		Path = get_current_dir_name();
#endif
		Path += "/" + tmp.substr(2,tmp.length());
	}
	size_t pos;
#if ! defined _WIN32
	pos = Path.find("~");
	if(pos != Path.npos) {
		Path.replace(pos, 2, getenv("HOME"));
	}
#endif
	// FIXME: It doesn't work on Windows
	pos = Path.find("../");
	while (pos != Path.npos) {
		Path.replace(pos, 3, "");
		pos = Path.find("../", pos);
	}
	int len = Path.length();
	if(Path.substr(len-1,len) != "/")
		Path.append("/");
}
void GetPath(const string FileName, string &Path, string &File)
{
	Path = FileName;
	size_t i = FileName.rfind("/");
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
	searchvar+="]";
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

string convertByte(__int64 byte, bool UnitType)
{
	static const char *byteUnit[] = {"B", "KB", "MB", "GB", "TB", "PB", "", "", ""};
	static const char *byteSecUnit[] = {"B/s", "KB/s", "MB/s", "GB/s", "TB/s", "PB/s", "", "", ""};
	//string result;
	int unit;

	double long lByte = byte;

	if(lByte < 1024) {
		unit = 0;
	} else {
		for(unit = 0; lByte > 1024; unit++) {
			lByte /= 1024;
		}
	}

	ostringstream os (ostringstream::out);
	os.precision(2);
	os << fixed << lByte << " ";
	if(UnitType)  os << byteSecUnit[unit];
	else os << byteUnit[unit];
	return os.str();
}

string StringFrom(__int64 const &ll)
{
	char buf[32];
#ifdef _WIN32
	sprintf(buf,"%I64d",ll);
#else
	sprintf(buf,"%lld",ll);
#endif
	return buf;
}
__int64 StringAsLL(const string &str)
{
#ifdef _WIN32
	__int64 result;
	sscanf(str.c_str(),"%I64d",&result);
	return result;
#else
	return strtoll(str.c_str(),NULL,10);
#endif
}

int CountLines(const string &str)
{
	int lines = 1;

	for (unsigned int pos = 0; pos < str.size(); pos++) {
		if (str[pos] == '\n') {
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

	}; // namespace nUtils
}; // namespace nVerliHub
