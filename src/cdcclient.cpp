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

#include "cdcclient.h"
#include "i18n.h"

namespace nVerliHub {
	namespace nTables {

		cDCClient::cDCClient():
			mName("Unknown")
		{
			mBan = false;
			mMinVersion = -1;
			mMaxVersion = -1;
			mMinVerUse = -1;
			mEnable = true;
		}

		cDCClient::~cDCClient()
		{}

		ostream &operator << (ostream &os, cDCClient &tr)
		{
			os << "\t" << tr.mName;

			if (tr.mName.size() <= 8)
				os << "\t";

			os << "\t\t" << tr.mTagID << "\t\t";

 			if ((tr.mMinVersion < 0) && (tr.mMaxVersion < 0))
 				os << _("Any") << "\t\t";
 			else if ((tr.mMinVersion >= 0) && (tr.mMaxVersion < 0))
 				os << autosprintf(_("Minimum: %.4f"), tr.mMinVersion);
 			else if ((tr.mMinVersion < 0) && (tr.mMaxVersion >= 0))
				os << autosprintf(_("Maximum: %.4f"), tr.mMaxVersion);
 			else
 				os << autosprintf("%.4f", tr.mMinVersion) << " - " << autosprintf("%.4f", tr.mMaxVersion);

			os << "\t";

			if (tr.mMinVerUse < 0)
 				os << _("Any") << "\t";
			else
				os << autosprintf("%.4f", tr.mMinVerUse);

			os << "\t" << (tr.mBan ? _("Yes") : _("No")) << "\t\t" << (tr.mEnable ? _("On") : _("Off"));
			return os;
		}

	}; // namespace nTables
}; // namespace nVerliHub
