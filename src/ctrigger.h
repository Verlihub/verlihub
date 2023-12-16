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

#ifndef NDIRECTCONNECTCTRIGGER_H
#define NDIRECTCONNECTCTRIGGER_H

#include <sstream>
#include <string>

using namespace std;

namespace nVerliHub {
	namespace nSocket {
		class cConnDC;
		class cServerDC;
	};

	namespace nEnums {
		/**
		 A *llowed flags and their meanings for a trigger
		 */
		enum
		{
			eTF_EXECUTE = 1 << 0, // Execute the content of the trigger message in as a shell command, todo: get rid of this flag, but we need to replace it or something, else current trigger flags get messed
			eTF_SENDPM = 1 << 1, // Send trigger message as private message
			eTF_MOTD = 1 << 2, // Trigger when an user logs in
			eTF_HELP = 1 << 3, // Trigger on +help or !help command
			eTF_DB = 1 << 4, // Trigger message is stored in DB (def column)
			eTF_VARS = 1 << 5, // Replace variables
			eTF_SENDTOALL = 1 << 6 // Send trigger message to all when triggered
		};
	};
	namespace nTables {

/**
Trigger class
User is able to define trigger string that show a message in mainchat or as private message

@author Daniel Muller
@author Simoncelli Davide
*/

class cTrigger
{
public:
	cTrigger();
	virtual ~cTrigger();
	int DoIt(istringstream &cmd_line, nSocket::cConnDC *conn, nSocket::cServerDC &server);
	/**
	 The trigger
	*/
	string mCommand;
	/**
	 The nick that sends the trigger message
	*/
	string mSendAs;

	/**
	 Timeout for trigger if there is an actived timer for it
	*/
	long mSeconds;
	long mLastTrigger;
	/**
	 The trigger flags that describe when and how to trigger it
	*/
	int mFlags;
	/**
	 The trigger message that can be a file name or a text
	*/
	string mDefinition;
	/**
	 Trigger description
	*/
	string mDescription;
	/**
	 Min class that can trigger
	*/
	int mMinClass;
	/**
	 Max class that can trigger
	*/
	int mMaxClass;
	/**
	 Max lines for definition (0 = unlimited)
	*/
	//int mMaxLines;
	/**
	 Max length for definition (0 = unlimited)
	*/
	//int mMaxSize;
	/**
	 Min delay before triggering for users
	*/
	//int mDelayUser;
	/**
	 Min overall delay in seconds
	*/
	//int mDelayTotal;

	virtual void OnLoad();
	friend ostream &operator << (ostream &, cTrigger &);
};
};
};

#endif
