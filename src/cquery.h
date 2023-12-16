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

#ifndef NMYSQLCQUERY_H
#define NMYSQLCQUERY_H
#include "cmysql.h"
#include "cobj.h"
#include <sstream>

using namespace std;
namespace nVerliHub {
	namespace nMySQL {

/**
mysql query, contains result etc..

@author Daniel Muller
*/
class cQuery : public cObj
{
	public:
		cQuery(cMySQL & mysql);
		cQuery(cQuery & query);
		~cQuery();
		// clear the query and result, etc..
		void Clear();
		// return the stream to build the query
		ostringstream & OStream(){ return mOS;}
		// perform the query, return -1 on error
		int Query();
		// store result for iterating through it
		int StoreResult();
		// fetch next row from result
		MYSQL_ROW Row();
		// return the number of colums in the query result
		int Cols();
		// seek data row by the given number
		void DataSeek(unsigned long long);
		// is there an existing result?
		bool GetResult();
		cMySQL& getMySQL() { return mMySQL;}
	private:
		cMySQL & mMySQL;
		MYSQL_RES *mResult;
		//string mQuery;
		ostringstream mOS;
};
	}; // namespace nMySQL
}; // namespace nVerliHub

#endif
