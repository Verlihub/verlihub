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

#ifndef TMYSQLMEMORYLIST_H
#define TMYSQLMEMORYLIST_H
#include <vector>
#include "cconfmysql.h"
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "stringutils.h"
#include <dirsettings.h>

using std::vector;
namespace nVerliHub {
	using nConfig::cConfMySQL;
	using namespace nUtils;
	namespace nConfig {

  /**
  * Load data to memory from a given table and store and modify them.
  *
  * @author Daniel Muller
  * @version 1.1
  */
template<class DataType, class OwnerType> class tMySQLMemoryList : public cConfMySQL
{
public:

	/**
	* Class constructor.
	    * @param mysql The connection object to MySQL database.
	    * @param owner An instance of the object that owns DataType.
	    * @param tablename The name of the table where to store data.
	*/
	tMySQLMemoryList(nMySQL::cMySQL &mysql, OwnerType *owner, const string& tablename) :
		cConfMySQL(mysql), mOwner(owner)
	{
		mMySQLTable.mName = tablename;
	}

	/**
	* Class destructor.
	* Allocated object are removed from the list and deleted
	*/
	virtual ~tMySQLMemoryList() {
		this->Empty();
	}

	/**
	* This method is called before loading data from mysql table to memory.
	* By default table is created if it does not exist, default sql files are loaded and queries run and then data are loaded into memory.
	    * @see AddFields()
	    * @see InstallDefaultData()
	    * @see ReloadAll()
	*/
	virtual void OnStart()
	{
		AddFields();
		SetBaseTo(&mModel);
		CreateTable();
		InstallDefaultData();
		ReloadAll();
	}

	typedef vector<DataType *> tMyDataType;
	typedef typename tMyDataType::iterator iterator;

	/**
	* This method is called when data are going to be loaded to memory.
	*/
	virtual void OnLoadData(DataType &Data)
	{
		Data.OnLoad();
	}

	/**
	* Add columns to the table and specify where to store the value of the column for each row.
	    * @see cConfMySQL::AddCol()
	*/
	virtual void AddFields()=0;

	/**
	* Clean the list and delete the object from memory.
	*/
	virtual void Empty() {
		for (typename tMyDataType::iterator it = mData.begin(); it != mData.end(); ++it) {
			if (*it != NULL) {
				delete *it;
				*it = NULL;
			}
		}
		mData.clear();
	}

	/**
	* Return the number of objects loaded into memory.
	    * @return The number of objects in the list.
	*/
	virtual int Size(){ return mData.size();}

	/**
	* Load all data from the table into memory.
	    * @return The number of loaded elements.
	*/
	virtual int ReloadAll()
	{
		nMySQL::cQuery Query(mQuery); // make a second query for safety reasons
		Empty();
		Query.Clear();
		SelectFields(Query.OStream());

		if(this->mWhereString.size())
			Query.OStream() << " WHERE " << this->mWhereString;
		if(this->mOrderString.size())
			Query.OStream() << " ORDER BY " << this->mOrderString;
		int n=0;
		db_iterator it;

		DataType CurData, *AddedData;
		SetBaseTo(&CurData);

		for(it = db_begin(Query); it != db_end(); ++it) {
			AddedData = this->AppendData(CurData);
			OnLoadData(*AddedData);
			n++;
		}
		Query.Clear();
		return n;
	}

	/**
	* Add new data to the list and return the allocated object.
	    * @return The allocated object.
	*/
	virtual DataType *AppendData(DataType const &data)
	{
		DataType *copy = new DataType(data);
		mData.push_back(copy);
		return copy;
	}

	/**
	* Add new data to the list and store it to the table. <b>Please notice</b> that this method differs from AppendData() because every element is also stored into the database.
	* Also no check is done if the same element already exist in memory or in the table, so duplicated may be added.
	    * @return The allocated object.
	*/
	virtual DataType *AddData(DataType const &data)
	{
		DataType *copy = this->AppendData(data);
		SetBaseTo(copy);
		SavePK();
		return copy;
	}

	/**
	* Update an existing row in the table.
	    * @return The number of affected rows.
	*/
	virtual int UpdateData(DataType &data)
	{
		SetBaseTo(&data);
		return UpdatePK();
	}

	/**
	* Delete data from the table and the memory.
	*/
	virtual void DelData(DataType &data)
	{
		SetBaseTo(&data);
		DeletePK();
		iterator it;

		for (it = begin(); it != end(); ++it) {
			DataType *CurrentData = *it;

			if ((CurrentData != NULL) && CompareDataKey(data, *CurrentData)) {
				mData.erase(it);
				delete CurrentData;
				CurrentData = NULL;
				break;
			}
		}
	}

	/**
	* Find out if DataType objects are equal.
	    * @return True if the two objects are equal or false otherwise.
	*/
	virtual bool CompareDataKey(const DataType &D1, const DataType &D2){ return false; }

	/**
	* Find an object into the list with the same key.
	* @return The object if it is in the list or NULL otherwise.
	*/
	virtual DataType *FindData(DataType &ByKey)
	{
		iterator it;
		for (it = begin(); it != end(); ++it)
		{
			if( CompareDataKey(ByKey, *(*it))) return *it;
		}
		return NULL;
	}

	/**
	* Set how the data should be ordered when fetching them from the table.
	    * @param Order The order. Allowed values are <i>ASC</i> for ascending order and <i>DESC</i> for descrising order.
	*/
	void SetSelectOrder(const string &Order)
	{
		this->mOrderString = Order;
	}

	/**
	* Set extra condition in WHERE clause.
	* <b>Make sure</b> the query is valid or no data is fetched from the database.
	*/
	void SetSelectWhere(const string &Order)
	{
		this->mWhereString = Order;
	}

	/**
	* Save data to the given position in the table.
	    * @param n The position
	*/
	void SaveData(size_t n)
	{
		if (n < mData.size())
		{
			SetBaseTo(mData[n]);
			Save();
		}
	}

	/**
	* Load default sql files and run them.
	*/
	bool InstallDefaultData()
	{
		mQuery.Clear();
		string buf, filename;
		/*
		#ifdef _WIN32
		filename =  ".\\sql\\default_" + mMySQLTable.mName + ".sql";
		#else
		*/
		filename =  DATADIR  "/sql/default_" + mMySQLTable.mName + ".sql";
		//#endif
		bool _Result = false;
		if(LoadFileInString(filename, buf))
		{
			mQuery.OStream() << buf;
			_Result = mQuery.Query();
			mQuery.Clear();
		}
		return _Result;
	}

	/**
	* Return the i-th element in the list.
	    * @return The i-th element.
	*/
	DataType * operator[](int i)
	{
		if( i < 0 || i >= Size() ) return NULL;
		return mData[i];
	}

	/**
	* Return an iterator to the first element of the list.
	* @return The i-th element.
	*/
	iterator begin() { return mData.begin();}

	/**
	* Return an iterator to the last element of the list.
	* @return The i-th element.
	*/
	iterator end() { return mData.end();}

private:
	// List containter
	tMyDataType mData;

protected:
	// How data should be ordered when fetching them
	string mOrderString;

	// Extra condition in WHERE clause
	string mWhereString;

	// Prototype of data
	DataType mModel;

	// The owner object
	OwnerType *mOwner;
};

	}; // namespace nConfig
}; // namespace nVerliHub

#endif
