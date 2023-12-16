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

#ifndef CCONFMYSQL_H
#define CCONFMYSQL_H

#include "cconfigbase.h"
#include "cconfigitembase.h"
#include <iostream>
#include "cmysql.h"
#include "cquery.h"

#define DEFAULT_COLLATION ""
#define DEFAULT_CHARSET ""

using namespace std;

namespace nVerliHub {
	namespace nConfig {

	// Config item for MySQL
	#define DeclareMySQLItemClass(TYPE,Suffix) \
		class cConfigItemMySQL##Suffix : public cConfigItemBase##Suffix \
		{ \
		public: \
			cConfigItemMySQL##Suffix(TYPE &var): cConfigItemBase##Suffix(var){}; \
			virtual std::ostream &WriteToStream (std::ostream& os); \
	}; \

	DeclareMySQLItemClass(string,String);
	DeclareMySQLItemClass(char *,PChar);
	/**a mysql configuratin class
	 * @author Daniel Muller
	 */
	}; // namespace nConfig

	namespace nMySQL {
	//using namespace nConfig;
class cMySQLItemCreator : public nConfig::cBasicItemCreator
{
public:
	cMySQLItemCreator(){};

	virtual nConfig::cConfigItemBaseString* NewItem(string &var)
 	{
		return new nConfig::cConfigItemMySQLString(var); // todo: where are these deleted?
 	};

	virtual nConfig::cConfigItemBasePChar * NewItem(char * &var)
	{
		return new nConfig::cConfigItemMySQLPChar(var); // todo: where are these deleted?
	};
};

class cMySQLColumn
{
public:
	cMySQLColumn();
	~cMySQLColumn();
	void AppendDesc(ostream &os) const;
	void ReadFromRow(const MYSQL_ROW &row);
	string mName;
	string mType;
	string mDefault;
	bool mNull;
	bool operator!=(const cMySQLColumn &col) const;
};

class cMySQLTable: public cObj
{
public:
	cMySQLTable(cMySQL &);
	~cMySQLTable();
	vector<cMySQLColumn> mColumns;
	const cMySQLColumn * GetColumn(const string &) const;
	bool GetCollation();
	bool GetDescription(const string &);
	bool CreateTable();
	void TruncateTable();
	bool AutoAlterTable(const cMySQLTable &original);
	void AppendColumnDesc(const cMySQLColumn &col, ostream &os);
	string mName;
	string mCollation;
	string mExtra;
	cQuery mQuery;
};

	}; // namespace nMySQL
	namespace nConfig {
/**a mysql configuratin class
  *@author Daniel Muller
  */

class cConfMySQL: public cConfigBase //<sMySQLItemCreator>
{
public:
	cConfMySQL(nMySQL::cMySQL &mysql);
	~cConfMySQL();

	nMySQL::cMySQL &mMySQL;
	nMySQL::cQuery mQuery;
	/** loads data from the mysql result */
	virtual int Load();
	virtual int Load(nMySQL::cQuery &);
	virtual int Save();
	/** do mysql query */
	int StartQuery(const string& query);
	int StartQuery();
	int StartQuery(nMySQL::cQuery &);
	int EndQuery();
	int EndQuery(nMySQL::cQuery &);
	void AddPrimaryKey(const char*);
	void WherePKey(ostream &os);
	void AllFields(ostream &, bool DoF=true, bool DoV=false, bool IsAff = false, const string &joint = ", ");
	void AllPKFields(ostream &, bool DoF=true, bool DoV=false, bool IsAff = false, const string &joint = ", ");
	void SelectFields(ostream &);
	void UpdateFields(ostream &os);
	bool UpdatePKVar(const char* var_name, string &new_val);
	bool UpdatePKVar(const char *);
	bool UpdatePKVar(cConfigItemBase *);
	bool UpdatePK();
	bool UpdatePK(nMySQL::cQuery &);
	bool LoadPK();
	bool SavePK(bool dup=false);
	static void WriteStringConstant(ostream &os, const string &str, bool like = false);
	void TruncateTable();
	void CreateTable();
	template <class T> void AddCol(const char *colName, const char *colType, const char *colDefault, bool colNull, T &var)
	{
		nMySQL::cMySQLColumn col;
		col.mName = colName;
		col.mType = colType;
		col.mDefault = colDefault;
		col.mNull = colNull;
		mMySQLTable.mColumns.push_back(col);
		Add(colName, var);
	}
	void DeletePK();

protected: // Protected attributes

	tItemHash mPrimaryKey;
	/**  */
	//int ok;
	// number of columns
	unsigned mCols;
	nMySQL::cMySQLTable mMySQLTable;

	/// UF to make equations and lists of values or field names
	struct ufEqual
	{
		ostream &mOS;
		string mJoint;
		bool start;
		bool mDoField, mDoValue;
		bool mIsAffect;
		ufEqual (ostream &os, const string& joint, bool DoF = true, bool DoV = true, bool IsAff = true):
			mOS(os), mJoint(joint), start(true),
			mDoField(DoF), mDoValue(DoV), mIsAffect(IsAff) {};
		ufEqual (ufEqual const &eq):
			mOS(eq.mOS),mJoint(eq.mJoint),start(eq.start),
			mDoField(eq.mDoField),mDoValue(eq.mDoValue),mIsAffect(eq.mIsAffect){}

		void operator()(cConfigItemBase* item);
	};

	/**
	  * Unary Function that loads current result's row into currently pointed structure
	*/
	struct ufLoad
	{
		MYSQL_ROW mRow;
		int i;

		ufLoad(const MYSQL_ROW &row):
			mRow(row),
			i(0)
		{}

		void operator()(cConfigItemBase *item)
		{
			if (mRow[i]) {
				item->ConvertFrom(mRow[i]);
			} else {
				string mEmpty;
				item->ConvertFrom(mEmpty);
			}

			i++;
		}
	};

public:
	/** database result iterator
	  * very useful
	  */
	struct db_iterator
	{
		cConfMySQL *mConf;
		nMySQL::cQuery *mQuery;
		db_iterator(cConfMySQL *conf = NULL ) : mConf(conf), mQuery(conf?&conf->mQuery:NULL){};
		db_iterator(cConfMySQL *conf, nMySQL::cQuery *query ) : mConf(conf), mQuery(query){};
		db_iterator &operator++();
		db_iterator &operator= (const db_iterator &it ){mConf = it.mConf; mQuery = it.mQuery; return *this;}
		db_iterator &operator= (cConfMySQL *conf){mConf = conf; mQuery=conf?&conf->mQuery:NULL; return *this;}
		bool operator==(db_iterator &it ){return (mConf == it.mConf) && (mQuery == it.mQuery);}
		bool operator!=(db_iterator &it ){return (mConf != it.mConf) || (mQuery != it.mQuery);}
	};

	db_iterator &db_begin(nMySQL::cQuery &);
	db_iterator &db_begin();
	db_iterator &db_end(){return mDBEnd;}
private:
	db_iterator mDBBegin;
	db_iterator mDBEnd;
};

	}; // namespace nConfig
}; // namespace nVerliHub

#endif
