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

#if HAVE_DLFCN_H
#include <dlfcn.h>
#endif
#include "cpluginbase.h"
#include "cpluginmanager.h"

namespace nVerliHub {
	namespace nPlugin {

cPluginBase::cPluginBase():cObj("PluginBase")
{
	mName = "";
	mVersion = "";
	mIsAlive = true;
	mManager = NULL;
}

cPluginBase::~cPluginBase(){}

int cPluginBase::StrLog(ostream & ostr, int level)
{
	if(cObj::StrLog(ostr,level)) {
		LogStream()   << '(' << mName << ") ";
		return 1;
	}
	return 0;
}

bool cPluginBase::RegisterCallBack(string id)
{
	if(mManager)
		return mManager->RegisterCallBack(id, this);
	else
		return false;
}

bool nPlugin::cPluginBase::UnRegisterCallBack(string id)
{
	if(mManager)
		return mManager->UnregisterCallBack(id, this);
	else
		return false;
}
	}; // namespace nPlugin
}; // namespace nVerliHub
