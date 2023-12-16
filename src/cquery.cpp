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

#include "cquery.h"

namespace nVerliHub {
	namespace nMySQL {

cQuery::cQuery(cMySQL &mysql):
	cObj("nMySQL::cQuery"),
	mMySQL(mysql),
	mResult(NULL)
{}

cQuery::cQuery(cQuery &query):
	cObj("nMySQL::cQuery"),
	mMySQL(query.mMySQL),
	mResult(NULL)
{}

cQuery::~cQuery()
{
	Clear();
}

void cQuery::Clear(void)
{
	if (mResult) {
		mysql_free_result(mResult);
		mResult = NULL;
	}

	mOS.str(mEmpty);
}

int cQuery::Query()
{
	const string qstr(mOS.str());

	if (Log(3))
		LogStream() << "Execute query ~" << qstr << '~' << endl;

	if (mysql_query(mMySQL.mDBHandle, qstr.c_str())) {
		if (ErrLog(2))
			LogStream() << "Error in query ~" << qstr << '~' << endl;

		mMySQL.Error(2, "Query error:");
		return -1;
	}

	return 1; //mysql_affected_rows(mMySQL.mDBHandle)
}

int cQuery::StoreResult()
{
	mResult = mysql_store_result(mMySQL.mDBHandle);
	if (mResult != NULL)
		return mysql_num_rows(mResult);
	else return 0;
}

MYSQL_ROW cQuery::Row()
{
	if(!mResult) return NULL;
	return mysql_fetch_row(mResult);
}

int cQuery::Cols()
{
	return mysql_num_fields(mResult);
}

void cQuery::DataSeek(unsigned long long r)
{
	mysql_data_seek(mResult, r);
}

bool cQuery::GetResult()
{
	return mResult ? true : false;
}

	}; // namespace nMySQL
}; // namespace nVerliHub
