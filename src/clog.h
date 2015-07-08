/*
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

#ifndef CLOG_H
#define CLOG_H
#ifdef ENABLE_SYSLOG

#include <ostream>
#include <streambuf>
#include <string>

#include <syslog.h>


class syslog_stream;

class syslog_streambuf: public std::streambuf
{
public:
	explicit syslog_streambuf(const std::string& name, bool is_daemon):
		std::basic_streambuf<char>()
	{
		openlog(name.size() ? name.data() : NULL, LOG_PID, is_daemon ? LOG_DAEMON : LOG_USER);
	}
	~syslog_streambuf() { closelog(); }

protected:
	int_type overflow(int_type c = traits_type::eof());
	int sync();

	friend class syslog_stream;
	void set_level(int new_level, bool is_error);

private:
	int level = LOG_INFO;

	std::string buffer;
};

class syslog_stream: public std::ostream
{
public:
	explicit syslog_stream(const std::string& name = std::string(), bool is_daemon = 1):
		std::ostream(&streambuf),
		streambuf(name, is_daemon)
	{ }

	void set_level(int level, bool is_error) { streambuf.set_level(level, is_error); }

private:
	syslog_streambuf streambuf;
};

#endif // ENABLE_SYSLOG
#endif // CLOG_H
