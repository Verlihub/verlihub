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

#include "ccustomredirects.h"
#include "cconfigitembase.h"
#include "cserverdc.h"
#include "i18n.h"

namespace nVerliHub {
	using namespace nEnums;
	namespace nTables {

	cRedirects::cRedirects( cServerDC *server ) :
		tMySQLMemoryList<cRedirect, cServerDC>(server->mMySQL, server, "custom_redirects")
	{
		SetClassName("nDC::cRedirects");
	}

	void cRedirects::AddFields()
	{
		AddCol("address", "varchar(125)", "", false, mModel.mAddress);
		AddPrimaryKey("address");
		AddCol("flag", "smallint(5)", "", false, mModel.mFlag);
		AddCol("enable", "tinyint(1)", "1", true, mModel.mEnable);
		mMySQLTable.mExtra = "PRIMARY KEY(address)";
		SetBaseTo(&mModel);
	}

	int cRedirects::MapTo(int Type)
	{
		switch (Type) {
			case eCR_INVALID_USER:
			case eCR_KICKED:
				return eKick;
				break;
			case eCR_USERLIMIT:
				return eUserLimit;
				break;
			case eCR_SHARE_LIMIT:
				return eShareLimit;
				break;
			case eCR_TAG_INVALID:
			case eCR_TAG_NONE:
				return eTag;
				break;
			case eCR_PASSWORD:
				return eWrongPasswd;
				break;
			case eCR_INVALID_KEY:
				return eInvalidKey;
				break;
			case eCR_HUB_LOAD:
				return eHubBusy;
				break;
			case eCR_RECONNECT:
				return eReconnect;
				break;
			case eCR_BADNICK:
				return eBadNick;
				break;
			case eCR_NOREDIR:
				return -1;
				break;
			default:
				return 0;
				break;
		}
	}

	/**

	Find a redirect address from a given type ID. If there is more than one address in the list...

	@param[in,out] os The stream where to store the description.
	@param[in,out] tr The cRedirect object that describes the redirect
	@return The stream
	*/

	char *cRedirects::MatchByType(int Type)
	{
		iterator it;
		cRedirect *redirect;
		char *redirects[10];
		char *DefaultRedirect[10];
		int i = 0, j = 0, iType = MapTo(Type);

		if (iType == -1) // do not redirect, special reason
			return (char*)"";

		for (it = begin(); it != end(); ++it) {
			if (i >= 10)
				break;

			redirect = *it;

			if (redirect->mEnable && (redirect->mFlag & iType)) {
				redirects[i] = (char*)redirect->mAddress.c_str();
				i++;
			}

			if (redirect->mEnable && !redirect->mFlag && (j < 10)) {
				DefaultRedirect[j] = (char*)redirect->mAddress.c_str();
				j++;
			}
		}

		if (!i) { // use default redirect
			if (!j)
				return (char*)"";

			Random(j);
			CountPlusPlus(DefaultRedirect[j]);
			return DefaultRedirect[j];
		}

		Random(i);
		CountPlusPlus(redirects[i]);
		return redirects[i];
	}

	void cRedirects::Random(int &key)
	{
		srand (time(NULL));
		int temp = (int) (1.0 * key * rand()/(RAND_MAX+1.0));
		if(temp < key) key = temp;
		else key -= 1;
	}

	void cRedirects::CountPlusPlus(char *addr)
	{
		iterator it;
		cRedirect *redirect;

		for (it = begin(); it != end(); ++it) {
			redirect = *it;

			if (addr == (char*)redirect->mAddress.c_str()) {
				redirect->mCount++; // increase counter
				break;
			}
		}
	}

	bool cRedirects::CompareDataKey(const cRedirect &D1, const cRedirect &D2)
	{
		return (D1.mAddress == D2.mAddress);
	}

	cRedirectConsole::cRedirectConsole(cDCConsole *console) : tRedirectConsoleBase(console)
	{
		this->AddCommands();
	}

	cRedirectConsole::~cRedirectConsole()
	{}

	void cRedirectConsole::GetHelpForCommand(int cmd, ostream &os)
	{
		string help_str;
		switch(cmd)
		{
			case eLC_LST:
				help_str = "!lstredirect\r\nShow the list of redirects";
				break;
			case eLC_ADD:
			case eLC_MOD:
				help_str = "!(add|mod)redirect <address>"
						"[ -f <\"redirect flag\">]"
						"[ -e <enable/disable>]";
				break;
			case eLC_DEL:
				help_str = "!delredirect <address>"; break;
				default: break;
		}
		cDCProto::EscapeChars(help_str,help_str);
		os << help_str;
	}

	void cRedirectConsole::GetHelp(ostream &os)
	{
		string help;

		help = "http://verlihub.net/doc/page/manual.redirects\r\n\r\n";

		help += " Available redirect flags:\r\n\r\n";
		help += " 0\t\t\t- For any other reason\r\n";
		help += " 1\t\t\t- Ban and kick\r\n";
		help += " 2\t\t\t- Hub is full\r\n";
		help += " 4\t\t\t- Too low or too high share\r\n";
		help += " 8\t\t\t- Wrong or unknown tag\r\n";
		help += " 16\t\t\t- Wrong password\r\n";
		help += " 32\t\t\t- Invalid key\r\n";
		help += " 64\t\t\t- Hub is busy\r\n";
		help += " 128\t\t\t- Too fast reconnect\r\n";
		help += " 256\t\t\t- Bad nick, already used, too short etc.\r\n\r\n";
		help += " Remember to make the sum of selected above flags.\r\n";

		cDCProto::EscapeChars(help, help);
		os << help;
	}

	const char * cRedirectConsole::GetParamsRegex(int cmd)
	{
		switch(cmd)
		{
			case eLC_ADD:
			case eLC_MOD:
				return "^(\\S+)("
						"( -f ?(-?\\d+))?|" //[ -f<flag>]
						"( -e ?(-?\\d))?|" // [ -e<1/0>]
						")*\\s*$"; // the end of message
			case eLC_DEL:
				return "(\\S+)";
				default: return "";break;
		};
	}

	bool cRedirectConsole::ReadDataFromCmd(cfBase *cmd, int CmdID, cRedirect &data)
	{
		enum {
			eADD_ALL,
   			eADD_ADDRESS, eADD_CHOICE,
   			eADD_FLAGp, eADD_FLAG,
			eADD_ENABLEp, eADD_ENABLE };
		cmd->GetParStr(eADD_ADDRESS,data.mAddress);
		cmd->GetParInt(eADD_FLAG,data.mFlag);
		cmd->GetParInt(eADD_ENABLE, data.mEnable);
		return true;
	}

	cRedirects *cRedirectConsole::GetTheList()
	{
		return mOwner->mRedirects;
	}

	const char *cRedirectConsole::CmdSuffix(){ return "redirect";}
	const char *cRedirectConsole::CmdPrefix(){ return "!";}

	void cRedirectConsole::ListHead(ostream *os)
	{
		(*os) << "\r\n ";
		(*os) << setw(10) << setiosflags(ios::left) << toUpper(_("Count"));
		(*os) << setw(35) << setiosflags(ios::left) << toUpper(_("Address"));
		(*os) << setw(35) << setiosflags(ios::left) << toUpper(_("Type"));
		(*os) << toUpper(_("Status")) << "\r\n";
		(*os) << " " << string(30 + 25 + 25, '=');
	}

	bool cRedirectConsole::IsConnAllowed(cConnDC *conn,int cmd)
	{
		return (conn && conn->mpUser && conn->mpUser->mClass >= eUC_ADMIN);
	}
	}; // namespace nTables
}; // namespace nVerliHub
