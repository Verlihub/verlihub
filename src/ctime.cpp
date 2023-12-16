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

#include "ctime.h"

#ifdef HAVE_CONFIG_H
	#include <config.h>
#endif

#include <sstream>
#include <string.h>

/*
#if defined _WIN32
	#include <windows.h>
#endif
*/

#include "i18n.h"

using namespace std;

namespace nVerliHub {
	namespace nUtils {

cTime::~cTime()
{}

string cTimePrint::AsString() const
{
	ostringstream os;
	os << (*this);
	return os.str();
}

std::ostream& operator<<(std::ostream &os, const cTimePrint &t)
{
	#define CTIME_BUFFSIZE 26
	static char buf[CTIME_BUFFSIZE + 1];
	long n, rest, i;
	ostringstream ostr;

	switch (t.mPrintType) {
		case 1:
			strftime(buf, CTIME_BUFFSIZE + 1, "%Y-%m-%d %H:%M:%S", localtime((const time_t*) & (t.tv_sec)));
			os << buf;
			break;

		case 2:
			rest = t.tv_sec;
			i = 0;
			n = rest / (24 * 3600 * 7);
			rest %= (24 * 3600 * 7);

			if (n > 0) {
				++i;
				ostr << ' ' << autosprintf(ngettext("%ld week", "%ld weeks", n), n);
			}

			n = rest / (24 * 3600);
			rest %= (24 * 3600);

			if (n > 0) {
				++i;
				ostr << ' ' << autosprintf(ngettext("%ld day", "%ld days", n), n);
			}

			n = rest / 3600;
			rest %= 3600;

			if ((n > 0) && (++i <= 2))
				ostr << ' ' << autosprintf(ngettext("%ld hour", "%ld hours", n), n);

			n = rest / 60;
			rest %= 60;

			if ((n > 0) && (++i <= 2))
				ostr << ' ' << autosprintf(ngettext("%ld min", "%ld mins", n), n);

			n = rest;
			rest = 0;

			if ((n > 0) && (++i <= 2))
				ostr << ' ' << autosprintf(ngettext("%ld sec", "%ld secs", n), n);

			n = (int)t.tv_usec / 1000;

			if ((n > 0) && (++i <= 2))
				ostr << ' ' << autosprintf(ngettext("%ld msec", "%ld msecs", n), n);

			//n = (int)t.tv_usec % 1000;

			//if ((n > 0) && (++i <= 2))
				//ostr << ' ' << autosprintf(ngettext("%ld usec", "%ld usecs", n), n);

			if (ostr.str().empty())
				os << autosprintf(_("%d msecs"), 0); // _("%d usecs")
			else
				os << ostr.str().substr(1); // strip space

			break;

		default:
			#ifdef HAVE_OPENBSD
				os << ' ' << autosprintf(ngettext("%lld sec", "%lld secs", t.tv_sec), t.tv_sec);
			#else
				os << ' ' << autosprintf(ngettext("%ld sec", "%ld secs", t.tv_sec), t.tv_sec);
			#endif

			os << ' ' << autosprintf(ngettext("%ld usec", "%ld usecs", t.tv_usec), t.tv_usec);
			break;
	}

	return os;
};

	}; // namespace nUtils
}; // namespace nVerliHub
