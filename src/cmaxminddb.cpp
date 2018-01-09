/*
	Copyright (C) 2003-2005 Daniel Muller, dan at verliba dot cz
	Copyright (C) 2006-2018 Verlihub Team, info at verlihub dot net

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

#include "cmaxminddb.h"
#include "cdcconsole.h"
#include "cbanlist.h"
#include "stringutils.h"
#include <sstream>
#include <iostream>
#include <libgen.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>

using namespace std;

namespace nVerliHub {
	namespace nUtils {

cMaxMindDB::cMaxMindDB(cServerDC *mS):
	cObj("cMaxMindDB"),
	mServ(mS),
	mDBCO(NULL),
	mDBCI(NULL),
	mDBAS(NULL)
{
	mDBCO = TryCountryDB(MMDB_MODE_MMAP);
	mDBCI = TryCityDB(MMDB_MODE_MMAP);
	mDBAS = TryASNDB(MMDB_MODE_MMAP);
}

cMaxMindDB::~cMaxMindDB()
{
	if (mDBCO) {
		MMDB_close(mDBCO);
		free(mDBCO);
		mDBCO = NULL;
	}

	if (mDBCI) {
		MMDB_close(mDBCI);
		free(mDBCI);
		mDBCI = NULL;
	}

	if (mDBAS) {
		MMDB_close(mDBAS);
		free(mDBAS);
		mDBAS = NULL;
	}
}

bool cMaxMindDB::GetCC(const string &host, string &cc)
{
	if (host.substr(0, 4) == "127.") {
		cc = "L1";
		return true;
	}

	unsigned long sip = cBanList::Ip2Num(host);

	if ((sip >= 167772160UL && sip <= 184549375UL) || (sip >= 2886729728UL && sip <= 2887778303UL) || (sip >= 3232235520UL && sip <= 3232301055UL)) {
		cc = "P1";
		return true;
	}

	bool res = false;
	string code = "--";

	if (mDBCO) {
		int gai_err, mmdb_err;
		MMDB_lookup_result_s dat = MMDB_lookup_string(mDBCO, host.c_str(), &gai_err, &mmdb_err);

		if ((gai_err == 0) && (mmdb_err == MMDB_SUCCESS) && dat.found_entry) {
			MMDB_entry_data_s ent;

			if ((MMDB_get_value(&dat.entry, &ent, "country", "iso_code", NULL) == MMDB_SUCCESS) && ent.has_data && (ent.type == MMDB_DATA_TYPE_UTF8_STRING) && (ent.data_size > 0)) { // country code
				string conv, back;
				conv.assign((const char*)ent.utf8_string, ent.data_size);
				code = FromUTF8(conv, back, (mServ->mC.hub_encoding.size() ? mServ->mC.hub_encoding : DEFAULT_HUB_ENCODING));
				res = true;
			}
		}
	}

	cc = code;
	return res;
}

bool cMaxMindDB::GetCN(const string &host, string &cn)
{
	if (host.substr(0, 4) == "127.") {
		cn = "Local Network";
		return true;
	}

	unsigned long sip = cBanList::Ip2Num(host);

	if ((sip >= 167772160UL && sip <= 184549375UL) || (sip >= 2886729728UL && sip <= 2887778303UL) || (sip >= 3232235520UL && sip <= 3232301055UL)) {
		cn = "Private Network";
		return true;
	}

	bool res = false;
	string name = "--";

	if (mDBCO) {
		int gai_err, mmdb_err;
		MMDB_lookup_result_s dat = MMDB_lookup_string(mDBCO, host.c_str(), &gai_err, &mmdb_err);

		if ((gai_err == 0) && (mmdb_err == MMDB_SUCCESS) && dat.found_entry) {
			MMDB_entry_data_s ent;

			if ((MMDB_get_value(&dat.entry, &ent, "country", "names", (mServ->mC.mmdb_names_lang.size() ? mServ->mC.mmdb_names_lang.c_str() : "en"), NULL) == MMDB_SUCCESS) && ent.has_data && (ent.type == MMDB_DATA_TYPE_UTF8_STRING) && (ent.data_size > 0)) { // english country name
				string conv, back;
				conv.assign((const char*)ent.utf8_string, ent.data_size);
				name = FromUTF8(conv, back, (mServ->mC.hub_encoding.size() ? mServ->mC.hub_encoding : DEFAULT_HUB_ENCODING));
				res = true;
			}
		}
	}

	cn = name;
	return res;
}

bool cMaxMindDB::GetCity(string &geo_city, const string &host, const string &db)
{
	if (host.substr(0, 4) == "127.") {
		geo_city = "Local Network";
		return true;
	}

	unsigned long sip = cBanList::Ip2Num(host);

	if ((sip >= 167772160UL && sip <= 184549375UL) || (sip >= 2886729728UL && sip <= 2887778303UL) || (sip >= 3232235520UL && sip <= 3232301055UL)) {
		geo_city = "Private Network";
		return true;
	}

	string city = "--";
	bool res = false, ok = false;
	MMDB_s *mmdb = NULL;

	if (db.size() && FileExists(db.c_str())) {
		mmdb = (MMDB_s*)malloc(sizeof(MMDB_s));

		if (mmdb)
			ok = MMDB_open(db.c_str(), MMDB_MODE_MMAP, mmdb) == MMDB_SUCCESS;
	}

	if ((ok && mmdb) || mDBCI) {
		int gai_err, mmdb_err;
		MMDB_lookup_result_s dat = MMDB_lookup_string((ok ? mmdb : mDBCI), host.c_str(), &gai_err, &mmdb_err);

		if ((gai_err == 0) && (mmdb_err == MMDB_SUCCESS) && dat.found_entry) {
			MMDB_entry_data_s ent;

			if ((MMDB_get_value(&dat.entry, &ent, "city", "names", (mServ->mC.mmdb_names_lang.size() ? mServ->mC.mmdb_names_lang.c_str() : "en"), NULL) == MMDB_SUCCESS) && ent.has_data && (ent.type == MMDB_DATA_TYPE_UTF8_STRING) && (ent.data_size > 0)) { // english city name
				string conv, back;
				conv.assign((const char*)ent.utf8_string, ent.data_size);
				city = FromUTF8(conv, back, (mServ->mC.hub_encoding.size() ? mServ->mC.hub_encoding : DEFAULT_HUB_ENCODING));
				res = true;
			}
		}
	}

	if (mmdb) {
		if (ok)
			MMDB_close(mmdb);

		free(mmdb);
	}

	geo_city = city;
	return res;
}

bool cMaxMindDB::GetGeoIP(string &geo_host, string &geo_ran_lo, string &geo_ran_hi, string &geo_cc, string &geo_ccc, string &geo_cn, string &geo_reg_code, string &geo_reg_name, string &geo_tz, string &geo_cont, string &geo_city, string &geo_post, double &geo_lat, double &geo_lon, unsigned short &geo_met, unsigned short &geo_area, const string &host, const string &db)
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

	unsigned long sip = cBanList::Ip2Num(host);

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

	bool res = false, ok = false;
	MMDB_s *mmdb = NULL;

	if (db.size() && FileExists(db.c_str())) {
		mmdb = (MMDB_s*)malloc(sizeof(MMDB_s));

		if (mmdb)
			ok = MMDB_open(db.c_str(), MMDB_MODE_MMAP, mmdb) == MMDB_SUCCESS;
	}

	if ((ok && mmdb) || mDBCI) {
		int gai_err, mmdb_err;
		MMDB_lookup_result_s dat = MMDB_lookup_string((ok ? mmdb : mDBCI), host.c_str(), &gai_err, &mmdb_err);

		if ((gai_err == 0) && (mmdb_err == MMDB_SUCCESS) && dat.found_entry) {
			unsigned long ran_lo = sip, ran_hi = sip; // ip range
			string conv, back = host;
			back += "/";
			back += StringFrom(dat.netmask - (ok ? mmdb->ipv4_start_node.netmask : mDBCI->ipv4_start_node.netmask));

			if (cDCConsole::GetIPRange(back, ran_lo, ran_hi)) {
				cBanList::Num2Ip(ran_lo, geo_ran_lo);
				cBanList::Num2Ip(ran_hi, geo_ran_hi);
				res = true;
			}

			MMDB_entry_data_s ent;

			if ((MMDB_get_value(&dat.entry, &ent, "country", "iso_code", NULL) == MMDB_SUCCESS) && ent.has_data && (ent.type == MMDB_DATA_TYPE_UTF8_STRING) && (ent.data_size > 0)) { // country code
				conv.assign((const char*)ent.utf8_string, ent.data_size);
				back.clear();
				geo_cc = FromUTF8(conv, back, (mServ->mC.hub_encoding.size() ? mServ->mC.hub_encoding : DEFAULT_HUB_ENCODING));
				geo_ccc = geo_cc; // todo: country_code3 no longer supported, get rid of it
				res = true;
			}

			if ((MMDB_get_value(&dat.entry, &ent, "country", "names", (mServ->mC.mmdb_names_lang.size() ? mServ->mC.mmdb_names_lang.c_str() : "en"), NULL) == MMDB_SUCCESS) && ent.has_data && (ent.type == MMDB_DATA_TYPE_UTF8_STRING) && (ent.data_size > 0)) { // english country name
				conv.assign((const char*)ent.utf8_string, ent.data_size);
				back.clear();
				geo_cn = FromUTF8(conv, back, (mServ->mC.hub_encoding.size() ? mServ->mC.hub_encoding : DEFAULT_HUB_ENCODING));
				res = true;
			}

			if ((MMDB_get_value(&dat.entry, &ent, "subdivisions", "0", "iso_code", NULL) == MMDB_SUCCESS) && ent.has_data && (ent.type == MMDB_DATA_TYPE_UTF8_STRING) && (ent.data_size > 0)) { // region code
				conv.assign((const char*)ent.utf8_string, ent.data_size);
				back.clear();
				geo_reg_code = FromUTF8(conv, back, (mServ->mC.hub_encoding.size() ? mServ->mC.hub_encoding : DEFAULT_HUB_ENCODING));
				res = true;
			}

			if ((MMDB_get_value(&dat.entry, &ent, "subdivisions", "0", "names", (mServ->mC.mmdb_names_lang.size() ? mServ->mC.mmdb_names_lang.c_str() : "en"), NULL) == MMDB_SUCCESS) && ent.has_data && (ent.type == MMDB_DATA_TYPE_UTF8_STRING) && (ent.data_size > 0)) { // english region name
				conv.assign((const char*)ent.utf8_string, ent.data_size);
				back.clear();
				geo_reg_name = FromUTF8(conv, back, (mServ->mC.hub_encoding.size() ? mServ->mC.hub_encoding : DEFAULT_HUB_ENCODING));
				res = true;
			}

			if ((MMDB_get_value(&dat.entry, &ent, "location", "time_zone", NULL) == MMDB_SUCCESS) && ent.has_data && (ent.type == MMDB_DATA_TYPE_UTF8_STRING) && (ent.data_size > 0)) { // time zone
				conv.assign((const char*)ent.utf8_string, ent.data_size);
				back.clear();
				geo_tz = FromUTF8(conv, back, (mServ->mC.hub_encoding.size() ? mServ->mC.hub_encoding : DEFAULT_HUB_ENCODING));
				res = true;
			}

			if ((MMDB_get_value(&dat.entry, &ent, "continent", "code", NULL) == MMDB_SUCCESS) && ent.has_data && (ent.type == MMDB_DATA_TYPE_UTF8_STRING) && (ent.data_size > 0)) { // continent code
				conv.assign((const char*)ent.utf8_string, ent.data_size);
				back.clear();
				geo_cont = FromUTF8(conv, back, (mServ->mC.hub_encoding.size() ? mServ->mC.hub_encoding : DEFAULT_HUB_ENCODING));
				res = true;
			}

			if ((MMDB_get_value(&dat.entry, &ent, "city", "names", (mServ->mC.mmdb_names_lang.size() ? mServ->mC.mmdb_names_lang.c_str() : "en"), NULL) == MMDB_SUCCESS) && ent.has_data && (ent.type == MMDB_DATA_TYPE_UTF8_STRING) && (ent.data_size > 0)) { // english city name
				conv.assign((const char*)ent.utf8_string, ent.data_size);
				back.clear();
				geo_city = FromUTF8(conv, back, (mServ->mC.hub_encoding.size() ? mServ->mC.hub_encoding : DEFAULT_HUB_ENCODING));
				res = true;
			}

			if ((MMDB_get_value(&dat.entry, &ent, "postal", "code", NULL) == MMDB_SUCCESS) && ent.has_data && (ent.type == MMDB_DATA_TYPE_UTF8_STRING) && (ent.data_size > 0)) { // postal code
				conv.assign((const char*)ent.utf8_string, ent.data_size);
				back.clear();
				geo_post = FromUTF8(conv, back, (mServ->mC.hub_encoding.size() ? mServ->mC.hub_encoding : DEFAULT_HUB_ENCODING));
				res = true;
			}

			if ((MMDB_get_value(&dat.entry, &ent, "location", "latitude", NULL) == MMDB_SUCCESS) && ent.has_data && (ent.type == MMDB_DATA_TYPE_DOUBLE)) { // latitude, can be negative
				geo_lat = (double)ent.double_value;
				res = true;
			}

			if ((MMDB_get_value(&dat.entry, &ent, "location", "longitude", NULL) == MMDB_SUCCESS) && ent.has_data && (ent.type == MMDB_DATA_TYPE_DOUBLE)) { // longitude, can be negative
				geo_lon = (double)ent.double_value;
				res = true;
			}

			if ((MMDB_get_value(&dat.entry, &ent, "location", "metro_code", NULL) == MMDB_SUCCESS) && ent.has_data && (ent.type == MMDB_DATA_TYPE_UINT16) && (ent.uint16 > 0)) { // metro code
				geo_met = (unsigned short)ent.uint16;
				res = true;
			}

			geo_area = 0; // todo: area_code no longer supported, get rid of it
			geo_host = host;
		}
	}

	if (mmdb) {
		if (ok)
			MMDB_close(mmdb);

		free(mmdb);
	}

	return res;
}

bool cMaxMindDB::GetASN(string &asn_name, const string &host, const string &db)
{
	if (host.substr(0, 4) == "127.") {
		asn_name = "Local Network";
		return true;
	}

	unsigned long sip = cBanList::Ip2Num(host);

	if (sip >= 167772160UL && sip <= 184549375UL) {
		asn_name = "Private Network";
		return true;
	}

	if (sip >= 2886729728UL && sip <= 2887778303UL) {
		asn_name = "Private Network";
		return true;
	}

	if (sip >= 3232235520UL && sip <= 3232301055UL) {
		asn_name = "Private Network";
		return true;
	}

	bool res = false, ok = false;
	MMDB_s *mmdb = NULL;

	if (db.size() && FileExists(db.c_str())) {
		mmdb = (MMDB_s*)malloc(sizeof(MMDB_s));

		if (mmdb)
			ok = MMDB_open(db.c_str(), MMDB_MODE_MMAP, mmdb) == MMDB_SUCCESS;
	}

	if ((ok && mmdb) || mDBAS) {
		int gai_err, mmdb_err;
		MMDB_lookup_result_s dat = MMDB_lookup_string((ok ? mmdb : mDBAS), host.c_str(), &gai_err, &mmdb_err);

		if ((gai_err == 0) && (mmdb_err == MMDB_SUCCESS) && dat.found_entry) {
			/*
				MMDB_entry_data_list_s *ent = NULL;

				if (MMDB_get_entry_data_list(&dat.entry, &ent) == MMDB_SUCCESS) {
					if (ent)
						MMDB_dump_entry_data_list(stdout, ent, 2);
				}

				if (ent)
					MMDB_free_entry_data_list(ent);
			*/

			MMDB_entry_data_s ent;

			if ((MMDB_get_value(&dat.entry, &ent, "autonomous_system_number", NULL) == MMDB_SUCCESS) && ent.has_data && (ent.type == MMDB_DATA_TYPE_UINT32) && (ent.uint32 > 0)) { // asn number
				asn_name = "AS";
				asn_name += StringFrom((unsigned int)ent.uint32);
				res = true;
			}

			if ((MMDB_get_value(&dat.entry, &ent, "autonomous_system_organization", NULL) == MMDB_SUCCESS) && ent.has_data && (ent.type == MMDB_DATA_TYPE_UTF8_STRING) && (ent.data_size > 0)) { // asn organization
				if (asn_name.size())
					asn_name += " ";

				string conv, back;
				conv.assign((const char*)ent.utf8_string, ent.data_size);
				asn_name += FromUTF8(conv, back, (mServ->mC.hub_encoding.size() ? mServ->mC.hub_encoding : DEFAULT_HUB_ENCODING));
				res = true;
			}
		}
	}

	if (mmdb) {
		if (ok)
			MMDB_close(mmdb);

		free(mmdb);
	}

	return res;
}

