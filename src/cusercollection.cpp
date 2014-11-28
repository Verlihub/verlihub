/*
	Copyright (C) 2003-2005 Daniel Muller, dan at verliba dot cz
	Copyright (C) 2006-2012 Verlihub Team, devs at verlihub-project dot org
	Copyright (C) 2013-2014 RoLex, webmaster at feardc dot net

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
using namespace std;
#include "cuser.h"
#include "cusercollection.h"
#include "cvhpluginmgr.h"

namespace nVerliHub {
	using namespace nUtils;
void cUserCollection::ufSend::operator() (cUserBase *User)
{
	if(User && User->CanSend())
		User->Send(mData, false);
}

void cUserCollection::ufSendWithNick::operator() (cUserBase *User)
{
	if(User && User->CanSend()) {
		User->Send(mDataStart, false, false);
		User->Send(User->mNick, false, false);
		User->Send(mDataEnd, true, true);
	}
}

void cUserCollection::ufSendWithClass::operator() (cUserBase *User)
{
	if(User && User->CanSend() && User->mClass <= max_class && User->mClass >= min_class) {
			User->Send(mData, false);
	}
}

void cUserCollection::ufSendWithFeature::operator() (cUserBase *User)
{
	if (User && User->CanSend() && User->HasFeature(feature)) {
		User->Send(mData, false);
	}
}

void cUserCollection::ufDoNickList::AppendList(string &List, cUserBase *User)
{
	List.append(User->mNick);
	List.append(mSep);
}

void cUserCollection::ufDoINFOList::AppendList(string &List, cUserBase *User)
{
	if(mComplete)
		List.append(User->mMyINFO);
	else
		List.append(User->mMyINFO_basic);
	List.append(mSep);
}

void cCompositeUserCollection::ufDoIpList::AppendList(string &List, cUserBase *User)
{
	cUser *user = static_cast<cUser *>(User);
	if (user->mxConn) {
		List.append(user->mNick);
		List.append(" ");
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

/**
 * void cUserCollection::SendToAll(string &Data, bool UseCache, bool AddPipe)
 * @param Data Datat to be sent
 * @param UseCache true woes not set data immediately, false will send al l previous cached data and hte current one
 * @param AddPipe apend a pipe character at the end
 */
void cUserCollection::SendToAll(string &Data, bool UseCache, bool AddPipe)
{
	if (AddPipe)
		Data.append("|");

	mSendAllCache.append(Data.data(), Data.size());

	if (!UseCache) {
		//if (Log(4))
			//CoutAllKeys();

		if (Log(4))
			LogStream() << "SendAll BEGIN" << endl;

		for_each(this->begin(), this->end(), ufSend(mSendAllCache));

		if (Log(4))
			LogStream() << "SendAll END" << endl;

		mSendAllCache.erase(0, mSendAllCache.size());
	}

	if (AddPipe)
		Data.erase(Data.size() - 1, 1);
}

void cUserCollection::SendToAllWithClass(string &Data, int min_class, int max_class, bool UseCache, bool AddPipe)
{
	if (AddPipe)
		Data.append("|");

	mSendAllCache.append(Data.data(), Data.size()); // todo: using cache here breaks the min and max class option

	if (!UseCache) {
		//if (Log(4))
			//CoutAllKeys();

		if (Log(4))
			LogStream() << "SendAll BEGIN" << endl;

		for_each(this->begin(), this->end(), ufSendWithClass(mSendAllCache, min_class, max_class));

		if (Log(4))
			LogStream() << "SendAll END" << endl;

		mSendAllCache.erase(0, mSendAllCache.size());
	}

	if (AddPipe)
		Data.erase(Data.size() - 1, 1);
}

void cUserCollection::SendToAllWithFeature(string &Data, unsigned feature, bool UseCache, bool AddPipe)
{
	if (AddPipe)
		Data.append("|");

	mSendAllCache.append(Data.data(), Data.size());

	if (!UseCache) {
		//if (Log(4))
			//CoutAllKeys();

		if (Log(4))
			LogStream() << "SendAll BEGIN" << endl;

		for_each(this->begin(), this->end(), ufSendWithFeature(mSendAllCache, feature));

		if (Log(4))
			LogStream() << "SendAll END" << endl;

		mSendAllCache.erase(0, mSendAllCache.size());
	}

	if (AddPipe)
		Data.erase(Data.size() - 1, 1);
}

void cUserCollection::SendToAllWithNick(string &Start, string &End)
{
	for_each(this->begin(),this->end(),ufSendWithNick(Start,End));
}

void cUserCollection::FlushForUser(cUserBase *User)
{
	if(mSendAllCache.size())
	{
		ufSend(mSendAllCache).operator()(User);
	}
}

void cUserCollection::FlushCache()
{
	string str;
	if(mSendAllCache.size())
	{
		SendToAll(str, false,false);
	}
}

int cUserCollection::StrLog(ostream & ostr, int level)
{
	if(cObj::StrLog(ostr,level)) {
		LogStream() << "(" << mNickListMaker.mStart ;
		LogStream() << ") "<< "[ " << Size() /* << "/" << mUserList.size()*/ << " ] ";
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
