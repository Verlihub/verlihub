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

#include "cdcclients.h"
#include "cconfigitembase.h"
#include "cserverdc.h"
#include "cpcre.h"
#include "i18n.h"

namespace nVerliHub {
	using nUtils::cPCRE;
	using namespace nEnums;
	cDCTagParser cDCClients::mParser;

	cDCTagParser::cDCTagParser()
	{
		/*
			0 - whole chunk
			1 - whole prefix
			2 - id in prefix
			3 - version in prefix
			4 - id
			5 - version
			6 - body
		*/

		if (!mTagRE.Compile("(<(.+) +v?(.+)>)?<(.+?) +[Vv]\\:?([^,]+),([^>]*)>"))
			throw "Error in tag general regular expression";

		/*
			0 - whole chunk
			1 - version number
			2 - additional information
		*/

		if (!mVersRE.Compile("\\(?r?([\\d\\.]+)\\)?(\\-[^\\d]+(\\d+)?)?(\\-(\\d+))?"))
			throw "Error in version regular expression";

		/*
			0 - whole chunk
			1 - mode name
		*/

		if (!mModeRE.Compile("[Mm]\\:([^,]+)"))
			throw "Error in mode regular expression";

		/*
			0 - whole chunk
			1 - guest hubs
			2 - registered hubs
			3 - operator hubs
		*/

		if (!mHubsRE.Compile("[Hh]\\:(\\d+)(\\/\\d+)?(\\/\\d+)?"))
			throw "Error in hubs regular expression";

		/*
			0 - whole chunk
			1 - slots
		*/

		if (!mSlotsRE.Compile("[Ss]\\:(\\d+)"))
			throw "Error in slots regular expression";

		/*
			0 - whole chunk
			1 - identifier
			2 - value
		*/

		if (!mLimitRE.Compile("([Bb]\\:|[Ll]\\:|[Ff]\\:\\d+\\/)(\\d+(\\.\\d)?)"))
			throw "Error in limiter regular expression";
	}

	namespace nTables {

	cDCClients::cDCClients(cServerDC *server):
		tMySQLMemoryList<cDCClient,cServerDC>(server->mMySQL, server, "client_list"),
		mServer(server)
	{
		SetClassName("nDC::cDCClients");
		mPositionInDesc = -1;
	}

	void cDCClients::AddFields()
	{
		AddCol("name", "varchar(125)", "", false, mModel.mName);
		AddPrimaryKey("name");
		AddCol("tag_id", "varchar(125)", "", false, mModel.mTagID);
		AddCol("min_version", "decimal(8,5)", "-1", false, mModel.mMinVersion);
		AddCol("max_version", "decimal(8,5)", "-1", false, mModel.mMaxVersion);
		AddCol("min_ver_use", "decimal(8,5)", "-1", false, mModel.mMinVerUse);
		AddCol("ban", "tinyint(1)", "0", false, mModel.mBan);
		AddCol("enable", "tinyint(1)", "1", false, mModel.mEnable);
		mMySQLTable.mExtra = "PRIMARY KEY(name)";
		SetBaseTo(&mModel);
	}

	cDCClient* cDCClients::FindTag(const string &tagID)
	{
		iterator it;
		cDCClient *client;

		for (it= begin(); it != end(); ++it) {
			client = *it;

			if (client && client->mEnable && (client->mTagID == tagID)) // todo: use map
				return client;
		}

		return NULL; // unknwon client
	}

	bool cDCClients::CompareDataKey(const cDCClient &D1, const cDCClient &D2)
	{
		return (D1.mName == D2.mName);
	}

	bool cDCClients::ParsePos(const string &desc)
	{
		mPositionInDesc = -1;

		if (mParser.mTagRE.Exec(desc) >= 0)
			mPositionInDesc = mParser.mTagRE.StartOf(0);

		return (mPositionInDesc > -1);
	}

