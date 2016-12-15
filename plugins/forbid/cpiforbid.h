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

#ifndef CPIFORBID_H
#define CPIFORBID_H

#include "src/tlistplugin.h"
#include "cconsole.h"
#include "cforbidden.h"
#include "src/cmessagedc.h"
#include "src/cconndc.h"
#include "src/cserverdc.h"

namespace nVerliHub {
	namespace nForbidPlugin {

typedef tpiListPlugin<cForbidden, cForbidConsole> tpiForbidBase;

/**
\brief a forbidden words plugin for verlihub

users may leave offline messages for registered users or reg users may leave offline messages for anyone

@author bourne
@author Daniel Muller (plugin part of it)

*/
class cpiForbid : public tpiForbidBase
{
public:
	cpiForbid();
	virtual ~cpiForbid();
	virtual bool RegisterAll();
	virtual bool OnOperatorCommand(nSocket::cConnDC *, string *);
	virtual bool OnParsedMsgChat(nSocket::cConnDC *, nProtocol::cMessageDC *);
	virtual bool OnParsedMsgPM(nSocket::cConnDC *, nProtocol::cMessageDC *);
	virtual void OnLoad(nSocket::cServerDC *);
	cForbidCfg *mCfg;
};
	}; // namespace nForbidPlugin
}; // namespace nVerliHub
#endif
