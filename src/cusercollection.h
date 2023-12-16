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

#ifndef NDIRECTCONNECTCUSERCOLLECTION_H
#define NDIRECTCONNECTCUSERCOLLECTION_H

/*
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
*/

#include <string>
#include <functional>
#include "thasharray.h"
#include "stringutils.h"

using std::string;

namespace nVerliHub {
	using namespace nUtils;
	class cUser;
	class cUserBase;

/*
	a structure that allows to insert and remove users, to quickly iterate and hold common sendall buffer, provides also number of sendall functions
	supports: quick iterating with (restrained) constant time increment (see std::slist) , logaritmic finding, adding, removing, testing for existence.... (see std::hash_map)
	@author Daniel Muller
*/

class cUserCollection: public tHashArray<cUserBase*>
{
public:
	class ufSendBase // base class for sending data to all users
	{
		protected:
			string &mData;
			bool mCache;
		public:
			ufSendBase(string &data, const bool cache):
				mData(data),
				mCache(cache)
			{}
	};

	struct ufSendMinMax // base structure for sending data to all users by class range
	{
		int mMinClass;
		int mMaxClass;

		ufSendMinMax(const int min_class, const int max_class):
			mMinClass(min_class),
			mMaxClass(max_class)
		{}
	};

	struct ufSend: public ufSendBase // unary function for sending data to all users
	{
		ufSend(string &data, const bool cache):
			ufSendBase(data, cache)
		{}

		void operator()(cUserBase *user);
	};

	struct ufSendWithNick // unary function for sending data start + nick + data end to all users
	{
		string &mDataStart, &mDataEnd;

		ufSendWithNick(string &data_s, string &data_e): // always flushes
			mDataStart(data_s),
			mDataEnd(data_e)
		{}

		void operator()(cUserBase *user);
	};

	struct ufSendWithClass: public ufSendBase, public ufSendMinMax // unary function for sending data to all users by class range
	{
		ufSendWithClass(string &data, const int min_class, const int max_class, const bool cache):
			ufSendBase(data, cache),
			ufSendMinMax(min_class, max_class)
		{}

		void operator()(cUserBase *user);
	};

	struct ufSendWithFeature: public ufSendBase // unary function for sending data to all users by feature in supports
	{
		unsigned mFeature;

		ufSendWithFeature(string &data, const unsigned feature, const bool cache):
			ufSendBase(data, cache),
			mFeature(feature)
		{}

		void operator()(cUserBase *user);
	};

	struct ufSendWithMyFlag: public ufSendBase // unary function for sending data to all users with flag in myinfo
	{
		unsigned short mFlag;

		ufSendWithMyFlag(string &data, const unsigned short flag, const bool cache):
			ufSendBase(data, cache),
			mFlag(flag)
		{}

		void operator()(cUserBase *user);
	};

	struct ufSendWithoutMyFlag: public ufSendBase // unary function for sending data to all users without flag in myinfo
	{
		unsigned short mFlag;

		ufSendWithoutMyFlag(string &data, const unsigned short flag, const bool cache):
			ufSendBase(data, cache),
			mFlag(flag)
		{}

		void operator()(cUserBase *user);
	};

	struct ufSendWithClassFeature: public ufSendBase, ufSendMinMax // unary function for sending data to all users by class range and feature in supports
	{
		unsigned mFeature;

		ufSendWithClassFeature(string &data, const int min_class, const int max_class, const unsigned feature, const bool cache):
			ufSendBase(data, cache),
			ufSendMinMax(min_class, max_class),
			mFeature(feature)
		{}

		void operator()(cUserBase *user);
	};

