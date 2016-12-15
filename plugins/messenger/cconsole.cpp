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

#include "src/cconndc.h"
#include "src/cserverdc.h"
#include "src/i18n.h"
#include "cconsole.h"
#include "cpimessanger.h"
#include "cmsglist.h"

namespace nVerliHub {
	using namespace nSocket;
	namespace nMessangerPlugin {

cConsole::cConsole(cpiMessanger *msn) :
	mMessanger(msn),
	mCmdMsgSend(0,"\\+msgsend ", "(\\S+)([^\\m\\n\\-]*)?(\\m?\\n|--)(.*)", &mcfMsgSend),
	mCmdMsgRead(1,"\\+msgread", "", &mcfMsgRead),
	mCmdr(this)
{
	mCmdr.Add(&mCmdMsgSend);
	mCmdr.Add(&mCmdMsgRead);
}


cConsole::~cConsole()
{}

int cConsole::DoCommand(const string &str, cConnDC * conn)
{
	ostringstream os;
	if(mCmdr.ParseAll(str, os, conn) >= 0) {
		mMessanger->mServer->DCPublicHS(os.str().data(),conn);
		return 1;
	}
	return 0;
}

bool cConsole::cfMessageSend::operator ( )()
{
	sMessage msg;
	cUser * receiver;
	msg.mSender = ((cConnDC*)this->mExtra)->mpUser->mNick;
	msg.mDateSent = cTime().Sec();
	msg.mDateExpires = msg.mDateSent + 7 * 24* 3600;
	msg.mSenderIP = ((cConnDC*)this->mExtra)->AddrIP();
	this->GetParStr(1,msg.mReceiver);
	this->GetParStr(2,msg.mSubject);
	this->GetParStr(4,msg.mBody);

	receiver = GetMessanger()->mServer->mUserList.GetUserByNick(msg.mReceiver);
	if ((receiver != NULL) && (receiver->mxConn != NULL)) {
		GetMessanger()->mMsgs->DeliverOnline(receiver, msg);
		(*mOS) << autosprintf(_("The message has been sent to %s because he is online."), msg.mReceiver.c_str());
	} else  {
		GetMessanger()->mMsgs->AddMessage(msg);
		(*mOS) << _("Message saved.");
	}
	return true;
}

bool cConsole::cfMessageRead::operator ( )()
{
	int messages = this->GetMessanger()->mMsgs->CountMessages(((cConnDC*) this->mExtra)->mpUser->mNick, false);
	if (messages) {
		(*mOS) << autosprintf(ngettext("You have %d message in your box.", "You have %d messages in your box.", messages), messages) << "\r\n";
		this->GetMessanger()->mMsgs->PrintSubjects(*mOS, ((cConnDC*) this->mExtra)->mpUser->mNick, false);
	}
	else
		(*mOS) << _("You have no new message in your box.");
	return true;
}

	}; // namespace nMessangerPlugin
}; // namespace nVerliHub
