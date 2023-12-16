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

#ifndef NPLUGINTPLUGINBASE_H
#define NPLUGINTPLUGINBASE_H

#include <string>
#include "cobj.h"

/*
#ifdef _WIN32
#include <windows.h>
#endif
*/

using std::string;

namespace nVerliHub {
	namespace nPlugin {

/**
the plugin base class suitable for any application

@author Daniel Muller
*/
class tPluginBase : public cObj
{
public:
	tPluginBase();
	~tPluginBase();
	bool Open();
	bool Close();
	string Error();
	int StrLog(ostream & ostr, int level);
protected:
	string mFileName;
	string mName;
	/*
	#ifdef _WIN32
	HINSTANCE mHandle;
	#else
	*/
	void *mHandle;
	//#endif
};

	}; // namespace nPlugin
}; // namespace nVerliHub

#endif
