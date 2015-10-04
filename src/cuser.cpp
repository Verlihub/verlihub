/*
	Copyright (C) 2003-2005 Daniel Muller, dan at verliba dot cz
	Copyright (C) 2006-2015 Verlihub Team, info at verlihub dot net

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

#include "cuser.h"
#include "cdcproto.h"
#include "cchatconsole.h"
#include "cserverdc.h"
#include "cban.h"
#include "cbanlist.h"
#include "i18n.h"
#include "stringutils.h"
#include <sys/time.h>

#define PADDING 25

namespace nVerliHub {
	using namespace nProtocol;
	using namespace nSocket;
	using namespace nUtils;
	using namespace nEnums;

cUserBase::cUserBase():
	cObj((const char *)"User"),
	mClass(eUC_NORMUSER),
	mInList(false)
{}

cUserBase::~cUserBase()
{}

cUserBase::cUserBase(const string &nick):
	cObj((const char *)"User"),
	mNick(nick),
	mClass(eUC_NORMUSER),
	mInList(false)
{}

bool cUserBase::CanSend()
{
	return false;
}

bool cUserBase::HasFeature(unsigned feature)
{
	return false;
}

void cUserBase::Send(string &data, bool, bool)
{}

cUser::cUser():
	mMyFlag(0),
	mRights(0),
	mBanTime(0),
	mToBan(false),
	mShare(0),
	mSearchNumber(0),
	mHideKicksForClass(eUC_NORMUSER)//,
	//mVisibleClassMin(eUC_NORMUSER),
	//mOpClassMin(eUC_NORMUSER)
{
	mxConn = NULL;
	mxServer = NULL;
	SetRight( eUR_CHAT,0, true );
	SetRight( eUR_PM,0, true );
	SetRight( eUR_SEARCH, 0, true );
	SetRight( eUR_CTM, 0, true );
	SetRight( eUR_KICK, 0, false );
	SetRight( eUR_REG, 0, false );
	SetRight( eUR_OPCHAT, 0, false );
	SetRight( eUR_DROP, 0, false );
	SetRight( eUR_TBAN, 0, false );
	SetRight( eUR_PBAN, 0, false );
	SetRight( eUR_NOSHARE, 0, false );
	mProtectFrom = 0;
	mHideKick = false;
	mHideShare = false;
	mHideCtmMsg = false;
	mSetPass = false;
	IsPassive = true;
	memset(mFloodHashes, 0, sizeof(mFloodHashes));
	memset(mFloodCounters, 0, sizeof(mFloodCounters));
}

/** Constructor */
cUser::cUser(const string &nick):
	cUserBase(nick),
	mxConn(NULL),
	mxServer(NULL),
	mMyFlag(0),
	mBanTime(0),
	mShare(0),
	mSearchNumber(0),
	mHideKicksForClass(eUC_NORMUSER)//,
	//mVisibleClassMin(eUC_NORMUSER),
	//mOpClassMin(eUC_NORMUSER)
{
	SetClassName("cUser");
	IsPassive = true;
	mRights = 0;
	mToBan = false;
	SetRight( eUR_CHAT, 0, true );
	SetRight( eUR_PM, 0, true );
	SetRight( eUR_SEARCH, 0, true );
	SetRight( eUR_CTM, 0, true );
	SetRight( eUR_KICK, 0, false );
	SetRight( eUR_REG, 0, false );
	SetRight( eUR_OPCHAT, 0, false );
	SetRight( eUR_DROP, 0, false );
	SetRight( eUR_TBAN, 0, false );
	SetRight( eUR_PBAN, 0, false );
	SetRight( eUR_NOSHARE, 0, false );
	mProtectFrom = 0;
	mHideKick = false;
	mHideShare = false;
	mHideCtmMsg = false;
	mSetPass = false;
	memset(mFloodHashes, 0, sizeof(mFloodHashes));
	memset(mFloodCounters, 0, sizeof(mFloodCounters));
}

cUser::~cUser()
{}

bool cUser::CanSend()
{
	return mInList && mxConn && mxConn->ok;
}

bool cUser::HasFeature(unsigned feature)
{
	return mxConn && (mxConn->mFeatures & feature);
}

void cUser::Send(string &data, bool pipe, bool flush)
{
	mxConn->Send(data, pipe, flush);
}

