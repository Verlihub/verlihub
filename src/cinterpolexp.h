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

#ifndef NUTILSCINTERPOLEXP_H
#define NUTILSCINTERPOLEXP_H
#include "cconfigbase.h"
#include "ctempfunctionbase.h"
#include <string>

namespace nVerliHub {
	namespace nUtils {
/**
a Utility to interpolate a value of a "cConfig' variable between two time events, expecting the increment function will be called in regular intervals during given period

@author Daniel Muller
*/
class cInterpolExp : public cTempFunctionBase, cObj
{
public:
	cInterpolExp(unsigned int &var, long toval, int togo, int skip);
	~cInterpolExp();

	/** the function to call at every time step
	increases (reduces) the value of a variable
	*/
	public: virtual void step();
	public: virtual bool done();

protected:

	unsigned int & mVariable;

	/** values for orientation*/
	long mTargetValue;
	long mInitValue;
	long mCurrentValue;

	/** number of steps to do, and number of steps to skip */
	int mStepsToGo;
	int mSkipSteps;
	int mSkipedSteps;

	long mNextStep;

};
	}; // namespace nUtils
}; // namespace nVerliHub

#endif
