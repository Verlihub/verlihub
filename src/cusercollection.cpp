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

#include <algorithm>
#include "cuser.h"
#include "cusercollection.h"

using namespace std;

namespace nVerliHub {
	using namespace nUtils;

void cUserCollection::ufSend::operator()(cUserBase *user)
{
	if (user && user->CanSend())
		user->Send(mData, false, !mCache); // no pipe
}

void cUserCollection::ufSendWithNick::operator()(cUserBase *user)
{
	if (user && user->CanSend()) {
		string _str(mDataStart + user->mNick + mDataEnd);
		user->Send(_str, true, true); // always flushes
	}
}

void cUserCollection::ufSendWithClass::operator()(cUserBase *user)
{
	if (user && user->CanSend() && (user->mClass <= mMaxClass) && (user->mClass >= mMinClass))
		user->Send(mData, false, !mCache); // no pipe
}

void cUserCollection::ufSendWithFeature::operator()(cUserBase *user)
{
	if (user && user->CanSend() && user->HasFeature(mFeature))
		user->Send(mData, false, !mCache); // no pipe
}

void cUserCollection::ufSendWithMyFlag::operator()(cUserBase *user)
{
	if (user && user->CanSend() && user->GetMyFlag(mFlag))
		user->Send(mData, false, !mCache); // no pipe
}

void cUserCollection::ufSendWithoutMyFlag::operator()(cUserBase *user)
{
	if (user && user->CanSend() && !user->GetMyFlag(mFlag))
		user->Send(mData, false, !mCache); // no pipe
}

void cUserCollection::ufSendWithClassFeature::operator()(cUserBase *user)
{
	if (user && user->CanSend() && (user->mClass <= mMaxClass) && (user->mClass >= mMinClass) && user->HasFeature(mFeature))
		user->Send(mData, false, !mCache); // no pipe
}

cUserCollection::cUserCollection(const bool keep_nick, const bool keep_info, const bool keep_ip):
	tHashArray<cUserBase*>(512), // todo: why so large array for small hubs? can we make it smaller? how is it extended?
	mNickListMaker(mNickList),
	mInfoListMaker(mInfoList),
	mIPListMaker(mIPList),
	mKeepNickList(keep_nick),
	mKeepInfoList(keep_info),
	mKeepIPList(keep_ip),
	mRemakeNextNickList(true),
	mRemakeNextInfoList(true),
	mRemakeNextIPList(true)
{
	SetClassName("cUsrColl");
}

cUserCollection::~cUserCollection()
{}

void cUserCollection::Nick2Key(const string &nick, string &key)
{
	key.assign(nick);
	std::transform(key.begin(), key.end(), key.begin(), ::tolower);
}

void cUserCollection::Nick2Hash(const string &nick, tHashType &hash)
{
	string key;
	Nick2Key(nick, key);
	hash = Key2Hash(key); //Key2HashLower(nick)
}

void cUserCollection::ufDoNickList::AppendList(string &list, cUserBase *user)
{
	list.append(user->mNick);
	list.append(mSep);
}

void cUserCollection::ufDoInfoList::AppendList(string &list, cUserBase *user)
{
	list.append(user->mFakeMyINFO);
	list.append(mSep);
}

void cUserCollection::ufDoIPList::AppendList(string &list, cUserBase *user)
{
	cUser *point = static_cast<cUser*>(user);

	if (point->mxConn) { // real user
		list.append(point->mNick);
		list.append(1, ' ');
		list.append(point->mxConn->AddrIP());

	} else { // bots have local ip
		list.append(point->mNick);
		list.append(" 127.0.0.1"); // size() = 1 + 9
	}

	list.append(mSep);
}

bool cUserCollection::Add(cUserBase *user)
{
	if (user)
		return AddWithHash(user, Nick2Hash(user->mNick));

	return false;
}

bool cUserCollection::Remove(cUserBase *user)
{
	if (user)
		return RemoveByHash(Nick2Hash(user->mNick));

	return false;
}

void cUserCollection::GetNickList(string &dest)
{
	if (mRemakeNextNickList && mKeepNickList) {
		mNickListMaker.Clear();
		for_each(begin(), end(), mNickListMaker);
		mRemakeNextNickList = false;
	}

	dest = mNickList;
}

void cUserCollection::GetInfoList(string &dest)
{
	if (mRemakeNextInfoList && mKeepInfoList) {
		mInfoListMaker.Clear();
		for_each(begin(), end(), mInfoListMaker);
		mRemakeNextInfoList = false;
	}

	dest = mInfoList;
}

void cUserCollection::GetIPList(string &dest)
{
	if (mRemakeNextIPList && mKeepIPList) {
		mIPListMaker.Clear();
		for_each(begin(), end(), mIPListMaker);
		mRemakeNextIPList = false;
	}

	dest = mIPList;
}

void cUserCollection::SendToAll(string &data, const bool cache, const bool pipe)
{
	AppendPipe(mSendAllCache, data, pipe);

	if (Log(4))
		LogStream() << "Start SendToAll" << endl;

	for_each(this->begin(), this->end(), ufSend(mSendAllCache, cache));

	if (Log(4))
		LogStream() << "Stop SendToAll" << endl;

	if (mSendAllCache.size())
		mSendAllCache.clear();

	ShrinkStringToFit(mSendAllCache);

	if (pipe)
		data.erase(data.size() - 1, 1);
}

void cUserCollection::SendToAllWithNick(string &start, string &end)
{
	for_each(this->begin(), this->end(), ufSendWithNick(start, end));
}

void cUserCollection::SendToAllWithClass(string &data, const int min_class, const int max_class, const bool cache, const bool pipe)
{
	AppendPipe(mSendAllCache, data, pipe);

	if (Log(4))
		LogStream() << "Start SendToAllWithClass" << endl;

	for_each(this->begin(), this->end(), ufSendWithClass(mSendAllCache, min_class, max_class, cache));

	if (Log(4))
		LogStream() << "Stop SendToAllWithClass" << endl;

	if (mSendAllCache.size())
		mSendAllCache.clear();

	ShrinkStringToFit(mSendAllCache);

	if (pipe)
		data.erase(data.size() - 1, 1);
}

void cUserCollection::SendToAllWithFeature(string &data, const unsigned feature, const bool cache, const bool pipe)
{
	AppendPipe(mSendAllCache, data, pipe);

	if (Log(4))
		LogStream() << "Start SendToAllWithFeature" << endl;

	for_each(this->begin(), this->end(), ufSendWithFeature(mSendAllCache, feature, cache));

	if (Log(4))
		LogStream() << "Stop SendToAllWithFeature" << endl;

	if (mSendAllCache.size())
		mSendAllCache.clear();

	ShrinkStringToFit(mSendAllCache);

	if (pipe)
		data.erase(data.size() - 1, 1);
}

void cUserCollection::SendToAllWithMyFlag(string &data, const unsigned short flag, const bool cache, const bool pipe)
{
	AppendPipe(mSendAllCache, data, pipe);

	if (Log(4))
		LogStream() << "Start SendToAllWithMyFlag" << endl;

	for_each(this->begin(), this->end(), ufSendWithMyFlag(mSendAllCache, flag, cache));

	if (Log(4))
		LogStream() << "Stop SendToAllWithMyFlag" << endl;

	if (mSendAllCache.size())
		mSendAllCache.clear();

	ShrinkStringToFit(mSendAllCache);

	if (pipe)
		data.erase(data.size() - 1, 1);
}

void cUserCollection::SendToAllWithoutMyFlag(string &data, const unsigned short flag, const bool cache, const bool pipe)
{
	AppendPipe(mSendAllCache, data, pipe);

	if (Log(4))
		LogStream() << "Start SendToAllWithoutMyFlag" << endl;

	for_each(this->begin(), this->end(), ufSendWithoutMyFlag(mSendAllCache, flag, cache));

	if (Log(4))
		LogStream() << "Stop SendToAllWithoutMyFlag" << endl;

	if (mSendAllCache.size())
		mSendAllCache.clear();

	ShrinkStringToFit(mSendAllCache);

	if (pipe)
		data.erase(data.size() - 1, 1);
}

void cUserCollection::SendToAllWithClassFeature(string &data, const int min_class, const int max_class, const unsigned feature, const bool cache, const bool pipe)
{
	AppendPipe(mSendAllCache, data, pipe);

	if (Log(4))
		LogStream() << "Start SendToAllWithClassFeature" << endl;

	for_each(this->begin(), this->end(), ufSendWithClassFeature(mSendAllCache, min_class, max_class, feature, cache));

	if (Log(4))
		LogStream() << "Stop SendToAllWithClassFeature" << endl;

	if (mSendAllCache.size())
		mSendAllCache.clear();

	ShrinkStringToFit(mSendAllCache);

	if (pipe)
		data.erase(data.size() - 1, 1);
}

/*
void cUserCollection::FlushForUser(cUserBase *user)
{
	ufSend(mSendAllCache, false).operator()(user); // mSendAllCache is empty here, thats what we want
}
*/

void cUserCollection::FlushCache()
{
	SendToAll(mSendAllCache, false, false); // mSendAllCache is empty here, thats what we want
}

int cUserCollection::StrLog(ostream &os, const int level) // todo: need this?
{
	if (cObj::StrLog(os, level)) {
		LogStream() << '(' << mNickListMaker.mStart << ") " << "[ " << Size() /* << '/' << mUserList.size()*/ << " ] ";
		return 1;
	}

	return 0;
}

};