/** return true if user needs a password and the password is correct */
bool cUser::CheckPwd(const string &pwd)
{
	if(!mxConn || !mxConn->mRegInfo)
		return false;
	return mxConn->mRegInfo->PWVerify(pwd);
}

/** perform a registration: set class, rights etc... precondition: password was al right */
void cUser::Register()
{
	if (!mxConn || !mxConn->mRegInfo)
		return;

	if (mxConn->mRegInfo->mPwdChange)
		return;

	mClass = (tUserCl)mxConn->mRegInfo->mClass;
	mProtectFrom = mxConn->mRegInfo->mClassProtect;
	mHideKicksForClass = mxConn->mRegInfo->mClassHideKick;
	mHideKick = mxConn->mRegInfo->mHideKick;
	mHideShare = mxConn->mRegInfo->mHideShare;
	mHideCtmMsg = mxConn->mRegInfo->mHideCtmMsg;

	if (mClass == eUC_PINGER) {
		SetRight(eUR_CHAT, 0, false);
		SetRight(eUR_PM, 0, false);
		SetRight(eUR_SEARCH, 0, false);
		SetRight(eUR_CTM, 0, false);
		SetRight(eUR_KICK, 0, false);
		SetRight(eUR_REG, 0, false);
		SetRight(eUR_OPCHAT, 0, false);
		SetRight(eUR_DROP, 0, false);
		SetRight(eUR_TBAN, 0, false);
		SetRight(eUR_PBAN, 0, false);
		SetRight(eUR_NOSHARE, 0, true);
	}
}

/*
long cUser::ShareEnthropy(const string &sharesize)
{
	char diff[20];
	int count[20];

	size_t i,j;
	long score = 0;
	// calculate counts of every byte in the sharesize string
	for(i = 0; i < sharesize.size(); i++) {
		count[i] = 0;
		for(j = i+1; j < sharesize.size(); j++)
			if(sharesize[i]==sharesize[j]) ++ count[i];
	}

	// make the weighted sum
	for(i = 0; i < sharesize.size();)
		score += count[i] * ++i;

	// calculate the differences
	for(i = 0; i < sharesize.size()-1; i++)
		diff[i] = 10 + sharesize[i-1] - sharesize[i];

	// calculate counts of every byte in the sharesize string differences
	for(i = 0; i < sharesize.size()-1; i++) {
		count[i] = 0;
		for(j = i+1; j < sharesize.size(); j++)
			if(diff[i]==diff[j]) ++ count[i];
	}
	for(i = 0; i < sharesize.size();)
		score += count[i] * ++i;

	return score;
}
*/

void cUser::DisplayInfo(ostream &os, int DisplClass)
{
	//static const char *ClassName[] = {"Guest", "Registred", "VIP", "Operator", "Cheef", "Admin", "6-err", "7-err", "8-err", "9-err", "Master"};
	//toUpper(ClassName[this->mClass])
	os << " [*] " << setw(PADDING) << setiosflags(ios::left) << _("Nickname") << mNick << "\r\n";
	if (this->mClass != this->mxConn->GetTheoricalClass()) os << " [*] " << setw(PADDING) << setiosflags(ios::left) << _("Default class") << this->mxConn->GetTheoricalClass() << "\r\n";
	if (DisplClass >= eUC_CHEEF) os << " [*] " << setw(PADDING) << setiosflags(ios::left) << _("In list") << this->mInList << "\r\n";

	if (!this->mxConn)
		os << " [*] " << setw(PADDING) << setiosflags(ios::left) << _("Special user") << _("Yes") << "\r\n";
	else {
		if (DisplClass >= eUC_OPERATOR) os << " [*] " << setw(PADDING) << setiosflags(ios::left) << _("IP") << mxConn->AddrIP() << "\r\n";
		if (DisplClass >= eUC_OPERATOR && mxConn->AddrHost().size()) os << " [*] " << setw(PADDING) << setiosflags(ios::left) << _("Host") << mxConn->AddrHost() << "\r\n";
		if (mxConn->mCC.size()) os << " [*] " << setw(PADDING) << setiosflags(ios::left) << _("Country") << mxConn->mCC << "=" << mxConn->mCN << "\r\n";
		if (mxConn->mCity.size()) os << " [*] " << setw(PADDING) << setiosflags(ios::left) << _("City") << mxConn->mCity << "\r\n";
		if (mxConn->mRegInfo != NULL) os << *(mxConn->mRegInfo);
	}
}

