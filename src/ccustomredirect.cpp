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

#include "ccustomredirect.h"
#include <iomanip>
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

		cRedirect::~cRedirect() {}

		ostream &operator << (ostream &os, cRedirect &tr)
		{
			string buff;
			os << " ";
			os << setw(10) << setiosflags(ios::left) << tr.mCount;
			os << setw(35) << setiosflags(ios::left) << tr.mAddress;
			os << setw(30) << setiosflags(ios::left);
			int flag = tr.mFlag;

			if (flag & eKick)
				buff += "ban and kick,";

			if (flag & eUserLimit)
				buff += "hub full,";

			if (flag & eShareLimit)
				buff += "share limit,";

			if (flag & eTag)
				buff += "invalid tag,";

			if (flag & eWrongPasswd)
				buff += "wrong password,";

			if (flag & eInvalidKey)
				buff += "invalid key,";

			if (flag & eHubBusy)
				buff += "hub busy,";

			if (flag & eReconnect)
				buff += "reconnect,";

			if (flag & eBadNick)
				buff += "bad nick,";

			if (flag & eClone)
				buff += "clone,";

			if (buff.empty())
				buff = "default";
			else
				buff.erase(buff.end() - 1);

			os << buff;
			os << setw(60) << setiosflags(ios::left);

			if (tr.mEnable)
				os << _("Enabled");
			else
				os << _("Disabled");

			return os;
		}

	}; // namespace nTables
}; // namespace nVerliHub
