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

#ifndef COBJ_H
#define COBJ_H

#include <iostream>
#include <sstream>

//#ifndef _WIN32
#define __int64 long long
//#endif

#define vhLog(level) if (Log(level)) LogStream()
#define vhErr(level) if (ErrLog(level)) LogStream()

using namespace std;

/** Base class for every object, inque method of logging events (virtual)
    @author Daniel Muller
  */
namespace nVerliHub {

class cObj
{
	public:
		/**  */
		cObj();
		/** with name constructor */
		cObj(const char *name);
		/** destructor */
		virtual ~cObj();

		/** Logs an event into a given stream */
		virtual int StrLog(ostream & ostr, int level);
		/** Log or not an event?? */
		int Log(int level);
		/** error Log or not an event */
		int ErrLog(int level);

		/** No descriptions */
		void SetClassName(const char *name){mClassName=name;}
		/** return selected log stream */
		ostream & LogStream();
		/** stderr log stream */
		virtual ostream & ErrLog();
		/** stdout log stream */
		virtual ostream & Log();
#ifdef ENABLE_SYSLOG
	protected:
		/** syslog log stream */
		virtual ostream & SysLog(int, bool is_error);
#endif

	public:
		/** the Ascii representaition of class name */
		const char * mClassName;
		/** all events higher than this are discarded */
		static int msLogLevel;
#ifdef ENABLE_SYSLOG
		/** send log messages to syslog */
		static bool msUseSyslog;
		/** syslog identifier */
		static string msSyslogIdent;
#endif
		/** a string stream variable to log */
		ostream *mToLog;
		static const string mEmpty;
		static unsigned int GetCount() {return msCounterObj;}
		
	private:
		/** object counter */
		static unsigned int msCounterObj;

};
} // namespace nVerliHub
#endif
