/*
	Copyright (C) 2003-2005 Daniel Muller, dan at verliba dot cz
	Copyright (C) 2006-2015 Verlihub Team, info at verlihub dot net

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

#ifndef NDIRECTCONNECTINFOSERVER_H
#define NDIRECTCONNECTINFOSERVER_H
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "cconfigbase.h"
#include <ostream>

using std::ostream;

namespace nVerliHub {
	namespace nSocket {
		class cServerDC;
	};

/**
info about the server
@author Daniel Muller
*/

class cInfoServer: public nConfig::cConfigBase
{
	public:
		cInfoServer();
		~cInfoServer();
		void PortInfo(ostream &os);
		void SystemInfo(ostream &os);
		void SetServer(nSocket::cServerDC *Server);
		void Output(ostream &os, int Class);

		virtual int Save() {
			return 0;
		}

		virtual int Load() {
			return 0;
		}
	private:
		nSocket::cServerDC *mServer;

		// port information
		struct sPortInfoItem
		{
			int mPort;
			int mCount;
		};

		typedef vector<sPortInfoItem*> tPortInfoList;
		tPortInfoList mPortInfoList;

		/*
		bool PortInfoSort(const sPortInfoItem &left_item, const sPortInfoItem &right_item) // todo: not compiling
		{
			return (left_item.mCount < right_item.mCount);
		}
		*/
};

}; // namespace nVerliHub

#endif
