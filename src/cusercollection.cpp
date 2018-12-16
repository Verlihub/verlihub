/*
	Copyright (C) 2003-2005 Daniel Muller, dan at verliba dot cz
	Copyright (C) 2006-2018 Verlihub Team, info at verlihub dot net

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

#include <algorithm>
#include "cuser.h"
#include "cusercollection.h"
#include "cvhpluginmgr.h"

using namespace std;

namespace nVerliHub {
	using namespace nUtils;

void cUserCollection::ufSend::operator()(cUserBase *User)
{
	if (User && User->CanSend())
		User->Send(mData, false, !flush); // no pipe
}

void cUserCollection::ufSendWithNick::operator()(cUserBase *User)
{
	if (User && User->CanSend()) {
		string _str;
		_str.reserve(mDataStart.size() + User->mNick.size() + mDataEnd.size() + 1); // first use, reserve for pipe
		_str = mDataStart + User->mNick + mDataEnd;
		User->Send(_str, true, true); // always flushes
	}
}

void cUserCollection::ufSendWithClass::operator()(cUserBase *User)
{
	if (User && User->CanSend() && (User->mClass <= max_class) && (User->mClass >= min_class))
		User->Send(mData, false, !flush); // no pipe
}

void cUserCollection::ufSendWithFeature::operator()(cUserBase *User)
{
	if (User && User->CanSend() && User->HasFeature(feature))
		User->Send(mData, false, !flush); // no pipe
}

void cUserCollection::ufSendWithClassFeature::operator()(cUserBase *User)
{
	if (User && User->CanSend() && (User->mClass <= max_class) && (User->mClass >= min_class) && User->HasFeature(feature))
		User->Send(mData, false, !flush); // no pipe
}

cUserCollection::cUserCollection(bool KeepNickList, bool KeepInfoList):
	tHashArray<cUserBase*>(512), // todo: why so large array for small hubs? can we make it smaller? how is it extended?
	mNickListMaker(mNickList),
	mInfoListMaker(mInfoList, mInfoListComplete),
	mKeepNickList(KeepNickList),
	mKeepInfoList(KeepInfoList),
	mRemakeNextNickList(true),
	mRemakeNextInfoList(true)
{
	SetClassName("cUsrColl");
}

cUserCollection::~cUserCollection()
{}

void cUserCollection::Nick2Key(const string &Nick, string &Key)
{
	Key.assign(Nick);
	std::transform(Key.begin(), Key.end(), Key.begin(), ::tolower);
}

void cUserCollection::Nick2Hash(const string &Nick, tHashType &Hash)
{
	string Key;
	Nick2Key(Nick, Key);
	Hash = Key2Hash(Key);
	//Hash = Key2HashLower(Nick);
}

void cUserCollection::ufDoNickList::AppendList(string &list, cUserBase *user)
{
	list.reserve(list.size() + user->mNick.size() + mSep.size()); // always reserve because we are adding new data every time
	list.append(user->mNick);
	list.append(mSep);
}

void cUserCollection::ufDoInfoList::AppendList(string &list, cUserBase *user)
{
	if (mComplete) {
		list.reserve(list.size() + user->mMyINFO.size() + mSep.size()); // always reserve because we are adding new data every time
		list.append(user->mMyINFO);
	} else {
		list.reserve(list.size() + user->mMyINFO_basic.size() + mSep.size()); // always reserve because we are adding new data every time
		list.append(user->mMyINFO_basic);
	}

	list.append(mSep);
}

void cCompositeUserCollection::ufDoIPList::AppendList(string &list, cUserBase *user)
{
	cUser *point = static_cast<cUser*>(user);

	if (point->mxConn) {
		list.reserve(list.size() + point->mNick.size() + 1 + point->mxConn->AddrIP().size() + mSep.size()); // always reserve because we are adding new data every time
		list.append(point->mNick);
		list.append(1, ' ');
		list.append(point->mxConn->AddrIP());
		list.append(mSep);
	}
}

bool cUserCollection::Add(cUserBase *User)
{
	if (User)
		return AddWithHash(User, Nick2Hash(User->mNick));

	return false;
}

bool cUserCollection::Remove(cUserBase *User)
{
	if (User)
		return RemoveByHash(Nick2Hash(User->mNick));

	return false;
}

void cUserCollection::GetNickList(string &dest, const bool pipe)
{
	if (mRemakeNextNickList && mKeepNickList) {
		mNickListMaker.Clear();
		for_each(begin(), end(), mNickListMaker);
		mRemakeNextNickList = false;
	}

	if (dest.capacity() < (mNickList.size() + (pipe ? 1 : 0)))
		dest.reserve((mNickList.size() + (pipe ? 1 : 0)));

	dest = mNickList;
}

void cUserCollection::GetInfoList(string &dest, const bool pipe, const bool complete)
{
	if (mRemakeNextInfoList && mKeepInfoList) {
		mInfoListMaker.Clear();
		for_each(begin(), end(), mInfoListMaker);
		mRemakeNextInfoList = false;
	}

	if (complete) {
		if (dest.capacity() < (mInfoListComplete.size() + (pipe ? 1 : 0)))
			dest.reserve((mInfoListComplete.size() + (pipe ? 1 : 0)));

		dest = mInfoListComplete;
	} else {
		if (dest.capacity() < (mInfoList.size() + (pipe ? 1 : 0)))
			dest.reserve((mInfoList.size() + (pipe ? 1 : 0)));

		dest = mInfoList;
	}
}

void cCompositeUserCollection::GetIPList(string &dest, const bool pipe)
{
	if (mRemakeNextIPList && mKeepIPList) {
		mIPListMaker.Clear();
		for_each(begin(), end(), mIPListMaker);
		mRemakeNextIPList = false;
	}

	if (dest.capacity() < (mIPList.size() + (pipe ? 1 : 0)))
		dest.reserve((mIPList.size() + (pipe ? 1 : 0)));

	dest = mIPList;
}

void cUserCollection::SendToAll(string &Data, bool cache, bool pipe)
{
	AppendReservePlusPipe(mSendAllCache, Data, pipe);

	if (Log(4))
		LogStream() << "Start SendToAll" << endl;

	for_each(this->begin(), this->end(), ufSend(mSendAllCache, cache));

	if (Log(4))
		LogStream() << "Stop SendToAll" << endl;

	mSendAllCache.erase(0, mSendAllCache.size());
	ShrinkStringToFit(mSendAllCache);

	if (pipe)
		Data.erase(Data.size() - 1, 1);
}

void cUserCollection::SendToAllWithNick(string &Start, string &End)
{
	for_each(this->begin(), this->end(), ufSendWithNick(Start, End));
}

void cUserCollection::SendToAllWithClass(string &Data, int min_class, int max_class, bool cache, bool pipe)
{
	AppendReservePlusPipe(mSendAllCache, Data, pipe);

	if (Log(4))
		LogStream() << "Start SendToAllWithClass" << endl;

	for_each(this->begin(), this->end(), ufSendWithClass(mSendAllCache, min_class, max_class, cache));

	if (Log(4))
		LogStream() << "Stop SendToAllWithClass" << endl;

	mSendAllCache.erase(0, mSendAllCache.size());
	ShrinkStringToFit(mSendAllCache);

	if (pipe)
		Data.erase(Data.size() - 1, 1);
}

void cUserCollection::SendToAllWithFeature(string &Data, unsigned feature, bool cache, bool pipe)
{
	AppendReservePlusPipe(mSendAllCache, Data, pipe);

	if (Log(4))
		LogStream() << "Start SendToAllWithFeature" << endl;

	for_each(this->begin(), this->end(), ufSendWithFeature(mSendAllCache, feature, cache));

	if (Log(4))
		LogStream() << "Stop SendToAllWithFeature" << endl;

	mSendAllCache.erase(0, mSendAllCache.size());
	ShrinkStringToFit(mSendAllCache);

	if (pipe)
		Data.erase(Data.size() - 1, 1);
}

void cUserCollection::SendToAllWithClassFeature(string &Data, int min_class, int max_class, unsigned feature, bool cache, bool pipe)
{
	AppendReservePlusPipe(mSendAllCache, Data, pipe);

	if (Log(4))
		LogStream() << "Start SendToAllWithClassFeature" << endl;

	for_each(this->begin(), this->end(), ufSendWithClassFeature(mSendAllCache, min_class, max_class, feature, cache));

	if (Log(4))
		LogStream() << "Stop SendToAllWithClassFeature" << endl;

	mSendAllCache.erase(0, mSendAllCache.size());
	ShrinkStringToFit(mSendAllCache);

	if (pipe)
		Data.erase(Data.size() - 1, 1);
}

void cUserCollection::FlushForUser(cUserBase *User)
{
	ufSend(mSendAllCache, false).operator()(User); // mSendAllCache is empty here, thats what we want
}

void cUserCollection::FlushCache()
{
	SendToAll(mSendAllCache, false, false); // mSendAllCache is empty here, thats what we want
}

int cUserCollection::StrLog(ostream &ostr, int level)
{
	if (cObj::StrLog(ostr, level)) {
		LogStream() << '(' << mNickListMaker.mStart << ") " << "[ " << Size() /* << '/' << mUserList.size()*/ << " ] ";
		return 1;
	}

	return 0;
}

