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

#include "cpiforbid.h"

namespace nVerliHub {
	using namespace nSocket;
	using namespace nProtocol;
	using namespace nEnums;
	namespace nForbidPlugin {

cpiForbid::cpiForbid() : mCfg(NULL)
{
	mName = "ForbiddenWords";
	mVersion = FORBID_VERSION;
}

void cpiForbid::OnLoad(cServerDC *server)
{
	tpiForbidBase::OnLoad(server);
	mCfg = new cForbidCfg(mServer);
	mCfg->Load();
	mCfg->Save();
}

bool cpiForbid::RegisterAll()
{
	RegisterCallBack("VH_OnOperatorCommand");
	RegisterCallBack("VH_OnParsedMsgChat");
	RegisterCallBack("VH_OnParsedMsgPM");
	return true;
}

cpiForbid::~cpiForbid()
{
	if (mCfg) delete mCfg;
	mCfg = NULL;
}

bool cpiForbid::OnParsedMsgPM(cConnDC *conn, cMessageDC *msg)
{
	string text = msg->ChunkString(eCH_PM_MSG);

	cUser *dest = mServer->mUserList.GetUserByNick(msg->ChunkString(eCH_PM_TO));
	if(dest && dest->mxConn && (dest->mClass > mCfg->max_class_dest))
		return true;

	/** Check that the user inputs forbidden words into PM */
	if(mList->ForbiddenParser(text, conn , cForbiddenWorker::eCHECK_PM) == 0)
		return false;

	return true;
}

bool cpiForbid::OnParsedMsgChat(cConnDC *conn, cMessageDC *msg)
{
	string text = msg->ChunkString(eCH_CH_MSG);

	/** Check that the user inputs forbidden words into GLOBAL*/
	if(mList->ForbiddenParser(text, conn , cForbiddenWorker::eCHECK_CHAT) == 0)
		return false;

	if(conn->mpUser->mClass < eUC_OPERATOR)
	{
		/** Check repeating characters */
		if((mCfg->max_repeat_char != 0) && (!mList->CheckRepeat(text, mCfg->max_repeat_char)))
		{
			/** send only to the bitch */
			mServer->DCPublic(conn->mpUser->mNick,text,conn);
			return false;
		}

		if(!mList->CheckUppercasePercent(text, mCfg->max_upcase_percent))
		{
			/** send only to the bitch */
			mServer->DCPublic(conn->mpUser->mNick,text,conn);
			return false;
		}
	}
	return true;
}

bool cpiForbid::OnOperatorCommand(cConnDC *conn, string *str)
{
	if( mConsole.DoCommand(*str, conn) ) return false;
	return true;
}
	}; // namespace nForbidPlugin
}; // namespace nVerliHub

REGISTER_PLUGIN(nVerliHub::nForbidPlugin::cpiForbid);
