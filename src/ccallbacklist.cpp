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

#include "ccallbacklist.h"
#include "cpluginbase.h"
#include "cpluginmanager.h"
#ifdef _WIN32
#pragma warning( disable : 4355)
#endif

namespace nVerliHub {
	namespace nPlugin {

cCallBackList::cCallBackList(cPluginManager *mgr, string id) :
mMgr(mgr),
mCallOne(mMgr,this),
mName(id)
{
	if(mMgr)
		mMgr->SetCallBack(id, this);
}

const string &cCallBackList::Name() const
{
	return mName;
}

cCallBackList::~cCallBackList()
{}

void cCallBackList::ufCallOne::operator()(cPluginBase *pi)
{
	if(mCall)
		mCall = mCBL->CallOne(pi);
	// If the plugin is not alive, unload it with plugin manager
	if(!pi->IsAlive())
		mMgr->UnloadPlugin(pi->Name());
}

bool cCallBackList::Register(cPluginBase *plugin)
{
	if(!plugin)
		return false;
	tPICont::iterator i = find(mPlugins.begin(), mPlugins.end(), plugin);
	if(i != mPlugins.end())
		return false;
	mPlugins.push_back(plugin);
	return true;
}

bool cCallBackList::Unregister(cPluginBase *plugin)
{
	if(!plugin)
		return false;
	tPICont::iterator i = find(mPlugins.begin(), mPlugins.end(), plugin);
	if(i == mPlugins.end())
		return false;
	mPlugins.erase(i);
	return true;
}

bool cCallBackList::CallAll()
{
	mCallOne.mCall = true;
	return for_each(mPlugins.begin() , mPlugins.end(), mCallOne).mCall;
}

void cCallBackList::ListRegs(ostream &os, const char *indent)
{
	for(tPICont::iterator i = mPlugins.begin(); i != mPlugins.end(); ++i)
		os << indent << (*i)->Name() << "\r\n";
}
	}; // namespace nPlugin
}; // namespace nVerliHub
