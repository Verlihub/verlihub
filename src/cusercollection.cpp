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
		_str.reserve(mDataStart.size() + User->mNick.size() + mDataEnd.size() + 1);
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

void cUserCollection::ufDoNickList::AppendList(string &List, cUserBase *User)
{
	List.reserve(List.size() + User->mNick.size() + mSep.size());
	List.append(User->mNick);
	List.append(mSep);
}

void cUserCollection::ufDoINFOList::AppendList(string &List, cUserBase *User)
{
	if (mComplete) {
		List.reserve(List.size() + User->mMyINFO.size() + mSep.size());
		List.append(User->mMyINFO);
	} else {
		List.reserve(List.size() + User->mMyINFO_basic.size() + mSep.size());
		List.append(User->mMyINFO_basic);
	}

	List.append(mSep);
}

void cCompositeUserCollection::ufDoIpList::AppendList(string &List, cUserBase *User)
{
	cUser *user = static_cast<cUser*>(User);

	if (user->mxConn) {
		List.reserve(List.size() + user->mNick.size() + 1 + user->mxConn->AddrIP().size() + mSep.size());
		List.append(user->mNick);
		List.append(1, ' ');
		List.append(user->mxConn->AddrIP());
		List.append(mSep);
	}
}

cUserCollection::cUserCollection(bool KeepNickList, bool KeepInfoList) :
	tHashArray< cUserBase* > (512),
	mNickListMaker(mNickList),
	mINFOListMaker(mINFOList,mINFOListComplete),
	mKeepNickList(KeepNickList),
	mKeepInfoList(KeepInfoList),
	mRemakeNextNickList(true),
	mRemakeNextInfoList(true)
{
	SetClassName("cUsrColl");
}

cUserCollection::~cUserCollection() {}


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

bool cUserCollection::Add(cUserBase *User)
{
	if(User)
		return AddWithHash(User, Nick2Hash(User->mNick));
	else
		return false;
}

bool  cUserCollection::Remove(cUserBase *User)
{
	if(User)
		return RemoveByHash(Nick2Hash(User->mNick));
	else
		return false;
}

string &cUserCollection::GetNickList()
{
	if(mRemakeNextNickList && mKeepNickList) {
		mNickListMaker.Clear();
		for_each(begin(),end(),mNickListMaker);
		mRemakeNextNickList = false;
	}
	return mNickList;
}

string &cUserCollection::GetInfoList(bool complete)
{
	if(mRemakeNextInfoList && mKeepInfoList) {
		mINFOListMaker.Clear();
		for_each(begin(),end(),mINFOListMaker);
		mRemakeNextInfoList = false;
	}
	if(complete)
		return mINFOListComplete;
	else
		return mINFOList;
}

string &cCompositeUserCollection::GetIPList()
{
	if(mRemakeNextIPList && mKeepIPList) {
		mIpListMaker.Clear();
		for_each(begin(),end(),mIpListMaker);
		mRemakeNextIPList = false;
	}
	return mIpList;
}

void cUserCollection::SendToAll(string &Data, bool UseCache, bool AddPipe)
{
	AppendReserve(mSendAllCache,Data,AddPipe);

	if (Log(4))
		LogStream() << "Start SendToAll" << endl;

	for_each(this->begin(), this->end(), ufSend(mSendAllCache, UseCache));

	if (Log(4))
		LogStream() << "Stop SendToAll" << endl;

	mSendAllCache.erase(0, mSendAllCache.size());
	ShrinkStringToFit(mSendAllCache);

	if (AddPipe)
		Data.erase(Data.size() - 1, 1);
}

void cUserCollection::SendToAllWithNick(string &Start, string &End)
{
	for_each(this->begin(), this->end(), ufSendWithNick(Start, End));
}

void cUserCollection::SendToAllWithClass(string &Data, int min_class, int max_class, bool UseCache, bool AddPipe)
{
	AppendReserve(mSendAllCache,Data,AddPipe);

	if (Log(4))
		LogStream() << "Start SendToAllWithClass" << endl;

	for_each(this->begin(), this->end(), ufSendWithClass(mSendAllCache, min_class, max_class, UseCache));

	if (Log(4))
		LogStream() << "Stop SendToAllWithClass" << endl;

	mSendAllCache.erase(0, mSendAllCache.size());
	ShrinkStringToFit(mSendAllCache);

	if (AddPipe)
		Data.erase(Data.size() - 1, 1);
}

void cUserCollection::SendToAllWithFeature(string &Data, unsigned feature, bool UseCache, bool AddPipe)
{
	AppendReserve(mSendAllCache,Data,AddPipe);

	if (Log(4))
		LogStream() << "Start SendToAllWithFeature" << endl;

	for_each(this->begin(), this->end(), ufSendWithFeature(mSendAllCache, feature, UseCache));

	if (Log(4))
		LogStream() << "Stop SendToAllWithFeature" << endl;

	mSendAllCache.erase(0, mSendAllCache.size());
	ShrinkStringToFit(mSendAllCache);

	if (AddPipe)
		Data.erase(Data.size() - 1, 1);
}

void cUserCollection::SendToAllWithClassFeature(string &Data, int min_class, int max_class, unsigned feature, bool UseCache, bool AddPipe)
{
	AppendReserve(mSendAllCache,Data,AddPipe);

	if (Log(4))
		LogStream() << "Start SendToAllWithClassFeature" << endl;

	for_each(this->begin(), this->end(), ufSendWithClassFeature(mSendAllCache, min_class, max_class, feature, UseCache));

	if (Log(4))
		LogStream() << "Stop SendToAllWithClassFeature" << endl;

	mSendAllCache.erase(0, mSendAllCache.size());
	ShrinkStringToFit(mSendAllCache);

	if (AddPipe)
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

int cUserCollection::StrLog(ostream & ostr, int level)
{
	if(cObj::StrLog(ostr,level)) {
		LogStream() << '(' << mNickListMaker.mStart ;
		LogStream() << ") "<< "[ " << Size() /* << '/' << mUserList.size()*/ << " ] ";
		return 1;
	}
	return 0;
}

cCompositeUserCollection::cCompositeUserCollection(bool keepNicks, bool keepInfos, bool keepips, cVHCBL_String* nlcb, cVHCBL_String *ilcb)
	: cUserCollection(keepNicks,keepInfos),
	mKeepIPList(keepips),
	mIpListMaker(mIpList),
	mInfoListCB(ilcb),
	mNickListCB(nlcb),
	mRemakeNextIPList(true)
{};

cCompositeUserCollection::~cCompositeUserCollection() {}

string &cCompositeUserCollection::GetNickList()
{
	if(mKeepNickList)
	{
		mCompositeNickList = cUserCollection::GetNickList();
#ifndef WITHOUT_PLUGINS
		if (mNickListCB) {
			mNickListCB->CallAll(&mCompositeNickList);
		}
#endif
	}
	return mCompositeNickList;
}

string &cCompositeUserCollection::GetInfoList(bool complete)
{
	if(mKeepInfoList)
	{
		mCompositeInfoList = cUserCollection::GetInfoList(complete);
#ifndef WITHOUT_PLUGINS
		if (mInfoListCB) {
			mInfoListCB->CallAll(&mCompositeInfoList);
		}
#endif
	}
	return mCompositeInfoList;
}

};
