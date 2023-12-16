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

#ifndef CLOG_H
#define CLOG_H
#ifdef ENABLE_SYSLOG

#include <ostream>
#include <streambuf>
#include <string>

#include <syslog.h>

namespace nVerliHub {
	namespace nLog {

class cSyslogStream;

class cSyslogStreamBuf: public std::streambuf
{
public:
	explicit cSyslogStreamBuf(const std::string& name, bool is_daemon):
		std::basic_streambuf<char>()
	{
		openlog(name.size() ? name.data() : NULL, LOG_PID, is_daemon ? LOG_DAEMON : LOG_USER);
		mLevel = LOG_INFO;
	}
	~cSyslogStreamBuf() { closelog(); }

protected:
	int_type overflow(int_type c = traits_type::eof());
	int sync();

	friend class cSyslogStream;
	void SetLevel(int level, bool is_error);

private:
	int mLevel;

	std::string buffer;
};

class cSyslogStream: public std::ostream
{
public:
	explicit cSyslogStream(const std::string &name = "", bool is_daemon = 1):
		std::ostream(&streambuf),
		streambuf(name, is_daemon)
	{ }

	void SetLevel(int level, bool is_error) { streambuf.SetLevel(level, is_error); }

private:
	cSyslogStreamBuf streambuf;
};

	}
}

#endif // ENABLE_SYSLOG
#endif // CLOG_H
