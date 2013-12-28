/*
	Copyright (C) 2003-2005 Daniel Muller, dan at verliba dot cz
	Copyright (C) 2006-2014 Verlihub Project, devs at verlihub-project dot org

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

#define HAVE_OSTREAM 1
#include <verlihub/cconndc.h>
#include "cconsole.h"
#include "cpiforbid.h"
#include "cforbidden.h"

using namespace nDirectConnect;

namespace nForbid
{

cConsole::cConsole(cpiForbid *forbid) :
	mForbid(forbid),
	mCmdForbidAdd(1,"\\+addforbidden ", "(\\S+)", &mcfForbidAdd),
	mCmdForbidGet(0,"\\+getforbidden", "", &mcfForbidGet),
	mCmdForbidDel(2,"\\+delforbidden ", "(\\S+)", &mcfForbidDel),
	mCmdr(this)
{
	mCmdr.Add(&mCmdForbidAdd);
	mCmdr.Add(&mCmdForbidDel);
	mCmdr.Add(&mCmdForbidGet);
}


cConsole::~cConsole()
{}

int cConsole::DoCommand(const string &str, cConnDC * conn)
{
	ostringstream os;
	cout << "cConsole::DoCommand" << endl;
	if(mCmdr.ParseAll(str, os, conn) >= 0)
	{
		mForbid->mServer->DCPublicHS(os.str().data(),conn);
		return 1;
	}
	return 0;
}

bool cConsole::cfGetForbidden::operator ( )()
{
	string word;
	
	(*mOS) << "Forbidden words: " << "\r\n";
	for(int i = 0; i < GetPI()->mForbidden->Size()-1; i++)
		(*mOS) << (*GetPI()->mForbidden)[i]->mWord << "\r\n";
	  
	return false;
}

bool cConsole::cfDelForbidden::operator ( )()
{
	string word;
	bool isInList=false;
	GetParStr(1, word);
	
	for(int i = 0; i < GetPI()->mForbidden->Size()-1; i++)
		if((* GetPI()->mForbidden)[i]->mWord == word)
			isInList = true;
	
	if(!isInList)
	{
		(*mOS) << "Forbidden word: " << word << " is NOT in list, so couldn't delete!" << "\r\n";
		return false;
	}
	
	cForbiddenWorker FWord;
	FWord.mWord = word;
	
	GetPI()->mForbidden->DelForbidden(FWord);
	(*mOS) << "Forbidden word: " << word << " deleted." << "\r\n";
	
	GetPI()->mForbidden->LoadAll();
	return true;
}

bool cConsole::cfAddForbidden::operator ( )()
{
	string word;
	GetParStr(1,word);	
	transform(word.begin(), word.end(), word.begin(), ::tolower);
	
	for(int i = 0; i < GetPI()->mForbidden->Size()-1; i++)
	    if((*GetPI()->mForbidden)[i]->mWord == word)
	    {
	        (*mOS) << "Forbidden word: " << word << " already in list! NOT added!" << "\r\n";
	        return false;
	    }
	
	cForbiddenWorker FWord;
	FWord.mWord = word;
	
	if (GetPI()->mForbidden->AddForbidden(FWord))
	    (*mOS) << "Forbidden word: " << word << " added." << "\r\n";
	else
	    (*mOS) << "Forbidden word: " << word << " NOT added!" << "\r\n";
	    
	GetPI()->mForbidden->LoadAll();
    
	return true;
}

};
