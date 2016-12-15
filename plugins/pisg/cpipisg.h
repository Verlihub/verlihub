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

#ifndef CPIPISG_H
#define CPIPISG_H

#include "src/cvhplugin.h"
#include "src/ctimeout.h"
#include "src/cmeanfrequency.h"
#include <fstream>

using namespace std;
using namespace nVerliHub::nPlugin;
using namespace nVerliHub;
using namespace nUtils;

class cpiPisg : public cVHPlugin
{
public:
	cpiPisg();
	virtual ~cpiPisg();
	virtual bool RegisterAll();
	bool OnParsedMsgChat(cConnDC *conn, cMessageDC *msg);
private:
	ofstream logFile;
};

#endif
