/*
	Copyright (C) 2003-2005 Daniel Muller, dan at verliba dot cz
	Copyright (C) 2006-2016 Verlihub Team, info at verlihub dot net

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

#include "cinfoserver.h"
#include "stringutils.h"
#include "cserverdc.h"
#if defined HAVE_LINUX
#include <unistd.h>
#include <sys/utsname.h>
#include <sys/sysinfo.h>
#include <sys/time.h>
#include <sys/resource.h>
#endif
//#include <algorithm>
#include "i18n.h"

#define PADDING 25

namespace nVerliHub {
	//using namespace std;
	using namespace nUtils;
	using namespace nSocket;
	using namespace nEnums;

cInfoServer::cInfoServer()
{
	mServer = NULL;
}

void cInfoServer::PortInfo(ostream &os)
{
	cUserCollection::iterator user_iter;
	tPortInfoList::iterator port_iter;
	cConnDC *conn;
	unsigned int conn_port;
	bool add_port;

	for (user_iter = mServer->mUserList.begin(); user_iter != mServer->mUserList.end(); ++user_iter) {
		conn = ((cUser*)(*user_iter))->mxConn;

		if (conn) {
			conn_port = conn->GetServPort();
			add_port = true;

			for (port_iter = mPortInfoList.begin(); port_iter != mPortInfoList.end(); ++port_iter) {
				if ((*port_iter) && ((*port_iter)->mPort == conn_port)) {
					(*port_iter)->mCount++;
					add_port = false;
					break;
				}
			}

			if (add_port) {
				sPortInfoItem *item = new sPortInfoItem;
				item->mPort = conn_port;
				item->mCount = 1;
				mPortInfoList.push_back(item);
			}
		}
	}

	os << _("Hub ports information") << ":\r\n\r\n";

	for (port_iter = mPortInfoList.begin(); port_iter != mPortInfoList.end(); ++port_iter) {
		if (*port_iter) {
			os << " [*] " << autosprintf(_("Port %d: %d"), (*port_iter)->mPort, (*port_iter)->mCount) << "\r\n";
			delete (*port_iter);
			(*port_iter) = NULL;
		}
	}

	mPortInfoList.clear();
}

void cInfoServer::HubURLInfo(ostream &os)
{
	cUserCollection::iterator user_iter;
	tHubURLInfoList::iterator url_iter;
	cConnDC *conn;
	string conn_url;
	bool add_url;
	unsigned int no_url = 0;

	for (user_iter = mServer->mUserList.begin(); user_iter != mServer->mUserList.end(); ++user_iter) {
		conn = ((cUser*)(*user_iter))->mxConn;

		if (conn) {
			if (conn->mHubURL.size()) {
				conn_url = conn->mHubURL;
				add_url = true;

				for (url_iter = mHubURLInfoList.begin(); url_iter != mHubURLInfoList.end(); ++url_iter) {
					if ((*url_iter) && ((*url_iter)->mURL == conn_url)) {
						(*url_iter)->mCount++;
						add_url = false;
						break;
					}
				}

				if (add_url) {
					sHubURLInfoItem *item = new sHubURLInfoItem;
					item->mURL = conn_url;
					item->mCount = 1;
					mHubURLInfoList.push_back(item);
				}
			} else {
				no_url++;
			}
		}
	}

	// todo: sort same as port information
	os << _("Hub URL information") << ":\r\n\r\n";

	for (url_iter = mHubURLInfoList.begin(); url_iter != mHubURLInfoList.end(); ++url_iter) {
		if (*url_iter) {
			os << " [*] " << (*url_iter)->mURL << " : " << (*url_iter)->mCount << "\r\n";
			delete (*url_iter);
			(*url_iter) = NULL;
		}
	}

	if (no_url) {
		if (mHubURLInfoList.size())
			os << "\r\n";

		os << " [*] " << autosprintf(_("Missing hub URL: %d"), no_url) << "\r\n";
	}

	mHubURLInfoList.clear();
}

void cInfoServer::ProtocolInfo(ostream &os)
{
	os << _("Protocol information") << ":\r\n\r\n";
	os << " [*] &#36;Search: " << mServer->mProtoCount[nEnums::eDC_SEARCH] << "\r\n";
	os << " [*] &#36;Search Hub: " << mServer->mProtoCount[nEnums::eDC_SEARCH_PAS] << "\r\n";
	os << " [*] &#36;MultiSearch: " << mServer->mProtoCount[nEnums::eDC_MSEARCH] << "\r\n";
	os << " [*] &#36;MultiSearch Hub: " << mServer->mProtoCount[nEnums::eDC_MSEARCH_PAS] << "\r\n";
	os << " [*] &#36;SR: " << mServer->mProtoCount[nEnums::eDC_SR] << "\r\n";
	os << " [*] &#36;ConnectToMe: " << mServer->mProtoCount[nEnums::eDC_CONNECTTOME] << "\r\n";
	os << " [*] &#36;RevConnectToMe: " << mServer->mProtoCount[nEnums::eDC_RCONNECTTOME] << "\r\n";
	os << " [*] &#36;MultiConnectToMe: " << mServer->mProtoCount[nEnums::eDC_MCONNECTTOME] << "\r\n";
	os << " [*] &#36;Key: " << mServer->mProtoCount[nEnums::eDC_KEY] << "\r\n";
	os << " [*] &#36;Supports: " << mServer->mProtoCount[nEnums::eDC_SUPPORTS] << "\r\n";
	os << " [*] &#36;ValidateNick: " << mServer->mProtoCount[nEnums::eDC_VALIDATENICK] << "\r\n";
	os << " [*] &#36;MyPass: " << mServer->mProtoCount[nEnums::eDC_MYPASS] << "\r\n";
	os << " [*] &#36;Version: " << mServer->mProtoCount[nEnums::eDC_VERSION] << "\r\n";
	os << " [*] &#36;MyHubURL: " << mServer->mProtoCount[nEnums::eDC_MYHUBURL] << "\r\n";
	os << " [*] &#36;GetNickList: " << mServer->mProtoCount[nEnums::eDC_GETNICKLIST] << "\r\n";
	os << " [*] &#36;MyINFO: " << mServer->mProtoCount[nEnums::eDC_MYINFO] << "\r\n";
	os << " [*] &#36;ExtJSON: " << mServer->mProtoCount[nEnums::eDC_EXTJSON] << "\r\n";
	os << " [*] &#36;IN: " << mServer->mProtoCount[nEnums::eDC_IN] << "\r\n";
	os << " [*] &#36;UserIP: " << mServer->mProtoCount[nEnums::eDCO_USERIP] << "\r\n";
	os << " [*] &#36;GetINFO: " << mServer->mProtoCount[nEnums::eDC_GETINFO] << "\r\n";
	os << " [*] &#36;To: " << mServer->mProtoCount[nEnums::eDC_TO] << "\r\n";
	os << " [*] &#36;MCTo: " << mServer->mProtoCount[nEnums::eDC_MCTO] << "\r\n";
	os << " [*] " << autosprintf(_("Chat: %d"), mServer->mProtoCount[nEnums::eDC_CHAT]) << "\r\n";
	os << " [*] &#36;OpForceMove: " << mServer->mProtoCount[nEnums::eDCO_OPFORCEMOVE] << "\r\n";
	os << " [*] &#36;Kick: " << mServer->mProtoCount[nEnums::eDCO_KICK] << "\r\n";
	os << " [*] &#36;Ban: " << mServer->mProtoCount[nEnums::eDCO_BAN] << "\r\n";
	os << " [*] &#36;TempBan: " << mServer->mProtoCount[nEnums::eDCO_TBAN] << "\r\n";
	os << " [*] &#36;UnBan: " << mServer->mProtoCount[nEnums::eDCO_UNBAN] << "\r\n";
	os << " [*] &#36;GetBanList: " << mServer->mProtoCount[nEnums::eDCO_GETBANLIST] << "\r\n";
	os << " [*] &#36;WhoIP: " << mServer->mProtoCount[nEnums::eDCO_WHOIP] << "\r\n";
	os << " [*] &#36;SetTopic: " << mServer->mProtoCount[nEnums::eDCO_SETTOPIC] << "\r\n";
	os << " [*] &#36;GetTopic: " << mServer->mProtoCount[nEnums::eDCO_GETTOPIC] << "\r\n";
	os << " [*] &#36;BotINFO: " << mServer->mProtoCount[nEnums::eDCB_BOTINFO] << "\r\n";
	os << " [*] &#36;Quit: " << mServer->mProtoCount[nEnums::eDC_QUIT] << "\r\n";
	os << " [*] &#36;MyNick: " << mServer->mProtoCount[nEnums::eDCC_MYNICK] << "\r\n";
	os << " [*] &#36;Lock: " << mServer->mProtoCount[nEnums::eDCC_LOCK] << "\r\n";
	os << " [*] " << autosprintf(_("Ping: %d"), mServer->mProtoCount[nEnums::eDC_UNKNOWN + 1]) << "\r\n";
	os << " [*] " << autosprintf(_("Unknown: %d"), mServer->mProtoCount[nEnums::eDC_UNKNOWN]) << "\r\n";
	os << "\r\n";

	double total_up = 0.;

	for (unsigned int zone = 0; zone <= USER_ZONES; zone++)
		total_up += mServer->mUploadZone[zone].GetMean(mServer->mTime);

	os << " [*] " << autosprintf(_("Total download: %s [%s]"), convertByte(mServer->mProtoTotal[0]).c_str(), convertByte(mServer->mDownloadZone.GetMean(mServer->mTime), true).c_str()) << "\r\n";
	os << " [*] " << autosprintf(_("Total upload: %s [%s]"), convertByte(mServer->mProtoTotal[1]).c_str(), convertByte(total_up, true).c_str()) << "\r\n";
}

void cInfoServer::SystemInfo(ostream &os)
{
#if defined HAVE_LINUX
	struct sysinfo serverInfo;

	if (sysinfo(&serverInfo)) {
		os << _("Unable to retrive system information.");
		return;
	}

	cTime uptime(serverInfo.uptime);
	utsname osname;
	os << _("System information") << ":";

	if (uname(&osname) == 0) {
		os << "\r\n [*] " << setw(PADDING) << setiosflags(ios::left) << "OS" << osname.sysname << " " << osname.release << " (" << osname.machine << ")";
	}

	os << "\r\n [*] " << setw(PADDING) << setiosflags(ios::left) << _("System uptime") << uptime.AsPeriod().AsString().c_str() << endl;
	os << " [*] " << setw(PADDING) << setiosflags(ios::left) << _("Load averages") << autosprintf("%.2f %.2f %.2f", serverInfo.loads[0]/65536.0, serverInfo.loads[1]/65536.0, serverInfo.loads[2]/65536.0) << endl;
#if defined (_SC_PHYS_PAGES) && defined (_SC_AVPHYS_PAGES) && defined (_SC_PAGESIZE)
	os << " [*] " << setw(PADDING) << setiosflags(ios::left) << _("Total memory") << convertByte((long long int) sysconf(_SC_PHYS_PAGES) * sysconf(_SC_PAGESIZE), false).c_str() << endl;
	os << " [*] " << setw(PADDING) << setiosflags(ios::left) << _("Free memory") << convertByte((long long int) sysconf(_SC_AVPHYS_PAGES) * sysconf(_SC_PAGESIZE), false).c_str() << endl;
#else
	os << " [*] " << setw(PADDING) << setiosflags(ios::left) << _("Total memory") << convertByte((long long int) serverInfo.totalram, false).c_str() << endl;
	os << " [*] " << setw(PADDING) << setiosflags(ios::left) << _("Free memory") << convertByte((long long int) serverInfo.freeram, false).c_str() << endl;
#endif
	os << " [*] " << setw(PADDING) << setiosflags(ios::left) << _("Shared memory") << convertByte((long long int) serverInfo.sharedram, false).c_str() << endl;
	os << " [*] " << setw(PADDING) << setiosflags(ios::left) << _("Memory in buffers") << convertByte((long long int) serverInfo.bufferram, false).c_str() << endl;
	os << " [*] " << setw(PADDING) << setiosflags(ios::left) << _("Free swap") << convertByte((long long int) serverInfo.freeswap, false).c_str() << "/" << convertByte((long long int) serverInfo.totalswap, false).c_str() << endl;
	os << " [*] " << setw(PADDING) << setiosflags(ios::left) << _("Number of processes") << serverInfo.procs;
	/*struct rusage resourceUsage; // not used
	getrusage(RUSAGE_SELF, &resourceUsage);*/