void cUser::DisplayRightsInfo(ostream &os, bool head)
{
	cTime now = cTime().Sec();

	if (head) {
		os << _("User rights information") << ":\r\n";
		os << " [*] " << autosprintf(_("Nick: %s"), this->mNick.c_str()) << "\r\n";
		os << " [*] " << autosprintf(_("Class: %d"), this->mClass) << "\r\n";
	}

	// main chat
	if (this->mClass >= eUC_ADMIN)
		os << " [*] " << autosprintf(_("Can use main chat: %s"), _("Yes")) << "\r\n";
	else if (!this->mGag)
		os << " [*] " << autosprintf(_("Can use main chat: %s"), _("No")) << "\r\n";
	else if (this->mGag > now) {
		ostringstream oss;
		oss << autosprintf(_("No [%s]"), cTime(this->mGag - now).AsPeriod().AsString().c_str());
		os << " [*] " << autosprintf(_("Can use main chat: %s"), oss.str().c_str()) << "\r\n";
	} else
		os << " [*] " << autosprintf(_("Can use main chat: %s"), _("Yes")) << "\r\n";

	// private chat
	if (this->mClass >= eUC_ADMIN)
		os << " [*] " << autosprintf(_("Can use private chat: %s"), _("Yes")) << "\r\n";
	else if (!this->mNoPM)
		os << " [*] " << autosprintf(_("Can use private chat: %s"), _("No")) << "\r\n";
	else if (this->mNoPM > now) {
		ostringstream oss;
		oss << autosprintf(_("No [%s]"), cTime(this->mNoPM - now).AsPeriod().AsString().c_str());
		os << " [*] " << autosprintf(_("Can use private chat: %s"), oss.str().c_str()) << "\r\n";
	} else
		os << " [*] " << autosprintf(_("Can use private chat: %s"), _("Yes")) << "\r\n";

	// operator chat
	if ((this->mClass < eUC_OPERATOR) && this->mCanOpchat && (this->mCanOpchat < now))
		os << " [*] " << autosprintf(_("Can use operator chat: %s"), _("No")) << "\r\n";
	else if (this->mCanOpchat > now) {
		ostringstream oss;
		oss << autosprintf(_("Yes [%s]"), cTime(this->mCanOpchat - now).AsPeriod().AsString().c_str());
		os << " [*] " << autosprintf(_("Can use operator chat: %s"), oss.str().c_str()) << "\r\n";
	} else
		os << " [*] " << autosprintf(_("Can use operator chat: %s"), _("Yes")) << "\r\n";

	// search files
	if (this->mClass >= eUC_ADMIN)
		os << " [*] " << autosprintf(_("Can search files: %s"), _("Yes")) << "\r\n";
	else if (!this->mNoSearch)
		os << " [*] " << autosprintf(_("Can search files: %s"), _("No")) << "\r\n";
	else if (this->mNoSearch > now) {
		ostringstream oss;
		oss << autosprintf(_("No [%s]"), cTime(this->mNoSearch - now).AsPeriod().AsString().c_str());
		os << " [*] " << autosprintf(_("Can search files: %s"), oss.str().c_str()) << "\r\n";
	} else
		os << " [*] " << autosprintf(_("Can search files: %s"), _("Yes")) << "\r\n";

	// download files
	if (this->mClass >= eUC_ADMIN)
		os << " [*] " << autosprintf(_("Can download files: %s"), _("Yes")) << "\r\n";
	else if (!this->mNoCTM)
		os << " [*] " << autosprintf(_("Can download files: %s"), _("No")) << "\r\n";
	else if (this->mNoCTM > now) {
		ostringstream oss;
		oss << autosprintf(_("No [%s]"), cTime(this->mNoCTM - now).AsPeriod().AsString().c_str());
		os << " [*] " << autosprintf(_("Can download files: %s"), oss.str().c_str()) << "\r\n";
	} else
		os << " [*] " << autosprintf(_("Can download files: %s"), _("Yes")) << "\r\n";

	// hide share
	if ((this->mClass < eUC_VIPUSER) && this->mCanShare0 && (this->mCanShare0 < now))
		os << " [*] " << autosprintf(_("Can hide share: %s"), _("No")) << "\r\n";
	else if (this->mCanShare0 > now) {
		ostringstream oss;
		oss << autosprintf(_("Yes [%s]"), cTime(this->mCanShare0 - now).AsPeriod().AsString().c_str());
		os << " [*] " << autosprintf(_("Can hide share: %s"), oss.str().c_str()) << "\r\n";
	} else
		os << " [*] " << autosprintf(_("Can hide share: %s"), _("Yes")) << "\r\n";

	// register users
	if ((this->mClass < mxServer->mC.min_class_register) && this->mCanReg && (this->mCanReg < now))
		os << " [*] " << autosprintf(_("Can register users: %s"), _("No")) << "\r\n";
	else if (this->mCanReg > now) {
		ostringstream oss;
		oss << autosprintf(_("Yes [%s]"), cTime(this->mCanReg - now).AsPeriod().AsString().c_str());
		os << " [*] " << autosprintf(_("Can register users: %s"), oss.str().c_str()) << "\r\n";
	} else
		os << " [*] " << autosprintf(_("Can register users: %s"), _("Yes")) << "\r\n";

	// drop users
	if ((this->mClass < eUC_OPERATOR) && this->mCanDrop && (this->mCanDrop < now))
		os << " [*] " << autosprintf(_("Can drop users: %s"), _("No")) << "\r\n";
	else if (this->mCanDrop > now) {
		ostringstream oss;
		oss << autosprintf(_("Yes [%s]"), cTime(this->mCanDrop - now).AsPeriod().AsString().c_str());
		os << " [*] " << autosprintf(_("Can drop users: %s"), oss.str().c_str()) << "\r\n";
	} else
		os << " [*] " << autosprintf(_("Can drop users: %s"), _("Yes")) << "\r\n";

	// kick users
	if ((this->mClass < eUC_OPERATOR) && this->mCanKick && (this->mCanKick < now))
		os << " [*] " << autosprintf(_("Can kick users: %s"), _("No")) << "\r\n";
	else if (this->mCanKick > now) {
		ostringstream oss;
		oss << autosprintf(_("Yes [%s]"), cTime(this->mCanKick - now).AsPeriod().AsString().c_str());
		os << " [*] " << autosprintf(_("Can kick users: %s"), oss.str().c_str()) << "\r\n";
	} else
		os << " [*] " << autosprintf(_("Can kick users: %s"), _("Yes")) << "\r\n";

	// temporarily ban users
	if ((this->mClass < eUC_OPERATOR) && this->mCanTBan && (this->mCanTBan < now))
		os << " [*] " << autosprintf(_("Can temporarily ban users: %s"), _("No")) << "\r\n";
	else if (this->mCanTBan > now) {
		ostringstream oss;
		oss << autosprintf(_("Yes [%s]"), cTime(this->mCanTBan - now).AsPeriod().AsString().c_str());
		os << " [*] " << autosprintf(_("Can temporarily ban users: %s"), oss.str().c_str()) << "\r\n";
	} else
		os << " [*] " << autosprintf(_("Can temporarily ban users: %s"), _("Yes")) << "\r\n";

	// permanently ban users
	if ((this->mClass < eUC_OPERATOR) && this->mCanPBan && (this->mCanPBan < now))
		os << " [*] " << autosprintf(_("Can permanently ban users: %s"), _("No"));
	else if (this->mCanPBan > now) {
		ostringstream oss;
		oss << autosprintf(_("Yes [%s]"), cTime(this->mCanPBan - now).AsPeriod().AsString().c_str());
		os << " [*] " << autosprintf(_("Can permanently ban users: %s"), oss.str().c_str());
	} else
		os << " [*] " << autosprintf(_("Can permanently ban users: %s"), _("Yes"));
}

