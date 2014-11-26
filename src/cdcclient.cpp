/*
	Copyright (C) 2003-2005 Daniel Muller, dan at verliba dot cz
	Copyright (C) 2006-2012 Verlihub Team, devs at verlihub-project dot org
	Copyright (C) 2013-2014 RoLex, webmaster at feardc dot net

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

#include "cdcclient.h"
#include <iomanip>
#include "i18n.h"

namespace nVerliHub {
	namespace nTables {

		cDCClient::cDCClient(): mName("Unknown")
		{
			mEnable = 1;
			mMinVersion = -1;
			mMaxVersion = -1;
			mBan = false;
		}

		cDCClient::~cDCClient() {}

		ostream &operator << (ostream &os, cDCClient &tr)
		{
			os << " ";
			os << setw(15) << setiosflags(ios::left) << tr.mName;
			os << setw(15) << setiosflags(ios::left) << tr.mTagID;
			os << setw(30) << setiosflags(ios::left);
 			if(tr.mMinVersion < 0 && tr.mMaxVersion < 0)
 				os << _("Any");
 			else if(tr.mMinVersion >= 0 && tr.mMaxVersion < 0)
 				os << autosprintf(_("Minimum: %.2f"), tr.mMinVersion);
 			else if(tr.mMinVersion < 0 && tr.mMaxVersion >= 0)
				os << autosprintf(_("Maximum: %.2f"), tr.mMaxVersion);
 			else
 				os << tr.mMinVersion << "-" << tr.mMaxVersion;
			os << setw(15) << setiosflags(ios::left) << (tr.mBan ? _("Yes") : _("No"));
 			os << (tr.mEnable ? _("Enabled") : _("Disabled"));
			return os;
		}
	}; // namespace nTables
}; // namespace nVerliHub