#else
	os << _("Information is not available.");
#endif
}

void cInfoServer::SetServer(cServerDC *Server)
{
	mServer = Server;
}

void cInfoServer::Output(ostream &os, int Class)
{
	iterator it;
	cTime theTime;
	os << _("Hub information") << ":";
	os << "\r\n [*] " << setw(PADDING) << setiosflags(ios::left) << _("Version") << HUB_VERSION_VERS << "\r\n";
	theTime = mServer->mTime;
	theTime -= mServer->mStartTime;
	os << " [*] " << setw(PADDING) << setiosflags(ios::left) << _("Uptime") << theTime.AsPeriod().AsString().c_str() << "\r\n";
	string loadType;

	if (mServer->mSysLoad >= eSL_RECOVERY)
		loadType = _("Recovery mode");
	if (mServer->mSysLoad >= eSL_CAPACITY)
		loadType = _("Near capacity");
	if (mServer->mSysLoad >= eSL_PROGRESSIVE)
		loadType = _("Progressive mode");
	if (mServer->mSysLoad >= eSL_NORMAL)
		loadType = _("Normal mode");
	else
		loadType = _("Not available");

	os << " [*] " << setw(PADDING) << setiosflags(ios::left) << _("Server frequency") << mServer->mFrequency.GetMean(mServer->mTime) << " " << "(" << loadType.c_str() << ")" << "\r\n";
	os << " [*] " << setw(PADDING) << setiosflags(ios::left) << _("Hub name") << mServer->mC.hub_name << "\r\n";
	string hubOwner;

	if (!mServer->mC.hub_owner.empty())
		hubOwner = mServer->mC.hub_owner;
	else
		hubOwner = "--";

	os << " [*] " << setw(PADDING) << setiosflags(ios::left) << _("Hub owner") << hubOwner << "\r\n";
	string hubCategory;

	if (!mServer->mC.hub_category.empty())
		hubCategory = mServer->mC.hub_category;
	else
		hubCategory = "--";

	os << " [*] " << setw(PADDING) << setiosflags(ios::left) << _("Hub category") << hubCategory << "\r\n";
	string hubLocale;

	if (!mServer->mDBConf.locale.empty())
		hubLocale = mServer->mDBConf.locale;
	else
		hubLocale = "--";

	os << " [*] " << setw(PADDING) << setiosflags(ios::left) << _("Hub locale") << hubLocale << "\r\n";
	os << "    " << string(30,'=') << "\r\n";
	os << " [*] " << setw(PADDING) << setiosflags(ios::left) << _("Online users") << mServer->mUserCountTot << " " << _("of") << " " << mServer->mC.max_users_total << "\r\n";
	os << " [*] " << setw(PADDING) << setiosflags(ios::left) << _("Total share") << convertByte(mServer->mTotalShare, false).c_str() << "\r\n";
	os << " [*] " << setw(PADDING) << setiosflags(ios::left) << _("User list count") << mServer->mUserList.Size() << "\r\n";
	os << " [*] " << setw(PADDING) << setiosflags(ios::left) << _("Hello user count") << mServer->mHelloUsers.Size() << "\r\n";
	os << " [*] " << setw(PADDING) << setiosflags(ios::left) << _("In progress users") << mServer->mInProgresUsers.Size() << "\r\n";
	os << " [*] " << setw(PADDING) << setiosflags(ios::left) << _("Active user count") << mServer->mActiveUsers.Size() << "\r\n";
	os << " [*] " << setw(PADDING) << setiosflags(ios::left) << _("Passive user count") << mServer->mPassiveUsers.Size() << "\r\n";
	os << " [*] " << setw(PADDING) << setiosflags(ios::left) << _("Operator user count") << mServer->mOpchatList.Size() << "\r\n";
	os << " [*] " << setw(PADDING) << setiosflags(ios::left) <<_("Bot user count") << mServer->mRobotList.Size() << "\r\n";
	os << "    " << string(30,'=') << "\r\n";
	double total = 0, curr;

	// print zone from 1 to 3
	for (int i = 1; i < 4; i++) {
		if (!mServer->mC.cc_zone[i-1].empty()) {
			string zone = mServer->mC.cc_zone[i-1];
			replace(zone.begin(), zone.end(), ':', ',');
			os << " [*] " << setw(PADDING) << setiosflags(ios::left) << autosprintf(_("Users in zone #%d"), i) << mServer->mUserCount[i] << "/" << mServer->mC.max_users[i];
			curr = mServer->mUploadZone[i].GetMean(mServer->mTime);
			total += curr;
			os << " (" << convertByte(curr, true).c_str() << ")" << " [" << zone.c_str() << "]" << "\r\n";
		} else
			os << " [*] " << setw(PADDING) << setiosflags(ios::left) << autosprintf(_("Users in zone #%d"), i) << _("Not set") << " " << _("[CC]") << "\r\n";
	}

	// print zone from 4 to 6
	for (int i = 4; i <= USER_ZONES; i++) {
		curr = mServer->mUploadZone[i].GetMean(mServer->mTime);
		total += curr;
		os << " [*] " << setw(PADDING) << setiosflags(ios::left) << autosprintf(_("Users in zone #%d"), i) << mServer->mUserCount[i] << "/" << mServer->mC.max_users[i];
		os << " (" << convertByte(curr, true).c_str() << ") " << _("[IP]") << "\r\n";
	}

	// print zone 0
	total += mServer->mUploadZone[0].GetMean(mServer->mTime);
	os << " [*] " << setw(PADDING) << setiosflags(ios::left) << _("All other users") << mServer->mUserCount[0] << "/" << mServer->mC.max_users[0] << " (" << convertByte(total, true).c_str() << ")";
}

cInfoServer::~cInfoServer(){}
}; // namespace nVerliHub
