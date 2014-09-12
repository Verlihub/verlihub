/*
	Copyright (C) 2003-2005 Daniel Muller, dan at verliba dot cz
	Copyright (C) 2006-2012 Verlihub Team, devs at verlihub-project dot org
	Copyright (C) 2013-2014 RoLex, webmaster at feardc dot net

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

#include "cserverdc.h"
#include "ctrigger.h"
#include "cuser.h"
#include "cconndc.h"
#include "curr_date_time.h"
#include "stringutils.h"
#if HAVE_LIBGEOIP
#include "cgeoip.h"
#endif
#include <stdlib.h>
#include <time.h>
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "i18n.h"

namespace nVerliHub {
	using namespace nUtils;
	using namespace nEnums;
	namespace nTables {
  /**

  Class constructor

  */

cTrigger::cTrigger()
{
	mSeconds = mLastTrigger = mFlags = 0;
	mMinClass=0;
	mMaxClass=10;
	mMaxLines=0;
	mMaxSize=0;
	mDelayUser=0;
	mDelayTotal=0;
}

  /**

  Class destructor

  */

cTrigger::~cTrigger(){}
  /**

  Send the trigger message. Message can be the content of a file or a string stored in the database (in this case def column)

  @param[in,out] conn The connection that triggers the trigger
  @param[in] server Reference to CServerDC
  @param[in] cmd_line The message of the trigger
  @return 0 if an error occurs or the user cannot see the trigger
  */

