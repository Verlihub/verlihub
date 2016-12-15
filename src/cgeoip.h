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

#ifndef NUTILSCGEOIP_H
#define NUTILSCGEOIP_H
#include <string>
#include "cobj.h"
#ifdef HAVE_LIBGEOIP
#include <GeoIP.h>
#include <GeoIPCity.h>
#endif

using std::string;

namespace nVerliHub {
	namespace nUtils {
		// todo: add to group core

		/*
			Wrapper class for Maxmind GeoIP library.
			This class is used to get the country code of an IP address.
			Author: Daniel Muller
		 */
		class cGeoIP: public cObj
		{
			public:
				// class constructor
				cGeoIP();

				// class destructor
				~cGeoIP();

				/*
					Return country code for the given hostname.
					host = The hostname.
					cc = String where to store country code.
					return = True if it is possible to get country code, false otherwise.
				*/
				bool GetCC(const string &host, string &cc);

				/*
					Return country name for the given hostname.
					host = The hostname.
					cn = String where to store country name.
					return = True if it is possible to get country name, false otherwise.
				*/
				bool GetCN(const string &host, string &cn);

				/*
					Return city for the given hostname.
					geo_city = String where to store city.
					host = The hostname.
					db = Alternative database name.
					return = True if it is possible to get city, false otherwise.
				*/
				bool GetCity(string &geo_city, const string &host, const string &db = "");

				/*
					Return GeoIP information for the given hostname.
					geo_host = String where to store hostname.
					geo_ran_lo = String where to store low range.
					geo_ran_hi = String where to store high range.
					geo_cc = String where to store 2 letter country code.
					geo_ccc = String where to store 3 letter country code.
					geo_cn = String where to store country name.
					geo_reg_code = String where to store region code.
					geo_reg_name = String where to store region name.
					geo_tz = String where to store time zone.
					geo_cont = String where to store continent code.
					geo_city = String where to store city.
					geo_post = String where to store postal code.
					geo_lat = Float where to store latitude.
					geo_lon = Float where to store longitude.
					geo_met = Integer where to store metro code.
					geo_area = Integer where to store area code.
					host = The hostname.
					db = Alternative database name.
					return = True if record was found, false otherwise.
				*/
				bool GetGeoIP(string &geo_host, string &geo_ran_lo, string &geo_ran_hi, string &geo_cc, string &geo_ccc, string &geo_cn, string &geo_reg_code, string &geo_reg_name, string &geo_tz, string &geo_cont, string &geo_city, string &geo_post, float &geo_lat, float &geo_lon, int &geo_met, int &geo_area, const string &host, const string &db = "");
			private:
				#ifdef HAVE_LIBGEOIP
				// pointer to geoip country instance
				GeoIP *mGICO;

				// pointer to geoip city instance
				GeoIP *mGICI;

				// country database initialization function
				GeoIP *TryCountryDB(int flags);

				// city database initialization function
				GeoIP *TryCityDB(int flags);
				#endif

				// helper functions
				bool FileExists(const char *name);
				static unsigned long IPToNum(const string &ip);
		};
	}; // namespace nUtils
}; // namespace nVerliHub

#endif