bool cUser::Can(unsigned Right, long now, int OtherClass)
{
	if( mClass >= nEnums::eUC_ADMIN ) return true;
	switch(Right)
	{
		case nEnums::eUR_CHAT: if(!mGag || (mGag > now)) return false; break;
		case nEnums::eUR_PM  : if(!mNoPM || (mNoPM > now)) return false; break;
		case nEnums::eUR_SEARCH: if(!mNoSearch || (mNoSearch > now)) return false; break;
		case nEnums::eUR_CTM: if(!mNoCTM || (mNoCTM > now)) return false; break;
		case nEnums::eUR_KICK   : if((mClass < nEnums::eUC_OPERATOR) && mCanKick && (mCanKick < now)) return false; break;
		case nEnums::eUR_DROP   : if((mClass < nEnums::eUC_OPERATOR) && mCanDrop && (mCanDrop < now)) return false; break;
		case nEnums::eUR_TBAN   : if((mClass < nEnums::eUC_OPERATOR) && mCanTBan && (mCanTBan < now)) return false; break;
		case nEnums::eUR_PBAN   : if((mClass < nEnums::eUC_OPERATOR) && mCanPBan && (mCanPBan < now)) return false; break;
		case nEnums::eUR_NOSHARE: if((mClass < nEnums::eUC_VIPUSER ) && mCanShare0 && (mCanShare0 < now)) return false; break;
		case nEnums::eUR_REG: if((mClass < mxServer->mC.min_class_register ) && mCanReg && (mCanReg < now)) return false; break;
		case nEnums::eUR_OPCHAT: if((mClass < eUC_OPERATOR ) && mCanOpchat && (mCanOpchat < now)) return false; break;
		default: break;
	};
	return true;
}

