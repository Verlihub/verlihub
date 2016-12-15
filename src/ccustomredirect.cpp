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

#include "ccustomredirect.h"
#include "i18n.h"

namespace nVerliHub {
	using namespace nEnums;
	namespace nTables {

		cRedirect::cRedirect()
		{
			mEnable = 1;
			mFlag = 0;
			mCount = 0;
		}

		cRedirect::~cRedirect()
		{}

		ostream &operator << (ostream &os, cRedirect &tr)
		{
			int flag = tr.mFlag;
			string temp;
			os << "\t" << tr.mCount;
			os << "\t" << tr.mAddress;

			if (tr.mAddress.size() <= 8)
				os << "\t";

			if (tr.mAddress.size() <= 16)
				os << "\t";

			if (tr.mAddress.size() <= 24)
				os << "\t";

			os << "\t" << (tr.mEnable ? _("On") : _("Off"));

			if (flag & eKick)
				temp += "ban,";

			if (flag & eUserLimit)
				temp += "full,";

			if (flag & eShareLimit)
				temp += "share,";

			if (flag & eTag)
				temp += "tag,";

			if (flag & eWrongPasswd)
				temp += "pass,";

			if (flag & eInvalidKey)
				temp += "key,";

			if (flag & eHubBusy)
				temp += "busy,";

			if (flag & eReconnect)
				temp += "reconn,";

			if (flag & eBadNick)
				temp += "nick,";

			if (flag & eClone)
				temp += "clone,";

			if (flag & eSelf)
				temp += "self,";

			if (temp.empty())
				temp = "any";
			else
				temp.erase(temp.end() - 1);

			os << "\t" << temp;
			return os;
		}

	}; // namespace nTables
}; // namespace nVerliHub
