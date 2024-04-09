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

#include "cpluginloader.h"

namespace nVerliHub {
	namespace nPlugin {

cPluginLoader::cPluginLoader(const string &filename):
	cObj("cPluginLoader"),
	mPlugin(NULL),
	mFileName(filename),
	mError(NULL),
	mHandle(NULL),
	mcbDelPluginFunc(NULL),
	mcbGetPluginFunc(NULL)
{}

cPluginLoader::~cPluginLoader()
{
	Close();
}

bool cPluginLoader::Open()
{
	/*
	#ifdef _WIN32
	mHandle = LoadLibrary(mFileName.c_str());
	*/

	dlerror(); // reset
	mHandle = dlopen(mFileName.c_str(), RTLD_NOW | RTLD_LAZY | RTLD_GLOBAL);

	if (!mHandle || IsError()) {
		if (!mHandle) // call it again
			IsError();

		if (ErrLog(0))
			LogStream() << "Unable to open plugin: " << mFileName << " [ " << Error() << " ]" << endl;

		return false;
	}

	return true;
}

bool cPluginLoader::Close()
{
	if (mPlugin && mcbDelPluginFunc) {
		try {
			mcbDelPluginFunc(mPlugin);
		} catch (const char *ex) {
			if (ErrLog(0))
				LogStream() << "Plugin delete function execution error: " << mFileName << " [ " << ex << " ]" << endl;
		}

		mPlugin = NULL;
	}

	/*
	#ifdef _WIN32
	FreeLibrary(mHandle)
	*/

	if (mHandle) {
		dlerror(); // reset
		dlclose(mHandle);
		mHandle = NULL;
	}

	if (IsError()) {
		if (ErrLog(0))
			LogStream() << "Unable to close plugin: " << mFileName << " [ " << Error() << " ]" << endl;

		return false;
	}

	return true;
}

bool cPluginLoader::LoadSym()
{
	if (!mcbGetPluginFunc)
		mcbGetPluginFunc = tcbGetPluginFunc(LoadSym("get_plugin"));

	if (!mcbGetPluginFunc) {
		mError = "Missing plugin get function.";
		return false;
	}

	if (!mcbDelPluginFunc)
		mcbDelPluginFunc = tcbDelPluginFunc(LoadSym("del_plugin"));

	if (!mcbDelPluginFunc) {
		mError = "Missing plugin delete function.";
		return false;
	}

	try {
		mPlugin = mcbGetPluginFunc();
	} catch (const char *ex) {
		if (ErrLog(0))
			LogStream() << "Plugin get function execution error: " << mFileName << " [ " << ex << " ]" << endl;

		mPlugin = NULL;
	}

	return (mPlugin != NULL);
}

void* cPluginLoader::LoadSym(const char *name)
{
	//if (!mHandle)
		//return NULL;

	/*
	#ifdef _WIN32
	void *func = (void*)GetProcAddress(mHandle, name);
	*/

	dlerror(); // reset
	void *func = dlsym(mHandle, name);

	if (!func || IsError()) {
		if (!func) // call it again
			IsError();

		if (ErrLog(0))
			LogStream() << "Unable to load " << name << " exported interface for plugin: " << mFileName << " [ " << Error() << " ]" << endl;

		return NULL;
	}

	return func;
}

int cPluginLoader::StrLog(ostream &ostr, int level)
{
	if (cObj::StrLog(ostr, level)) {
		LogStream() << '(' << mFileName << ") ";
		return 1;
	}

	return 0;
}

	}; // namespace nPlugin
}; // namespace nVerliHub
