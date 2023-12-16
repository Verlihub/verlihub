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

#ifndef NDIRECTCONNECT_NTABLES_CSETUPLIST_H
#define NDIRECTCONNECT_NTABLES_CSETUPLIST_H
#include "cconfmysql.h"
#include <string>

using namespace std;
namespace nVerliHub {
	using namespace nConfig;
	namespace nTables {

/**
table containing hub's setup variables that used to be in config file

@author Daniel Muller
*/

class cSetupList: public cConfMySQL
{
public:
	/*
		a structure representing setup data (setup-file, varname and var value)
	*/
	class cSetup
	{
		public:
			cSetup()
			{};

			cSetup(const string &file, const string &var, const string &val):
				mFile(file),
				mVarName(var),
				mVarValue(val)
			{}

			string mFile;
			string mVarName;
			string mVarValue;
	};

	cSetupList(nMySQL::cMySQL &mysql);
	~cSetupList();
	void LoadFileTo(cConfigBaseBase *, const char *);
	void SaveFileTo(cConfigBaseBase *, const char *);
	void OutputFile(const string &, ostream &os);
	void FilterFiles(const string &var, ostream &os, const string &file = "");
	bool SaveItem(const char *InFile, cConfigItemBase *);
	bool LoadItem(const char *FromFile, cConfigItemBase *);
private:
	cSetup mModel;
};

	}; // namespace nTables
}; // namespace VerliHub

#endif
