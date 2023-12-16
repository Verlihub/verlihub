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

#include "cconfigbase.h"

namespace nVerliHub {
	namespace nConfig {

hHashStr<cConfigBaseBase::tItemHashType> cConfigBaseBase::msHasher;

cConfigBaseBase::cConfigBaseBase():
	cObj("cConfigBase")
{
	mBasePtr = NULL;
	mItemCreator = new cBasicItemCreator;
}

cConfigBaseBase::~cConfigBaseBase()
{
	tItemHashType Hash;
	tItemVec::iterator it;
	cConfigItemBase *item;

	for (it = mvItems.begin(); it != mvItems.end(); ++it) {
		Hash = *it;
		item = mhItems.GetByHash(Hash); // todo: not checked for valid pointer
		mhItems.RemoveByHash(Hash);
		delete item;
		item = NULL;
	}

	if (mItemCreator) {
		delete mItemCreator;
		mItemCreator = NULL;
	}
}

/** add existing item pointed by the argument at the end of mvItems , and bind a nick to it*/
cConfigItemBase * cConfigBaseBase::Add(const string &nick, cConfigItemBase *ci)
{
	tItemHashType Hash = msHasher(nick);

	if (!mhItems.AddWithHash(ci, Hash)) {
		if (Log(1)) {
			cConfigItemBase *other = mhItems.GetByHash(Hash);
			LogStream() << "Error adding " << nick << " because of " << (other ? other->mName.c_str() : "NULL") << endl;
		}
	}

	mvItems.push_back(Hash);
	ci->mName = nick;
	return ci;
}

/** access operators */
cConfigItemBase * cConfigBaseBase::operator[](int i)
{
	return mhItems.GetByHash(mvItems[i]);
}

cConfigItemBase * cConfigBaseBase::operator[](const string &n)
{
	tItemHashType Hash = msHasher(n);
	return mhItems.GetByHash(Hash);
}

void cConfigBaseBase::SetBaseTo(void *new_base)
{
	if (mBasePtr) {
		for (tIVIt it = mvItems.begin(); it != mvItems.end(); ++it)
			mhItems.GetByHash(*it)->mAddr = (void*)(long(mhItems.GetByHash(*it)->mAddr) + (long(new_base) - long(mBasePtr)));
	}

	mBasePtr = new_base;
}

/** create and add an item of template type with nick too */
#define DefineAddMethodWithDefValue(TYPE) \
cConfigItemBase * cConfigBase::Add(const string &name, TYPE &var, TYPE const &def) \
{ \
	cConfigItemBase *ci = this->Add(name, var); \
	*ci = def; \
	return ci; \
}

#define DefineAddMethodWithoutDefValue(TYPE) \
cConfigItemBase * cConfigBase::Add(const string &name, TYPE &var) \
{ \
	cConfigItemBase *ci = this->mItemCreator->NewItem(var); \
	return this->cConfigBaseBase::Add(name, ci); \
}

#define DefineAddMethods(TYPE) \
DefineAddMethodWithoutDefValue(TYPE) \
DefineAddMethodWithDefValue(TYPE)

DefineAddMethods(bool);
DefineAddMethods(char);
DefineAddMethods(int);
DefineAddMethods(unsigned);
DefineAddMethods(long);
DefineAddMethods(unsigned long);
DefineAddMethods(__int64);
DefineAddMethods(unsigned __int64);
DefineAddMethods(string);
DefineAddMethods(char *);
DefineAddMethods(double);

	}; // namespace nConfig
}; // namespace nVerliHub
