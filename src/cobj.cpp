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

#include "cobj.h"
#include "clog.h"
#include "ctime.h"
#include <iostream>
using namespace std;

#ifndef left
#define left ""
#endif
#ifndef right
#define right ""
#endif

namespace nVerliHub {
	int cObj::msLogLevel = 4;
#ifdef ENABLE_SYSLOG
	bool cObj::msUseSyslog = 0;
	string cObj::msSyslogIdent = "verlihub";
#endif
	unsigned int cObj::msCounterObj = 0;
	const string cObj::mEmpty;
	using nUtils::cTimePrint;
/** with name constructor */
cObj::cObj(const char *name) : mClassName(name), mToLog(&cout)
{
	msCounterObj++;//cout << "Constructor cObj(char*), count = " << msCounterObj << endl;
}

cObj::cObj(): mClassName("Obj"), mToLog(&cout)
{
	msCounterObj++;//cout << "Constructor cObj(), count = " << msCounterObj << endl;
}

cObj::~cObj()
{
	msCounterObj--;//cout << "Destructor cObj, count = " << msCounterObj << endl;
}

/** log something into a given stream */
int cObj::StrLog(ostream &ostr, int level)
{
	if(level <= msLogLevel)
	{
#ifdef ENABLE_SYSLOG
		if (msUseSyslog)
		{
			ostr << " (" << level << ") "
			<< mClassName << " # ";
		}
		else
#endif
		{
			cTimePrint now;
			ostr << " (" << level << ") ";
			if(1)
			{
				ostr.width(26);
				ostr << left << now.AsDate() << " # ";
			}
			ostr.width(15);
			ostr << right << mClassName;
			ostr.width(0);
			ostr << left << " - " ;
		}
		return 1;
	}
	return 0;
}

int cObj::Log(int level)
{
#ifdef ENABLE_SYSLOG
	if (msUseSyslog)
		mToLog = &SysLog(level, 0);
	else
#endif
	mToLog = &Log();
	return StrLog(*mToLog, level);
}

/** error Log or not an event */
int cObj::ErrLog(int level)
{
#ifdef ENABLE_SYSLOG
	if (msUseSyslog)
		mToLog = &SysLog(level, 1);
	else
#endif
	mToLog = &ErrLog();
	return StrLog(*mToLog, level);
}

/** return the streal where logging  goes to */
ostream & cObj::Log()
{
	return cout;
}

/** error log stream */
ostream & cObj::ErrLog()
{
	return cerr;
}

/** return selected log stream */
ostream & cObj::LogStream()
{
	return *mToLog;
}

#ifdef ENABLE_SYSLOG
/** syslog log stream */
ostream & cObj::SysLog(int level, bool is_error)
{
	static nLog::cSyslogStream sysLog(msSyslogIdent, 1);
	sysLog.SetLevel(level, is_error);
	return sysLog;
}
#endif

} // namespace nVerliHub