MMDB_s *cMaxMindDB::TryCountryDB(unsigned int flags)
{
	MMDB_s *mmdb = (MMDB_s*)malloc(sizeof(MMDB_s));

	if (mmdb) {
		int ok = MMDB_FILE_OPEN_ERROR;

		if (mServ->mDBConf.mmdb_path.size()) { // mmdb_path
			string path = mServ->mDBConf.mmdb_path;
			path += "/GeoIP2-Country.mmdb";

			if ((ok != MMDB_SUCCESS) && FileExists(path.c_str()))
				ok = MMDB_open(path.c_str(), flags, mmdb);

			path = mServ->mDBConf.mmdb_path;
			path += "/GeoLite2-Country.mmdb";

			if ((ok != MMDB_SUCCESS) && FileExists(path.c_str()))
				ok = MMDB_open(path.c_str(), flags, mmdb);
		} else { // defaults
			if ((ok != MMDB_SUCCESS) && FileExists("/usr/share/GeoIP/GeoIP2-Country.mmdb"))
				ok = MMDB_open("/usr/share/GeoIP/GeoIP2-Country.mmdb", flags, mmdb);

			if ((ok != MMDB_SUCCESS) && FileExists("/usr/local/share/GeoIP/GeoIP2-Country.mmdb"))
				ok = MMDB_open("/usr/local/share/GeoIP/GeoIP2-Country.mmdb", flags, mmdb);

			if ((ok != MMDB_SUCCESS) && FileExists("./GeoIP2-Country.mmdb"))
				ok = MMDB_open("./GeoIP2-Country.mmdb", flags, mmdb);

			if ((ok != MMDB_SUCCESS) && FileExists("/usr/share/GeoIP/GeoLite2-Country.mmdb"))
				ok = MMDB_open("/usr/share/GeoIP/GeoLite2-Country.mmdb", flags, mmdb);

			if ((ok != MMDB_SUCCESS) && FileExists("/usr/local/share/GeoIP/GeoLite2-Country.mmdb"))
				ok = MMDB_open("/usr/local/share/GeoIP/GeoLite2-Country.mmdb", flags, mmdb);

			if ((ok != MMDB_SUCCESS) && FileExists("./GeoLite2-Country.mmdb"))
				ok = MMDB_open("./GeoLite2-Country.mmdb", flags, mmdb);
		}

		if (ok != MMDB_SUCCESS) {
			if (mmdb) {
				free(mmdb);
				mmdb = NULL;
			}

			vhLog(0) << "Database error: " << MMDB_strerror(ok) << " [ MaxMind Country > http://geolite.maxmind.com/download/geoip/database/GeoLite2-Country.tar.gz ]" << endl;
		} else {
			vhLog(0) << "Database loaded: MaxMind Country > " << mmdb->filename << endl;
		}
	}

	return mmdb;
}