	/*
	struct ufSend: public unary_function<void, iterator>
	{
		string &mData;
		bool mCache;

		ufSend(string &data, const bool cache):
			mData(data),
			mCache(cache)
		{}

		void operator()(cUserBase *user);
	};

	struct ufSendWithNick: public unary_function<void, iterator>
	{
		string &mDataStart, &mDataEnd;

		ufSendWithNick(string &data_s, string &data_e): // always flushes
			mDataStart(data_s),
			mDataEnd(data_e)
		{}

		void operator()(cUserBase *user);
	};

	struct ufSendWithClass: public unary_function<void, iterator>
	{
		string &mData;
		int mMinClass, mMaxClass;
		bool mCache;

		ufSendWithClass(string &data, const int min_class, const int max_class, const bool cache):
			mData(data),
			mMinClass(min_class),
			mMaxClass(max_class),
			mCache(cache)
		{}

		void operator()(cUserBase *user);
	};

	struct ufSendWithFeature: public unary_function<void, iterator>
	{
		string &mData;
		unsigned mFeature;
		bool mCache;

		ufSendWithFeature(string &data, const unsigned feature, const bool cache):
			mData(data),
			mFeature(feature),
			mCache(cache)
		{}

		void operator()(cUserBase *user);
	};

	struct ufSendWithMyFlag: public unary_function<void, iterator>
	{
		string &mData;
		unsigned short mFlag;
		bool mCache;

		ufSendWithMyFlag(string &data, const unsigned short flag, const bool cache):
			mData(data),
			mFlag(flag),
			mCache(cache)
		{}

		void operator()(cUserBase *user);
	};

	struct ufSendWithoutMyFlag: public unary_function<void, iterator>
	{
		string &mData;
		unsigned short mFlag;
		bool mCache;

		ufSendWithoutMyFlag(string &data, const unsigned short flag, const bool cache):
			mData(data),
			mFlag(flag),
			mCache(cache)
		{}

		void operator()(cUserBase *user);
	};

	struct ufSendWithClassFeature: public unary_function<void, iterator>
	{
		string &mData;
		int mMinClass, mMaxClass;
		unsigned mFeature;
		bool mCache;

		ufSendWithClassFeature(string &data, const int min_class, const int max_class, const unsigned feature, const bool cache):
			mData(data),
			mMinClass(min_class),
			mMaxClass(max_class),
			mFeature(feature),
			mCache(cache)
		{}

		void operator()(cUserBase *user);
	};

	struct ufDoNickList: public unary_function<void, iterator>
	*/
	struct ufDoNickList // unary function that constructs nick list
	{
		string &mList;
		string mStart;
		string mSep;

		ufDoNickList(string &list):
			mList(list)
		{
			//mSep = "$$";
		}

		virtual ~ufDoNickList()
		{}

		virtual void Clear()
		{
			if (mList.size())
				mList.clear();

			ShrinkStringToFit(mList);

			if (mStart.size())
				mList.append(mStart);
		}

		virtual void AppendList(string &list, cUserBase *user);

		virtual void operator()(cUserBase *user)
		{
			AppendList(mList, user);
		}
	};

	struct ufDoInfoList: ufDoNickList // unary function that constructs myinfo list
	{
		ufDoInfoList(string &list):
			ufDoNickList(list)
		{
			mSep = "|";
		}

		virtual void AppendList(string &list, cUserBase *user);

		virtual void operator()(cUserBase *user)
		{
			AppendList(mList, user);
		}
	};

	struct ufDoIPList: ufDoNickList // unary function that constructs ip list
	{
		ufDoIPList(string &list):
			ufDoNickList(list)
		{
			mSep = "$$";
			mStart = "$UserIP ";
		}

		virtual void AppendList(string &list, cUserBase *user);

		virtual void operator()(cUserBase *user)
		{
			AppendList(mList, user);
		}
	};

private:
	string mSendAllCache;
	string mNickList;
	string mInfoList;
	string mIPList;
	ufDoNickList mNickListMaker;
	ufDoInfoList mInfoListMaker;
	ufDoIPList mIPListMaker;

protected:
	bool mKeepNickList;
	bool mKeepInfoList;
	bool mKeepIPList;
	bool mRemakeNextNickList;
	bool mRemakeNextInfoList;
	bool mRemakeNextIPList;

public:
	cUserCollection(const bool keep_nick, const bool keep_info, const bool keep_ip);
	virtual ~cUserCollection();
	virtual int StrLog(ostream &os, const int level);

	void SetNickListStart(const string &start)
	{
		mNickListMaker.mStart = start;
	}

	void SetNickListSeparator(const string &sep)
	{
		mNickListMaker.mSep = sep;
	}

