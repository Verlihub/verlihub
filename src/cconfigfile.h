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

#ifndef CCONFIGFILE_H
#define CCONFIGFILE_H
#include "cconfigbase.h"
#include <string>


using namespace std;
namespace nVerliHub {
	namespace nConfig {
/**configuration lodable from a file
  *@author Daniel Muller
  */

class cConfigFile : public cConfigBase
{
public:
	cConfigFile(const string &file, bool load=true);
	/** The config load function - from a file */
	int Load();
	~cConfigFile();
	/** save config, to be able to load it after */
	int Save();
	int Save(ostream &);
protected: // Protected attributes
	/** filename */
	string mFile;

};
	}; // namespace nConfig
}; // namespace nVerliHub
#endif
