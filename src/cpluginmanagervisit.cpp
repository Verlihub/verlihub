/*
	Copyright (C) 2006-2026 Verlihub Team, info at verlihub dot net

	Verlihub is free software; You can redistribute it
	and modify it under the terms of the GNU General
	Public License as published by the Free Software
	Foundation, either version 3 of the license, or at
	your option any later version.
*/

#include "cpluginmanager.h"
#include "cpluginloader.h"

namespace nVerliHub {
	namespace nPlugin {

bool cPluginManager::ForEachPlugin(tPluginVisitor visitor, void *data)
{
	if (!visitor)
		return false;

	for (tPlugins::iterator it = mPlugins.begin(); it != mPlugins.end(); ++it) {
		cPluginLoader *loader = *it;

		if (!loader || !loader->mPlugin)
			continue;

		if (!visitor(loader->mPlugin, data))
			return false;
	}

	return true;
}

	}; // namespace nPlugin
}; // namespace nVerliHub
