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

#ifndef CPIPLUG_H
#define CPIPLUG_H

#include <tlistplugin.h>
#include "cconsole.h"

namespace nVerliHub {
	namespace nSocket {
		class cConnDC;
		class cServerDC;
	};

	namespace nPlugMan {
		class cPlug;
		class cPlugs;

typedef tpiListPlugin<cPlugs,cPlugConsole> tpiPlugBase;

class cpiPlug : public tpiPlugBase
{
public:
	cpiPlug();
	virtual ~cpiPlug();
	virtual void OnLoad(nSocket::cServerDC *server);

	virtual bool RegisterAll();
	virtual bool OnOperatorCommand(nSocket::cConnDC *, string *);
};
	}; // namespace nPlugMan
}; // namespace nVerliHub
#endif
