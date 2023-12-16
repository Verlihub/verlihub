/*
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

#ifdef ENABLE_SYSLOG
#include "clog.h"

namespace nVerliHub {
	namespace nLog {

cSyslogStreamBuf::int_type cSyslogStreamBuf::overflow(int_type c)
{
	if(traits_type::eq_int_type(c, traits_type::eof()))
		sync();
	else buffer += traits_type::to_char_type(c);

	return c;
}

int cSyslogStreamBuf::sync()
{
	if(buffer.size())
	{
		syslog(mLevel, "%s", buffer.data());

		buffer.clear();
	}
	return 0;
}

void cSyslogStreamBuf::SetLevel(int level, bool is_error)
{
	int _lvl = mLevel;
	if (is_error)
	{
		_lvl = level < 2 ? LOG_ERR : LOG_WARNING;
	}
	else
	{
		switch (level) {
		case 0:
			_lvl = LOG_INFO;
			break;
		case 1:
			_lvl = LOG_NOTICE;
			break;
		default:
			_lvl = LOG_DEBUG;
			break;
		}
	}
	if (_lvl != mLevel)
		sync();
	mLevel = _lvl;
}

	}
}

#endif
