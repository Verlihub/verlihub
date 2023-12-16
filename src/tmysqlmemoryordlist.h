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

#ifndef NCONFIGTMYSQLMEMORYORDLIST_H
#define NCONFIGTMYSQLMEMORYORDLIST_H
#include "tmysqlmemorylist.h"

namespace nVerliHub {
	using namespace nUtils;
	namespace nConfig {

/**
a list mirroring the mysql data, loaded in ram, and ordered by some key

@author Daniel Muller
*/
template<class DataType, class OwnerType>
class tMySQLMemoryOrdList : public tMySQLMemoryList<DataType,OwnerType>
{
public:
	virtual int OrderTwoItems(const DataType &Data1, const DataType &Data2) =0;

	typedef vector<DataType*> tDataIndex;
public:
	tMySQLMemoryOrdList(nMySQL::cMySQL& mysql, OwnerType* owner, const string &tablename, const string &db_order):
		tMySQLMemoryList<DataType, OwnerType> (mysql, owner, tablename)
	{
		this->SetSelectOrder(db_order);
	}

	virtual ~tMySQLMemoryOrdList() {
		this->Empty();
	}

	virtual DataType* FindDataPosition(DataType const &data, int &CurPos)
	{
		// first adjust the CurPos, and init limits
		int MinPos = 0;
		int MaxPos = this->mDataIndex.size()-1;
		if(CurPos > MaxPos)
			CurPos = MaxPos;
		if(CurPos < MinPos)
			CurPos = MinPos;

		// Chech if the Proposed CurPos might be ok
		DataType *Data2 = NULL;
		int Order = -1, CurOrder = 0;
		if(CurPos <=MaxPos) {
			Data2 = this->GetDataAtOrder(CurPos);
			Order = this->OrderTwoItems(data, *Data2);
			if(!Order)
				return Data2;
			CurOrder = Order; // keep this for later - optimisation
		}

		// Try the Upper Limit
		if (MaxPos>=0) {
			if(MaxPos != CurPos) {
				Data2 = this->GetDataAtOrder(MaxPos);
				Order = this->OrderTwoItems(data, *Data2);
			}

			switch (Order) {
				case  0:
					CurPos = MaxPos;
					return Data2;
				case  1:
					CurPos = MaxPos+1;
					return NULL;
				default: break;
			}
		}

		// Try The Lower limit
		if ((MinPos != CurPos)&&(MinPos<=MaxPos)) {
			Data2 = this->GetDataAtOrder(MinPos);
			Order = this->OrderTwoItems(data,*Data2);

			switch (Order) {
				case  0:
					CurPos = MinPos;
					return Data2;
				case -1:
					CurPos = MinPos;
					return NULL;
				default: break;
			}
		}

		// nope, it's somewhere in the middle
		if((CurOrder > 0) && (CurPos < MaxPos))
			return this->FindDataPositionLimited(data, CurPos, MaxPos, CurPos);
		if((CurOrder < 0) && (CurPos > MinPos))
			return this->FindDataPositionLimited(data, MinPos, CurPos, CurPos);
		return NULL; // this won't happen, probably
	}

	virtual DataType* GetDataAtOrder(int Pos)
	{
		return this->mDataIndex[Pos];
	}

	virtual DataType* AppendData(DataType const& data)
	{
		int ExpectedPosition = this->Size();
		this->FindDataPosition(data, ExpectedPosition);
		DataType* pData = tMySQLMemoryList<DataType,OwnerType>::AppendData(data);
		mDataIndex.insert(mDataIndex.begin()+ExpectedPosition, pData);
		return pData;
	}

	virtual void DelData(DataType& data)
	{
		int ExpectedPosition = 0;
		this->FindDataPosition(data, ExpectedPosition);

		tMySQLMemoryList<DataType,OwnerType>::DelData(data);
		mDataIndex.erase(mDataIndex.begin()+ExpectedPosition);
	}

	virtual void Empty()
	{
		tMySQLMemoryList<DataType,OwnerType>::Empty();
		mDataIndex.clear();
	}

protected:
	tDataIndex mDataIndex;

	DataType *FindDataPositionLimited(const DataType &data, int MinPos, int MaxPos, int &CurPos)
	{
		// compare data to last, first, middle
		DataType *Data2 = NULL;
		int Order;

		if (MaxPos > MinPos) {
			int MidPos = (MaxPos+MinPos+1)/2;
			CurPos = MidPos;
			Data2 = this->GetDataAtOrder(MidPos);
			Order = this->OrderTwoItems(data, *Data2);

			switch(Order) {
				case  0:
					return Data2;
				case  1:
					if(MidPos < MaxPos)
						return this->FindDataPositionLimited(data, MidPos, MaxPos, CurPos);
					else
						CurPos = MidPos +1;
					return NULL;
				case -1:
					if(MidPos > MinPos+1)
						return this->FindDataPositionLimited(data, MinPos, MidPos -1, CurPos);
					else
						return NULL;
			}
		} else {
			CurPos = MinPos;
			return NULL;
		}
		CurPos = -1;
		return NULL;
	}
};

	}; // namespace nConfig
}; // namespace nVerliHub
#endif
