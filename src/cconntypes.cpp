/*
	Copyright (C) 2003-2005 Daniel Muller, dan at verliba dot cz
	Copyright (C) 2006-2014 Verlihub Project, devs at verlihub-project dot org

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

#include <ios>
#include "cserverdc.h"
//#include "cconntypes.h"
#include "cdcconsole.h"
#include "i18n.h"
#include "stringutils.h"

using namespace std;
namespace nVerliHub {
	using namespace nConfig;
	using namespace nUtils;
	using namespace nEnums;
	namespace nTables {

cConnType::cConnType() :
	mTagMaxSlots(100),
	mTagMinSlots(0),
	mTagMinLimit(-1.),
	mTagMinLSRatio(-1.)
{}

cConnType::~cConnType()
{}

void cConnType::OnLoad()
{}

ostream &operator << (ostream &os, cConnType &ct)
{
	os << " ";
	os << setw(15) << setiosflags(ios::left) << ct.mIdentifier;
	os << setw(12) << setiosflags(ios::left) << ct.mTagMinSlots;
	os << setw(10) << setiosflags(ios::left) << ct.mTagMaxSlots;
	os << setw(15) << setiosflags(ios::left) << ct.mTagMinLimit;
	os << setw(18) << setiosflags(ios::left) << ct.mTagMinLSRatio;
	os << ct.mDescription.substr(0,20).c_str();


/*	os << ct.mIdentifier;
	os << ": ";
	os << autosprintf(_("Slots: %d..%d Min limiter: %.2f, %.2f/slot - %s"), ct.mTagMinSlots, ct.mTagMaxSlots, ct.mTagMinLimit, ct.mTagMinLSRatio, ct.mDescription.c_str());*/
	return os;
}


cConnTypes::cConnTypes(cServerDC *server)
        : tConnTypesBase(server->mMySQL, server, "conn_types")
{}


cConnTypes::~cConnTypes() {}

void cConnTypes::AddFields()
{
	AddCol("identifier","varchar(16)","", false, mModel.mIdentifier);
	AddPrimaryKey("identifier");
	AddCol("description","varchar(64)","no description",true,mModel.mDescription);
	AddCol("tag_min_slots","INT(4)", "0", true, mModel.mTagMinSlots);
	AddCol("tag_max_slots","INT(4)", "0", true, mModel.mTagMaxSlots);
	AddCol("tag_min_limit","DOUBLE", "-1", true, mModel.mTagMinLimit);
	AddCol("tag_min_ls_ratio","DOUBLE", "-1", true, mModel.mTagMinLSRatio);
	mMySQLTable.mExtra =" PRIMARY KEY(identifier)";
}

bool cConnTypes::CompareDataKey(const cConnType &D1, const cConnType &D2)
{
	return D1.mIdentifier == D2.mIdentifier;
}

cConnType *cConnTypes::FindConnType(const string &identifier)
{
	iterator it;
	cConnType *ConnType, *DefaultType = NULL;
	for( it=begin(); it != end(); ++it) {
		ConnType = *it;
		if(ConnType->mIdentifier == identifier)
			return ConnType;
		if(ConnType->mIdentifier == "default")
			DefaultType = ConnType;
	}
	if(DefaultType != NULL) return DefaultType;
	return &mModel;
}

cConnTypeConsole::cConnTypeConsole(cDCConsole *console) : tConnTypeConsoleBase(console)
{
	this->AddCommands();
}

cConnTypeConsole::~cConnTypeConsole()
{}

void cConnTypeConsole::GetHelpForCommand(int cmd, ostream &os)
{
	string help_str;
	switch(cmd)
	{
		case eLC_LST:
		help_str = "!lstconntype\r\nGive a list of registered connection types";
		break;
		case eLC_ADD:
		case eLC_MOD:
		help_str = "!(add|mod)conntype <type>[ -d <\"desc\">][ -S <max_slots>][ -s <min_slots>][ -l <min_limiter>][ -ls <min_ls_ratio>]\r\n"
		"      add or edit a connection type\r\n"
		"     * type - textual part of the connection type name\r\n"
		"     * max_slots, min_slots - slot settings\r\n"
		"     * desc - for your info\r\n"
		"     * min_limiter - minimum value for upload limiter (decimal)\r\n"
		"     * min_ls_ratio - minimum upload per slot";
		break;
		case eLC_DEL:
		help_str = "!delconntype <type>"; break;
		default: break;
	}

	cDCProto::EscapeChars(help_str,help_str);
	os << help_str;
}

const char * cConnTypeConsole::GetParamsRegex(int cmd)
{
	switch(cmd)
	{
		case eLC_ADD:
		case eLC_MOD:
			return "^(\\S+)("
			"( -d ?(\")?((?(4)[^\"]+?|\\S+))(?(4)\"))?|" //[ -d<description>|"<<description>>]"
			"( -S ?(-?\\d+))?|" //[ -S<max_slots>]
			"( -s ?(-?\\d+))?|" //[ -s<min_slots>]
			"( -l ?(-?[0-9.]+))?|" //[ -l<min_limit>]
			"( -ls ?(-?[0-9.]+))?|" // [ -ls<min_ls_ratio>]
			")*\\s*$"; // the end of message
		case eLC_DEL:
			return "(\\S+)";
		default: return "";break;
	};
}

bool cConnTypeConsole::ReadDataFromCmd(cfBase *cmd, int CmdID, cConnType &data)
{
	enum { 	eADD_ALL, eADD_IDENT,
		eADD_PARAM,
		eADD_DESCp, eADD_QUOTE, eADD_DESC,
		eADD_MAX_SLOTSp, eADD_MAX_SLOTS,
		eADD_MIN_SLOTSp, eADD_MIN_SLOTS,
		eADD_MIN_LIMITp, eADD_MIN_LIMIT,
		eADD_MIN_LS_RATIOp, eADD_MIN_LS_RATIO
		};

	cmd->GetParStr(eADD_IDENT,data.mIdentifier);
	cmd->GetParStr(eADD_DESC, data.mDescription);
	cmd->GetParInt(eADD_MAX_SLOTS, data.mTagMaxSlots);
	cmd->GetParInt(eADD_MIN_SLOTS, data.mTagMinSlots);
	cmd->GetParDouble(eADD_MIN_LIMIT, data.mTagMinLimit);
	cmd->GetParDouble(eADD_MIN_LS_RATIO, data.mTagMinLSRatio);
	return true;
}

cConnTypes *cConnTypeConsole::GetTheList()
{
	return mOwner->mServer->mConnTypes;
}

const char *cConnTypeConsole::CmdSuffix(){ return "conntype";}
const char *cConnTypeConsole::CmdPrefix(){ return "!";}

void cConnTypeConsole::ListHead(ostream *os)
{
	(*os) << "\r\n ";
	(*os) << setw(15) << setiosflags(ios::left) << toUpper(_("Name"));
	(*os) << setw(12) << setiosflags(ios::left) << toUpper(_("Min slot"));
	(*os) << setw(10) << setiosflags(ios::left) << toUpper(_("Max slot"));
	(*os) << setw(15) << setiosflags(ios::left) << toUpper(_("Min upload"));
	(*os) << setw(18) << setiosflags(ios::left) << toUpper(_("Min upload/slot"));
	(*os) << toUpper(_("Description")) << "\r\n";
	(*os) << " " << string(15+12+10+15+35,'=');
}

bool cConnTypeConsole::IsConnAllowed(cConnDC *conn,int cmd)
{
	return (conn && conn->mpUser && conn->mpUser->mClass >= eUC_ADMIN);
}

	}; // namespace nTables
}; // namespace nVerliHub
