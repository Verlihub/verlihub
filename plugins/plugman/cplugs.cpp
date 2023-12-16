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

#include "cplugs.h"
#include "cvhpluginmgr.h"
#include "cserverdc.h"
#include "stringutils.h"
#include "src/i18n.h"
#include <sys/stat.h>

namespace nVerliHub {
	using namespace nPlugin;
	using namespace nUtils;
	using namespace nEnums;
	namespace nPlugMan {

cPlug::cPlug():
	mLoadOnStartup(true),
	//mReloadNext(false),
	//mUnloadNext(false),
	mLoadTime(0),
	mOwner(NULL),
	mMakeTime(0)
{}

void cPlug::OnLoad()
{
	if(IsScript()) {
		if(!FindDestPlugin()) {
			mLastError = _("Plugin not found.");
			SaveMe();
			return;
		}
	}
	if(mLoadOnStartup) {
		//mReloadNext = false;
		//mUnloadNext = false;
		Plugin();
	}

	/*
	if(mReloadNext) {
		mReloadNext = false;
		mUnloadNext = false;
		Replug();

	}
	if(mUnloadNext) {
		mUnloadNext = false;
		Plugout();
		SaveMe();
	}
	*/
}

cPlug *cPlug::FindDestPlugin() const
{
	return mOwner ? mOwner->FindPlug(mDest) : NULL;
}

cVHPlugin *cPlug::GetDestPlugin() const
{
	cPlug *OwnerPlugin = FindDestPlugin();
	if(OwnerPlugin)
		return OwnerPlugin->IsLoaded();
	else return NULL;
}

cVHPlugin *cPlug::IsLoaded() const
{
	if(IsScript()) {
		cVHPlugin *plugin = GetDestPlugin();

		if (plugin && plugin->IsScriptLoaded(mPath))
			return plugin;
		else
			return NULL;
	} else {
		cVHPluginMgr *pm = ((mOwner) ? mOwner->mPM : NULL);

		if (pm)
			return (cVHPlugin *)pm->GetPluginByLib(mPath);
		else
			return NULL;
	}
}

bool cPlug::Plugin()
{
	cVHPluginMgr *pm = mOwner?mOwner->mPM:NULL;
	if (pm && !IsLoaded() && CheckMakeTime()) {
		if (IsScript()) {
			ostringstream os;
			bool result = false;

			cVHPlugin *dest = GetDestPlugin();
			if (dest) {
				if (dest->SupportsMultipleScripts())
					result = dest->AddScript(mPath, os);
				else if (dest->SupportsScripts())
					result = dest->LoadScript(mPath, os);
				else {
					mLastError = _("Plugin does not support scripts.");
					SaveMe();
					return false;
				}

				if (result)
					os << _("Script loaded.");
				else
					os << _("Error loading script.");
				mLastError = os.str();
				SaveMe();
				return result;
			}
		} else {
			if (pm->LoadPlugin(mPath)) {
				mLoadTime = cTime().Sec();
				mLastError = _("Plugin loaded.");
				SaveMe();
				return true;
			} else {
				mLastError = pm->GetError();
				SaveMe();
				return false;
			}
		}
	}
	return false;
}

bool cPlug::CheckMakeTime()
{
	if (IsScript()) // script dont need to be checked
		return true;

	mMakeTime = mOwner->GetFileTime(mPath);

	if (mMakeTime && (mMakeTime < mOwner->mVHTime)) {
		mLastError = _("Plugin needs to be recompiled.");
		SaveMe();
		return false;
	}

	return true;
}

bool cPlug::Plugout()
{
	cVHPluginMgr *pm = mOwner?mOwner->mPM:NULL;
	cVHPlugin *pi = IsLoaded();
	if (pm && pi) {
		if(IsScript())
			return pi->UnLoadScript(mPath);
		else
			return pm->UnloadPlugin(pi->Name());
	}
	return false;
}

bool cPlug::Replug()
{
	cVHPluginMgr *pm = mOwner?mOwner->mPM:NULL;
	cVHPlugin *pi=IsLoaded();
	if (pm && pi && CheckMakeTime()) {
		if (pm->ReloadPlugin(pi->Name())) {
			mLastError = _("Plugin reloaded.");
			SaveMe();
			return true;
		} else {
			mLastError = pm->GetError();
			SaveMe();
			return false;
		}
	}
	return false;
}

bool cPlug::IsScript() const
{
	return mDest.size() > 0;
}

void cPlug::SaveMe()
{
	mOwner->UpdateData(*this);
}

ostream& operator << (ostream &os, const cPlug &plug)
{
	os << " [*] " << autosprintf(_("Name: %s [%s] [%s]"), plug.mNick.c_str(), (plug.IsLoaded() ? _("On") : _("Off")), (plug.mLoadOnStartup ? _("Automated") : _("Manual"))) << "\r\n";
	os << " [*] " << autosprintf(_("Path: %s"), plug.mPath.c_str()) << "\r\n";

	if (plug.mDesc.size())
		os << " [*] " << autosprintf(_("Description: %s"), plug.mDesc.c_str()) << "\r\n";

	if (plug.mLastError.size())
		os << " [*] " << autosprintf(_("Last error: %s"), plug.mLastError.c_str()) << "\r\n";

	return os;
}

//--------------------------

cPlugs::cPlugs(cVHPlugin *pi) : tPlugListBase(pi, "pi_plug") , mPM (NULL), mVHTime(0)
{};

void cPlugs::AddFields()
{
	AddCol("nick","varchar(10)","",false,mModel.mNick);
	AddPrimaryKey("nick");
	AddCol("path","varchar(128)","",false,mModel.mPath);
	AddCol("dest","varchar(10)","",true,mModel.mDest);
	AddCol("detail","varchar(255)","",true,mModel.mDesc);
	AddCol("autoload","tinyint(1)","0",true,mModel.mLoadOnStartup);
	//AddCol("reload","tinyint(1)","0",true,mModel.mReloadNext);
	//AddCol("unload","tinyint(1)","0",true,mModel.mUnloadNext);
	AddCol("error","varchar(255)","",true,mModel.mLastError);
	AddCol("lastload","int(11)","",true,mModel.mLoadTime);
	mMySQLTable.mExtra = "PRIMARY KEY(nick)";
}

cPlug * cPlugs::FindPlug(const string &nick)
{
	iterator it;
	cPlug tmp;
	tmp.mNick = nick;
	return FindData(tmp);
}

void cPlugs::PluginAll(int method)
{
	bool IsAuto = false;

	switch (method) {
		case ePLUG_AUTOLOAD:
		//case ePLUG_AUTOUNLOAD:
		//case ePLUG_AUTORELOAD:
			IsAuto = true;
			break;

		default:
			break;
	}

	iterator it;

	for (it = begin(); it != end(); ++it) {
		switch (method) {
			case ePLUG_AUTOLOAD:
			//case ePLUG_LOAD:
				if ((!IsAuto || (*it)->mLoadOnStartup) && (*it)->mPath.size() > 0)
					(*it)->Plugin();

				break;

			/*
			case ePLUG_AUTORELOAD:
			case ePLUG_RELOAD:
				(*it)->Replug();
				break;

			case ePLUG_AUTOUNLOAD:
			case ePLUG_UNLOAD:
				(*it)->Plugout();
				break;
			*/

			default:
				break;
		}
	}
}

bool cPlugs::CompareDataKey(const cPlug &D1, const cPlug &D2)
{
	return D1.mNick == D2.mNick;
}

void cPlugs::OnLoadData(cPlug &plug)
{
	plug.mOwner = this;
	plug.OnLoad();
}

time_t cPlugs::GetFileTime(const string &filename)
{
	struct stat fs;

	if (stat(filename.data(), &fs) == 0) //success
		return fs.st_ctime;

	return 0;
}

	}; // namespace nPlugMan
}; // namespace nVerliHub
