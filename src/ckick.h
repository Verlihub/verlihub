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

#ifndef CKICK_H
#define CKICK_H
#include <string>

using namespace std;

//#if (!defined _WIN32) && (!defined __int64)
#define __int64 long long
//#endif

namespace nVerliHub {
	namespace nTables {
/**this represents kick by ops, has all information, that mey serve to ban
  *@author Daniel Muller
  */

class cKick {
public:
	cKick();
	~cKick();
	string mNick;
	string mReason;
	long mTime;
	string mIP;
	string mOp;
	bool mIsDrop;
	string mHost;
	unsigned __int64 mShare;
};

	}; // namespace nTables
}; //namespace nVerliHub

#endif
