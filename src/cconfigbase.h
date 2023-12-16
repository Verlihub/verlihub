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

#ifndef CCONFIGBASE_H
#define CCONFIGBASE_H
#include <vector>
#include "tchashlistmap.h"
#include "cconfigitembase.h"
#include "cobj.h"

using std::vector;

namespace nVerliHub {
	using namespace nUtils;
	namespace nConfig {

#define NewItemMethod(TYPE,Suffix) virtual cConfigItemBase##Suffix * NewItem(TYPE &var){ return new cConfigItemBase##Suffix(var);};

class cBasicItemCreator
{
	public:
		cBasicItemCreator(){};
		virtual ~cBasicItemCreator(){};
		NewItemMethod(bool, Bool);
		NewItemMethod(char, Char);
		NewItemMethod(int, Int);
		NewItemMethod(unsigned int, UInt);
		NewItemMethod(long, Long);
		NewItemMethod(__int64, Int64);
		NewItemMethod(unsigned __int64, UInt64);
		NewItemMethod(unsigned long, ULong);
		NewItemMethod(char *, PChar);
		NewItemMethod(string, String);
		NewItemMethod(double, Double);
};


/**configuration base class
  *@author Daniel Muller
  */
class cConfigBaseBase : public cObj
{
	public:
		typedef unsigned tItemHashType;
		cConfigBaseBase();
		virtual ~cConfigBaseBase();
		/** save config, to be able to load it after */
		virtual int Save() = 0;
		/** The config load function - whetre from a file, or from database, or whatever
			return >= 0 on success, otherwise error code
		*/
		virtual int Load() = 0;

		cBasicItemCreator *mItemCreator;

		/********** typedefs */
		/** the itemlist type */
		// nicked acess
		//typedef tStringHashMap<cConfigItemBase*> tItemHash;
		// numerated access
		typedef vector<size_t> tItemVec;
		typedef tItemVec::iterator tIVIt;

		typedef tcHashListMap<cConfigItemBase*,unsigned > tItemHash;
		typedef tItemHash::iterator tIHIt;

		// set the base pointer to relative adresses
		void SetBaseTo(void * new_base);

		void *mBasePtr;
		tItemHash mhItems;
		tItemVec  mvItems;
		static hHashStr<tItemHashType> msHasher;

		/**
			Item iterator
		*/
		struct iterator
		{
			iterator():
				mC(NULL)
			{}

			iterator (class cConfigBaseBase *C,const tIVIt &it):mC(C),mIT(it){}
			cConfigItemBase * operator* () { return mC->mhItems.GetByHash(*mIT);}
			iterator &operator ++() { ++mIT; return *this; }

			bool operator!=(iterator &it) const
			{
				return mIT != it.mIT;
			}

			cConfigBaseBase *mC;
			tIVIt mIT;
			iterator &Set(class cConfigBaseBase *C,const tIVIt &it){mC=C;mIT=it; return *this;}

			iterator &operator=(iterator &it)
			{
				mIT = it.mIT;
				mC = it.mC;
				return *this;
			}

			private:
				iterator(const iterator &it);
		};

		iterator mBegin;
		iterator mEnd;
		iterator &begin(){return mBegin.Set(this,mvItems.begin());}
		iterator &end()  {return mEnd.Set(this,mvItems.end());  }
		/** add existing item pointed by the argument at the end of mvItems , and bind a nick to it*/
		cConfigItemBase * Add(const string &, cConfigItemBase *);

		/** bind a nick to a given vaiable */
		void BindNick(int , const string &);


		/** access operators */
		cConfigItemBase * operator[](int);
		cConfigItemBase * operator[](const string &);
};


#define DeclareAddMethods(TYPE) \
cConfigItemBase* Add(const string &name, TYPE &var); \
cConfigItemBase* Add(const string &name, TYPE &var, TYPE const &def);

/**
This is a base class for every configuration like structure.
You can bind real variables with their names. You can get or affect their values. Convert from/to a std::string
read/write into/from a stream
very useful
@author Daniel Muller
*/
class cConfigBase : public cConfigBaseBase
{
	public:
		cConfigBase(){};
		virtual ~cConfigBase(){};

		DeclareAddMethods(bool);
		DeclareAddMethods(char);
		DeclareAddMethods(int);
		DeclareAddMethods(unsigned);
		DeclareAddMethods(long);
		DeclareAddMethods(unsigned long);
		DeclareAddMethods(__int64);
		DeclareAddMethods(unsigned __int64);
		DeclareAddMethods(string);
		DeclareAddMethods(char *);
		DeclareAddMethods(double);
};

	}; // namespace nConfig
}; // namespace nVerliHub

#endif
