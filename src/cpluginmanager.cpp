/*
	Copyright (C) 2003-2005 Daniel Muller, dan at verliba dot cz
	Copyright (C) 2006-2017 Verlihub Team, info at verlihub dot net

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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <sys/types.h>
#include <dirent.h>
#include "cpluginmanager.h"
#include "cpluginloader.h"
#include "ccallbacklist.h"
#include "stringutils.h"
#include "i18n.h"

namespace nVerliHub {
	using namespace nUtils;

	namespace nPlugin {

cPluginManager::cPluginManager(const string &path):
	cObj("cPluginMgr"),
	mPluginDir(path)
{
	if (mPluginDir[mPluginDir.size()-1] != '/')
		mPluginDir.append("/");
}

cPluginManager::~cPluginManager()
{
	//this->UnLoadAll();
}

bool cPluginManager::LoadAll()
{
	if(Log(0))
		LogStream() << "Open dir: " << mPluginDir << endl;
	DIR *dir = opendir(mPluginDir.c_str());
	if(dir == NULL) {
		if(Log(1))
			LogStream() << "Open dir error" << endl;
		return false;
	}
	struct dirent * ent = NULL;

	string filename;
	string pathname;

	while (NULL!= (ent=readdir(dir))) {
		filename = ent->d_name;
		if(Log(3))
			LogStream() << "filename: " << filename << endl;
		if((filename.size() > 3) && (0 == StrCompare(filename,filename.size()-3,3,".so"))) {
			pathname = mPluginDir+filename;
			LoadPlugin(pathname);
		}
	}
	closedir(dir);
	return true;
}

/*
	todo: crash

void cPluginManager::UnLoadAll()
{
	tPlugins::iterator it;

	for (it = mPlugins.begin(); it != mPlugins.end(); ++it) {
		if (*it)
			this->UnloadPlugin((*it)->mPlugin->Name());
	}
}
*/

bool cPluginManager::LoadPlugin(const string &file)
{
#if ! defined _WIN32
	cPluginLoader *plugin;
	mLastLoadError = "";
	if(Log(3))
		LogStream() << "Attempt loading plugin: " << file << endl;
	plugin = new cPluginLoader(file);
	if(!plugin)
		return false;
	if(!plugin->Open() || !plugin->LoadSym() || !mPlugins.AddWithHash(plugin, mPlugins.Key2Hash(plugin->mPlugin->Name()))) {
		mLastLoadError = plugin->Error();
		delete plugin;
		return false;
	}

	try {
		plugin->mPlugin->SetMgr(this);
		plugin->mPlugin->RegisterAll();

		OnPluginLoad(plugin->mPlugin);
	} catch (...) {
		if(ErrLog(1))
			LogStream() << "Plugin " << file << " caused an exception" << endl;
	}
	if(Log(1))
		LogStream() << "Succes loading plugin: " << file << endl;
#endif
	return true;
}

bool cPluginManager::UnloadPlugin(const string &name)
{
	cPluginLoader *plugin = mPlugins.GetByHash(mPlugins.Key2Hash(name));
	if(!plugin || !mPlugins.RemoveByHash(mPlugins.Key2Hash(name))) {
		if(ErrLog(2))
			LogStream() << "Can't unload plugin name: '" << name << "'" << endl;
		return false;
	}

	tCBList::iterator it;
	for(it=mCallBacks.begin();it!=mCallBacks.end(); ++it)
		(*it)->Unregister(plugin->mPlugin);
	delete plugin;
	return true;
}

bool cPluginManager::ReloadPlugin(const string &name)
{
	cPluginLoader *plugin = mPlugins.GetByHash(mPlugins.Key2Hash(name));
	if (plugin) {
		string filename = plugin->GetFilename();
		if(!UnloadPlugin(name))
			return false;
		if(!LoadPlugin(filename))
			return false;
		return true;
	}
	return false;
}

bool cPluginManager::SetCallBack(string  id, cCallBackList *cb)
{
	if(!cb)
		return false;
	if(id.size() == 0)
		return false;
	return mCallBacks.AddWithHash(cb,mCallBacks.Key2Hash(id));
}

bool cPluginManager::RegisterCallBack(string id, cPluginBase *pi)
{
	cCallBackList *cbl = mCallBacks.GetByHash(mCallBacks.Key2Hash(id));
	if(!cbl)
		return false;
	if(!pi)
		return false;
	return cbl->Register(pi);
}

bool cPluginManager::UnregisterCallBack(string id, cPluginBase *pi)
{
	cCallBackList *cbl = mCallBacks.GetByHash(mCallBacks.Key2Hash(id));
	if(!cbl)
		return false;
	if(!pi)
		return false;
	return cbl->Unregister(pi);
}

void cPluginManager::List(ostream &os)
{
	tPlugins::iterator it;
	os << "\t" << _("Name");
	os << "\t\t" << _("Version");
	os << "\r\n\t" << string(40, '-') << "\r\n\r\n";

	for (it = mPlugins.begin(); it != mPlugins.end(); ++it) {
		if ((*it) && (*it)->mPlugin)
			os << "\t" << (*it)->mPlugin->Name() << "\t\t" << (*it)->mPlugin->Version() << "\r\n";
	}
}

void cPluginManager::ListAll(ostream &os)
{
	tCBList::iterator it;
	os << "\t" << _("Name");
	os << "\t\t\t\t\t" << _("Plugins");
	os << "\r\n\t" << string(75, '-') << "\r\n\r\n";

	for (it = mCallBacks.begin(); it != mCallBacks.end(); ++it) {
		if (*it) {
			os << "\t" << (*it)->Name() << "\t";

			if ((*it)->Name().size() <= 8)
				os << "\t";

			if ((*it)->Name().size() <= 16)
				os << "\t";

			if ((*it)->Name().size() <= 24)
				os << "\t";

			if ((*it)->Name().size() <= 32)
				os << "\t";

			(*it)->ListRegs(os, " ");
			os << "\r\n";
		}
	}
}

cPluginBase * cPluginManager::GetPlugin(const string &Name)
{
	cPluginLoader *pi;
	pi = mPlugins.GetByHash(mPlugins.Key2Hash(Name));
	if(pi)
		return pi->mPlugin;
	else
		return NULL;
}

cPluginBase * cPluginManager::GetPluginByLib(const string &path)
{
	tPlugins::iterator it;

	for (it = mPlugins.begin(); it != mPlugins.end(); ++it) {
		if ((*it)->GetFilename() == path)
			return (*it)->mPlugin;
	}

	return NULL;
}

	}; // namespace nPlugin
}; // namespace nVerliHub
