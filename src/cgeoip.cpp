/*
	Copyright (C) 2003-2005 Daniel Muller, dan at verliba dot cz
	Copyright (C) 2006-2017 Verlihub Team, info at verlihub dot net

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

#include "cgeoip.h"
#include <sstream>
#include <iostream>
#include <libgen.h>
#include <stdio.h>
#include <unistd.h>

using namespace std;

namespace nVerliHub {
	namespace nUtils {

#ifdef HAVE_LIBGEOIP
cGeoIP::cGeoIP():
	cObj("cGeoIP"),
	mGICO(TryCountryDB(GEOIP_STANDARD)),
	mGICI(TryCityDB(GEOIP_STANDARD))
{}

cGeoIP::~cGeoIP()
{
	if (mGICO)
		GeoIP_delete(mGICO);

	if (mGICI)
		GeoIP_delete(mGICI);
}
#endif

bool cGeoIP::GetCC(const string &host, string &cc)
{
	if (host.substr(0, 4) == "127.") {
		cc = "L1";
		return true;
	}

	unsigned long sip = IPToNum(host);

	if ((sip >= 167772160UL && sip <= 184549375UL) || (sip >= 2886729728UL && sip <= 2887778303UL) || (sip >= 3232235520UL && sip <= 3232301055UL)) {
		cc = "P1";
		return true;
	}

	bool res = false;
	string code = "--";

#ifdef HAVE_LIBGEOIP
	if (mGICO) {
		const char *geo_code = GeoIP_country_code_by_name(mGICO, host.c_str());

		if (geo_code) {
			code = geo_code;
			res = true;
		}
	}
#endif

	cc = code;
	return res;
}

bool cGeoIP::GetCN(const string &host, string &cn)
{
	if (host.substr(0, 4) == "127.") {
		cn = "Local Network";
		return true;
	}

	unsigned long sip = IPToNum(host);

	if ((sip >= 167772160UL && sip <= 184549375UL) || (sip >= 2886729728UL && sip <= 2887778303UL) || (sip >= 3232235520UL && sip <= 3232301055UL)) {
		cn = "Private Network";
		return true;
	}

	bool res = false;
	string name = "--";

#ifdef HAVE_LIBGEOIP
	if (mGICO) {
		const char *geo_name = GeoIP_country_name_by_name(mGICO, host.c_str());

		if (geo_name) {
			name = geo_name;
			res = true;
		}
	}
#endif

	cn = name;
	return res;
}

bool cGeoIP::GetCity(string &geo_city, const string &host, const string &db)
{
	if (host.substr(0, 4) == "127.") {
		geo_city = "Local Network";
		return true;
	}

	unsigned long sip = IPToNum(host);

	if ((sip >= 167772160UL && sip <= 184549375UL) || (sip >= 2886729728UL && sip <= 2887778303UL) || (sip >= 3232235520UL && sip <= 3232301055UL)) {
		geo_city = "Private Network";
		return true;
	}

	bool res = false, own = false;
	string city = "--";

#ifdef HAVE_LIBGEOIP
	GeoIP *gi;

	if (!db.empty() && FileExists(db.c_str())) {
		gi = GeoIP_open(db.c_str(), GEOIP_STANDARD);

		if (gi)
			own = true;
	}

	if (own ? gi : mGICI) {
		GeoIPRecord *gir = GeoIP_record_by_name(own ? gi : mGICI, host.c_str());

		if (gir) {
			if (gir->city) {
				city = gir->city;
				res = true;
			}

			GeoIPRecord_delete(gir);
		}

		if (own)
			GeoIP_delete(gi);
	}
#endif

	geo_city = city;
	return res;
}

bool cGeoIP::GetGeoIP(string &geo_host, string &geo_ran_lo, string &geo_ran_hi, string &geo_cc, string &geo_ccc, string &geo_cn, string &geo_reg_code, string &geo_reg_name, string &geo_tz, string &geo_cont, string &geo_city, string &geo_post, float &geo_lat, float &geo_lon, int &geo_met, int &geo_area, const string &host, const string &db)
{
	if (host.substr(0, 4) == "127.") {
		geo_ran_lo = "127.0.0.0";
		geo_ran_hi = "127.255.255.255";
		geo_cc = "L1";
		geo_cn = "Local Network";
		geo_city = "Local Network";
		geo_host = host;
		return true;
	}

	unsigned long sip = IPToNum(host);

	if (sip >= 167772160UL && sip <= 184549375UL) {
		geo_ran_lo = "10.0.0.0";
		geo_ran_hi = "10.255.255.255";
		geo_cc = "P1";
		geo_cn = "Private Network";
		geo_city = "Private Network";
		geo_host = host;
		return true;
	}

	if (sip >= 2886729728UL && sip <= 2887778303UL) {
		geo_ran_lo = "172.16.0.0";
		geo_ran_hi = "172.31.255.255";
		geo_cc = "P1";
		geo_cn = "Private Network";
		geo_city = "Private Network";
		geo_host = host;
		return true;
	}

	if (sip >= 3232235520UL && sip <= 3232301055UL) {
		geo_ran_lo = "192.168.0.0";
		geo_ran_hi = "192.168.255.255";
		geo_cc = "P1";
		geo_cn = "Private Network";
		geo_city = "Private Network";
		geo_host = host;
		return true;
	}

	bool res = false, own = false;

#ifdef HAVE_LIBGEOIP
	GeoIP *gi;

	if (!db.empty() && FileExists(db.c_str())) {
		gi = GeoIP_open(db.c_str(), GEOIP_STANDARD);

		if (gi)
			own = true;
	}

	if (own ? gi : mGICI) {
		GeoIPRecord *gir = GeoIP_record_by_name(own ? gi : mGICI, host.c_str());

		if (gir) {
			char **ran = GeoIP_range_by_ip(own ? gi : mGICI, host.c_str());

			if (ran && ran[0] && ran[1]) {
				geo_ran_lo = ran[0];
				geo_ran_hi = ran[1];
			}

			if (gir->country_code)
				geo_cc = gir->country_code;

			if (gir->country_code3)
				geo_ccc = gir->country_code3;

			if (gir->country_name)
				geo_cn = gir->country_name;

			if (gir->region)
				geo_reg_code = gir->region;

			if (gir->country_code && gir->region) {
				const char *reg = GeoIP_region_name_by_code(gir->country_code, gir->region);

				if (reg)
					geo_reg_name = reg;

				const char *tz = GeoIP_time_zone_by_country_and_region(gir->country_code, gir->region);

				if (tz)
					geo_tz = tz;
			}

			if (gir->continent_code)
				geo_cont = gir->continent_code;

			if (gir->city)
				geo_city = gir->city;

			if (gir->postal_code)
				geo_post = gir->postal_code;

			geo_lat = gir->latitude;
			geo_lon = gir->longitude;
			geo_met = gir->metro_code;
			geo_area = gir->area_code;
			GeoIPRecord_delete(gir);
			geo_host = host;
			res = true;
		}

		if (own)
			GeoIP_delete(gi);
	}
#endif

	return res;
}

#ifdef HAVE_LIBGEOIP
GeoIP *cGeoIP::TryCountryDB(int flags)
{
	// todo: try more directories
	mGICO = GeoIP_new(flags);

	if (!mGICO)
		vhLog(1) << "Database not detected: Maxmind GeoIP Country" << endl;

	return mGICO;
}

GeoIP *cGeoIP::TryCityDB(int flags)
{
	if (GeoIP_db_avail(GEOIP_CITY_EDITION_REV1))
		mGICI = GeoIP_open_type(GEOIP_CITY_EDITION_REV1, flags);
	else if (GeoIP_db_avail(GEOIP_CITY_EDITION_REV0))
		mGICI = GeoIP_open_type(GEOIP_CITY_EDITION_REV0, flags);

	if (!mGICI) {
		char *path = GeoIPDBFileName[GEOIP_COUNTRY_EDITION];

		if (path) {
			char *dir = dirname(path);

			if (dir) {
				string lite = dir;

				if (!lite.empty()) {
					lite += "/GeoLiteCity.dat";

					if (FileExists(lite.c_str()))
						mGICI = GeoIP_open(lite.c_str(), flags);

					if (!mGICI) {
						string full = dir;
						full += "/GeoIPCity.dat";

						if (FileExists(full.c_str()))
							mGICI = GeoIP_open(full.c_str(), flags);
					}
				}
			}
		}
	}

	if (!mGICI && FileExists("/usr/share/GeoIP/GeoLiteCity.dat"))
		mGICI = GeoIP_open("/usr/share/GeoIP/GeoLiteCity.dat", flags);

	if (!mGICI && FileExists("/usr/share/GeoIP/GeoIPCity.dat"))
		mGICI = GeoIP_open("/usr/share/GeoIP/GeoIPCity.dat", flags);

	if (!mGICI && FileExists("/usr/local/share/GeoIP/GeoLiteCity.dat"))
		mGICI = GeoIP_open("/usr/local/share/GeoIP/GeoLiteCity.dat", flags);

	if (!mGICI && FileExists("/usr/local/share/GeoIP/GeoIPCity.dat"))
		mGICI = GeoIP_open("/usr/local/share/GeoIP/GeoIPCity.dat", flags);

	if (!mGICI && FileExists("./GeoLiteCity.dat"))
		mGICI = GeoIP_open("./GeoLiteCity.dat", flags);

	if (!mGICI && FileExists("./GeoIPCity.dat"))
		mGICI = GeoIP_open("./GeoIPCity.dat", flags);

	if (!mGICI)
		vhLog(1) << "Database not detected: Maxmind GeoIP City" << endl;

	return mGICI;
}
#endif

bool cGeoIP::FileExists(const char *name)
{
	return access(name, 0) != -1;
}

unsigned long cGeoIP::IPToNum(const string &ip)
{
	int i;
	char c;
	istringstream is(ip);
	unsigned long mask = 0;

	is >> i >> c;
	mask += i & 0xFF;
	mask <<= 8;

	is >> i >> c;
	mask += i & 0xFF;
	mask <<= 8;

	is >> i >> c;
	mask += i & 0xFF;
	mask <<= 8;

	is >> i;
	mask += i & 0xFF;

	return mask;
}

	}; // namespace nUtils
}; // namespace nVerliHub
