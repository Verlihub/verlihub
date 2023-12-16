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

#ifndef NTHREADSCTHREADWORK_H
#define NTHREADSCTHREADWORK_H

namespace nVerliHub {
	namespace nThread {

/**
a definition of work what a WorkerThread should do..
a base class
@author Daniel Muller
*/

class cThreadWork
{
public:
	cThreadWork();
	virtual ~cThreadWork();
	/**
	 * \brief Thread will call this without parameters
	 * \return 0 on success, otherwise error code
	 **/
	virtual int DoTheWork()=0;
};

template<class ClassType, class Typ1, class Typ2, class Typ3> class tThreadWork3T : public cThreadWork
{
public :
	typedef int (ClassType::*tT3Callback)(Typ1, Typ2, Typ3);
	tThreadWork3T(Typ1 const &par1, Typ2 const & par2, Typ3 const & par3, ClassType *object, tT3Callback cb) : mCB(cb), mObject(object), mPar1(par1), mPar2(par2), mPar3(par3)
	{
	}

	virtual int DoTheWork()
	{
		return (mObject->*mCB)(mPar1, mPar2, mPar3);
	}
protected:
	tT3Callback mCB;
	ClassType * mObject;
	Typ1 mPar1;
	Typ2 mPar2;
	Typ3 mPar3;
};

template<class ClassType, class Typ1, class Typ2> class tThreadWork2T: public cThreadWork
{
public:
	typedef int(ClassType::*tT2Callback)(Typ1, Typ2);

	tThreadWork2T(Typ1 const &par1, Typ2 const &par2, ClassType *object, tT2Callback cb):
		mCB(cb),
		mObject(object),
		mPar1(par1),
		mPar2(par2)
	{}

	virtual int DoTheWork()
	{
		return (mObject->*mCB)(mPar1, mPar2);
	}
protected:
	tT2Callback mCB;
	ClassType *mObject;
	Typ1 mPar1;
	Typ2 mPar2;
};

	}; // namespace nThread
}; // namespace nVerliHub

#endif
