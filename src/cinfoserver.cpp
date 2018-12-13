/*
	Copyright (C) 2003-2005 Daniel Muller, dan at verliba dot cz
	Copyright (C) 2006-2018 Verlihub Team, info at verlihub dot net

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
#include <sys/times.h>
#include <sys/vtimes.h>
#endif

#include "i18n.h"

namespace nVerliHub {
	//using namespace std;
	using namespace nUtils;
	using namespace nSocket;
	using namespace nEnums;

	// cpu usage values
	static clock_t last_cpu, last_sys_cpu, last_user_cpu;
	static int num_cpu;

cInfoServer::cInfoServer()
{
	mServer = NULL;
	num_cpu = 0; // get number of cpu cores if we have linux

	#if defined HAVE_LINUX
		struct tms time_sample;
		last_cpu = times(&time_sample);
		last_sys_cpu = time_sample.tms_stime;
		last_user_cpu = time_sample.tms_utime;
		FILE *file = fopen("/proc/cpuinfo", "r");

		if (file) {
			char line[128];

			while (fgets(line, 128, file)) {
				if (strncmp(line, "processor", 9) == 0)
					num_cpu++;
			}

			fclose(file);

			if (num_cpu == 0) // we dont want to divide by zero
				num_cpu = 1;
		}
	#endif
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
	os << " [*] &#36;SA: " << mServer->mProtoCount[nEnums::eDC_TTHS] << "\r\n";
	os << " [*] &#36;SP: " << mServer->mProtoCount[nEnums::eDC_TTHS_PAS] << "\r\n";
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

	os << " [*] " << autosprintf(_("Total download: %s [ %s ]"), convertByte(mServer->mProtoTotal[0]).c_str(), convertByte(mServer->mDownloadZone.GetMean(mServer->mTime), true).c_str()) << "\r\n";
	os << " [*] " << autosprintf(_("Total upload: %s [ %s ]"), convertByte(mServer->mProtoTotal[1]).c_str(), convertByte(total_up, true).c_str()) << "\r\n";

	cUserCollection::iterator user_iter;
	cAsyncConn *conn;
	unsigned __int64 total_buf_up = 0, total_cap_up = 0;
	unsigned int bufs = 0;

	for (user_iter = mServer->mUserList.begin(); user_iter != mServer->mUserList.end(); ++user_iter) {
		conn = ((cUser*)(*user_iter))->mxConn;

		if (conn && conn->ok) {
			total_buf_up += conn->GetFlushSize() + conn->GetBufferSize();
			total_cap_up += conn->GetFlushCapacity() + conn->GetBufferCapacity();
			bufs++;
		}
	}

	os << " [*] " << autosprintf(_("Upload buffers: %d [ %s / %s ]"), bufs, convertByte(total_buf_up).c_str(), convertByte(total_cap_up).c_str()) << "\r\n";
	os << " [*] " << autosprintf(_("Upload saved with zLib: %s"), convertByte(mServer->mProtoSaved[0]).c_str()) << "\r\n";
	os << " [*] " << autosprintf(_("Upload saved with TTHS: %s"), convertByte(mServer->mProtoSaved[1]).c_str()) << "\r\n";
	os << "\r\n";
	os << " [*] " << autosprintf(_("Size of input zLib buffer: %s"), convertByte(mServer->mZLib->GetInBufLen()).c_str()) << "\r\n";
	os << " [*] " << autosprintf(_("Size of output zLib buffer: %s"), convertByte(mServer->mZLib->GetOutBufLen()).c_str()) << "\r\n";
	os << " [*] " << autosprintf(_("Compress level zLib: %d"), mServer->mC.zlib_compress_level) << "\r\n";
}

void cInfoServer::SystemInfo(ostream &os)
{
	#if defined HAVE_LINUX
		struct sysinfo serverInfo;

		if (sysinfo(&serverInfo)) {
			os << _("Unable to get system information.");
			return;
		}

		utsname osname;
		os << _("System information") << ":\r\n\r\n";

		if (uname(&osname) == 0) {
			os << " [*] " << autosprintf(_("OS: %s"), osname.sysname) << "\r\n";
			os << " [*] " << autosprintf(_("Kernel: %s [%s]"), osname.release, osname.machine) << "\r\n\r\n";
		}

		os << " [*] " << autosprintf(_("Uptime: %s"), cTimePrint(serverInfo.uptime).AsPeriod().AsString().c_str()) << "\r\n";
		os << " [*] " << autosprintf(_("Load averages: %.2f %.2f %.2f"), (serverInfo.loads[0] / 65536.0), (serverInfo.loads[1] / 65536.0), (serverInfo.loads[2] / 65536.0)) << "\r\n";
		os << " [*] " << autosprintf(_("Total processes: %d"), serverInfo.procs) << "\r\n\r\n";

		unsigned __int64 size_val;

		/*#if defined (_SC_PHYS_PAGES) && defined (_SC_AVPHYS_PAGES) && defined (_SC_PAGESIZE)
			os << " [*] " << autosprintf(_("Total RAM: %s"), convertByte((__int64)(sysconf(_SC_PHYS_PAGES) * sysconf(_SC_PAGESIZE))).c_str()) << "\r\n";
			os << " [*] " << autosprintf(_("Free RAM: %s"), convertByte((__int64)(sysconf(_SC_AVPHYS_PAGES) * sysconf(_SC_PAGESIZE))).c_str()) << "\r\n";
		#else*/
			size_val = (unsigned __int64)serverInfo.totalram;
			size_val *= (unsigned __int64)serverInfo.mem_unit;
			os << " [*] " << autosprintf(_("Total RAM: %s"), convertByte(size_val).c_str()) << "\r\n";

			size_val = (unsigned __int64)serverInfo.freeram;
			size_val *= (unsigned __int64)serverInfo.mem_unit;
			os << " [*] " << autosprintf(_("Free RAM: %s"), convertByte(size_val).c_str()) << "\r\n";
		//#endif

		size_val = (unsigned __int64)serverInfo.sharedram;
		size_val *= (unsigned __int64)serverInfo.mem_unit;
		os << " [*] " << autosprintf(_("Shared RAM: %s"), convertByte(size_val).c_str()) << "\r\n";

		size_val = (unsigned __int64)serverInfo.bufferram;
		size_val *= (unsigned __int64)serverInfo.mem_unit;
		os << " [*] " << autosprintf(_("RAM in buffers: %s"), convertByte(size_val).c_str()) << "\r\n\r\n";

		size_val = (unsigned __int64)serverInfo.totalswap;
		size_val *= (unsigned __int64)serverInfo.mem_unit;
		os << " [*] " << autosprintf(_("Total swap: %s"), convertByte(size_val).c_str()) << "\r\n";

		size_val = (unsigned __int64)serverInfo.freeswap;
		size_val *= (unsigned __int64)serverInfo.mem_unit;
		os << " [*] " << autosprintf(_("Free swap: %s"), convertByte(size_val).c_str()) << "\r\n\r\n";

		int size_vm = 0, size_res = 0; // self memory sizes and cpu usage
		double perc_cpu;
		FILE *file = fopen("/proc/self/status", "r");

		if (file) {
			int found = 0;
			char line[128];

			while (fgets(line, 128, file) != NULL) {
				if (strncmp(line, "VmSize:", 7) == 0) { // virtual memory
					size_vm = ParseMemSizeLine(line);
					found += 1;
				} else if (strncmp(line, "VmRSS:", 6) == 0) { // resident memory
					size_res = ParseMemSizeLine(line);
					found += 1;
				}

				if (found >= 2)
					break;
			}

			fclose(file);
		}

		struct tms time_sample;
		clock_t now = times(&time_sample);

		if ((now <= last_cpu) || (time_sample.tms_stime < last_sys_cpu) || (time_sample.tms_utime < last_user_cpu)) { // overflow detection
			perc_cpu = .0;
		} else { // calculate cpu usage
			perc_cpu = (time_sample.tms_stime - last_sys_cpu) + (time_sample.tms_utime - last_user_cpu);
			perc_cpu /= (now - last_cpu);
			//perc_cpu /= num_cpu; // we are single threaded, show only one core
			perc_cpu *= 100;
		}

		last_cpu = now;
		last_sys_cpu = time_sample.tms_stime;
		last_user_cpu = time_sample.tms_utime;

		os << " [*] " << autosprintf(_("Virtual RAM usage: %s"), convertByte(size_vm * 1024).c_str()) << "\r\n";
		os << " [*] " << autosprintf(_("Resident RAM usage: %s"), convertByte(size_res * 1024).c_str()) << "\r\n\r\n";

		os << " [*] " << autosprintf(_("CPU cores: %d"), num_cpu) << "\r\n";
		os << " [*] " << autosprintf(_("CPU usage: %.2f%%"), perc_cpu) << "\r\n";

		/*
		struct rusage resourceUsage;
		getrusage(RUSAGE_SELF, &resourceUsage);
		*/
	#else
		os << _("System information not available.");
	#endif
}

