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

#ifndef CPLUG_H
#define CPLUG_H

#include <string>
#include <ctime.h>
#include <tlistplugin.h>

using namespace std;
namespace nVerliHub {
	namespace nPlugin {
		class cVHPluginMgr;
	};

	namespace nEnums {
		enum {
			ePLUG_AUTOLOAD/*,
			ePLUG_LOAD,
			ePLUG_AUTORELOAD,
			ePLUG_RELOAD,
			ePLUG_AUTOUNLOAD,
			ePLUG_UNLOAD
			*/
		};
	};
	namespace nPlugMan {
		class cpiPlug;
		class cPlugs;

class cPlug
{
public:
	// -- stored data
	string mNick; // plugin's nick, easy to remember
	string mPath; // the fiilename to load
	string mDesc; // public description
	string mDest; // destination plugin
	bool mLoadOnStartup;
	//bool mReloadNext; // reload if it's loaded
	//bool mUnloadNext; // unload if it's loaded
	string mLastError;
	unsigned mLoadTime;

	// -- memory data
	string mLastVersion;
	string mLastName;
	cPlugs *mOwner;
	time_t mMakeTime;

	// -- methods
	cVHPlugin *IsLoaded() const;
	bool Plugin();
	bool Plugout();
	bool Replug();
	bool CheckMakeTime();
	void SaveMe();
	bool IsScript() const;
	cPlug *FindDestPlugin() const;
	cVHPlugin *GetDestPlugin() const;

	// -- required methods
	cPlug();
	virtual ~cPlug(){};
	virtual void OnLoad();
	friend ostream& operator << (ostream &, const cPlug &plug);

};

typedef class tList4Plugin<cPlug, cpiPlug> tPlugListBase;

class cPlugs : public tPlugListBase
{
public:

	// -- usable methods
	cPlug *FindPlug(const string &nick);
	void PluginAll(int method);
	time_t GetFileTime(const string &filename);

	// -- required methods
	cPlugs(nPlugin::cVHPlugin *pi);
	virtual void AddFields();
	virtual bool CompareDataKey(const cPlug &D1, const cPlug &D2);

	// -- optional overrides
	virtual void OnLoadData(cPlug &plug);

	// -- data
	nPlugin::cVHPluginMgr *mPM;
	time_t mVHTime;

};

	}; // namespace nPlugMan
}; // namespace nVerliHub

#endif//CPLUG_H
