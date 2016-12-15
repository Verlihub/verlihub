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

#ifndef CMGSLIST_H
#define CMGSLIST_H

#include <string>
#include "src/cconfmysql.h"
#include "src/tcache.h"

using namespace std;
namespace nVerliHub {
	namespace nSocket {
		class cServerDC;
	};
	class cUser;
	namespace nMessangerPlugin {

struct sMessage
{
	string mSender;
	string mSenderIP;
	string mReceiver;
	time_t mDateSent;
	time_t mDateExpires;
	string mSubject;
	string mBody;

	sMessage() { mDateSent = mDateExpires = 0; mPrintType = AS_SUBJECT; };
	// output stuff
	mutable enum { AS_SUBJECT, AS_BODY, AS_DELIVERY, AS_ONLINE } mPrintType;
	sMessage &AsSubj() { mPrintType = AS_SUBJECT; return *this; }
	sMessage &AsOnline() { mPrintType = AS_ONLINE ; return *this; }
	sMessage &AsBody() { mPrintType = AS_BODY; return *this; }
	sMessage &AsDelivery() { mPrintType = AS_DELIVERY; return *this; }
	friend ostream & operator << (ostream &os, sMessage &Msg);
};

/**
@author Daniel Muller
*/
class cMsgList : public nConfig::cConfMySQL
{
public:
	cMsgList(nSocket::cServerDC *server);
	virtual ~cMsgList();
	virtual void CleanUp();
	virtual void AddFields();

	int CountMessages(const string &nick, bool IsSender);
	bool AddMessage(sMessage &msg );
	int PrintSubjects(ostream &os, const string &nick, bool IsSender);
	int DeliverMessagesForUser(cUser *dest);
	int DeliverMessagesSinceSync(unsigned sync);
	int DeliverModelToUser(cUser *dest);

	void DeliverOnline(cUser *dest, sMessage &msg);
	int SendAllTo(cUser *, bool IsSender);
	void UpdateCache();

	nConfig::tCache<string> mCache;
	sMessage mModel;
	nSocket::cServerDC *mServer;
};
	}; // namespace nMessangerPlugin
}; // namespace nVerliHub
#endif
