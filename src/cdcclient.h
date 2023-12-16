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

#ifndef NDIRECTCONNECTCLIENT_H
#define NDIRECTCONNECTCLIENT_H
#include <sstream>
#include <string>

using namespace std;

namespace nVerliHub {
	namespace nSocket {
		class cConnDC;
	};

	namespace nTables {

		/**
		* This class represents a client with a TAG
		*
		*  @author Simoncelli Davide
		*  @version 1.0
		*/

		class cDCClient
		{
			public:
				/**
				* Class constructor
				*/
				cDCClient();

				/**
				* Class destructor
				*/
				virtual ~cDCClient();

				/**
				* This function is called when cDCClient object is created. Here it is not useful so the body is empty
				*/
				virtual void OnLoad() {};

				/**
				* Redefine << operator to print a client and show its description
				    * @param os The stream where to store the description.
				    * @param tr The cDCClient object that describes the redirect
				    * @return The stream
				*/
				friend ostream &operator << (ostream &, cDCClient &);

				// Name of the client
				string mName;

				// Unique string to identify the client
				string mTagID;

				// Min version allowed
				double mMinVersion;
				// Max version allowed
				double mMaxVersion;
				// minimum version to use hub
				double mMinVerUse;

				// Client is banned from the hub
				bool mBan;

				// Enable or disable a client
				bool mEnable;
		};

	}; // namespace nTables
}; // namespace nVerliHub

#endif
