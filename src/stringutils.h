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

#ifndef STRINGUTILS_H
#define STRINGUTILS_H

#include <string>
#include <cstring>

//#if (!defined _WIN32) && (!defined __int64)
	#define __int64 long long
//#endif

using namespace std;

namespace nVerliHub {
	namespace nUtils {

int StrCompare(const string &str1, int Start, int Count, const string &str2);
int StrCompare(const string &str1, int Start, int Count, const char *str2);
string toLower(const string &str, bool cyr = false);
string toUpper(const string &str, bool cyr = false);
void ShrinkStringToFit(string &str);
void StrCutLeft(string &, size_t);
void StrCutLeft(const string &str1, string &str2, size_t cut);
bool LoadFileInString(const string &FileName, string &dest);
void GetPath(const string &FileName, string &Path, string &File);
void FilterPath(string &Path);
void ExpandPath(string &Path);
void ReplaceVarInString(const string &src,const string &var,string &dest, const string& by);
void ReplaceVarInString(const string &src,const string &var,string &dest, double by);
void ReplaceVarInString(const string &src,const string &var,string &dest, int by);
void ReplaceVarInString(const string &src,const string &var,string &dest, long by);
void ReplaceVarInString(const string &src,const string &var,string &dest, __int64 by);
string convertByte(__int64 byte, bool UnitSec = false);
string StringFrom(__int64 const &val);
__int64 StringAsLL(const string &);
bool IsNumber(const char *num);
unsigned int CountLines(const string &);
bool LimitLines(const string &str, int max);
string StrByteList(const string &data, const string &sep = " ");
string StrToMD5ToHex(const string &str);
string StrToHex(const string &str);

inline void AppendPipe(string &dest, string &data, const bool pipe)
{
	if (pipe)
		data.append(1, '|');

	dest.append(data);
}

	}; // namespace nUtils
}; // namespace nVerliHub

#endif
