/*
	Copyright (C) 2003-2005 Daniel Muller, dan at verliba dot cz
	Copyright (C) 2006-2017 Verlihub Team, info at verlihub dot net

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

#ifndef NDIRECTCONNECTCUSERCOLLECTION_H
#define NDIRECTCONNECTCUSERCOLLECTION_H
#if defined _WIN32
namespace std {
	inline unsigned long _Atomic_swap(unsigned long * __p, unsigned long __q)
	{
    		// __gthread_mutex_lock(&_Swap_lock_struct<0>::_S_swap_lock);
    		unsigned long __result = *__p;
    		*__p = __q;
    		// __gthread_mutex_unlock(&_Swap_lock_struct<0>::_S_swap_lock);
    		return __result;
	}
};
#endif

#include <string>
#include <functional>
#include "thasharray.h"

using std::string;
using std::unary_function;

namespace nVerliHub {
	using namespace nUtils;
	class cUser;
	class cUserBase;

namespace nPlugin {
	template <class Type1> class tVHCBL_1Type;
	typedef tVHCBL_1Type<string> cVHCBL_String;
};

using namespace nPlugin;

/**
a structure that allows to insert and remove users, to quickly iterate and hold common sendall buffer, provides also number of sendall functions

supports: quick iterating with (restrained) constant time increment (see std::slist) , logaritmic finding, adding, removing, testing for existence.... (see std::hash_map)

@author Daniel Muller
*/

class cUserCollection : public tHashArray<cUserBase*>
{
public:
	// unary function for sending data to all users
	struct ufSend: public unary_function<void, iterator>
	{
		string &mData;
		bool flush;

		ufSend(string &Data, bool _flush): mData(Data)
		{
			flush = _flush;
		};

		void operator()(cUserBase *User);
	};

	// unary function for sending DataS + Nick + DataE to all users
	struct ufSendWithNick: public unary_function<void, iterator>
	{
		string &mDataStart, &mDataEnd;

		ufSendWithNick(string &DataS, string &DataE): mDataStart(DataS), mDataEnd(DataE)
		{
			// always flushes
		};

		void operator()(cUserBase *User);
	};

	// unary function for sending data to all users by class range
	struct ufSendWithClass: public unary_function<void, iterator>
	{
		string &mData;
		int min_class, max_class;
		bool flush;

		ufSendWithClass(string &Data, int _min_class, int _max_class, bool _flush): mData(Data)
		{
			min_class = _min_class;
			max_class = _max_class;
			flush = _flush;
		};

		void operator()(cUserBase *User);
	};

	// unary function for sending data to all users by feature in supports
	struct ufSendWithFeature: public unary_function<void, iterator>
	{
		string &mData;
		unsigned feature;
		bool flush;

		ufSendWithFeature(string &Data, unsigned _feature, bool _flush): mData(Data)
		{
			feature = _feature;
			flush = _flush;
		};

		void operator()(cUserBase *User);
	};

	// unary function for sending data to all users by class range and feature in supports
	struct ufSendWithClassFeature: public unary_function<void, iterator>
	{
		string &mData;
		int min_class, max_class;
		unsigned feature;
		bool flush;

		ufSendWithClassFeature(string &Data, int _min_class, int _max_class, unsigned _feature, bool _flush): mData(Data)
		{
			min_class = _min_class;
			max_class = _max_class;
			feature = _feature;
			flush = _flush;
		};

		void operator()(cUserBase *User);
	};

	/** Unary function that constructs a nick list
	*/
	struct ufDoNickList : public unary_function<void, iterator>
	{
		ufDoNickList(string &List):mList(List){}
		virtual ~ufDoNickList(){};

		virtual void Clear() {
			mList.erase(0,mList.size());
			mList.append(mStart.data(), mStart.size());
		}
		virtual void operator() (cUserBase *usr){ AppendList(mList, usr);};
		virtual void AppendList(string &List, cUserBase *User);

		string mStart;
		string mSep;
		string &mList;
	};

	/**
	  * Creates MyInfo List ( for the Quick list extension )
	  */
	struct ufDoINFOList : ufDoNickList
	{
		string &mListComplete;
		bool mComplete;
		ufDoINFOList(string &List, string &CompleteList):ufDoNickList(List),mListComplete(CompleteList),mComplete(false){mSep="|";}
		virtual ~ufDoINFOList(){};
		virtual void AppendList(string &List, cUserBase *User);
		virtual void Clear() {
			ufDoNickList::Clear();
			mListComplete.erase(0,mListComplete.size());
			mListComplete.append(mStart.data(), mStart.size());
		}
		virtual void operator() (cUserBase *usr){
			mComplete=true;
			AppendList(mListComplete, usr);
			mComplete=false;
			AppendList(mList, usr);
		};
	};

private:
	//string mSendAllCache;
	string mNickList;
	string mINFOList;
	string mINFOListComplete;
	ufDoNickList mNickListMaker;
	ufDoINFOList mINFOListMaker;
protected:
	bool mKeepNickList;
	bool mKeepInfoList;
	bool mRemakeNextNickList;
	bool mRemakeNextInfoList;

public:
	cUserCollection(bool KeepNickList=false, bool KeepINFOList = false);
	virtual ~cUserCollection();