	cDCTag* cDCClients::ParseTag(const string &desc)
	{
		string str, ver("0");
		cDCTag *tag = new cDCTag(mServer);

		/*
			0 - whole chunk
			1 - whole prefix
			2 - id in prefix
			3 - version in prefix
			4 - id
			5 - version
			6 - body
		*/

		enum {
			eTP_COMPLETE,
			eTP_PREFIX, eTP_PTAGID, eTP_PVERSION,
			eTP_TAGID, eTP_VERSION,
			eTP_BODY
		};

		tag->mClientMode = eCM_NOTAG; // todo: detect invalid tag
		mPositionInDesc = -1;

		if (mParser.mTagRE.Exec(desc) >= 3) {
			mPositionInDesc = mParser.mTagRE.StartOf(eTP_COMPLETE); // copy tag parts
			mParser.mTagRE.Extract(eTP_COMPLETE, desc, tag->mTag);
			mParser.mTagRE.Extract(eTP_BODY, desc, tag->mTagBody);
			mParser.mTagRE.Extract(eTP_TAGID, desc, tag->mTagID);
			mParser.mTagRE.Extract(eTP_VERSION, desc, str);

			/*
				0 - whole chunk
				1 - main version
				2 - crap information
				3 - crap number
				4 - build information
				5 - build number
			*/

			enum {
				eVP_COMPLETE,
				eVP_MAINVER,
				eVP_CRAPINFO, eVP_CRAPNUM,
				eVP_BUILDINFO, eVP_BUILDVER
			};

			if (mParser.mVersRE.Exec(str) >= 1) { // client version
				mParser.mVersRE.Extract(eVP_MAINVER, str, ver);

				if (mParser.mVersRE.PartFound(eVP_BUILDINFO)) {
					mParser.mVersRE.Extract(eVP_BUILDVER, str, str);
					ver.append(1, '.');
					ver.append(str);
				}

				size_t pos = ver.find('.');

				if (pos != ver.npos) {
					pos = ver.find('.', pos + 1);

					while (pos != ver.npos) {
						ver.erase(pos, 1);
						pos = ver.find('.', pos);
					}
				}
			}

			/*
			if (mParser.mTagRE.PartFound(eTP_PREFIX)) {
				mParser.mTagRE.Extract(eTP_PTAGID, desc, tag->mTagID);
				mParser.mTagRE.Extract(eTP_PVERSION, desc, str);
			}
			*/
		}

		tag->client = FindTag(tag->mTagID); // todo: use FindData

		if (mParser.mModeRE.Exec(tag->mTagBody) >= 1) { // client mode
			mParser.mModeRE.Extract(1, tag->mTagBody, str);

			if (str == "A")
				tag->mClientMode = eCM_ACTIVE;
			else if (str == "P")
				tag->mClientMode = eCM_PASSIVE;
			else if (str == "5")
				tag->mClientMode = eCM_SOCK5;
			else
				tag->mClientMode = eCM_OTHER;
		}

		istringstream is(ver);
		is >> tag->mClientVersion;
		is.clear();

		int hubs = -1, hubs_usr = -1, hubs_reg = -1, hubs_op = -1, tmp;
		char c;

		if (mParser.mHubsRE.Exec(tag->mTagBody) >= 1) { // number of hubs
			mParser.mHubsRE.Extract(1, tag->mTagBody, str); // where user is guest
			is.str(str);
			is >> hubs;
			is.clear();
			hubs_usr = hubs;

			if (mParser.mHubsRE.PartFound(2)) { // where user is registered
				tmp = 0;
				mParser.mHubsRE.Extract(2, tag->mTagBody, str);
				is.str(str);
				is >> c >> tmp;
				is.clear();

				if (mServer->mC.tag_sum_hubs >= 2)
					hubs += tmp;

				hubs_reg = tmp;
			}

			if (mParser.mHubsRE.PartFound(3)) { // where user is operator
				tmp = 0;
				mParser.mHubsRE.Extract(3, tag->mTagBody, str);
				is.str(str);
				is >> c >> tmp;
				is.clear();

				if (mServer->mC.tag_sum_hubs >= 3)
					hubs += tmp;

				hubs_op = tmp;
			}

			tag->mTotHubs = hubs;
			tag->mHubsUsr = hubs_usr;
			tag->mHubsReg = hubs_reg;
			tag->mHubsOp = hubs_op;
		}

		if (mParser.mSlotsRE.Exec(tag->mTagBody) >= 1) { // number of slots
			mParser.mSlotsRE.Extract(1, tag->mTagBody, str);
			is.str(str);
			is >> tag->mSlots;
			is.clear();
		}

		if (mParser.mLimitRE.Exec(tag->mTagBody) >= 2) { // limiter
			mParser.mLimitRE.Extract(2, tag->mTagBody, str);
			is.str(str);
			is >> tag->mLimit;
			is.clear();
		}

		return tag;
	}

	cDCClientConsole::cDCClientConsole(cDCConsole *console):
		tDCClientConsoleBase(console)
	{
		this->AddCommands();
	}

	cDCClientConsole::~cDCClientConsole()
	{}

	void cDCClientConsole::GetHelpForCommand(int cmd, ostream &os)
	{
		string help_str;

		switch (cmd) {
			case eLC_LST:
				help_str = "!lstclient\r\n" + string(_("Show list of clients"));
				break;

			case eLC_ADD:
			case eLC_MOD:
				help_str = "!(add|mod)client <\"name\">"
					"[ -t <\"id\">]"
					"[ -b <1/0>]"
					"[ -v <min_ver>]"
					"[ -V <max_ver>]"
					"[ -u <min_ver_use>]"
					"[ -e <1/0>]";

				break;

			case eLC_DEL:
				help_str = "!delclient <\"name\">";
				break;

			default:
				break;
		}

		if (help_str.size()) {
			cDCProto::EscapeChars(help_str, help_str);
			os << help_str;
		}
	}

