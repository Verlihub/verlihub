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

#include "cconfmysql.h"
#include <algorithm>
#include <string.h>

using namespace std;

namespace nVerliHub {
	using namespace nConfig;
	using namespace nMySQL;
	using namespace nEnums;

	namespace nMySQL {

cMySQLColumn::cMySQLColumn():
	mNull(false)
{}

cMySQLColumn::~cMySQLColumn()
{}

void cMySQLColumn::AppendDesc(ostream &os) const
{
	string isNull;
	mNull ? isNull = "" : isNull = " not null";
	os << mName << ' ' << mType << isNull;

	if (mDefault.size()) {
		os << " default '";
		cConfMySQL::WriteStringConstant(os, mDefault);
		os << '\'';
	}
}

void cMySQLColumn::ReadFromRow(const MYSQL_ROW &row)
{
	mName = row[0] ? row[0] : ""; // just for sure
	mType = row[1] ? row[1] : "";
	mDefault = row[4] ? row[4] : "";
	mNull = ((row[2] != NULL) && (row[2][0] != '\0'));
}

bool cMySQLColumn::operator!=(const cMySQLColumn &col) const
{
	return (this->mType != col.mType) || ((this->mDefault != col.mDefault) && this->mDefault.size());
}

cMySQLTable::cMySQLTable(cMySQL &mysql): cObj("cMySQLTable"), mQuery(mysql)
{}

cMySQLTable::~cMySQLTable()
{
	mQuery.Clear();
}

bool cMySQLTable::GetCollation()
{
	int i = 0, n;
	MYSQL_ROW row;
	mQuery.OStream() << "select table_collation from information_schema.tables where table_name = '" << mName << "' and table_schema = '" << mQuery.getMySQL().GetDBName() << '\'';

	if (mQuery.Query() <= 0) {
		mQuery.Clear();
		return false;
	}

	n = mQuery.StoreResult();
	cMySQLColumn col;

	for (; i < n; i++) {
		row = mQuery.Row();
		mCollation = row[0];
	}

	mQuery.Clear();
	return true;
}

bool cMySQLTable::GetDescription(const string &tableName)
{
	int i = 0, n;
	MYSQL_ROW row;
	mName = tableName;
	mQuery.OStream() << "show columns from " << tableName;

	if (mQuery.Query() <= 0) {
		mQuery.Clear();
		return false;
	}

	n = mQuery.StoreResult();

	cMySQLColumn col;

	for (; i < n; i++) { // 0=Field, 1=Type, 2=Null, 3=Key, 4=Default, 5=Extra
		row = mQuery.Row();
		col.ReadFromRow(row);
		mColumns.push_back(col);
	}

	mQuery.Clear();
	return true;
}

const cMySQLColumn * cMySQLTable::GetColumn(const string &colName) const
{
	vector<cMySQLColumn>::const_iterator it;

	for (it = mColumns.begin(); it != mColumns.end(); ++it) {
		if (it->mName == colName)
				return &(*it);
	}

	return NULL;
}

bool cMySQLTable::CreateTable()
{
	vector<cMySQLColumn>::iterator it;
	bool IsFirstCol = true;
	mQuery.OStream() << "create table if not exists " << mName << " ("; // try to create first

	for (it = mColumns.begin(); it != mColumns.end(); ++it) {
		mQuery.OStream() << (IsFirstCol ? "" : ", ");
		it->AppendDesc(mQuery.OStream());
		IsFirstCol = false;
	}

	if (mExtra.size())
		mQuery.OStream() << ", " << mExtra;

	mQuery.OStream() << ")";

	if (strcmp(DEFAULT_CHARSET, "") != 0)
		mQuery.OStream() << " character set " << DEFAULT_CHARSET;

	if (strcmp(DEFAULT_COLLATION, "") != 0)
		mQuery.OStream() << " collate " << DEFAULT_COLLATION;

	mQuery.Query();
	mQuery.Clear();
	return true;
}

void cMySQLTable::TruncateTable()
{
	mQuery.OStream() << "truncate table " << mName;
	mQuery.Query();
	mQuery.Clear();
}

bool cMySQLTable::AutoAlterTable(const cMySQLTable &original)
{
	vector<cMySQLColumn>::iterator it;
	const cMySQLColumn *col;
	bool NeedAdd = false, NeedModif = false, result = false;

	for (it = mColumns.begin(); it != mColumns.end(); ++it) {
		NeedAdd = NeedModif = false;

		if ((col = original.GetColumn(it->mName)) == NULL)
			NeedAdd = true;
		else
			NeedModif = ((*it) != (*col));

		if (NeedAdd || NeedModif) {
			result = true;

			if (Log(2))
				LogStream() << "Altering table " << mName << (NeedAdd ? " add column " : " modify column") << it->mName << " with type: " << it->mType << endl;

			mQuery.OStream() << "alter table " << mName << ' ' << (NeedAdd ? "add" : "modify") << " column ";
			it->AppendDesc(mQuery.OStream());
			mQuery.Query();
			mQuery.Clear();
		}
	}

	if ((strcmp(DEFAULT_COLLATION, "") != 0) && GetCollation() && (strcmp(mCollation.c_str(), DEFAULT_COLLATION) != 0)) {
		if (Log(2))
			LogStream() << "Altering table " << mName << ", setting character set to " << ((strcmp(DEFAULT_CHARSET, "") != 0) ? DEFAULT_CHARSET : "<default>") << " and collation to " << DEFAULT_COLLATION << endl;

		mQuery.OStream() << "alter table " << mName;

		if (strcmp(DEFAULT_CHARSET, "") != 0)
			mQuery.OStream() << " character set " << DEFAULT_CHARSET;

		mQuery.OStream() << " collate " << DEFAULT_COLLATION;
		mQuery.Query();
		mQuery.Clear();
	}

	return result;
}

	}; // namespace nMySQL
	namespace nConfig {

cConfMySQL::cConfMySQL(cMySQL &mysql):
	mMySQL(mysql),
	mQuery(mMySQL),
	mCols(0),
	mMySQLTable(mMySQL)
{
	if (mItemCreator)
		delete mItemCreator;

	mItemCreator = new cMySQLItemCreator;
}

cConfMySQL::~cConfMySQL()
{
	mQuery.Clear();

	if (mItemCreator) {
		delete mItemCreator;
		mItemCreator = NULL;
	}
}

void cConfMySQL::CreateTable()
{
	cMySQLTable existing_desc(mMySQL);

	if (existing_desc.GetDescription(mMySQLTable.mName))
		mMySQLTable.AutoAlterTable(existing_desc);
	else
		mMySQLTable.CreateTable();
}

void cConfMySQL::TruncateTable()
{
	mMySQLTable.TruncateTable();
}

int cConfMySQL::Save()
{
	SavePK(false);
	return 0;
}

int cConfMySQL::Load()
{
	return Load(mQuery);
}

int cConfMySQL::Load(cQuery &Query)
{
	MYSQL_ROW row;

	if (!(row = Query.Row()))
		return -1;

	for_each(mhItems.begin(), mhItems.end(), ufLoad(row));
	return 0;
}

/** return -1 on error, otherwinse number of results */
int cConfMySQL::StartQuery()
{
	return StartQuery(mQuery);
}

int cConfMySQL::StartQuery(cQuery &Query)
{
	int ret = Query.Query();

	if (ret <= 0) {
		Query.Clear();
		return ret;
	}

	Query.StoreResult();
	mCols = Query.Cols();
	return ret;
}

/** return -1 on error, otherwinse number of results */
int cConfMySQL::StartQuery(const string& query)
{
	mQuery.OStream() << query;
	return StartQuery();
}

int cConfMySQL::EndQuery()
{
	return EndQuery(mQuery);
}

int cConfMySQL::EndQuery(cQuery &Query)
{
	Query.Clear();
	return 0;
}

void cConfMySQL::AddPrimaryKey(const char *key)
{
	string Key(key);
	tItemHashType Hash = msHasher(Key);
	cConfigItemBase *item = mhItems.GetByHash(Hash);

	if (item != NULL)
		mPrimaryKey.AddWithHash(item, Hash);
}

void cConfMySQL::WherePKey(ostream &os)
{
	os << " where (";
	AllPKFields(os, true, true, false, " and ");
	os << ')';
}

void cConfMySQL::AllFields(ostream &os, bool DoF, bool DoV, bool IsAff, const string &joint)
{
	 for_each(mhItems.begin(), mhItems.end(), ufEqual(os, joint , DoF, DoV, IsAff));
}

void cConfMySQL::AllPKFields(ostream &os, bool DoF, bool DoV, bool IsAff, const string &joint)
{
	 for_each(mPrimaryKey.begin(), mPrimaryKey.end(), ufEqual(os, joint , DoF, DoV, IsAff));
}

void cConfMySQL::SelectFields(ostream &os)
{
	os << "select ";
	AllFields(os, true, false, false, ", ");
	os << " from " << mMySQLTable.mName << ' ';
}

void cConfMySQL::UpdateFields(ostream &os)
{
	os << "update " << mMySQLTable.mName << " set ";
	AllFields(mQuery.OStream(), true, true, true, ", ");
}

bool cConfMySQL::LoadPK()
{
	ostringstream query;
	SelectFields(query);
	WherePKey(query);

	if (StartQuery(query.str()) == -1)
		return false;

	bool found = (Load() >= 0);
	EndQuery();
	return found;
}

bool cConfMySQL::SavePK(bool dup)
{
	mQuery.OStream() << "insert" << (dup ? "" : " ignore") << " into " << mMySQLTable.mName << " (";
	AllFields(mQuery.OStream(), true, false, false, ", ");
	mQuery.OStream() << ") values (";
	AllFields(mQuery.OStream(), false, true, true, ", ");
	mQuery.OStream() << ')';

	if (dup) {
		mQuery.OStream() << " on duplicate key update ";
		AllFields(mQuery.OStream(), true, true, true, ", ");
	}

	bool ret = mQuery.Query();
	mQuery.Clear();
	return ret;
}

void cConfMySQL::DeletePK()
{
	mQuery.Clear();
	mQuery.OStream() << "delete from " << mMySQLTable.mName << ' ';
	WherePKey(mQuery.OStream());
	mQuery.Query();
	mQuery.Clear();
}

cConfMySQL::db_iterator &cConfMySQL::db_begin()
{
	return db_begin(mQuery);
}

cConfMySQL::db_iterator &cConfMySQL::db_begin(cQuery &Query)
{
	if ((-1 == this->StartQuery(Query)) || (Load(Query) < 0))
		mDBBegin = NULL;
	else
		mDBBegin = db_iterator(this, &Query);

	return mDBBegin;
}

cConfMySQL::db_iterator &cConfMySQL::db_iterator::operator++()
{
	if ((mConf != NULL) && (mQuery != NULL)) {
		if (mConf->Load(*mQuery) < 0) {
			mConf->EndQuery(*mQuery);
			mConf = NULL;
			mQuery = NULL;
		}
	}

	return *this;
}

ostream &cConfigItemMySQLPChar::WriteToStream (ostream& os)
{
	if (!IsEmpty()) {
		os << '"';
		cConfMySQL::WriteStringConstant(os, this->Data());
		os << '"';
	} else
		os << " null ";

	return os;
}

ostream &cConfigItemMySQLString::WriteToStream (ostream& os)
{
	if (!IsEmpty()) {
		os << '"';
		cConfMySQL::WriteStringConstant(os, this->Data());
		os << '"';
	} else
		os << " null ";

	return os;
}

bool cConfMySQL::UpdatePKVar(const char* var_name, string &new_val)
{
	cConfigItemBase * item = NULL;
	string var(var_name);
	item = operator[](var);

	if (!item)
		return false;

	LoadPK();
	item->ConvertFrom(new_val);
	return UpdatePKVar(item);
}

bool cConfMySQL::UpdatePKVar(cConfigItemBase *item)
{
	mQuery.OStream() << "update " << mMySQLTable.mName << " set ";
	ufEqual(mQuery.OStream(), ", ", true, true, true)(item);
	WherePKey(mQuery.OStream());
	bool ret = mQuery.Query();
	mQuery.Clear();
	return ret;
}

bool cConfMySQL::UpdatePK()
{
	return UpdatePK(mQuery);
}

bool cConfMySQL::UpdatePK(cQuery &Query)
{
	UpdateFields(Query.OStream());
	WherePKey(Query.OStream());
	bool ret = Query.Query();
	Query.Clear();
	return ret;
}

bool cConfMySQL::UpdatePKVar(const char *field)
{
	cConfigItemBase *item = operator[](field);

	if (!item)
		return false;

	return UpdatePKVar(item);
}

void cConfMySQL::WriteStringConstant(ostream &os, const string &str, bool like)
{
	string tmp, escs("\\\"'`");
	size_t pos = 0, lastpos = 0;
	char c;

	if (like) // note: % is not escaped
		escs.append(1, '_');

	while ((str.npos != lastpos) && (str.npos != (pos = str.find_first_of(escs, lastpos)))) {
		tmp.append(str, lastpos, pos - lastpos);
		tmp.append("\\");
		c = str[pos];
		tmp.append(&c, 1);
		lastpos = pos + 1;
	}

	if (str.npos != lastpos)
		tmp.append(str, lastpos, pos - lastpos);

	os << tmp;
}

void cConfMySQL::ufEqual::operator()(cConfigItemBase* item)
{
	if (!start)
		mOS << mJoint;
	else
		start = false;

	if (mDoField)
		mOS << (item->mName);

	if (mDoValue) {
		tItemType TypeId = item->GetTypeID();
		bool IsNull = item->IsEmpty() && ((TypeId == eIT_TIMET) || (TypeId == eIT_LONG));

		if (mDoField) {
			if (IsNull && !mIsAffect)
				mOS << " is ";
			else
				mOS << " = ";
		}

		if (IsNull)
			mOS << "null ";
		else
			item->WriteToStream(mOS);
	}
}

	}; // namespace nConfig
}; // namespace nVerliHub
