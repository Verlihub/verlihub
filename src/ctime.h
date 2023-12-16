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

#ifndef NUTILS_CTIME_H
#define NUTILS_CTIME_H

#ifdef HAVE_CONFIG_H
	#include <config.h>
#endif

#include <ostream>
#include <time.h>

/*
#if defined _WIN32
#include <Winsock2.h>
#endif
*/

#include <string>
#include <sys/time.h>

//#ifndef _WIN32
#define __int64 long long
//#endif

namespace nVerliHub {
	namespace nUtils {

class cTime : public timeval
{
	public:
	~cTime();
	cTime(){Get();}
	cTime(double sec){tv_sec=(long)sec; tv_usec=long((sec-tv_sec)*1000000);}
	cTime(long sec, long usec=0){tv_sec=sec; tv_usec=usec;}
	int operator> (const cTime &t) const { if(tv_sec > t.tv_sec) return 1; if(tv_sec < t.tv_sec) return 0; return (tv_usec > t.tv_usec);}
	int operator>= (const cTime &t) const { if(tv_sec > t.tv_sec) return 1; if(tv_sec < t.tv_sec) return 0; return (tv_usec >= t.tv_usec);}
	int operator< (const cTime &t) const { if(tv_sec < t.tv_sec) return 1; if(tv_sec > t.tv_sec) return 0; return (tv_usec < t.tv_usec);}
	int operator<= (const cTime &t) const { if(tv_sec < t.tv_sec) return 1; if(tv_sec > t.tv_sec) return 0; return (tv_usec <= t.tv_usec);}
	int operator== (const cTime &t) const { return ((tv_usec == t.tv_usec) && (tv_sec == t.tv_sec));}
	cTime & Get(){gettimeofday(this,NULL);return *this;}
	cTime   operator- (const cTime &t) const {long sec = tv_sec-t.tv_sec; long usec=tv_usec-t.tv_usec; return cTime(sec,usec).Normalize();}
	cTime   operator+ (const cTime &t) const {long sec = tv_sec+t.tv_sec; long usec=tv_usec+t.tv_usec; return cTime(sec,usec).Normalize();}
	cTime   operator+ (int _sec) const {long sec = tv_sec+_sec; return cTime(sec,tv_usec).Normalize();}
	cTime   operator- (int _sec) const {long sec = tv_sec-_sec; return cTime(sec,tv_usec).Normalize();}
	cTime & operator-= (const cTime &t){tv_sec-=t.tv_sec;tv_usec-=t.tv_usec; Normalize(); return *this;}

	cTime & operator+= (const cTime &t){tv_sec+=t.tv_sec;tv_usec+=t.tv_usec; Normalize(); return *this;}
	cTime & operator+= (int msec){tv_usec+=1000*msec; Normalize(); return *this;}
	cTime & operator+= (long usec){tv_usec+=usec; Normalize(); return *this;}

	cTime & operator/= (int i){long sec=tv_sec/i; tv_usec+=1000000*(tv_sec % i); tv_usec/=i; tv_sec=sec; Normalize(); return *this;}
	cTime & operator*= (int i){tv_sec*=i;tv_usec*=i;Normalize(); return *this;}
	cTime   operator/ (int i) const {long sec=tv_sec/i; long usec=tv_usec+1000000*(tv_sec % i); usec/=i; return cTime(sec,usec).Normalize();}
	cTime   operator* (int i) const {long sec=tv_sec*i; long usec=tv_usec*i; return cTime(sec,usec).Normalize();}
	operator double(){ return double(tv_sec)+double(tv_usec)/1000000.;}
	operator long() const { return long(tv_sec)*1000000+long(tv_usec);}
	operator bool() const { return !(!tv_sec && !tv_usec);}
	int operator! () const { return !tv_sec && !tv_usec;}
	long Sec() const { return tv_sec; }
	__int64 MiliSec() { return (__int64)(tv_sec)*1000+(__int64)(tv_usec)/1000; }

	cTime & Normalize()
	{
		if(tv_usec >= 1000000 || tv_usec <= -1000000)
		{
			tv_sec += tv_usec/1000000;
			tv_usec %= 1000000;
		}
		if( tv_sec < 0 && tv_usec > 0)
		{
			tv_sec ++;
			tv_usec-= 1000000;
		}
		if( tv_sec > 0 && tv_usec < 0)
		{
			tv_sec --;
			tv_usec+= 1000000;
		}
		return *this;
	};

};

class cTimePrint: public cTime
{
public:
	cTimePrint():mPrintType(0){}
	cTimePrint(double sec){tv_sec=(long)sec; tv_usec=long((sec-tv_sec)*1000000);mPrintType=0;}
	cTimePrint(long sec, long usec=0){tv_sec=sec; tv_usec=usec;mPrintType=0;}
	cTimePrint(const cTime& t) {timeval& t_val = *this;t_val = t; mPrintType=0;}

private:
	mutable int mPrintType;

public:
	cTimePrint operator/ (int i) const {long sec=tv_sec/i; long usec=tv_usec+1000000*(tv_sec % i); usec/=i; return cTimePrint(cTime(sec,usec).Normalize());}
	const cTimePrint & AsDate() const { mPrintType=1; return *this;}
	const cTimePrint & AsPeriod() const { mPrintType=2; return *this;}
	//operator cTime() {return *this;}
	std::string AsString() const;
	friend std::ostream & operator<< (std::ostream &os, const cTimePrint &t);
};

	}; // namespace nUtils
}; // namespace nVerliHub
#endif
