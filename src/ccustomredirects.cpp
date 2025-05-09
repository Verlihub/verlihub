/*
	Copyright (C) 2003-2005 Daniel Muller, dan at verliba dot cz
	Copyright (C) 2006-2025 Verlihub Team, info at verlihub dot net

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

#include "ccustomredirects.h"
#include "cconfigitembase.h"
#include "cserverdc.h"
#include "stringutils.h"
#include "i18n.h"

namespace nVerliHub {
	using namespace nEnums;
	namespace nTables {

	cRedirects::cRedirects(cServerDC *server):
		tMySQLMemoryList<cRedirect, cServerDC>(server->mMySQL, server, "custom_redirects")
	{
		SetClassName("nDC::cRedirects");
	}

	void cRedirects::AddFields()
	{
		AddCol("address", "varchar(125)", "", false, mModel.mAddress);
		AddPrimaryKey("address");
		AddCol("flag", "smallint(5)", "0", false, mModel.mFlag);
		AddCol("start", "tinyint(2)", "0", false, mModel.mStart);
		AddCol("stop", "tinyint(2)", "0", false, mModel.mStop);
		AddCol("country", "varchar(50)", "", true, mModel.mCountry);
		AddCol("secure", "tinyint(1)", "2", false, mModel.mSecure);
		AddCol("share", "int(11)", "0", false, mModel.mShare);
		AddCol("enable", "tinyint(1)", "1", false, mModel.mEnable);
		mMySQLTable.mExtra = "PRIMARY KEY(address)";
		SetBaseTo(&mModel);
	}

	int cRedirects::MapTo(unsigned int rype)
	{
		switch (rype) {
			case eCR_INVALID_USER:
			case eCR_KICKED:
				return eKick;

			case eCR_USERLIMIT:
				return eUserLimit;

			case eCR_SHARE_LIMIT:
				return eShareLimit;

			case eCR_TAG_INVALID:
			case eCR_TAG_NONE:
				return eTag;

			case eCR_PASSWORD:
				return eWrongPasswd;

			case eCR_INVALID_KEY:
				return eInvalidKey;

			case eCR_HUB_LOAD:
				return eHubBusy;

			case eCR_RECONNECT:
				return eReconnect;

			case eCR_CLONE:
				return eClone;

			case eCR_SELF:
				return eSelf;

			case eCR_BADNICK:
				return eBadNick;

			case eCR_NOREDIR:
			case eCR_FORCEMOVE: // we already sent redirect
				return -1;

			default:
				return 0;
		}
	}

	/*
		find redirect url from a given type
	*/
	char* cRedirects::MatchByType(unsigned int rype, const string &cc, bool issec, unsigned __int64 shar)
	{
		int rmap = MapTo(rype);

		if (rmap == -1) // do not redirect, special reason
			return NULL;

		iterator it;
		cRedirect *redir;
		const char *rist[10]; // todo: why max 10 matches?
		int cnt = 0;
		time_t curr_time;
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

		for (it = begin(); it != end(); ++it) {
			if (cnt >= 10)
				break;

			redir = (*it);

			if (redir && redir->mEnable && (!redir->mFlag || (redir->mFlag & rmap))) {
				if ((redir->mStart == redir->mStop) || ((int(lt->tm_hour) >= redir->mStart) && (int(lt->tm_hour) <= redir->mStop))) { // redirect hours
					if (redir->mCountry.empty() || cc.empty() || (toUpper(redir->mCountry).find(cc) != redir->mCountry.npos)) { // country match
						if ((redir->mSecure == 2) || ((redir->mSecure == 1) == issec)) { // secure connection
							if ((redir->mShare == 0) || (shar >= (unsigned __int64)((unsigned long)redir->mShare * 1024ul * 1024ul * 1024ul))) { // minimal share in gb
								rist[cnt] = redir->mAddress.c_str();
								cnt++;
							}
						}
					}
				}
			}
		}

		if (!cnt) // no match
			return NULL;

		Random(cnt);

		if (!rist[cnt] || (rist[cnt][0] == '\0'))
			return NULL;

		for (it = begin(); it != end(); ++it) {
			redir = (*it);

			if (redir && redir->mEnable && (!redir->mFlag || (redir->mFlag & rmap)) && (StrCompare(redir->mAddress, 0, redir->mAddress.size(), rist[cnt]) == 0)) {
				redir->mCount++; // increase counter
				break;
			}
		}

		string lodr = toLower(rist[cnt]);

		for (unsigned int i = 1; i < 2; i++) {
			if (lodr.find(mOldMap[i]) != lodr.npos)
				return strdup(mOldMap[0].c_str());
		}

		return strdup(rist[cnt]);
	}

	void cRedirects::Random(int &key)
	{
		srand (time(NULL));
		int temp = int(1.0 * key * rand() / (RAND_MAX + 1.0));

		if (temp < key)
			key = temp;
		else
			key -= 1;
	}

	bool cRedirects::CompareDataKey(const cRedirect &D1, const cRedirect &D2)
	{
		return (D1.mAddress == D2.mAddress);
	}

	cRedirectConsole::cRedirectConsole(cDCConsole *console):
		tRedirectConsoleBase(console)
	{
		this->AddCommands();
	}

	cRedirectConsole::~cRedirectConsole()
	{}

	void cRedirectConsole::GetHelpForCommand(int cmd, ostream &os)
	{
		string help_str;

		switch (cmd) {
			case eLC_LST:
				help_str = "!lstredirect\r\n" + string(_("Show list of redirects"));
				break;

			case eLC_ADD:
			case eLC_MOD:
				help_str = "!(add|mod)redirect <url>"
					"[ -f <flags>]"
					"[ -a <start>]"
					"[ -z <stop>]"
					"[ -c <:cc:>]"
					"[ -s <2=any/1=yes/0=no>]"
					"[ -g <share>]"
					"[ -e <1=yes/0=no>]";

				break;

			case eLC_DEL:
				help_str = "!delredirect <url>";
				break;

			default:
				break;
		}

		if (help_str.size()) {
			cDCProto::EscapeChars(help_str, help_str);
			os << help_str;
		}
	}

	void cRedirectConsole::GetHelp(ostream &os)
	{
		string help("https://github.com/verlihub/verlihub/wiki/redirects/\r\n\r\n");

		help += " Available redirect flags:\r\n\r\n";
		help += " 0\t\t\t- For any reason\r\n";
		help += " 1\t\t\t- Ban and kick\r\n";
		help += " 2\t\t\t- Hub is full\r\n";
		help += " 4\t\t\t- Too low or too high share\r\n";
		help += " 8\t\t\t- Wrong or unknown tag\r\n";
		help += " 16\t\t\t- Wrong password\r\n";
		help += " 32\t\t\t- Invalid key\r\n";
		help += " 64\t\t\t- Hub is busy\r\n";
		help += " 128\t\t\t- Too fast reconnect\r\n";
		help += " 256\t\t\t- Bad nick, already used, too short, etc\r\n";
		help += " 512\t\t\t- Clone detection\r\n";
		help += " 1024\t\t\t- Same user connects twice\r\n\r\n";
		help += " Remember to make the sum of selected above flags.\r\n";

		cDCProto::EscapeChars(help, help);
		os << help;
	}

	const char* cRedirectConsole::GetParamsRegex(int cmd)
	{
		switch (cmd) {
			case eLC_ADD:
			case eLC_MOD:
				return "^(\\S+)("
					"( -f ?(\\d+))?|"
					"( -a ?(\\d+))?|"
					"( -z ?(\\d+))?|"
					"( -c ?(\\S*))?|"
					"( -s ?(2|1|0))?|"
					"( -g ?(\\d+))?|"
					"( -e ?(1|0))?|"
					")*\\s*$";

			case eLC_DEL:
				return "(\\S+)";

			default:
				return "";
		}
	}

	bool cRedirectConsole::ReadDataFromCmd(cfBase *cmd, int CmdID, cRedirect &data)
	{
		enum {
			eADD_ALL,
   			eADD_ADDRESS,
			eADD_CHOICE,
   			eADD_FLAGp, eADD_FLAG,
			eADD_STARTp, eADD_START,
			eADD_STOPp, eADD_STOP,
			eADD_COUNTRYp, eADD_COUNTRY,
			eADD_SECUREp, eADD_SECURE,
			eADD_SHAREp, eADD_SHARE,
			eADD_ENABLEp, eADD_ENABLE
		};

		cmd->GetParStr(eADD_ADDRESS, data.mAddress);
		cmd->GetParInt(eADD_FLAG, data.mFlag);
		cmd->GetParInt(eADD_START, data.mStart);
		cmd->GetParInt(eADD_STOP, data.mStop);
		cmd->GetParStr(eADD_COUNTRY, data.mCountry);
		cmd->GetParInt(eADD_SECURE, data.mSecure);
		cmd->GetParInt(eADD_SHARE, data.mShare);
		cmd->GetParInt(eADD_ENABLE, data.mEnable);
		return true;
	}

	cRedirects *cRedirectConsole::GetTheList()
	{
		return mOwner->mRedirects;
	}

	const char* cRedirectConsole::CmdSuffix() { return "redirect"; }
	const char* cRedirectConsole::CmdPrefix() { return "!"; }

	void cRedirectConsole::ListHead(ostream *os)
	{
		(*os) << "\r\n\r\n\t" << _("Hits");
		(*os) << "\t" << _("Start");
		(*os) << "\t" << _("Stop");
		(*os) << "\t" << _("URL");
		(*os) << "\t\t\t\t" << _("Status");
		(*os) << "\t" << _("Type");
		(*os) << "\t\t" << _("Country");
		(*os) << "\t" << _("Secure");
		(*os) << "\t" << _("Share");
		(*os) << "\r\n\t" << string(150, '-') << "\r\n";
	}

	bool cRedirectConsole::IsConnAllowed(cConnDC *conn, int cmd)
	{
		if (!conn || !conn->mpUser)
			return false;

		switch (cmd) {
			case eLC_ADD:
			case eLC_MOD:
			case eLC_DEL:
			case eLC_HELP:
				return (conn->mpUser->mClass >= mOwner->mServer->mC.min_class_redir);
			case eLC_LST:
				return (conn->mpUser->mClass >= mOwner->mServer->mC.min_class_lstredir);
			default:
				return false;
		}
	}

	}; // namespace nTables
}; // namespace nVerliHub
