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

#ifndef NUTILSCTEMPFUNCTIONBASE_H
#define NUTILSCTEMPFUNCTIONBASE_H

namespace nVerliHub {
	namespace nUtils {
/**
the interface for temporary functions that are periodialy called to do some stuff, until they are done

@author Daniel Muller
*/
class cTempFunctionBase
{
public:
    cTempFunctionBase();
    virtual ~cTempFunctionBase();
    virtual bool done() = 0;
    virtual void step() = 0;
};

	}; // namespace nUtils
}; // namespace nVerliHub
#endif
