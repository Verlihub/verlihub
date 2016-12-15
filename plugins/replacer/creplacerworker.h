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

#ifndef NDIRECTCONNECTCREPLACERWORKER_H
#define NDIRECTCONNECTCREPLACERWORKER_H

#include <string>
#include "src/cpcre.h"

using namespace std;

namespace nVerliHub {
	namespace nReplacePlugin {

/**
@author Daniel Muller
@changes for Replacer Pralcio
*/

class cReplacerWorker
{
public:
	cReplacerWorker();
	~cReplacerWorker();
	bool CheckMsg(const string &msg);
	bool PrepareRegex();
	// the word to change
	string mWord;
	// the word that will replace mWord
	string mRepWord;
	// affected class
	int mAfClass;
private:
	nUtils::cPCRE mRegex;
};
	}; // namespace nReplacePlugin
}; // namespace nVerliHub

#endif