	void cDCClientConsole::GetHelp(ostream &os)
	{
		string help;
		help = "https://github.com/verlihub/verlihub/wiki/clients/\r\n\r\n";
		help += " -t\tClient ID (in <++ V:0.871,M:A,H:1/0/0,S:1> ID is '++')\r\n";
		help += " -b\tBan client with any version (0 - no, 1 - yes)\r\n";
		help += " -v\tMinimum allowed version number\r\n";
		help += " -V\tMaximum allowed version number\r\n";
		help += " -u\tMinimum version to use hub\r\n";
		help += " -e\tEnable or disable this rule (0 - off, 1 - on)\r\n";
		cDCProto::EscapeChars(help, help);
		os << help;
	}

	const char* cDCClientConsole::GetParamsRegex(int cmd)
	{
		switch (cmd) {
			case eLC_ADD:
			case eLC_MOD:
				return "^(\"[^\"]+?\"|\\S+)("
					"( -t ?(\"[^\"]+?\"|\\S+))?|"
					"( -b ?(0|1))?|"
					"( -v ?(\\-?\\d+(\\.\\d+)?))?|"
					"( -V ?(\\-?\\d+(\\.\\d+)?))?|"
					"( -u ?(\\-?\\d+(\\.\\d+)?))?|"
					"( -e ?(0|1))?"
					")*\\s*$";

			case eLC_DEL:
				return "(\".+?\"|\\S+)";

			default:
				return "";
		}
	}

	bool cDCClientConsole::ReadDataFromCmd(cfBase *cmd, int CmdID, cDCClient &data)
	{
		cDCClient temp = data;

		enum {
			eDATA_ALL,
   			eDATA_NAME, eDATA_CHOICE,
   			eDATA_TAGIDp, eDATA_TAGID,
			eDATA_CLIENTBANNEDp, eDATA_CLIENTBANNED,
			eDATA_MINVp, eDATA_MINV, eDATA_MINVDECp,
			eDATA_MAXVp, eDATA_MAXV, eDATA_MAXVDECp,
			eDATA_MINVUp, eDATA_MINVU, eDATA_MINVUDECp,
			eDATA_ENABLEp, eDATA_ENABLE
		};

		cmd->GetParStr(eDATA_NAME, temp.mName);
		size_t slen = temp.mName.size();

		if (slen) {
			if (temp.mName[0] == '"') {
				temp.mName.erase(0, 1);
				slen--;
			}

			if (slen && (temp.mName[slen - 1] == '"'))
				temp.mName.erase(slen - 1, 1);
		}

		cmd->GetParStr(eDATA_TAGID, temp.mTagID);
		slen = temp.mTagID.size();

		if (slen) {
			if (temp.mTagID[0] == '"') {
				temp.mTagID.erase(0, 1);
				slen--;
			}

			if (slen && (temp.mTagID[slen - 1] == '"')) {
				temp.mTagID.erase(slen - 1, 1);
				slen--;
			}
		}

		if ((CmdID == eLC_ADD) && !slen) {
			//os << _("Client ID can't be empty.");
			return false;
		}

		cmd->GetParBool(eDATA_CLIENTBANNED, temp.mBan);
		cmd->GetParDouble(eDATA_MINV, temp.mMinVersion);
		cmd->GetParDouble(eDATA_MAXV, temp.mMaxVersion);
		cmd->GetParDouble(eDATA_MINVU, temp.mMinVerUse);
		cmd->GetParBool(eDATA_ENABLE, temp.mEnable);
		data = temp;
		return true;
	}

	cDCClients *cDCClientConsole::GetTheList()
	{
		return mOwner->mDCClients;
	}

	const char *cDCClientConsole::CmdSuffix() { return "client"; }
	const char *cDCClientConsole::CmdPrefix() { return "!"; }

	void cDCClientConsole::ListHead(ostream *os)
	{
		(*os) << "\r\n\r\n\t" << _("Name");
		(*os) << "\t\t\t" << _("ID");
		(*os) << "\t\t" << _("Version");
		(*os) << "\t\t\t" << _("Use hub");
		(*os) << "\t\t" << _("Banned");
		(*os) << "\t" << _("Status");
		(*os) << "\r\n\t" << string(116, '-') << "\r\n";
	}

	bool cDCClientConsole::IsConnAllowed(cConnDC *conn, int cmd)
	{
		return (conn && conn->mpUser && (conn->mpUser->mClass >= eUC_ADMIN));
	}

	}; // namespace nTables
}; // namespace nVerliHub
