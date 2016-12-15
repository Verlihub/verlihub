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

#include "src/cbanlist.h"
#include "src/cconndc.h"
#include "src/cdcproto.h"
#include "src/stringutils.h"
#include "cisps.h"
#include "cpiisp.h"
#include "src/i18n.h"

namespace nVerliHub {
	using namespace nTables;
	namespace nIspPlugin {
cISP::cISP() :
	mIPMin(0),
	mIPMax(0),
	mpNickRegex(NULL),
	mpConnRegex(NULL),
	mOK(false),
	mPI(NULL)
{
	for(int i=0; i < 4; i++)
	{
		mMinShare[i] = -1;
		mMaxShare[i] = -1;
	}

	mPatternMessage = _("Your nick should match %[pattern]");
	mConnMessage = _("Your connection type should match %[pattern]");
}

cISP::~cISP()
{
	if(mpNickRegex) delete mpNickRegex;
	mpNickRegex = NULL;
	if(mpConnRegex) delete mpConnRegex;
	mpConnRegex = NULL;
}

void cISP::OnLoad()
{
	int pcre_options = PCRE_EXTENDED;
	mpNickRegex = new cPCRE();
	mpConnRegex = new cPCRE();
	if (mNickPattern.size())
	{
		if( !mPI->mCfg->case_sensitive_nick_pattern ) pcre_options |= PCRE_CASELESS;
		ReplaceVarInString(mNickPattern,"CC",mNickPattern,"(?P<CC>..)");
		mOK = mpNickRegex->Compile(mNickPattern.c_str(),pcre_options);
	} else mOK = true;
	if (mOK && mConnPattern.size())
		mOK = mpConnRegex->Compile(mConnPattern.c_str(),PCRE_CASELESS);

}

bool cISP::CheckNick(const string &Nick, const string &cc)
{
	if (mNickPattern.length() > 0 && mOK)
	{
		if (mpNickRegex->Exec(Nick) < 0) return false;
		if (mpNickRegex->GeStringNumber("CC") >=0)
			return mpNickRegex->Compare("CC",Nick,cc) == 0;
		else return true;
	}
	else return true;
}

bool cISP::CheckConn(const string &ConnType)
{
	if (mConnPattern.length() > 0 && mOK) {
		return (mpConnRegex->Exec(ConnType) >= 0);
	}
	else return true;
}

int cISP::CheckShare(int cls, __int64 share, __int64 min_unit, __int64 max_unit)
{
	__int64 min_share, max_share;
	min_share = share/ min_unit;
	max_share = share/ max_unit;
	if ((cls < 0) || (cls > 3)) return 0;
	if (min_share < mMinShare[cls]) return 1;
	if ((mMaxShare[cls] >= 0) && (max_share > mMaxShare[cls])) return -1;
	return 0;
}

ostream& operator << (ostream &os, const cISP &isp)
{
	string ip, tmp;
	os << isp.mName << " -- ";
	cBanList::Num2Ip(isp.mIPMin, ip);
	os << ip << "..";
	cBanList::Num2Ip(isp.mIPMax, ip);
	os << ip << "/(" << isp.mCC << ")\r\n"
		<< "DescPrefix: " << isp.mAddDescPrefix << "    ";
	cDCProto::EscapeChars(isp.mNickPattern, tmp);
	os	<< "NickPattern: " << tmp << "    ";
	cDCProto::EscapeChars(isp.mConnPattern, tmp);
	os	<< "Conn type: " << tmp << "\r\n"
		<< "Err Message: " << isp.mPatternMessage << "\r\n"
		<< "Min/Max share [guest,reg,vip,op]: ["
			<< isp.mMinShare[0] << "/" << isp.mMaxShare[0] << ","
			<< isp.mMinShare[1] << "/" << isp.mMaxShare[1] << ","
			<< isp.mMinShare[2] << "/" << isp.mMaxShare[2] << ","
			<< isp.mMinShare[3] << "/" << isp.mMaxShare[3] << "]\r\n";

	return os;
}

//--------------------------


cISPCfg::cISPCfg(cServerDC *s) : mS(s)
{
	Add("max_check_conn_class",max_check_conn_class,2);
	Add("max_check_nick_class",max_check_nick_class,0);
	Add("max_check_isp_class",max_check_isp_class,2);
	Add("max_insert_desc_class",max_insert_desc_class,2);
	Add("unit_min_share_bytes",unit_min_share_bytes,1024*1024*1024);
	Add("unit_max_share_bytes",unit_max_share_bytes,1024*1024*1024);
	Add("msg_share_more",msg_share_more,"Please share more!!");
	Add("msg_share_less",msg_share_less,"Please share less!!");
	Add("msg_no_isp",msg_no_isp,"You are not allowed to enter this hub. Your ISP is not allowed.");
	Add("allow_all_connections",allow_all_connections,true);
	Add("case_sensitive_nick_pattern",case_sensitive_nick_pattern, false);
}

int cISPCfg::Load()
{
	mS->mSetupList.LoadFileTo(this,"pi_isp");
	return 0;
}

int cISPCfg::Save()
{
	mS->mSetupList.SaveFileTo(this,"pi_isp");
	return 0;
}

//--------------------------

cISPs::cISPs(cVHPlugin *pi) : tISPListBase(pi, "pi_isp", "ipmin asc")
{};

void cISPs::AddFields()
{
	AddCol("ipmin","bigint(20)","0",false,mModel.mIPMin);
	AddCol("ipmax","bigint(20)","0",false,mModel.mIPMax);
	AddCol("cc","varchar(32)","",true,mModel.mCC);
	AddPrimaryKey("ipmin");
	AddCol("name","varchar(64)","",true,mModel.mName);
	AddCol("descprefix","varchar(16)","[???]",true, mModel.mAddDescPrefix);
	AddCol("nickpattern","varchar(64)","\\[---\\]",true, mModel.mNickPattern);
	AddCol("errmsg","varchar(128)","Your nick must be like this patern %[pattern]", true, mModel.mPatternMessage);
	AddCol("conntype","varchar(64)","",true,mModel.mConnPattern);
	AddCol("connmsg","varchar(128)","Your connection type does not match %[pattern]",true,mModel.mConnMessage);
	AddCol("minshare"   ,"int(11)","-1",true,mModel.mMinShare[0]);
	AddCol("minsharereg","int(11)","-1",true,mModel.mMinShare[1]);
	AddCol("minsharevip","int(11)","-1",true,mModel.mMinShare[2]);
	AddCol("minshareop" ,"int(11)","-1",true,mModel.mMinShare[3]);
	AddCol("maxshare"   ,"int(11)","-1",true,mModel.mMaxShare[0]);
	AddCol("maxsharereg","int(11)","-1",true,mModel.mMaxShare[1]);
	AddCol("maxsharevip","int(11)","-1",true,mModel.mMaxShare[2]);
	AddCol("maxshareop" ,"int(11)","-1",true,mModel.mMaxShare[3]);
	mMySQLTable.mExtra = "PRIMARY KEY(ipmin)";
}

void cISPs::OnLoadData(cISP &Data)
{
	Data.mPI = this->mOwner;
	Data.OnLoad();
	//for(int i=0; i < Size(); i++)cout << GetDataAtOrder(i)->mIPMin << " ";
	//cout << endl;
}

cISP * cISPs::FindISP(const string &ip, const string &cc)
{
	unsigned long ipnum = cBanList::Ip2Num(ip);
	iterator it;
	cISP *isp4all = NULL;
       	cISP *CurISP  = NULL;

	cISP Sample;
	int Pos = 0;

	Sample.mIPMin = ipnum;
	// find the closest bigger or equal item
	//cout << "Searching: " << Sample.mIPMin << endl;
	CurISP  = this->FindDataPosition(Sample, Pos);
	//cout << "Position : " << Pos << (CurISP?"eq":"ne") <<endl;
	// if it was not equal, but there is one smaller
	if (!CurISP && Pos) CurISP = this->GetDataAtOrder(Pos -1);
	// verify it
	if (CurISP && (CurISP->mIPMax >= ipnum)) return CurISP;
	//if (CurISP) cout << "Didn't match " << CurISP->mIPMax << " to " << ipnum << endl;
	// none found by ip range, try the cc
	CurISP = this->FindISPByCC(cc);
	if (CurISP) return CurISP;

	// look for the zeroth element - O..xx
	Sample.mIPMin = 0;
	isp4all = this->FindDataPosition(Sample, Pos);
	if(isp4all && (isp4all->mIPMin == 0)) return isp4all;
	return NULL; // there is not 0..xx ISP
}

int cISPs::OrderTwoItems(const cISP &D1, const cISP &D2)
{
	if(D1.mIPMin < D2.mIPMin) return -1;
	if(D1.mIPMin > D2.mIPMin) return  1;
	return 0;
}

bool cISPs::CompareDataKey(const cISP &D1, const cISP &D2)
{
	return D1.mIPMin == D2.mIPMin;
}

cISP *cISPs::AddData(cISP const &isp)
{
	cISP* pData = tISPListBase::AddData(isp);
	if (isp.mCC.size()) {
		mCCList.push_back(pData);
	}
	return pData;
}

void cISPs::DelData(cISP &isp)
{
	cISP *pData = this->FindData(isp);
	if (isp.mCC.size() && pData) {
		tISPCCList::iterator it;
		it = find(mCCList.begin(), mCCList.end(), pData);
		if( it != mCCList.end()) mCCList.erase(it);
	}
	tISPListBase::DelData(isp);
}

cISP *cISPs::FindISPByCC(const string &cc)
{
	tISPCCList::iterator it;
	if(!cc.size()) return NULL;
	for(it = mCCList.begin(); it != mCCList.end(); ++it)
	{
		cISP *CurISP = *it;
		if (CurISP->mCC.find(cc) != CurISP->mCC.npos)
			return CurISP;
	}

	return NULL;
}
	}; // namespace nIspPlugin
}; // namespace nVerliHub
