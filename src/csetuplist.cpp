/*
	Copyright (C) 2003-2005 Daniel Muller, dan at verliba dot cz
	Copyright (C) 2006-2015 Verlihub Team, info at verlihub dot net

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

namespace nVerliHub {
	using namespace nConfig;
	using namespace nMySQL;
	using namespace nProtocol;
	namespace nTables {

cSetupList::cSetupList(cMySQL &mysql):cConfMySQL(mysql)
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
	mQuery.OStream() << " WHERE file='" << file << "'";

	for (it = db_begin(); it != db_end(); ++it) {
		item = (*Config)[mModel.mVarName];

		if (item) {
			if (mModel.mVarName == "hub_security" || mModel.mVarName == "opchat_name") { // replace special chars in nick
				string newval;

				if (ReplaceNickChars(mModel.mVarValue, newval) > 0)
					mModel.mVarValue = newval;
			}

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
		mQuery.OStream() << " WHERE file LIKE 'pi_%'";
	else
		mQuery.OStream() << " WHERE file='" << file << "'";

	mQuery.OStream() << " ORDER BY `var` ASC";
	string val;

	for (it = db_begin(); it != db_end(); ++it) {
		cDCProto::EscapeChars(mModel.mVarValue, val);
		string varName = mModel.mVarName;

		if (file == "plugins")
			varName = mModel.mFile + "." + varName;

		os << " [*] " << varName << " = " << val << "\r\n";
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

		if (mModel.mVarName == "hub_security" || mModel.mVarName == "opchat_name") { // replace special chars in nick
			string newval;

			if (ReplaceNickChars(mModel.mVarValue, newval) > 0)
				mModel.mVarValue = newval;
		}

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
	LoadPK();
	ci->ConvertFrom(mModel.mVarValue);
	return true;
}

unsigned int cSetupList::ReplaceNickChars(const string &src, string &dst)
{
	unsigned int count = 0;
	size_t pos;
	dst = src;

	pos = dst.find("$"); // dollar

	while (pos != dst.npos) {
		dst.replace(pos, 1, "_");
		pos = dst.find("$", pos);
		count++;
	}

	pos = dst.find("|"); // pipe

	while (pos != dst.npos) {
		dst.replace(pos, 1, "_");
		pos = dst.find("|", pos);
		count++;
	}

	pos = dst.find(" "); // space

	while (pos != dst.npos) {
		dst.replace(pos, 1, "_");
		pos = dst.find(" ", pos);
		count++;
	}

	pos = dst.find("<"); // less

	while (pos != dst.npos) {
		dst.replace(pos, 1, "_");
		pos = dst.find("<", pos);
		count++;
	}

	pos = dst.find(">"); // greater

	while (pos != dst.npos) {
		dst.replace(pos, 1, "_");
		pos = dst.find(">", pos);
		count++;
	}

	return count;
}

	}; // namespace nTables
}; // namespace nVerliHub
