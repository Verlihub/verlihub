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

#include "cconfigfile.h"
#include <fstream>
#include <sstream>

namespace nVerliHub {
	namespace nConfig {

/*
 	to be used in a following way..
	* add as many fields as nedded, all binded to some physical locations and refering to the types
	* variable given by the name can be found and loaded using a >> operator (virtual template)
	* variable can be also outputed by the << operator
*/

cConfigFile::cConfigFile(const string &file, bool load):
	mFile(file)
{
	if (load)
		Load();
}

cConfigFile::~cConfigFile()
{}

int cConfigFile::Load() // config load function from a file
{
	ifstream is(mFile.c_str());

	if (!is.is_open()) {
		if (ErrLog(1))
			LogStream() << "Unable to open configuration file: " << mFile << endl;

		return 0;
	}

	string name;
	string str;
	istringstream *ss;
	cConfigItemBase *ci;
	char ch;

	while (!is.eof()) {
		is >> name;

		if (name.empty())
			continue;

		ch = ' ';

		if (name[name.size() - 1] != '=') {
			is >> ch >> ws;

			if (ch == ' ')
				break;
		} else {
			ch = '=';
			name.assign(name, 0, name.size() - 1);
		}

		getline(is, str);

		if (ch != '=')
			break;

		if ((ci = operator[](name))) {
			ss = new istringstream(str);
			//ss->str(str);
			ss->seekg(0, istream::beg);
			(*ss) >> *ci;
			delete ss;
			ss = NULL;
		} else {
			if (ErrLog(3))
				LogStream() << "Unknown configuration variable " << name << " in file: " << mFile << endl;
		}
	}

	is.close();
	return 1;
}

/** save config, to be able to load it after */
int cConfigFile::Save(ostream &os)
{
	for(tIHIt it = mhItems.begin();it != mhItems.end(); it++)
		os << (*it)->mName << " = " << (*it) << "\r\n";
	return 0;
}

int cConfigFile::Save()
{
	ofstream of(mFile.c_str());
	Save(of);
	of.close();
	return 0;
}
	}; // namespace nConfig
}; // namespace nVerliHub
