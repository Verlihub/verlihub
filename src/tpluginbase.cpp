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

#include "tpluginbase.h"
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <dlfcn.h>

namespace nVerliHub {
	namespace nPlugin {

tPluginBase::tPluginBase():cObj("PluginBase"), mHandle(NULL)
{}

tPluginBase::~tPluginBase()
{}

bool tPluginBase::Open()
{
	/*
	#ifdef _WIN32
	mHandle = LoadLibrary(mFileName.c_str());
	if(mHandle == NULL)
	#else
	*/

	#ifdef HAVE_FREEBSD
	/*
	* Reset dlerror() since it can contain error from previous
	* call to dlopen()/dlsym().
	*/
	dlerror();
	#endif

	mHandle = dlopen(mFileName.c_str(), RTLD_NOW | RTLD_LAZY | RTLD_GLOBAL);
	if(!mHandle)
	//#endif
	{
		if(ErrLog(1)) LogStream() << "Can't open plugin '" << mFileName << "' because:" << Error() << endl;
		return false;
	}
	return true;
}

bool tPluginBase::Close()
{
	/*
	#ifdef _WIN32
	if(!FreeLibrary(mHandle))
	#else
	*/
	if(dlclose(mHandle))
	//#endif
	{
		if(ErrLog(1)) LogStream() << "Can't close :" << Error() << endl;
	}
	return true;
}

string tPluginBase::Error()
{
	const char *error;
	/*
	#ifdef _WIN32
		LPVOID buff;
		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_HMODULE,
			mHandle,
			GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPTSTR) &buff,
			0,
			NULL
		);
		error= (const char *) buff;
		LocalFree(buff);
	#else
	*/
	error = dlerror();
	//#endif
	return string(error?error:"ok");
}

int tPluginBase::StrLog(ostream & ostr, int level)
{
	if(cObj::StrLog(ostr,level))
	{
		LogStream()   << '(' << mName << ") ";
		return 1;
	}
	return 0;
}

	}; // namespace nPlugin
}; // namespace nVerliHub
