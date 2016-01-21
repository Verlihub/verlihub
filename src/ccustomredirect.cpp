/*
	Copyright (C) 2003-2005 Daniel Muller, dan at verliba dot cz
	Copyright (C) 2006-2016 Verlihub Team, info at verlihub dot net

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

		cRedirect::~cRedirect()
		{}

		ostream &operator << (ostream &os, cRedirect &tr)
		{
			int flag = tr.mFlag;
			string buff;
			os << "\t" << tr.mCount << "\t" << tr.mAddress << "\t\t" << (tr.mEnable ? _("On") : _("Off")) << "\t";

			if (flag & eKick)
				buff += "ban,";

			if (flag & eUserLimit)
				buff += "full,";

			if (flag & eShareLimit)
				buff += "share,";

			if (flag & eTag)
				buff += "tag,";

			if (flag & eWrongPasswd)
				buff += "pass,";

			if (flag & eInvalidKey)
				buff += "key,";

			if (flag & eHubBusy)
				buff += "busy,";

			if (flag & eReconnect)
				buff += "reconn,";

			if (flag & eBadNick)
				buff += "nick,";

			if (flag & eClone)
				buff += "clone,";

			if (flag & eSelf)
				buff += "self,";

			if (buff.empty())
				buff = "any";
			else
				buff.erase(buff.end() - 1);

			os << buff;
			return os;
		}

	}; // namespace nTables
}; // namespace nVerliHub