void cUser::SetRight(unsigned Right, long until, bool allow, bool notify)
{
	string msg;

	switch (Right) {
		case eUR_CHAT:
			if (!allow) {
				msg = _("You're no longer allowed to use main chat for %s.");
				mGag = until;
			} else {
				msg = _("You're now allowed to use main chat.");
				mGag = 1;
			}

			break;
		case nEnums::eUR_PM:
			if (!allow) {
				msg = _("You're no longer allowed to use private chat for %s.");
				mNoPM = until;
			} else {
				msg = _("You're now allowed to use private chat.");
				mNoPM = 1;
			}

			break;
		case nEnums::eUR_SEARCH:
			if (!allow) {
				msg = _("You're no longer allowed to search files for %s.");
				mNoSearch = until;
			} else {
				msg = _("You're now allowed to search files.");
				mNoSearch = 1;
			}

			break;
		case nEnums::eUR_CTM:
			if (!allow) {
				msg = _("You're no longer allowed to download files for %s.");
				mNoCTM = until;
			} else {
				msg = _("You're now allowed to download files.");
				mNoCTM = 1;
			}

			break;
		case nEnums::eUR_KICK:
			if (allow) {
				msg = _("You're no longer allowed to kick users.");
				mCanKick = until;
			} else {
				msg = _("You're now allowed to kick users for %s.");
				mCanKick = 1;
			}

			break;
		case nEnums::eUR_REG:
			if (allow) {
				msg = _("You're no longer allowed to register users.");
				mCanReg = until;
			} else {
				msg = _("You're now allowed to register users for %s.");
				mCanReg = 1;
			}

			break;
		case nEnums::eUR_OPCHAT:
			if (allow) {
				msg = _("You're no longer allowed to use operator chat.");
				mCanOpchat = until;
			} else {
				msg = _("You're now allowed to use operator chat for %s.");
				mCanOpchat = 1;
			}

			break;
		case nEnums::eUR_NOSHARE:
			if (allow) {
				msg = _("You're no longer allowed to hide share.");
				mCanShare0 = until;
			} else {
				msg = _("You're now allowed to hide share for %s.");
				mCanShare0 = 1;
			}

			break;
		case nEnums::eUR_DROP:
			if (allow) {
				msg = _("You're no longer allowed to drop users.");
				mCanDrop = until;
			} else {
				msg = _("You're now allowed to drop users for %s.");
				mCanDrop = 1;
			}

			break;
		case nEnums::eUR_TBAN:
			if (allow) {
				msg = _("You're no longer allowed to temporarily ban users.");
				mCanTBan = until;
			} else {
				msg = _("You're now allowed to temporarily ban users for %s.");
				mCanTBan = 1;
			}

			break;
		case nEnums::eUR_PBAN:
			if (allow) {
				msg = _("You're no longer allowed to permanently ban users.");
				mCanPBan = until;
			} else {
				msg = _("You're now allowed to permanently ban users for %s.");
				mCanPBan = 1;
			}

			break;
		default:
			break;
	};

	if (notify && !msg.empty() && (mxConn != NULL)) mxServer->DCPublicHS(autosprintf(msg.c_str(), cTime(until - cTime().Sec()).AsPeriod().AsString().c_str()), mxConn);
}

