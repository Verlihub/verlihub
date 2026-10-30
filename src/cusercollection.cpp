/*
	Copyright (C) 2003-2005 Daniel Muller, dan at verliba dot cz
	Copyright (C) 2006-2026 Verlihub Team, info at verlihub dot net

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

cUserCollection::tHashType cUserCollection::Nick2Hash(const string &nick)
{
	/*
		same steps as Nick2Key and HashString together, so value is same bit by bit
		each char goes through tolower and back to char like in transform, then it is added like in HashString, which stops at zero byte
	*/

	tHashType hash = 0;

	for (const char c: nick) {
		if (!c)
			break;

		hash = 33 * hash + (char)(::tolower(c));
	}

	return hash;
}

void cUserCollection::Nick2Hash(const string &nick, tHashType &hash)
{
	hash = Nick2Hash(nick);
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

const string &cUserCollection::NickListRef()
{
	if (mRemakeNextNickList && mKeepNickList) {
		mNickListMaker.Clear();
		for_each(begin(), end(), mNickListMaker);
		mNickListMaker.Fit();
		mRemakeNextNickList = false;
	}

	return mNickList;
}

const string &cUserCollection::InfoListRef()
{
	if (mRemakeNextInfoList && mKeepInfoList) {
		mInfoListMaker.Clear();
		for_each(begin(), end(), mInfoListMaker);
		mInfoListMaker.Fit();
		mRemakeNextInfoList = false;
		mInfoListVer++;
	}

	return mInfoList;
}

const string &cUserCollection::IPListRef()
{
	if (mRemakeNextIPList && mKeepIPList) {
		mIPListMaker.Clear();
		for_each(begin(), end(), mIPListMaker);
		mIPListMaker.Fit();
		mRemakeNextIPList = false;
	}

	return mIPList;
}

void cUserCollection::GetNickList(string &dest)
{
	dest = NickListRef();
}

void cUserCollection::GetInfoList(string &dest)
{
	dest = InfoListRef();
}

void cUserCollection::GetIPList(string &dest)
{
	dest = IPListRef();
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
