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

#ifndef CSIPS_H
#define CSIPS_H

#include <string>
#include "src/cpcre.h"
#include "src/cconndc.h"
#include "src/tlistplugin.h"

using namespace std;
namespace nVerliHub {
	namespace nSocket {
		class cServerDC;
	};
	namespace nIspPlugin {
		class cpiISP;
		class cISPs;
class cISP
{
public:
	cISP();
	virtual ~cISP();

	// -- stored data
	unsigned long mIPMin;
	unsigned long mIPMax;
	string mCC;
	string mName;
	string mAddDescPrefix;
	string mNickPattern;
	string mPatternMessage;
	string mConnPattern;
	string mConnMessage;
	// min share in GB
	long mMinShare[4];
	long mMaxShare[4];

	// --- memory data
	nUtils::cPCRE *mpNickRegex;
	nUtils::cPCRE *mpConnRegex;
	bool mOK;

	//-- methods
	bool CheckNick(const string &Nick, const string &cc);
	bool CheckConn(const string &ConnType);
	int CheckShare(int cls, __int64 share, __int64 min_unit, __int64 max_unit);

	cpiISP *mPI;
	virtual void OnLoad();
	friend ostream& operator << (ostream &, const cISP &isp);
};


class cISPCfg : public nConfig::cConfigBase
{
public:
	cISPCfg(nSocket::cServerDC *);
	int max_check_nick_class;
	int max_check_conn_class;
	int max_check_isp_class;
	int max_insert_desc_class;
	long unit_min_share_bytes;
	long unit_max_share_bytes;
	string msg_share_more;
	string msg_share_less;
	string msg_no_isp;
	bool allow_all_connections;
	bool case_sensitive_nick_pattern;

	nSocket::cServerDC *mS;
	virtual int Load();
	virtual int Save();
};

typedef class tOrdList4Plugin<cISP, cpiISP> tISPListBase;

class cISPs : public tISPListBase
{
public:
	cISPs(nPlugin::cVHPlugin *pi);
	virtual void AddFields();
	cISP *FindISPByCC(const string &cc);
	cISP *FindISP(const string &ip,const string &cc);
	virtual bool CompareDataKey(const cISP &D1, const cISP &D2);
	virtual int OrderTwoItems(const cISP &D1, const cISP &D2);
	virtual cISP *AddData(cISP const &isp);
	virtual void DelData(cISP &isp);
	virtual void OnLoadData(cISP &Data);
protected:
	// CC hash ?? either quickly locate the isp by given CC - complicates the CC parsing
	// or just maintain a list of CC isps - simplier, slower (but probably not much, easier to use)
	// I deciced for the second way -
	typedef vector<cISP*> tISPCCList;
	tISPCCList mCCList;
};
	}; // namespace nIspPlugin
}; // namespace nVerliHub
#endif//CSIPS_H
