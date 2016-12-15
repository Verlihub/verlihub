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

#ifndef CPIISP_H
#define CPIISP_H

#include "src/tlistplugin.h"
#include "cisps.h"
#include "cconsole.h"
#include "src/cserverdc.h"
#include "src/cconndc.h"
#include "src/cmessagedc.h"

namespace nVerliHub {
	namespace nIspPlugin {

typedef tpiListPlugin<cISPs,cISPConsole> tpiISPBase;

class cpiISP : public tpiISPBase
{
public:
	cpiISP();
	virtual ~cpiISP();
	virtual void OnLoad(nSocket::cServerDC *server);

	virtual bool RegisterAll();
	virtual bool OnParsedMsgMyINFO(nSocket::cConnDC * conn, nProtocol::cMessageDC *mess);
	virtual bool OnParsedMsgValidateNick(nSocket::cConnDC * conn, nProtocol::cMessageDC *mess);
	virtual bool OnOperatorCommand(nSocket::cConnDC *, string *);
	cISPCfg *mCfg;
};
	}; // namespace nIspPlugin
}; // namespace nVerliHub

#endif
