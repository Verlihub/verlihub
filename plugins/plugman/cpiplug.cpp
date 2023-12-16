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

#include "cpiplug.h"
#include "cplugs.h"
#include "cserverdc.h"

namespace nVerliHub {
	using namespace nSocket;

	namespace nPlugMan {

cpiPlug::cpiPlug()
{
	mName = PLUGMAN_NAME;
	mVersion = PLUGMAN_VERSION;
}

void cpiPlug::OnLoad(cServerDC *server)
{
	tpiPlugBase::OnLoad(server);
	mList->mPM = &(server->mPluginManager);
	mList->mVHTime = mList->GetFileTime(mServer->mExecPath);
	mList->PluginAll(ePLUG_AUTOLOAD);
}

cpiPlug::~cpiPlug() {}

bool cpiPlug::RegisterAll()
{
	RegisterCallBack("VH_OnOperatorCommand");
	return true;
}

bool cpiPlug::OnOperatorCommand(cConnDC *conn, string *str)
{
	// Redirect command to plugman console
	if(mConsole.DoCommand(*str, conn))
		return false;
	return true;
}
	}; // namespace nPlugMan
}; // namespace nVerliHub

REGISTER_PLUGIN(nVerliHub::nPlugMan::cpiPlug);
