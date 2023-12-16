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

#include "cserverdc.h"
#include "ctrigger.h"
#include "cuser.h"
#include "cconndc.h"
#include "stringutils.h"
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
	mSeconds = 0;
	mLastTrigger = 0;
	mFlags = 0;
	mMinClass = 0;
	mMaxClass = 10;
	//mMaxLines = 0;
	//mMaxSize = 0;
	//mDelayUser = 0;
	//mDelayTotal = 0;
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

int cTrigger::DoIt(istringstream &cmd_line, cConnDC *conn, cServerDC &server)
{
	bool user = conn && conn->mpUser;
	int clas = 0;

	if (user) { // check rights when triggered by user
		clas = conn->mpUser->mClass;

		if ((clas < eUC_NORMUSER) || (clas < this->mMinClass) || (clas > this->mMaxClass))
			return 0;
	}

	string buf, filename, sender;
	string par1, end1, parall, geo;

	if (cmd_line.str().size() > mCommand.size()) {
		parall.assign(cmd_line.str(), mCommand.size() + 1, string::npos);
	}

	cmd_line >> par1;
	end1 = cmd_line.str();
	sender = server.mC.hub_security; // replace sender

	if (mSendAs.size())
		sender = mSendAs;

	ReplaceVarInString(sender, "PAR1", sender, par1);

	if (user)
		ReplaceVarInString(sender, "NICK", sender, conn->mpUser->mNick);

	if (mFlags & eTF_DB) {
		buf = mDefinition;

	} else {
		ReplaceVarInString(mDefinition, "CFG", filename, server.mConfigBaseDir);

		if (user && (filename.find("%[CC]") != filename.npos)) { // only if found
			geo = conn->GetGeoCC(); // country code
			ReplaceVarInString(filename, "CC", filename, geo);
		}

		if (!LoadFileInString(filename, buf))
			return 0;
	}

	if (mFlags & eTF_VARS) {
		ReplaceVarInString(buf, "PARALL", buf, parall);
		ReplaceVarInString(buf, "PAR1", buf, par1);
		ReplaceVarInString(buf, "END1", buf, end1);

		if (user) {
			ReplaceVarInString(buf, "CLASS", buf, clas);
			ReplaceVarInString(buf, "CLASSNAME", buf, server.UserClassName(nEnums::tUserCl(clas)));
			const size_t pos = buf.find("%[C");

			if (pos != buf.npos) { // only if found
				if (buf.find("%[CC]", pos) != buf.npos) {
					geo = conn->GetGeoCC(); // country code
					ReplaceVarInString(buf, "CC", buf, geo);
				}

				if (buf.find("%[CN]", pos) != buf.npos) {
					geo = conn->GetGeoCN(); // country name
					ReplaceVarInString(buf, "CN", buf, geo);
				}

				if (buf.find("%[CITY]", pos) != buf.npos) {
					geo = conn->GetGeoCI(); // city name
					ReplaceVarInString(buf, "CITY", buf, geo);
				}
			}

			ReplaceVarInString(buf, "IP", buf, conn->AddrIP());
			ReplaceVarInString(buf, "TLS", buf, ((conn->mTLSVer.size() && (conn->mTLSVer != "0.0")) ? conn->mTLSVer : _("No")));
			ReplaceVarInString(buf, "CLI_TLS", buf, (conn->mpUser->GetMyFlag(eMF_TLS) ? _("Yes") : _("No")));
			ReplaceVarInString(buf, "CLI_NAT", buf, (conn->mpUser->GetMyFlag(eMF_NAT) ? _("Yes") : _("No")));
			ReplaceVarInString(buf, "MODE", buf, (conn->mpUser->mPassive ? _("Passive") : _("Active")));
			ReplaceVarInString(buf, "HOST", buf, conn->AddrHost());
			ReplaceVarInString(buf, "NICK", buf, conn->mpUser->mNick);
			ReplaceVarInString(buf, "SHARE", buf, convertByte(conn->mpUser->mShare));
			ReplaceVarInString(buf, "SHARE_EXACT", buf, (__int64)conn->mpUser->mShare); // exact share size
		}

		ReplaceVarInString(buf, "USERS", buf, (int)server.mUserList.Size());
		ReplaceVarInString(buf, "USERS_ACTIVE", buf, (int)server.mActiveUsers.Size());
		ReplaceVarInString(buf, "USERS_PASSIVE", buf, (int)server.mPassiveUsers.Size());
		ReplaceVarInString(buf, "USERSPEAK", buf, (int)server.mUsersPeak);

		cTimePrint theTime(server.mTime); // uptime
		theTime -= server.mStartTime;
		ReplaceVarInString(buf, "UPTIME", buf, theTime.AsPeriod().AsString());

		ReplaceVarInString(buf, "VERSION", buf, HUB_VERSION_VERS);
		ReplaceVarInString(buf, "HUBNAME", buf, server.mC.hub_name);
		ReplaceVarInString(buf, "HUBTOPIC", buf, server.mC.hub_topic);
		ReplaceVarInString(buf, "HUBDESC", buf, server.mC.hub_desc);
		ReplaceVarInString(buf, "TOTAL_SHARE", buf, convertByte(server.mTotalShare));
		ReplaceVarInString(buf, "SHAREPEAK", buf, convertByte(server.mTotalSharePeak)); // peak total share

		time_t curr_time; // current time
		time(&curr_time);
		struct tm *lt;

		/*
		#ifdef _WIN32
			lt = localtime(&curr_time); // todo: do we really need reentrant version?
		#else
		*/
			struct tm lt_obj;
			lt = &lt_obj;
			localtime_r(&curr_time, lt);
		//#endif

		char tmf[3];
		sprintf(tmf, "%02d", lt->tm_sec);
		ReplaceVarInString(buf, "ss", buf, tmf); // todo: why not %[SS] ?
		sprintf(tmf, "%02d", lt->tm_min);
		ReplaceVarInString(buf, "mm", buf, tmf);
		sprintf(tmf, "%02d", lt->tm_hour);
		ReplaceVarInString(buf, "HH", buf, tmf);
		sprintf(tmf, "%02d", lt->tm_mday);
		ReplaceVarInString(buf, "DD", buf, tmf);
		sprintf(tmf, "%02hd", short(lt->tm_mon + 1));
		ReplaceVarInString(buf, "MM", buf, tmf);
		ReplaceVarInString(buf, "YY", buf, 1900 + lt->tm_year);
	}

	if (mFlags & eTF_SENDTOALL) { // to all
		if (mFlags & eTF_SENDPM) { // pm
			string start, end;
			server.mP.Create_PMForBroadcast(start, end, sender, sender, buf);

			if (mFlags & eTF_VARS) // use vars
				server.SendToAllWithNickVars(start, end, this->mMinClass, this->mMaxClass);
			else // no vars
				server.SendToAllWithNick(start, end, this->mMinClass, this->mMaxClass);

		} else { // mc
			if (mFlags & eTF_VARS) { // use vars
				string msg;
				server.mP.Create_Chat(msg, sender, buf);
				server.SendToAllNoNickVars(msg, this->mMinClass, this->mMaxClass);
				server.OnPublicBotMessage(&sender, &buf, this->mMinClass, this->mMaxClass); // todo: make it discardable if needed

			} else { // no vars
				server.DCPublicToAll(sender, buf, this->mMinClass, this->mMaxClass, server.mC.delayed_chat);
			}
		}

	} else if (user) { // to single
		if (mFlags & eTF_SENDPM) // pm
			server.DCPrivateHS(buf, conn, &sender, &sender);
		else // mc
			server.DCPublic(sender, buf, conn);
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
	string def(tr.mDefinition.substr(0, 30));
	replace(def.begin(), def.end(), '\r', ' ');
	replace(def.begin(), def.end(), '\n', ' ');

	os << "\t" << tr.mCommand;

	if (tr.mCommand.size() <= 8)
		os << "\t";

	os << "\t" << tr.mFlags;
	os << "\t" << tr.mMinClass << " - " << tr.mMaxClass << "\t";

	if (tr.mSeconds) {
		cTimePrint timeout = cTimePrint(tr.mSeconds);
		os << timeout.AsPeriod();
	} else {
		os << _("No");
	}

	os << "\t" << (tr.mSendAs.size() ? tr.mSendAs : _("Not set"));
	os << "\t\t" << def;
	return os;
}

};
};
