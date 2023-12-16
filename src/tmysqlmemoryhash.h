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

#ifndef NCONFIGTMYSQLMEMORYHASH_H
#define NCONFIGTMYSQLMEMORYHASH_H
#include "tmysqlmemorylist.h"
#include "thasharray.h"

namespace nVerliHub {
	using namespace nUtils;
	namespace nConfig {

/**
a list mirroring the mysql data, loaded in ram, and hashed for searching

@author Daniel Muller
*/
template<class DataType, class OwnerType>
class tMySQLMemoryHash : public tMySQLMemoryList<DataType,OwnerType>
{
protected:
	typedef tHashArray<DataType*> tDataHashArray;
	typedef typename tDataHashArray::tHashType tDataHashType;

	virtual tDataHashType GetHash(DataType & data) = 0;

	tDataHashArray mDataHash;

public:
	tMySQLMemoryHash(nMySQL::cMySQL& mysql, OwnerType* owner, const string& tablename):
		tMySQLMemoryList<DataType, OwnerType> (mysql, owner, tablename)
	{}

	virtual ~tMySQLMemoryHash() {
		this->Empty();
	}

	virtual DataType* AppendData(DataType const& data)
	{
		DataType* pData = tMySQLMemoryList<DataType,OwnerType>::AppendData(data);
		tDataHashType Hash = this->GetHash(*pData);
		mDataHash.AddWithHash(pData, Hash);
		return pData;
	}

	virtual void DelData(DataType& data)
	{
		tDataHashType Hash = this->GetHash(data);
		tMySQLMemoryList<DataType,OwnerType>::DelData(data);
		mDataHash.RemoveByHash(Hash);
	}

	virtual void Empty()
	{
		tMySQLMemoryList<DataType,OwnerType>::Empty();
		mDataHash.Clear();
	}

};

	}; // namespace nConfig
}; // namespace nVerliHub

#endif
