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

#include "cforbidden.h"
#include "src/cconfigitembase.h"
#include "src/cserverdc.h"
#include "i18n.h"
#include "cpiforbid.h"

namespace nVerliHub {
	using namespace nSocket;
	using namespace nEnums;
	namespace nForbidPlugin {

cForbiddenWorker::cForbiddenWorker() : mpRegex(NULL)
{
	mCheckMask = 1+4;  // default mask is 1 for public chat only, notify ops by default
	mAfClass = 4; // default maximum affected class is 4 (operator)
}

cForbiddenWorker::~cForbiddenWorker()
{
	if(mpRegex)
		delete mpRegex;
	mpRegex = NULL;
}

bool cForbiddenWorker::CheckMsg(const string &msg)
{
	return (mpRegex->Exec(msg) > 0);
}

bool cForbiddenWorker::PrepareRegex()
{
	mpRegex = new cPCRE();
	return mpRegex->Compile(mWord.c_str(),PCRE_CASELESS);
}

void cForbiddenWorker::OnLoad()
{
	this->PrepareRegex();
}

int cForbiddenWorker::DoIt(const string & cmd_line, cConnDC *conn, cServerDC *server, int mask)
{
	string text;

	if(mReason.size()) {
		// User is kick user by hub security
		ostringstream os;
		cUser *OP = server->mUserList.GetUserByNick(server->mC.hub_security);
		server->DCKickNick(&os, OP, conn->mpUser->mNick, mReason, eKCK_Drop | eKCK_Reason | eKCK_PM | eKCK_TBAN);
	}

	// Notify it in opchat
	if (eNOTIFY_OPS & mCheckMask) {

		ostringstream os;
		os << autosprintf(_("FORBID: User sent forbidden words in %s: %s"), (eCHECK_CHAT & mask) ? _("PUBLIC chat") : _("PRIVATE chat"), cmd_line.c_str());
		text = os.str();
		server->ReportUserToOpchat(conn, text, false);

		/* Send to the sender only :) */
		/* Don't display to public if its a PM! Client already did it */
		if(eCHECK_CHAT & mask)
			server->DCPublic(conn->mpUser->mNick, cmd_line, conn);
	}
	return 1;
}

ostream &operator<<(ostream &os, cForbiddenWorker &fw)
{
	string word;
	cDCProto::EscapeChars(fw.mWord, word);
	os << word << " -f " << fw.mCheckMask << " -C " << fw.mAfClass << " -r \"" << fw.mReason << "\"";
	return os;
}


//----------

cForbidden::cForbidden(cVHPlugin *pi) : tForbiddenBase(pi, "pi_forbid")
{
	SetClassName("nDC::cForbidden");
}

void cForbidden::AddFields()
{
	AddCol("word","varchar(100)","", false,mModel.mWord);
	AddPrimaryKey("word");
	AddCol("check_mask","tinyint(4)","1",true,mModel.mCheckMask); // mask 1 -> public, mask 2 -> private
	AddCol("afclass","tinyint(4)", "4", true, mModel.mAfClass); // affected class. normal=1, vip=2, cheef=3, op=4, admin=5, master=10
	AddCol("banreason", "varchar(50)", "", true, mModel.mReason);// reason of kick
	mMySQLTable.mExtra =" PRIMARY KEY(word)";
}

bool cForbidden::CompareDataKey(const cForbiddenWorker &D1, const cForbiddenWorker &D2)
{
	return D1.mWord == D2.mWord;
}

int cForbidden::ForbiddenParser(const string & str, cConnDC * conn, int mask)
{
	iterator it;
	cForbiddenWorker *forbid;
	for(it = begin(); it != end(); ++it) {
		forbid = *it;
		if((forbid->mCheckMask & mask) && forbid->CheckMsg(str)) {
			if(forbid->mAfClass >= conn->mpUser->mClass) {
				forbid->DoIt(str, conn, mOwner->mServer, mask);
				return 0;
			}
		}
	}
	return 1;
}

int cForbidden::CheckRepeat(const string & str, int r)
{
	unsigned int i = 0;
	int j = 0;

	for(; i < str.size() - 1; i++) {
		if(str[i] == str[i+1])
			++j;
		else
			j=0;
		if(j == r)
			return 0;
	}

	return 1;
}

int cForbidden::CheckUppercasePercent(const string & str, int percent)
{
	unsigned int i = 0, j = 0 , k = 0;

	for(; i < str.size(); i++) {
		if(str[i] >= 'a' && str[i] <= 'z')
			k++;
		if(str[i] >= 'A' && str[i] <= 'Z') {
			k++;
			j++;
		}
	}

	if((k * percent) < (j * 100)) {
		return 0;
	}

	return 1;
}

cForbidCfg::cForbidCfg(cServerDC *s) : mS(s)
{
	Add("max_upcase_percent",max_upcase_percent,100);
	Add("max_repeat_char", max_repeat_char, 0 );
	Add("max_class_dest", max_class_dest,2);
}

int cForbidCfg::Load()
{
	mS->mSetupList.LoadFileTo(this,"pi_forbid");
	return 0;
}

int cForbidCfg::Save()
{
	mS->mSetupList.SaveFileTo(this,"pi_forbid");
	return 0;
}

	}; // namespace nForbidPlugin
}; // namespace nVerliHub
