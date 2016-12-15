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

#ifndef CPIREPLACE_H
#define CPIREPLACE_H

#include "src/cconndc.h"
#include "src/cserverdc.h"
#include "src/cvhplugin.h"
#include "cconsole.h"
#include "creplacer.h"

namespace nVerliHub {
	namespace nReplacePlugin {

/**
\brief a plugin that replaces chosen words with other in main chat of DC
@author bourne
@author Daniel Muller (plugin part of it)
@author Pralcio
*/
class cpiReplace : public nPlugin::cVHPlugin
{
public:
	cpiReplace();
	virtual ~cpiReplace();
	virtual bool RegisterAll();
	virtual bool OnOperatorCommand(nSocket::cConnDC *, string *);
	virtual bool OnParsedMsgChat(nSocket::cConnDC *, nProtocol::cMessageDC *);
	virtual void OnLoad(nSocket::cServerDC *);
	cConsole mConsole;
	cReplacer *mReplacer;
	cReplaceCfg *mCfg;
};

	}; // namespace nReplacePlugin
}; // namespace nVerliHub

#endif
