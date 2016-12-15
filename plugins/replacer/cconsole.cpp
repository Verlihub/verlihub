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
#include "cpireplace.h"
#include "creplacer.h"
#include "src/stringutils.h"

namespace nVerliHub {
	using namespace nUtils;
	using namespace nSocket;
	namespace nReplacePlugin {

cConsole::cConsole(cpiReplace *replace) :
	mReplace(replace),
	/** syntax : !addreplacer word replace class  */
	mCmdReplaceAdd(1,"!addreplacer ", "(.*[^ \\s]) (.*[^ \\d])( \\d+)?", &mcfReplaceAdd),
	mCmdReplaceGet(0,"!getreplacer", "", &mcfReplaceGet),
	mCmdReplaceDel(2,"!delreplacer ", "(.*)", &mcfReplaceDel),
	mCmdr(this)
{
	mCmdr.Add(&mCmdReplaceAdd);
	mCmdr.Add(&mCmdReplaceDel);
	mCmdr.Add(&mCmdReplaceGet);
}


cConsole::~cConsole()
{}

int cConsole::DoCommand(const string &str, cConnDC * conn)
{
	ostringstream os;
	if(mCmdr.ParseAll(str, os, conn) >= 0) {
		mReplace->mServer->DCPublicHS(os.str().data(),conn);
		return 1;
	}
	return 0;
}

bool cConsole::cfGetReplacer::operator ( )()
{
	string word;
	string rep_word;
	cReplacerWorker *fw;
	(*mOS) << _("Replaced words") << ":\r\n";

	(*mOS) << "\r\n ";
	(*mOS) << "\t" << _("Word");
	(*mOS) << "\t" << _("Replace with");
	(*mOS) << _("Affected class") << "\n";
	(*mOS) << " " << string(20+20+15,'-') << "\r\n";
	for(int i = 0; i < GetPI()->mReplacer->Size(); i++) {
		fw = (*GetPI()->mReplacer)[i];
		cDCProto::EscapeChars(fw->mWord, word);
		cDCProto::EscapeChars(fw->mRepWord, rep_word);
		(*mOS) << " " << "\t" << word.c_str();
		(*mOS) << "\t" << rep_word.c_str();
		(*mOS) << fw->mAfClass << "\r\n";
	}

	return true;
}

bool cConsole::cfDelReplacer::operator ( )()
{
	string word, word_backup;
	GetParStr(1,word_backup);
	cDCProto::UnEscapeChars(word_backup, word);

	bool isInList=false;

	for(int i = 0; i < GetPI()->mReplacer->Size(); i++)
		if((* GetPI()->mReplacer)[i]->mWord == word)
			isInList = true;

	if(!isInList) {
		(*mOS) << autosprintf(_("Word '%s' does not exist."), word_backup.c_str()) << " ";
		return false;
	}

	cReplacerWorker FWord;
	FWord.mWord = word;

	GetPI()->mReplacer->DelReplacer(FWord);
	(*mOS) << autosprintf(_("Word '%s' deleted."), word_backup.c_str()) << " ";

	GetPI()->mReplacer->LoadAll();
	return true;
}

bool cConsole::cfAddReplacer::operator ( )()
{
	string word, word_backup, rep_word;
	GetParStr(1,word_backup);
	cReplacerWorker FWord;
	string num;
	GetParStr(2,rep_word);

	/** third parameter is the affected class */
	if(this->GetParStr(3,num)) {
		istringstream is(num);
		is >> FWord.mAfClass;
	}

	cPCRE TestRE;
	cDCProto::UnEscapeChars(word_backup, word);
	if(!TestRE.Compile(word.data(), 0)) {
		(*mOS) << _("Sorry the regular expression you provided cannot be parsed.") << " ";
		return false;
	}

	for(int i = 0; i < GetPI()->mReplacer->Size(); i++) {
		if((*GetPI()->mReplacer)[i]->mWord == word) {
			(*mOS) << autosprintf(_("Word '%s' already exists."), word.c_str()) << " ";
			return false;
		}
	}

	FWord.mWord = word;
	FWord.mRepWord = rep_word;

	if(GetPI()->mReplacer->AddReplacer(FWord))
		(*mOS) << autosprintf(_("Added word %s. This word will be filtered in public chat for users with class that is less than or equal to %d."), word_backup.c_str(), FWord.mAfClass) << " ";
	else
	    (*mOS) << autosprintf(_("Error adding word '%s'."), word_backup.c_str()) << " ";

	GetPI()->mReplacer->LoadAll();

	return true;
}
	}; // namespace nReplacePlugin
}; // namespace nVerliHub