	virtual void GetNickList(string &dest);
	virtual void GetInfoList(string &dest);
	virtual void GetIPList(string &dest);
	void Nick2Hash(const string &nick, tHashType &hash);

	tHashType Nick2Hash(const string &nick)
	{
		string key;
		Nick2Key(nick, key);
		return Key2Hash(key); //Key2HashLower(nick)
	}

	void Nick2Key(const string &nick, string &key);

	cUserBase* GetUserBaseByKey(const string &key)
	{
		if (key.size())
			return GetByHash(Key2Hash(key));

		return NULL;
	}

	cUser* GetUserByKey(const string &key)
	{
		if (key.size())
			return (cUser*)GetByHash(Key2Hash(key));

		return NULL;
	}

	cUser* GetUserByHash(const tHashType &hash)
	{
		return (cUser*)GetByHash(hash);
	}

	cUserBase* GetUserBaseByNick(const string &nick)
	{
		if (nick.size())
			return GetByHash(Nick2Hash(nick));

		return NULL;
	}

	cUserBase* GetUserBaseByHash(const tHashType &hash)
	{
		return GetByHash(hash);
	}

	cUser* GetUserByNick(const string &nick)
	{
		if (nick.size())
			return (cUser*)GetByHash(Nick2Hash(nick));

		return NULL;
	}

	bool ContainsKey(const string &key)
	{
		return ContainsHash(Key2Hash(key));
	}

	bool ContainsNick(const string &nick)
	{
		return ContainsHash(Nick2Hash(nick));
	}

	bool AddWithKey(cUserBase *user, const string &key)
	{
		return AddWithHash(user, Key2Hash(key));
	}

	bool AddWithNick(cUserBase *user, const string &nick)
	{
		return AddWithHash(user, Nick2Hash(nick));
	}

	bool Add(cUserBase *user);

	bool RemoveByKey(const string &key)
	{
		return RemoveByHash(Key2Hash(key));
	}

	bool RemoveByNick(const string &nick)
	{
		return RemoveByHash(Nick2Hash(nick));
	}

	bool Remove(cUserBase *user);

	void SendToAll(string &Data, const bool cache, const bool pipe);
	void SendToAllWithNick(string &start, string &end);
	void SendToAllWithClass(string &data, const int min_class, const int max_class, const bool cache, const bool pipe);
	void SendToAllWithFeature(string &data, const unsigned feature, const bool cache, const bool pipe);
	void SendToAllWithMyFlag(string &data, const unsigned short flag, const bool cache, const bool pipe);
	void SendToAllWithoutMyFlag(string &data, const unsigned short flag, const bool cache, const bool pipe);
	void SendToAllWithClassFeature(string &data, const int min_class, const int max_class, const unsigned feature, const bool cache, const bool pipe);
	void FlushCache();
	//void FlushForUser(cUserBase *user);

	virtual void OnAdd(cUserBase *user)
	{
		if (!mRemakeNextNickList && mKeepNickList)
			mNickListMaker(user);

		if (!mRemakeNextInfoList && mKeepInfoList)
			mInfoListMaker(user);

		if (!mRemakeNextIPList && mKeepIPList)
			mIPListMaker(user);
	}

	virtual void OnRemove(cUserBase *user)
	{
		mRemakeNextNickList = mKeepNickList;
		mRemakeNextInfoList = mKeepInfoList;
		mRemakeNextIPList = mKeepIPList;
	}

	unsigned int GetCacheSize() const
	{
		return mSendAllCache.size();
	}

	unsigned int GetCacheCapacity() const
	{
		return mSendAllCache.capacity();
	}

	unsigned int GetNickListSize() const
	{
		return mNickList.size();
	}

	unsigned int GetNickListCapacity() const
	{
		return mNickList.capacity();
	}

	unsigned int GetInfoListSize() const
	{
		return mInfoList.size();
	}

	unsigned int GetInfoListCapacity() const
	{
		return mInfoList.capacity();
	}

	unsigned int GetIPListSize() const
	{
		return mIPList.size();
	}

	unsigned int GetIPListCapacity() const
	{
		return mIPList.capacity();
	}
};

}; // namespace nVerliHub

#endif
