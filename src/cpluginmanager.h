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

#ifndef NPLUGINCPLUGINMANAGER_H
#define NPLUGINCPLUGINMANAGER_H

#include <string>
#include <iostream>

#include "tchashlistmap.h"

using std::string;
using std::ostream;

namespace nVerliHub {
	namespace nPlugin {
		using namespace nUtils;
		class cPluginLoader;
		class cCallBackList;
		class cPluginBase;
/**
the plugin manager, let's you to load, and unload plugins

@author Daniel Muller
*/
class cPluginManager : public cObj
{
public:
	cPluginManager(const string &dir);
	~cPluginManager();
	bool LoadAll();
	void UnLoadAll();
	bool LoadPlugin(const string &name);
	bool UnloadPlugin(const string &name, bool remove = true);
	bool ReloadPlugin(const string &name);
	bool SetCallBack(string id, cCallBackList*);
	bool RegisterCallBack(string id, cPluginBase *pi);
	bool UnregisterCallBack(string id, cPluginBase *pi);
	void List(ostream &os);
	void ListAll(ostream &os);
	virtual void OnPluginLoad(cPluginBase *) = 0;
	typedef tcHashListMap<cPluginLoader*> tPlugins;
	string &GetError(){ return mLastLoadError;}
	cPluginBase * GetPlugin(const string &Name);
	cPluginBase * GetPluginByLib(const string &path);
protected:
	string mPluginDir;
	typedef tcHashListMap<cCallBackList*> tCBList;
	tPlugins mPlugins;
	tCBList mCallBacks;
	string mLastLoadError;
};

}; // namespace nPlugin
}; // namespace nVerliHub

#endif
