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

#ifndef _CPIFLOODPROT_H_
#define _CPIFLOODPROT_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "cfloodprotect.h"
#include "src/ctime.h"
#include "src/cconndc.h"
#include "src/cvhplugin.h"
#include "src/cmessagedc.h"
#include "src/cserverdc.h"

namespace nVerliHub {
	namespace nFloodProtectPlugin {
class cpiFloodprot : public nPlugin::cVHPlugin
{
public:
	cpiFloodprot();
	virtual ~cpiFloodprot();
	virtual void OnLoad(cServerDC *);
	virtual bool RegisterAll();
	virtual bool OnTimer(__int64 msec);
	virtual bool OnParsedMsgChat(nSocket::cConnDC *, nProtocol::cMessageDC *);
	virtual bool OnParsedMsgPM(nSocket::cConnDC *, nProtocol::cMessageDC *);
	virtual bool OnParsedMsgSearch(nSocket::cConnDC *, nProtocol::cMessageDC *);
	virtual bool OnParsedMsgMyINFO(nSocket::cConnDC *, nProtocol::cMessageDC *);
	virtual bool OnNewConn(nSocket::cConnDC *);
	virtual bool OnUserLogin(cUser *);
	virtual bool OnUserLogout(cUser *);
	virtual bool OnCloseConn(nSocket::cConnDC *);

	cFloodprotect * mFloodprotect;
};
	}; // namespace nFloodProtectPlugin
}; // namespace nVerliHub
#endif
