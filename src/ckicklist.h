/*
	Copyright (C) 2003-2005 Daniel Muller, dan at verliba dot cz
	Copyright (C) 2006-2016 Verlihub Team, info at verlihub dot net

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

#ifndef NDIRECTCONNECT_NTABLESCKICKLIST_H
#define NDIRECTCONNECT_NTABLESCKICKLIST_H
#include "cconfmysql.h"
#include "ckick.h"

namespace nVerliHub {
	namespace nSocket {
		class cConnDC;
	};
	namespace nTables {

/**
@author Daniel Muller
*/
class cKickList : public nConfig::cConfMySQL
{
public:
	cKickList(nMySQL::cMySQL &mysql);
	~cKickList();
	bool AddKick(nSocket::cConnDC *,const string& OPNick, const string *, cKick &);
	bool FindKick(cKick &Dest,const string& Nick, const string &OpNick, unsigned Age, bool WithReason, bool WithDrop, bool IsNick=true);
	void Cleanup();
protected:
	cKick mModel;
};

	}; // namespace nTables
}; // namespace nVerliHub

#endif
