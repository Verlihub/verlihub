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

#include "cvhpluginmgr.h"
#include "cvhplugin.h"

namespace nVerliHub {
	using namespace nSocket;
	namespace nPlugin {

cVHPluginMgr::cVHPluginMgr(cServerDC *server,const string &pluginDir):
	cPluginManager(pluginDir), mServer(server)
{
	SetClassName("cVHPluginMgr");
	vhLog(0) << "Plugin working directory: " << pluginDir << endl;
}

cVHPluginMgr::~cVHPluginMgr()
{}

void cVHPluginMgr::OnPluginLoad(cPluginBase *pi)
{
	vhLog(0) << "OnPluginLoad: " << pi->Name() << endl;
	((cVHPlugin *)pi)->OnLoad(mServer);
}

	}; // namespace nPlugin
}; // namespace nVerliHub
