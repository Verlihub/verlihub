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

#include "cvhplugin.h"
#include "cuser.h"
#include "cserverdc.h"
#include "i18n.h"

namespace nVerliHub {
	using namespace nEnums;
	using namespace nSocket;
	namespace nPlugin {
cVHPlugin::cVHPlugin()
{
	mServer = 0;
	mUserDataTable = NULL;
}


cVHPlugin::~cVHPlugin()
{
	cUserCollection::iterator it;
	cPluginRobot * robot;
	for(it = mRobots.begin(); it != mRobots.end();) {
		robot = (cPluginRobot *) *it;
		++it;
		DelRobot(robot);
	}
	if (mUserDataTable) {
		delete mUserDataTable;
		mUserDataTable = NULL;
	}
}

bool cVHPlugin::AddRobot(cUserRobot *robot)
{
	if (!mServer->AddRobot(robot))
		return false;

	if (!mRobots.Add((cUser*)robot)) {
		mServer->DelRobot(robot);
		return false;
	}

	return true;
}

cPluginRobot * cVHPlugin::NewRobot(const string &Nick, int uclass)
{
	cPluginRobot *robot = new cPluginRobot(Nick, this, mServer);
	//set class before adding to list, otherwise user can't be op
	robot->mClass = tUserCl(uclass);
	if (AddRobot(robot))
		return robot;
	else {
		delete robot;
		return NULL;
	}
}

bool cVHPlugin::DelRobot(cUserRobot *robot)
{
	bool result = mRobots.Remove(robot);
	mServer->DelRobot(robot);
	delete robot;
	return result;
}

bool cVHPlugin::AddScript(const string &filename, ostream &os)
{
	os << autosprintf(_("Plugin %s %s cannot load extra script."), mName.c_str(), mVersion.c_str());
	return false;
}

bool cVHPlugin::LoadScript(const string &filename, ostream &os)
{
	os << autosprintf(_("Plugin %s %s cannot load script '%s'."), mName.c_str(), mVersion.c_str(), filename.c_str());
	return false;
}

cPluginUserData *cVHPlugin::GetPluginUserData( cUser * User )
{
	if (mUserDataTable) {
		tHashArray<cPluginUserData*>::tHashType Hash = (tHashArray<cPluginUserData*>::tHashType) User;
		return mUserDataTable->GetByHash(Hash);
	} else return NULL;
}

cPluginUserData *cVHPlugin::SetPluginUserData( cUser *User, cPluginUserData *NewData )
{
	if (!User || !NewData)
		return NULL;

	if (!mUserDataTable)
		mUserDataTable = new tHashArray<cPluginUserData*>();

	tHashArray<cPluginUserData*>::tHashType Hash = (tHashArray<cPluginUserData*>::tHashType) User;
	cPluginUserData *OldData = mUserDataTable->GetByHash(Hash);
	mUserDataTable->SetByHash(Hash, NewData);
	return OldData;
}

	}; // namespace nPlugin
}; // namespace nVerliHub