MMDB_s *cMaxMindDB::TryCityDB(unsigned int flags)
{
	MMDB_s *mmdb = (MMDB_s*)malloc(sizeof(MMDB_s));

	if (mmdb) {
		int ok = MMDB_FILE_OPEN_ERROR;

		if (mServ->mDBConf.mmdb_path.size()) { // mmdb_path
			string path = mServ->mDBConf.mmdb_path;
			path += "/GeoIP2-City.mmdb";

			if ((ok != MMDB_SUCCESS) && FileExists(path.c_str()))
				ok = MMDB_open(path.c_str(), flags, mmdb);

			path = mServ->mDBConf.mmdb_path;
			path += "/GeoLite2-City.mmdb";

			if ((ok != MMDB_SUCCESS) && FileExists(path.c_str()))
				ok = MMDB_open(path.c_str(), flags, mmdb);
		} else { // defaults
			if ((ok != MMDB_SUCCESS) && FileExists("/usr/share/GeoIP/GeoIP2-City.mmdb"))
				ok = MMDB_open("/usr/share/GeoIP/GeoIP2-City.mmdb", flags, mmdb);

			if ((ok != MMDB_SUCCESS) && FileExists("/usr/local/share/GeoIP/GeoIP2-City.mmdb"))
				ok = MMDB_open("/usr/local/share/GeoIP/GeoIP2-City.mmdb", flags, mmdb);

			if ((ok != MMDB_SUCCESS) && FileExists("./GeoIP2-City.mmdb"))
				ok = MMDB_open("./GeoIP2-City.mmdb", flags, mmdb);

			if ((ok != MMDB_SUCCESS) && FileExists("/usr/share/GeoIP/GeoLite2-City.mmdb"))
				ok = MMDB_open("/usr/share/GeoIP/GeoLite2-City.mmdb", flags, mmdb);

			if ((ok != MMDB_SUCCESS) && FileExists("/usr/local/share/GeoIP/GeoLite2-City.mmdb"))
				ok = MMDB_open("/usr/local/share/GeoIP/GeoLite2-City.mmdb", flags, mmdb);

			if ((ok != MMDB_SUCCESS) && FileExists("./GeoLite2-City.mmdb"))
				ok = MMDB_open("./GeoLite2-City.mmdb", flags, mmdb);
		}

		if (ok != MMDB_SUCCESS) {
			if (mmdb) {
				free(mmdb);
				mmdb = NULL;
			}

			vhLog(0) << "Database error: " << MMDB_strerror(ok) << " [ MaxMind City > http://geolite.maxmind.com/download/geoip/database/GeoLite2-City.tar.gz ]" << endl;
		} else {
			vhLog(0) << "Database loaded: MaxMind City > " << mmdb->filename << endl;
		}
	}

	return mmdb;
}

