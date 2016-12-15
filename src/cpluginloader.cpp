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

#include "cpluginloader.h"

namespace nVerliHub {
	namespace nPlugin {

cPluginLoader::cPluginLoader(const string &filename) :
	cObj("cPluginLoader"),
	mFileName(filename)
{
	mHandle = NULL;
	mError = NULL;
	mPlugin = NULL;
	mcbGetPluginFunc = NULL;
	mcbDelPluginFunc = NULL;
}


cPluginLoader::~cPluginLoader()
{
	if (mHandle) Close();
	if (mPlugin && mcbDelPluginFunc)
	{
		mcbDelPluginFunc(mPlugin);
		mPlugin = NULL;
	}
}

bool cPluginLoader::Open()
{
	#ifdef _WIN32
	mHandle = LoadLibrary(mFileName.c_str());
	if(mHandle == NULL) {
	#else

	#ifdef HAVE_FREEBSD
	/*
	* Reset dlerror() since it can contain error from previous
	* call to dlopen()/dlsym().
	*/
	dlerror();
	#endif

	mHandle = dlopen(mFileName.c_str(), RTLD_NOW | RTLD_LAZY | RTLD_GLOBAL);
	if(!mHandle || IsError()) // Note that || operator evaluates only the first statement if that one is true
	{
		if (!mHandle) IsError(); // Call it again
	#endif
		if(ErrLog(1)) LogStream() << "Cannot open plugin '" << mFileName << "': " << Error() << endl;
		return false;
	}
	return true;
}

bool cPluginLoader::Close()
{
	mcbDelPluginFunc(mPlugin);
	mPlugin = NULL;
	#ifdef _WIN32
	if(!FreeLibrary(mHandle))
	#else
	dlclose(mHandle);
	if(IsError())
	#endif
	{
		if(ErrLog(1)) LogStream() << "Cannot close plugin:" << Error() << endl;
		return false;
	}
	mHandle = NULL;
	return true;
}

/** log the event */
int cPluginLoader::StrLog(ostream & ostr, int level)
{
	if(cObj::StrLog(ostr,level))
	{
		LogStream()   << "(" << mFileName << ") ";
		return 1;
	}
	return 0;
}

bool cPluginLoader::LoadSym()
{
	#ifdef HAVE_FREEBSD
		/*
			Reset dlerror() since it can contain error from previous call to dlopen() or dlsym()
		*/

		dlerror();
	#endif

	if (!mcbGetPluginFunc)
		mcbGetPluginFunc = tcbGetPluginFunc(LoadSym("get_plugin"));

	if (!mcbDelPluginFunc)
		mcbDelPluginFunc = tcbDelPluginFunc(LoadSym("del_plugin"));

	if (!mcbGetPluginFunc || !mcbDelPluginFunc)
		return false;

	return (mPlugin = mcbGetPluginFunc()) != NULL;
}

void * cPluginLoader::LoadSym(const char *name)
{
	#ifdef _WIN32
	void *func = (void *) GetProcAddress(mHandle, name);
	if(func == NULL) {
		if(ErrLog(1)) LogStream() << "Can't load " << name <<" exported interface :" << GetLastError() << endl;
		return NULL;
	}
	#else
	void *func = dlsym( mHandle, name);
	if(IsError())
	{
		if(ErrLog(1)) LogStream() << "Can't load " << name <<" exported interface :" << Error() << endl;
		return NULL;
	}
	#endif
	return func;
}
	}; // namespace nPlugin
}; // namespace nVerliHub