int cInfoServer::ParseMemSizeLine(char *line)
{
	int len = strlen(line);
	const char* pos = line;

	while (((*pos) < '0') || ((*pos) > '9'))
		pos++;

	line[len - 3] = '\0';
	len = atoi(pos);
	return len;
}

void cInfoServer::SetServer(cServerDC *Server)
{
	mServer = Server;
}

void cInfoServer::Output(ostream &os, int Class)
{
	os << _("Hub information") << ":\r\n\r\n";

	os << " [*] " << autosprintf(_("Version: %s"), HUB_VERSION_VERS) << "\r\n";
	os << " [*] " << autosprintf(_("Uptime: %s"), cTimePrint(mServer->mTime - mServer->mStartTime).AsPeriod().AsString().c_str()) << "\r\n";
	os << " [*] " << autosprintf(_("Frequency: %.4f [%s]"), mServer->mFrequency.GetMean(mServer->mTime), mServer->SysLoadName()) << "\r\n\r\n";
	os << " [*] " << autosprintf(_("Name: %s"), (mServer->mC.hub_name.size() ? mServer->mC.hub_name.c_str() : _("Not set"))) << "\r\n";
	os << " [*] " << autosprintf(_("Owner: %s"), (mServer->mC.hub_owner.size() ? mServer->mC.hub_owner.c_str() : _("Not set"))) << "\r\n";
	os << " [*] " << autosprintf(_("Category: %s"), (mServer->mC.hub_category.size() ? mServer->mC.hub_category.c_str() : _("Not set"))) << "\r\n";
	os << " [*] " << autosprintf(_("Locale: %s"), (mServer->mDBConf.locale.size() ? mServer->mDBConf.locale.c_str() : _("Not set"))) << "\r\n\r\n";

	os << " [*] " << autosprintf(_("Users: %d of %d"), mServer->mUserCountTot, mServer->mC.max_users_total) << "\r\n";
	os << " [*] " << autosprintf(_("Share: %s"), convertByte(mServer->mTotalShare).c_str()) << "\r\n";
	os << " [*] " << autosprintf(_("User list: %d"), mServer->mUserList.Size()) << "\r\n";
	os << " [*] " << autosprintf(_("Progress users: %d"), mServer->mInProgresUsers.Size()) << "\r\n";
	os << " [*] " << autosprintf(_("Active users: %d"), mServer->mActiveUsers.Size()) << "\r\n";
	os << " [*] " << autosprintf(_("Passive users: %d"), mServer->mPassiveUsers.Size()) << "\r\n";
	os << " [*] " << autosprintf(_("Operator count: %d of %d"), mServer->mOpchatList.Size(), mServer->mOpList.Size()) << "\r\n";
	os << " [*] " << autosprintf(_("Bot count: %d"), mServer->mRobotList.Size()) << "\r\n\r\n";

	string temp;
	double total = 0, curr;

	for (unsigned int i = 1; i < 4; i++) { // print zone 1 to 3
		curr = mServer->mUploadZone[i].GetMean(mServer->mTime);
		total += curr;
		os << " [*] " << autosprintf(_("Users in country zone #%d: %d of %d [%s] [%s]"), i, mServer->mUserCount[i], mServer->mC.max_users[i], convertByte(curr, true).c_str(), (mServer->mC.cc_zone[i - 1].size() ? mServer->mC.cc_zone[i - 1].c_str() : _("Not set"))) << "\r\n";
	}

	for (unsigned int i = 4; i <= USER_ZONES; i++) { // print zone 4 to 6
		if (i == 4) {
			if (mServer->mC.ip_zone4_min.size()) {
				temp = mServer->mC.ip_zone4_min;

				if (mServer->mC.ip_zone4_max.size()) {
					temp += '-';
					temp += mServer->mC.ip_zone4_max;
				}
			} else {
				temp = _("Not set");
			}
		} else if (i == 5) {
			if (mServer->mC.ip_zone5_min.size()) {
				temp = mServer->mC.ip_zone5_min;

				if (mServer->mC.ip_zone5_max.size()) {
					temp += '-';
					temp += mServer->mC.ip_zone5_max;
				}
			} else {
				temp = _("Not set");
			}
		} else if (i == 6) {
			if (mServer->mC.ip_zone6_min.size()) {
				temp = mServer->mC.ip_zone6_min;

				if (mServer->mC.ip_zone6_max.size()) {
					temp += '-';
					temp += mServer->mC.ip_zone6_max;
				}
			} else {
				temp = _("Not set");
			}
		}

		curr = mServer->mUploadZone[i].GetMean(mServer->mTime);
		total += curr;
		os << " [*] " << autosprintf(_("Users in IP zone #%d: %d of %d [%s] [%s]"), i, mServer->mUserCount[i], mServer->mC.max_users[i], convertByte(curr, true).c_str(), temp.c_str()) << "\r\n";
	}

	total += mServer->mUploadZone[0].GetMean(mServer->mTime); // print zone 0
	os << " [*] " << autosprintf(_("Other users: %d of %d [%s]"), mServer->mUserCount[0], mServer->mC.max_users[0], convertByte(total, true).c_str()) << "\r\n";
}

cInfoServer::~cInfoServer()
{}

}; // namespace nVerliHub