int cTrigger::DoIt(istringstream &cmd_line, cConnDC *conn, cServerDC &server, bool timer)
{
	bool timeTrigger = timer && conn == NULL;
	int uclass = 0;

	if (!timeTrigger) { // check if it has been triggered by timeout, if not, check the connection and the user rights
		if (!conn)
			return 0;

		if (!conn->mpUser)
			return 0;

		uclass = conn->mpUser->mClass;

		if ((uclass < this->mMinClass) || (uclass > this->mMaxClass))
			return 0;
	}

	string buf, filename, sender;
	string par1, end1, parall;

	if (cmd_line.str().size() > mCommand.size()) {
		parall.assign(cmd_line.str(), mCommand.size() + 1, string::npos);
	}

	cmd_line >> par1;
	end1 = cmd_line.str();
	sender = server.mC.hub_security; // replace sender

	if (mSendAs.size())
		sender = mSendAs;

	ReplaceVarInString(sender, "PAR1", sender, par1);

	if (!timeTrigger)
		ReplaceVarInString(sender, "NICK", sender, conn->mpUser->mNick);

	if (mFlags & eTF_DB) {
		buf = mDefinition;
	} else {
		ReplaceVarInString(mDefinition, "CFG", filename, server.mConfigBaseDir);

		if (!timeTrigger)
			ReplaceVarInString(filename, "CC", filename, conn->mCC);

		if (!LoadFileInString(filename, buf))
			return 0;
	}

	if (mFlags & eTF_VARS) {
		cTime theTime(server.mTime);
		time_t curr_time;
		time(&curr_time);

		#ifdef _WIN32
			struct tm *lt = localtime(&curr_time); // todo: do we really need reentrant version?
		#else
			struct tm *lt = new tm();
			localtime_r(&curr_time, lt);
		#endif

		theTime -= server.mStartTime;
		ReplaceVarInString(buf, "PARALL", buf, parall);
		ReplaceVarInString(buf, "PAR1", buf, par1);
		ReplaceVarInString(buf, "END1", buf, end1);

		if (!timeTrigger) {
			ReplaceVarInString(buf, "CC", buf, conn->mCC);
			ReplaceVarInString(buf, "CN", buf, conn->mCN);
			ReplaceVarInString(buf, "CITY", buf, conn->mCity);
			ReplaceVarInString(buf, "IP", buf, conn->AddrIP());
			ReplaceVarInString(buf, "HOST", buf, conn->AddrHost());
			ReplaceVarInString(buf, "NICK", buf, conn->mpUser->mNick);
			ReplaceVarInString(buf, "CLASS", buf, uclass);

			switch (uclass) {
				case eUC_PINGER:
					ReplaceVarInString(buf, "CLASSNAME", buf, _("Pinger"));
					break;
				case eUC_NORMUSER:
					ReplaceVarInString(buf, "CLASSNAME", buf, _("Guest"));
					break;
				case eUC_REGUSER:
					ReplaceVarInString(buf, "CLASSNAME", buf, _("Registered"));
					break;
				case eUC_VIPUSER:
					ReplaceVarInString(buf, "CLASSNAME", buf, _("VIP"));
					break;
				case eUC_OPERATOR:
					ReplaceVarInString(buf, "CLASSNAME", buf, _("Operator"));
					break;
				case eUC_CHEEF:
					ReplaceVarInString(buf, "CLASSNAME", buf, _("Cheef"));
					break;
				case eUC_ADMIN:
					ReplaceVarInString(buf, "CLASSNAME", buf, _("Administator"));
					break;
				case eUC_MASTER:
					ReplaceVarInString(buf, "CLASSNAME", buf, _("Master"));
					break;
				default:
					ReplaceVarInString(buf, "CLASSNAME", buf, _("Unknown"));
					break;
			}

			ReplaceVarInString(buf, "SHARE", buf, convertByte(conn->mpUser->mShare, false));
			ReplaceVarInString(buf, "SHARE_EXACT", buf, (__int64)conn->mpUser->mShare); // exact share size
		}

		ReplaceVarInString(buf, "USERS", buf, (int)server.mUserList.Size());
		ReplaceVarInString(buf, "USERS_ACTIVE", buf, (int)server.mActiveUsers.Size());
		ReplaceVarInString(buf, "USERS_PASSIVE", buf, (int)server.mPassiveUsers.Size());
		ReplaceVarInString(buf, "USERSPEAK", buf, (int)server.mUsersPeak);
		ReplaceVarInString(buf, "UPTIME", buf, theTime.AsPeriod().AsString());
		ReplaceVarInString(buf, "VERSION", buf, VERSION);
		ReplaceVarInString(buf, "HUBNAME", buf, server.mC.hub_name);
		ReplaceVarInString(buf, "HUBTOPIC", buf, server.mC.hub_topic);
		ReplaceVarInString(buf, "HUBDESC", buf, server.mC.hub_desc);
		ReplaceVarInString(buf, "VERSION_DATE", buf, __CURR_DATE_TIME__);
		ReplaceVarInString(buf, "TOTAL_SHARE", buf, convertByte(server.mTotalShare, false));
		ReplaceVarInString(buf, "SHAREPEAK", buf, convertByte(server.mTotalSharePeak, false)); // peak total share

		char tmf[3];
		sprintf(tmf, "%02d", lt->tm_sec);
		ReplaceVarInString(buf, "ss", buf, tmf);
		sprintf(tmf, "%02d", lt->tm_min);
		ReplaceVarInString(buf, "mm", buf, tmf);
		sprintf(tmf, "%02d", lt->tm_hour);
		ReplaceVarInString(buf, "HH", buf, tmf);
		sprintf(tmf, "%02d", lt->tm_mday);
		ReplaceVarInString(buf, "DD", buf, tmf);
		sprintf(tmf, "%02d", lt->tm_mon + 1);
		ReplaceVarInString(buf, "MM", buf, tmf);
		ReplaceVarInString(buf, "YY", buf, 1900 + lt->tm_year);

		#ifndef _WIN32
			delete lt;
		#endif
	}

	if (mFlags & eTF_SENDTOALL) { // to all
		if (mFlags & eTF_SENDPM) { // pm
			string start, end;
			server.mP.Create_PMForBroadcast(start, end, sender, sender, buf);

			if (mFlags & eTF_VARS) { // use vars
				server.SendToAllWithNickVars(start, end, this->mMinClass, this->mMaxClass);
			} else { // no vars
				server.SendToAllWithNick(start, end, this->mMinClass, this->mMaxClass);
			}
		} else { // mc
			if (mFlags & eTF_VARS) { // use vars
				string msg;
				server.mP.Create_Chat(msg, sender, buf);
				server.SendToAllNoNickVars(msg, this->mMinClass, this->mMaxClass);
			} else { // no vars
				server.DCPublicToAll(sender, buf, this->mMinClass, this->mMaxClass);
			}
		}
	} else if (!timeTrigger) { // to single
		if (mFlags & eTF_SENDPM) { // pm
			server.DCPrivateHS(buf, conn, &sender);
		} else { // mc
			server.DCPublic(sender, buf, conn);
		}
	}

	return 1;
}

  /**

  This function is called when cTrigger object is created

  */

void cTrigger::OnLoad()
{}

  /**

Redefine << operator to show and describe a trigger

  @param[in,out] ostream The stream where to write
  @param[in] tr Reference to cTrigger
  @return The stream
  */

ostream &operator << (ostream &os, cTrigger &tr)
{
		os << " ";
		os << setw(30) << setiosflags(ios::left) << tr.mCommand.c_str();
		os << setw(25) << setiosflags(ios::left) << tr.mDefinition.substr(0,24).c_str();
		os << setw(8) << setiosflags(ios::left) << tr.mFlags;
		os << setw(15) << setiosflags(ios::left) << tr.mSendAs.c_str();
		os << setw(10) << setiosflags(ios::left) << tr.mMinClass;
		os << tr.mMaxClass;

	if(tr.mSeconds) {
		cTime timeout = cTime(tr.mSeconds);
		os << " Timeout: " <<  timeout.AsPeriod();
	}
	return os;
}

};
};