MMDB_s *cMaxMindDB::TryASNDB(unsigned int flags)
{
	MMDB_s *mmdb = (MMDB_s*)malloc(sizeof(MMDB_s));

	if (mmdb) {
		int ok = MMDB_FILE_OPEN_ERROR;

		if (mServ->mDBConf.mmdb_path.size()) { // mmdb_path
			string path = mServ->mDBConf.mmdb_path;
			path += "/GeoIP2-ASN.mmdb";

			if ((ok != MMDB_SUCCESS) && FileExists(path.c_str()))
				ok = MMDB_open(path.c_str(), flags, mmdb);

			path = mServ->mDBConf.mmdb_path;
			path += "/GeoLite2-ASN.mmdb";

			if ((ok != MMDB_SUCCESS) && FileExists(path.c_str()))
				ok = MMDB_open(path.c_str(), flags, mmdb);
		} else { // defaults
			if ((ok != MMDB_SUCCESS) && FileExists("/usr/share/GeoIP/GeoIP2-ASN.mmdb"))
				ok = MMDB_open("/usr/share/GeoIP/GeoIP2-ASN.mmdb", flags, mmdb);

			if ((ok != MMDB_SUCCESS) && FileExists("/usr/local/share/GeoIP/GeoIP2-ASN.mmdb"))
				ok = MMDB_open("/usr/local/share/GeoIP/GeoIP2-ASN.mmdb", flags, mmdb);

			if ((ok != MMDB_SUCCESS) && FileExists("./GeoIP2-ASN.mmdb"))
				ok = MMDB_open("./GeoIP2-ASN.mmdb", flags, mmdb);

			if ((ok != MMDB_SUCCESS) && FileExists("/usr/share/GeoIP/GeoLite2-ASN.mmdb"))
				ok = MMDB_open("/usr/share/GeoIP/GeoLite2-ASN.mmdb", flags, mmdb);

			if ((ok != MMDB_SUCCESS) && FileExists("/usr/local/share/GeoIP/GeoLite2-ASN.mmdb"))
				ok = MMDB_open("/usr/local/share/GeoIP/GeoLite2-ASN.mmdb", flags, mmdb);

			if ((ok != MMDB_SUCCESS) && FileExists("./GeoLite2-ASN.mmdb"))
				ok = MMDB_open("./GeoLite2-ASN.mmdb", flags, mmdb);
		}

		if (ok != MMDB_SUCCESS) {
			if (mmdb) {
				free(mmdb);
				mmdb = NULL;
			}

			vhLog(0) << "Database error: " << MMDB_strerror(ok) << " [ MaxMind ASN > http://geolite.maxmind.com/download/geoip/database/GeoLite2-ASN.tar.gz ]" << endl;
		} else {
			vhLog(0) << "Database loaded: MaxMind ASN > " << mmdb->filename << endl;
		}
	}

	return mmdb;
}

