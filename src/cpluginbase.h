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

#ifndef NPLUGINCPLUGINBASE_H
#define NPLUGINCPLUGINBASE_H

#include <string>
#include "cobj.h"

#ifndef REGISTER_PLUGIN
#define REGISTER_PLUGIN(__classname)\
	extern "C" {\
		nVerliHub::nPlugin::cPluginBase* get_plugin(void)\
		{\
			return new (__classname);\
		}\
		void del_plugin(nVerliHub::nPlugin::cPluginBase *plugin)\
		{\
			if (plugin) {\
				delete plugin;\
				plugin = NULL;\
			}\
		}\
	}
#endif

using std::string;

namespace nVerliHub {
	namespace nPlugin {

	class cPluginManager;
/**
the plugin base class suitable for any application

@author Daniel Muller
*/
class cPluginBase : public cObj
{
private:
	bool mIsAlive;
public:
	cPluginBase();
	virtual ~cPluginBase();
	const string &Name(){return mName;}
	const string &Version(){return mVersion;}
	//void Suicide(){mIsAlive = false;}
	bool IsAlive(){return mIsAlive;}
	void SetMgr( cPluginManager *mgr){ mManager = mgr; };
	bool RegisterCallBack(string id);
	bool UnRegisterCallBack(string id);

	virtual int StrLog(ostream & ostr, int level);
	virtual bool RegisterAll() = 0;
protected:
	string mName;
	string mVersion;
	cPluginManager * mManager;
};

	}; // namespace nPlugin
}; // namespace nVerliHub

#endif