	virtual int StrLog(ostream & ostr, int level);
	void SetNickListStart(const string &Start){mNickListMaker.mStart = Start;}
	void SetNickListSeparator(const string &Separator){mNickListMaker.mSep = Separator;}
	virtual string &GetNickList();
	virtual string &GetInfoList(bool complete=false);
	//const char *GetNickListChar() {return GetNickList().c_str();}

	void Nick2Hash(const string &Nick, tHashType &Hash);
	tHashType Nick2Hash(const string &Nick) { string Key; Nick2Key(Nick, Key); return Key2Hash(Key); /*return Key2HashLower(Nick);*/ }
	void Nick2Key(const string &Nick, string &Key);

	cUserBase* GetUserBaseByKey(const string &Key)
	{
		if (Key.size())
			return GetByHash(Key2Hash(Key));
		else
			return NULL;
	}

	cUserBase* GetUserBaseByNick(const string &Nick)
	{
		if (Nick.size())
			return GetByHash(Nick2Hash(Nick));
		else
			return NULL;
	}

	bool   ContainsKey (const string &Key ){ return ContainsHash(Key2Hash(Key)); }
	bool   ContainsNick(const string &Nick){ return ContainsHash(Nick2Hash(Nick)); }

	bool   AddWithKey (cUserBase *User, const string &Key ){ return AddWithHash(User, Key2Hash(Key));}
	bool   AddWithNick(cUserBase *User, const string &Nick){ return AddWithHash(User, Nick2Hash(Nick)); }
	bool   Add        (cUserBase *User                    );

	bool   RemoveByKey (const string &Key ){ return RemoveByHash(Key2Hash(Key)); }
	bool   RemoveByNick(const string &Nick){ return RemoveByHash(Nick2Hash(Nick));}
	bool   Remove      (cUserBase *User       );

	void SendToAll(string &Data, bool UseCache = false, bool AddPipe = true);
	void SendToAllWithNick(string &Start, string &End);
	void SendToAllWithClass(string &Data, int min_class, int max_class, bool UseCache = false, bool AddPipe = true);
	void SendToAllWithFeature(string &Data, unsigned feature, bool UseCache = false, bool AddPipe = true);
	void SendToAllWithClassFeature(string &Data, int min_class, int max_class, unsigned feature, bool UseCache = false, bool AddPipe = true);
	void FlushCache();
	void FlushForUser(cUserBase *User);

	virtual void OnAdd(cUserBase *User)
	{
		if( !mRemakeNextNickList && mKeepNickList ) mNickListMaker(User);
		if( !mRemakeNextInfoList && mKeepInfoList ) mINFOListMaker(User);
	}
	virtual void OnRemove(cUserBase *)
	{
		mRemakeNextNickList = mKeepNickList;
		mRemakeNextInfoList = mKeepInfoList;
	}

};

class cCompositeUserCollection : public cUserCollection
{
public:
	cCompositeUserCollection(bool keepNicks, bool keepInfos, bool keepips, cVHCBL_String* nlcb, cVHCBL_String *ilcb);
	virtual ~cCompositeUserCollection();
	virtual string &GetNickList();
	virtual string &GetInfoList(bool complete=false);

	/**
	  * Creates IP List ( for the UserIP list extension )
	  */
	struct ufDoIpList : cUserCollection::ufDoNickList
	{
		ufDoIpList(string &List):ufDoNickList(List){mSep="$$";mStart="$UserIP ";}
		virtual ~ufDoIpList(){};
		virtual void operator()(cUserBase *usr) {AppendList(mList, usr);}
		virtual void AppendList(string &List, cUserBase *User);
	};

	virtual void OnAdd(cUserBase *User)
	{
		cUserCollection::OnAdd(User);
		if( !mRemakeNextIPList   && mKeepIPList   ) mIpListMaker(User);
	}
	virtual void OnRemove(cUserBase *user)
	{
		cUserCollection::OnRemove(user);
		mRemakeNextIPList = mKeepIPList;
	}

	cUser* GetUserByKey(const string &Key)
	{
		if (Key.size())
			return (cUser*)GetByHash(Key2Hash(Key));
		else
			return NULL;
	}

	cUser* GetUserByNick(const string &Nick)
	{
		if (Nick.size())
			return (cUser*)GetByHash(Nick2Hash(Nick));
		else
			return NULL;
	}

	string &GetIPList();

	bool mKeepIPList;
	string mIpList;
	ufDoIpList mIpListMaker;
	cVHCBL_String *mInfoListCB;
	cVHCBL_String *mNickListCB;
	bool mRemakeNextIPList;

	string mCompositeNickList;
	string mCompositeInfoList;
};

}; // namespace nVerliHub

#endif