void cUser::ApplyRights(cPenaltyList::sPenalty &pen)
{
	mGag = pen.mStartChat;
	mNoPM = pen.mStartPM;
	mNoSearch = pen.mStartSearch;
	mNoCTM = pen.mStartCTM;
	mCanKick = pen.mStopKick;
	mCanShare0 = pen.mStopShare0;
	mCanReg = pen.mStopReg;
	mCanOpchat = pen.mStopOpchat;
}

bool cUserRobot::SendPMTo(cConnDC *conn, const string &msg)
{
	if (conn && conn->mpUser)
	{
		string pm;
		cDCProto::Create_PM(pm, mNick,conn->mpUser->mNick, mNick, msg);
		conn->Send(pm, true);
		return true;
	}
	return false;
}

cChatRoom::cChatRoom(const string &nick, cUserCollection *col, cServerDC *server) :
	cUserRobot(nick, server), mCol(col)
{
	mConsole = new cChatConsole(mxServer, this);
	mConsole->AddCommands();
};

cChatRoom::~cChatRoom()
{
	if (mConsole) delete mConsole;
	mConsole = NULL;
}

void cChatRoom::SendPMToAll(const string &data, cConnDC *conn, bool fromplug)
{
	if (!this->mCol || !this->mxServer)
		return;

	string from, start, end;

	if (conn && conn->mpUser)
		from = conn->mpUser->mNick;
	else
		from = mNick;

	this->mxServer->mP.Create_PMForBroadcast(start, end, mNick, from, data);
	bool temp = false;

	if (conn && conn->mpUser) {
		temp = conn->mpUser->mInList;
		conn->mpUser->mInList = false;
	}

	this->mCol->SendToAllWithNick(start, end);

	if (conn && conn->mpUser)
		conn->mpUser->mInList = temp;

	#ifndef WITHOUT_PLUGINS
		if (!fromplug) {
			string msg(data);
			this->mxServer->OnOpChatMessage(&from, &msg);
		}
	#endif
}

bool cChatRoom::ReceiveMsg(cConnDC *conn, cMessageDC *msg)
{
	if (conn && conn->mpUser && conn->mpUser->mInList && mCol && msg && (msg->mType == eDC_TO)) {
		bool InList = mCol->ContainsNick(conn->mpUser->mNick);

		if (InList || IsUserAllowed(conn->mpUser)) {
			if (!InList) // auto join
				mCol->Add(conn->mpUser);

			string &chat = msg->ChunkString(eCH_PM_MSG);

			if (chat.size()) {
				if (chat[0] == '+') {
					if (!mConsole->DoCommand(chat, conn))
						SendPMTo(conn, _("Unknown chatroom command specified."));
				} else
					SendPMToAll(chat, conn);
			}

			return true;
		} else
			SendPMTo(conn, _("You can't use this chatroom, use main chat instead."));
	}

	return false;
}

bool cChatRoom::IsUserAllowed(cUser *)
{
	return false;
}

cOpChat::cOpChat(cServerDC *server):
	cChatRoom(server->mC.opchat_name, &server->mOpchatList, server)
{
}

bool cOpChat::IsUserAllowed(cUser *user)
{
	if (user && (user->mClass >= eUC_OPERATOR))
		return true;
	else
		return false;
}

bool cMainRobot::ReceiveMsg(cConnDC *conn, cMessageDC *message)
{
	if (message->mType == eDC_TO) {
		string &msg = message->ChunkString(eCH_PM_MSG);

		if (!mxServer->mP.ParseForCommands(msg, conn, 1)) {
			cUser *other = mxServer->mUserList.GetUserByNick(mxServer->LastBCNick);

			if (other && other->mxConn)
				mxServer->DCPrivateHS(message->ChunkString(eCH_PM_MSG), other->mxConn, &conn->mpUser->mNick, &conn->mpUser->mNick);
			else
				mxServer->DCPrivateHS(_("Hub security doesn't accept private messages, use main chat instead."), conn);
		}
	}

	return true;
}

cPluginRobot::cPluginRobot(const string &nick, cVHPlugin *pi, cServerDC *server) :
	cUserRobot(nick, server), mPlugin(pi)
{}

bool cPluginRobot::ReceiveMsg(cConnDC *conn, cMessageDC *message)
{
	if (message->mType == eDC_TO)
	{
		mPlugin->RobotOnPM( this, message, conn);
	}
	return true;
}

};  // namespace nVerliHub
