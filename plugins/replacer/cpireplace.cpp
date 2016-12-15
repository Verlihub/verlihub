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

#include "cpireplace.h"

namespace nVerliHub {
	using namespace nSocket;
	using namespace nProtocol;
	using namespace nEnums;
	namespace nReplacePlugin {

cpiReplace::cpiReplace() : mConsole(this), mReplacer(NULL), mCfg(NULL)
{
	mName = "Replacer";
	mVersion = REPLACER_VERSION;
}

cpiReplace::~cpiReplace()
{
	if(mReplacer)
		delete mReplacer;
	mReplacer = NULL;
	if(mCfg)
		delete mCfg;
	mCfg = NULL;
}


void cpiReplace::OnLoad(cServerDC *server)
{
	cVHPlugin::OnLoad(server);
	mReplacer = new cReplacer(server);
	mReplacer->CreateTable();
	mReplacer->LoadAll();
	mCfg = new cReplaceCfg(mServer);
	mCfg->Load();
	mCfg->Save();
}

bool cpiReplace::RegisterAll()
{
	RegisterCallBack("VH_OnOperatorCommand");
	RegisterCallBack("VH_OnParsedMsgChat");
	return true;
}

bool cpiReplace::OnParsedMsgChat(cConnDC *conn, cMessageDC *msg)
{
	string & text = msg->ChunkString(eCH_CH_MSG);
	text = mReplacer->ReplacerParser(text, conn);

	msg->ApplyChunk(eCH_CH_MSG);

	return true;
}

bool cpiReplace::OnOperatorCommand(cConnDC *conn, string *str)
{
	if(mConsole.DoCommand(*str, conn))
		return false;
	return true;
}
	}; // namespace nReplacePlugin
}; // namespace nVerliHub

REGISTER_PLUGIN(nVerliHub::nReplacePlugin::cpiReplace);
