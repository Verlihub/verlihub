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

#include "csetuplist.h"
#include "cdcproto.h"
#include "cserverdc.h"

namespace nVerliHub {
	using namespace nConfig;
	using namespace nMySQL;
	using namespace nProtocol;
	namespace nTables {

cSetupList::cSetupList(cMySQL &mysql):
	cConfMySQL(mysql)
{
	mMySQLTable.mName = "SetupList";
	AddCol("file", "varchar(30)", "", false, mModel.mFile);
	AddPrimaryKey("file");
	AddCol("var", "varchar(50)", "", false, mModel.mVarName);
	AddPrimaryKey("var");
	AddCol("val", "text", "", true, mModel.mVarValue);
	mMySQLTable.mExtra = "PRIMARY KEY (file, var)";
	SetBaseTo(&mModel);
}

cSetupList::~cSetupList()
{
}

void cSetupList::LoadFileTo(cConfigBaseBase *Config, const char *file)
{
	db_iterator it;
	cConfigItemBase *item = NULL;
	SelectFields(mQuery.OStream());
	mQuery.OStream() << " where `file` = '" << file << '\'';

	for (it = db_begin(); it != db_end(); ++it) {
		item = (*Config)[mModel.mVarName];

		if (item) {
			if ((mModel.mVarName == "hub_security") || (mModel.mVarName == "opchat_name")) // replace bad nick chars
				cServerDC::RepBadNickChars(mModel.mVarValue);
			else if (((mModel.mVarName == "cmd_start_op") || (mModel.mVarName == "cmd_start_user")) && mModel.mVarValue.empty()) // dont allow empty
				mModel.mVarValue = string(DEFAULT_COMMAND_TRIGS);

			item->ConvertFrom(mModel.mVarValue);
		}
	}

	mQuery.Clear();
}

void cSetupList::OutputFile(const string &file, ostream &os)
{
	db_iterator it;
	SelectFields(mQuery.OStream());

	if (file == "plugins")
		mQuery.OStream() << " where `file` like 'pi_%'";
	else
		mQuery.OStream() << " where `file` = '" << file << '\'';

	mQuery.OStream() << " order by `var` asc";
	string val;

	for (it = db_begin(); it != db_end(); ++it) {
		cDCProto::EscapeChars(mModel.mVarValue, val);
		string varName = mModel.mVarName;

		if (file == "plugins")
			varName = mModel.mFile + '.' + varName;

		os << " [*] " << varName << " = " << val << "\r\n";
	}

	mQuery.Clear();
}

void cSetupList::FilterFiles(const string &var, ostream &os, const string &file)
{
	db_iterator it;
	SelectFields(mQuery.OStream());
	string mask_var(var), mask_file(file);
	replace(mask_var.begin(), mask_var.end(), '*', '%');
	mQuery.OStream() << " where `var` like '";
	WriteStringConstant(mQuery.OStream(), mask_var, true);
	mQuery.OStream() << '\'';

	if (file.size()) {
		replace(mask_file.begin(), mask_file.end(), '*', '%');
		mQuery.OStream() << " and `file` like '";
		WriteStringConstant(mQuery.OStream(), mask_file, true);
		mQuery.OStream() << '\'';
	}

	mQuery.OStream() << " order by `file` asc, `var` asc";
	string val;

	for (it = db_begin(); it != db_end(); ++it) {
		cDCProto::EscapeChars(mModel.mVarValue, val);
		os << " [*] " << mModel.mFile + '.' << mModel.mVarName << " = " << val << "\r\n";
	}

	mQuery.Clear();
}

void cSetupList::SaveFileTo(cConfigBaseBase *Config, const char *file)
{
	cConfigBaseBase::iterator it;
	mModel.mFile = file;
	SetBaseTo(&mModel);

	for(it = Config->begin(); it != Config->end(); ++it) {
		mModel.mVarName = (*it)->mName;
		(*it)->ConvertTo(mModel.mVarValue);
		SavePK();
	}
}

bool cSetupList::SaveItem(const char *InFile, cConfigItemBase *ci)
{
	mModel.mFile = InFile;
	mModel.mVarName = ci->mName;
	ci->ConvertTo(mModel.mVarValue);
	DeletePK();
	SavePK(false);
	return true;
}

bool cSetupList::LoadItem(const char *FromFile, cConfigItemBase *ci)
{
	mModel.mFile = FromFile;
	mModel.mVarName = ci->mName;
	bool res = LoadPK();
	ci->ConvertFrom(mModel.mVarValue);
	return res;
}

	}; // namespace nTables
}; // namespace nVerliHub