cCompositeUserCollection::cCompositeUserCollection(bool keepNicks, bool keepInfos, bool keepips/*, cVHCBL_String* nlcb, cVHCBL_String *ilcb*/):
	cUserCollection(keepNicks, keepInfos),
	mKeepIPList(keepips),
	//mInfoListCB(ilcb),
	//mNickListCB(nlcb),
	mRemakeNextIPList(true),
	mIPListMaker(mIPList)
{}

cCompositeUserCollection::~cCompositeUserCollection()
{}

void cCompositeUserCollection::GetNickList(string &dest, const bool pipe)
{
	if (mKeepNickList) {
		cUserCollection::GetNickList(mCompositeNickList, pipe);

/*
	#ifndef WITHOUT_PLUGINS
		if (mNickListCB)
			mNickListCB->CallAll(&mCompositeNickList);
	#endif
*/
	}

	dest = mCompositeNickList;
}

void cCompositeUserCollection::GetInfoList(string &dest, const bool pipe, const bool complete)
{
	if (mKeepInfoList) {
		cUserCollection::GetInfoList(mCompositeInfoList, pipe, complete);

/*
	#ifndef WITHOUT_PLUGINS
		if (mInfoListCB)
			mInfoListCB->CallAll(&mCompositeInfoList);
	#endif
*/
	}

	dest = mCompositeInfoList;
}

};