void cMaxMindDB::ReloadAll()
{
	if (mDBCO) {
		MMDB_close(mDBCO);
		free(mDBCO);
	}

	mDBCO = TryCountryDB(MMDB_MODE_MMAP);

	if (mDBCI) {
		MMDB_close(mDBCI);
		free(mDBCI);
	}

	mDBCI = TryCityDB(MMDB_MODE_MMAP);

	if (mDBAS) {
		MMDB_close(mDBAS);
		free(mDBAS);
	}

	mDBAS = TryASNDB(MMDB_MODE_MMAP);
}

bool cMaxMindDB::FileExists(const char *name)
{
	return access(name, 0) != -1;
}

unsigned long cMaxMindDB::FileSize(const char *name)
{
	struct stat fs;

	if (stat(name, &fs) == 0)
		return fs.st_size;

	return 0;
}

void cMaxMindDB::ShowInfo(ostream &os)
{
	os << _("MaxMindDB information") << ":\r\n\r\n"; // general
	os << " [*] " << autosprintf(_("Library version: %s"), MMDB_lib_version()) << "\r\n";
	os << " [*] " << autosprintf(_("Database path: %s"), (mServ->mDBConf.mmdb_path.size() ? mServ->mDBConf.mmdb_path.c_str() : _("Not set"))) << "\r\n\r\n";

	os << " " << _("Country database") << ":\r\n\r\n"; // country
	os << " [*] " << autosprintf(_("Status: %s"), (mDBCO ? _("Loaded") : _("Not loaded"))) << "\r\n";

	if (mDBCO) {
		os << " [*] " << autosprintf(_("Type: %s"), mDBCO->metadata.database_type) << "\r\n";
		os << " [*] " << autosprintf(_("Binary version: %d.%d"), (unsigned short)mDBCO->metadata.binary_format_major_version, (unsigned short)mDBCO->metadata.binary_format_minor_version) << "\r\n";
		os << " [*] " << autosprintf(_("Build time: %s"), cTime((long)mDBCO->metadata.build_epoch).AsDate().AsString().c_str()) << "\r\n";
		os << " [*] " << autosprintf(_("Node count: %d"), (unsigned int)mDBCO->metadata.node_count) << "\r\n";
		os << " [*] " << autosprintf(_("File name: %s"), mDBCO->filename) << "\r\n";

		if (FileExists(mDBCO->filename))
			os << " [*] " << autosprintf(_("File size: %s"), convertByte(FileSize(mDBCO->filename)).c_str()) << "\r\n";
	}

	os << "\r\n " << _("City database") << ":\r\n\r\n"; // city
	os << " [*] " << autosprintf(_("Status: %s"), (mDBCI ? _("Loaded") : _("Not loaded"))) << "\r\n";

	if (mDBCI) {
		os << " [*] " << autosprintf(_("Type: %s"), mDBCI->metadata.database_type) << "\r\n";
		os << " [*] " << autosprintf(_("Binary version: %d.%d"), (unsigned short)mDBCI->metadata.binary_format_major_version, (unsigned short)mDBCI->metadata.binary_format_minor_version) << "\r\n";
		os << " [*] " << autosprintf(_("Build time: %s"), cTime((long)mDBCI->metadata.build_epoch).AsDate().AsString().c_str()) << "\r\n";
		os << " [*] " << autosprintf(_("Node count: %d"), (unsigned int)mDBCI->metadata.node_count) << "\r\n";
		os << " [*] " << autosprintf(_("File name: %s"), mDBCI->filename) << "\r\n";

		if (FileExists(mDBCI->filename))
			os << " [*] " << autosprintf(_("File size: %s"), convertByte(FileSize(mDBCI->filename)).c_str()) << "\r\n";
	}

	os << "\r\n " << _("ASN database") << ":\r\n\r\n"; // asn
	os << " [*] " << autosprintf(_("Status: %s"), (mDBAS ? _("Loaded") : _("Not loaded"))) << "\r\n";

	if (mDBAS) {
		os << " [*] " << autosprintf(_("Type: %s"), mDBAS->metadata.database_type) << "\r\n";
		os << " [*] " << autosprintf(_("Binary version: %d.%d"), (unsigned short)mDBAS->metadata.binary_format_major_version, (unsigned short)mDBAS->metadata.binary_format_minor_version) << "\r\n";
		os << " [*] " << autosprintf(_("Build time: %s"), cTime((long)mDBAS->metadata.build_epoch).AsDate().AsString().c_str()) << "\r\n";
		os << " [*] " << autosprintf(_("Node count: %d"), (unsigned int)mDBAS->metadata.node_count) << "\r\n";
		os << " [*] " << autosprintf(_("File name: %s"), mDBAS->filename) << "\r\n";

		if (FileExists(mDBAS->filename))
			os << " [*] " << autosprintf(_("File size: %s"), convertByte(FileSize(mDBAS->filename)).c_str()) << "\r\n";
	}
}

	};
};
