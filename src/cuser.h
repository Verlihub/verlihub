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

#ifndef CUSER_H
#define CUSER_H

#include <string>
#include "cobj.h"
#include "cconndc.h"
#include "cfreqlimiter.h"
#include "cpenaltylist.h"
#include "ctime.h"

using namespace std;

namespace nVerliHub {
	namespace nEnums {

		/** several types of users with some differences .. for later
			everyone is allowed to create no more then he is -1
		*/
		enum tUserCl
		{
			eUC_PINGER = -1, /// pinger user
			eUC_NORMUSER = 0, ///< Regular user
			eUC_REGUSER = 1, ///< Registered user
			eUC_VIPUSER = 2, ///< VIP user
			eUC_OPERATOR = 3, ///< Operator
			eUC_CHEEF = 4, ///< Cheef operator
			eUC_ADMIN = 5,///< Hub Admin
			eUC_MASTER = 10 ///< Hub master, creates aminds, etc...
		};

		/** user rights, there will be defaults for every class, but they can be changed */
		typedef enum
		{
			//eUR_NOINFO  = 0x000001, //< can login without user info
			eUR_NOSHARE = /*0x000002*/0x000001, //< can login with less than share limit
			eUR_CHAT 	= /*0x000004*/0x000002, //<  can talk in the main chat
			eUR_SEARCH  = /*0x000008*/0x000004, //< can search
			//eUR_STPM    = 0x000010, //< stealth PM (with other nick, that doesn't exist or not registered)
			eUR_OPCHAT	= /*0x000020*/0x000008, //< can opchat
			/*
			eUR_REDIR   = 0x000040, //< can op force move to a selected hublist
			eUR_REDIRANY= 0x000080, //< can op force move to any hub
			*/
			eUR_KICK    = /*0x000100*/0x000010, //< can kick (with a previous chat message)
			eUR_DROP    = /*0x000200*/0x000020, //< can drop users (without the chat message)
			eUR_TBAN  	= /*0x000400*/0x000040, //< can use tban up to a configurable limit
			eUR_PBAN 	= /*0x000800*/0x000080, //< can ban longer than the tban limit
			/*
			eUR_GETIP  	= 0x001000, //< get user's ip
			eUR_FULL1   = 0x002000, //< connection on almost full hub
			eUR_FULL2   = 0x004000, //< connection on completely full hub (someone is doconnected)
			eUR_MASSMSG = 0x008000, //<
			eUR_MASSRED = 0x010000, //< masss redirect
			eUR_S_MAXU  = 0x020000, //< set max users
			eUR_S_MINS  = 0x040000, //< set minshare
			eUR_S_HUBN  = 0x080000, //< set hubname
			eUR_S_REDI	= 0x100000,  //< set redirhub(s) etc...
			*/
			eUR_CTM     = /*0x200000*/0x000100,  // start download
			eUR_PM      = /*0x400000*/0x000200,   // private messages
			eUR_REG     = /*0x800000*/0x000400 //< can create or edit registered users (lowr classes)
		} tUserRights;

		typedef enum
		{
			eFH_SEARCH,
			eFH_CHAT,
			eFH_PM,
			eFH_MCTO,
			eFH_LAST_FH
		} tFloodHashes;

		typedef enum
		{
			eFC_PM,
			eFC_MCTO,
			eFC_LAST_FC
		} tFloodCounters;

		typedef enum // myinfo flags
		{
			eMF_NORM = 0x01,
			eMF_AWAY = 0x02,
			eMF_SERV = 0x04,
			eMF_FIRE = 0x08,
			eMF_TLS = 0x10, // tls support
			eMF_NAT = 0x20 // nat support
		} tMyFlags;
	};

using namespace nTables;

/** I should define for each class of users a mask of rights that they can't get ad of
 * those that they always get.. this should be configurable 2DO TODO*/

//class cConnDC;
	namespace nSocket {
		class cServerDC;
	};

/*
	basic class for users, every users must have at least this info
	@author Daniel Muller
*/

class cUserBase: public cObj
{
public:
	cUserBase(const string &nick = "");

	virtual ~cUserBase()
	{}

	virtual bool CanSend();
	virtual bool HasFeature(unsigned feature);
	virtual bool GetMyFlag(unsigned short flag) const { return false; }
	virtual void Send(string &data, bool pipe, bool flush = true);
public:
	string mNick;
	string mMyINFO; // real myinfo used only for comparing
	string mFakeMyINFO; // we send only this modified myinfo

	// store user nick hash and use it as much as possible instead of nick
	typedef unsigned long tHashType;
	tHashType mNickHash;

	// users class
	nEnums::tUserCl mClass;
	// if the user was added to the list
	bool mInList;
};

namespace nProtocol {
	class cMessageDC;
};

using nProtocol::cMessageDC;

/**Any type of dc user, contains info abou the connected users
  *@author Daniel Muller
  */
class cUser: public cUserBase
{
public:
	cUser(const string &nick = "");

	virtual ~cUser()
	{}

	virtual bool CanSend();
	virtual bool HasFeature(unsigned feature);
	virtual void Send(string &data, bool pipe, bool flush = true);

	// client flag in myinfo
	bool GetMyFlag(unsigned short flag) const { return (mMyFlag & flag) == flag; }
	void SetMyFlag(unsigned short flag) { mMyFlag |= flag; }
	void UnsetMyFlag(unsigned short flag) { mMyFlag &= ~flag; }

	/** check for the right to ... */
	inline int HaveRightTo(unsigned int mask){ return mRights & mask; }
	/** return tru if user needs a password and the password is correct */
	bool CheckPwd(const string &pwd);
	/** perform a registration: set class, rights etc... precondition: password was al right */
	void Register();

	public:
	/* Pointer to the connection */
	nSocket::cConnDC *mxConn;
	/* Pointer to the srever (this pointer must never be deleted) */
	nSocket::cServerDC *mxServer;

	unsigned short mMyFlag; // status flag in myinfo
	bool mPassive; // user is in passive mode
	bool mLan; // user has lan ip

	/* Rights of the user */
	unsigned long mRights;

	struct sTimes // different time stamps
	{
		// connection time
		cTime connect;
		// login time, when the user is pushed in userlist
		cTime login;
		// last search time
		cTime search;
		// time when myinfo was sent
		cTime info;
		// last chat message time
		cTime chat;
		// last getnicklist time
		cTime nicklist;
		// last private message time to any user
		cTime pm;
		cTime msg_ctm; // last use hub message times
		cTime msg_search;

		sTimes():
			//connect(0l),
			login(0l),
			search(0l),
			info(0l),
			chat(0l),
			nicklist(0l),
			//pm(0l),
			msg_ctm(0l),
			msg_search(0l)
		{}
	};

	sTimes mT;
	typedef tHashArray<void*>::tHashType tFloodHashType;
	tFloodHashType mFloodHashes[nEnums::eFH_LAST_FH];
 	unsigned int mFloodCounters[nEnums::eFC_LAST_FC];

	// rctm data
	unsigned int mRCTMCount;
	cTime mRCTMTime;
	bool mRCTMLock;

	/** 0 means perm ban, otherwiese in seconds */
	//long mBanTime;
	/** indicates whether user is to ban after the following kick */
	//bool mToBan;
	/** minimal class users that can see this one */
	//nEnums::tUserCl mVisibleClassMin;
	/** minimal class users that can see this one as operator */
	//nEnums::tUserCl mOpClassMin;
	/** User share */
	unsigned __int64 mShare;
	/** chat discrimination */
	long mGag;
	long mNoPM;
	long mNoSearch;
	long mNoCTM;
	long mCanKick;
	long mCanDrop;
	long mCanTBan;
	long mCanPBan;
	long mCanShare0;
	long mCanReg;
	long mCanOpchat;

	/** kick messages hide from chat */
	bool mHideKick;
	/** hide share **/
	bool mHideShare;
	// hide chat
	bool mHideChat;
	// hide no ctm messages
	bool mHideCtmMsg;
	// set password request flag
	bool mSetPass;
	/** class protection against kicking */
	int mProtectFrom;
	/* Numeber of searches */
	unsigned int mSearchNumber;
	/* The class over which the users are able to see kick messages*/
	int mHideKicksForClass;

	// last extjson
	string mExtJSON;
	// language
	//string mLang;

	public:
	//long ShareEnthropy(const string &sharesize);
	void DisplayInfo(ostream &os);
	void DisplayRightsInfo(ostream &os, bool head = false);

	/*!
		\fn Can(unsigned Right, long now = 0, int OtherClass = 0)
		return true if the user has given rights
	*/
	bool Can(unsigned Right, long now = 0, int OtherClass = 0);
	void SetRight(unsigned Right, long until, bool allow = false, bool notify = false);
	void ApplyRights(cPenaltyList::sPenalty &pen);
};

class cUserRobot: public cUser
{
public:
	cUserRobot(nSocket::cServerDC *serv = NULL)
	{
		mxServer = serv;
	}

	virtual ~cUserRobot()
	{}

	cUserRobot(const string &nick, nSocket::cServerDC *serv = NULL):
		cUser(nick)
	{
		mxServer = serv;
	}

	virtual bool ReceiveMsg(nSocket::cConnDC *conn, cMessageDC *msg) = 0;
	bool SendPMTo(nSocket::cConnDC *conn, const string &msg);
};

class cUserCollection;
class cChatConsole;

class cChatRoom : public cUserRobot
{
public:
	cChatRoom(const string &nick, cUserCollection *col, nSocket::cServerDC *server = NULL);
	virtual ~cChatRoom();
	cUserCollection *mCol;
	virtual bool ReceiveMsg(cConnDC *conn, nProtocol::cMessageDC *msg);
	virtual void SendPMToAll(const string &data, nSocket::cConnDC *conn, bool fromplug = false, bool skipself = true);
	virtual bool IsUserAllowed(cUser *);
	cChatConsole *mConsole;
};

class cOpChat: public cChatRoom
{
	public:
		cOpChat(const string &nick, nSocket::cServerDC *server);
		virtual bool IsUserAllowed(cUser *user);
};

class cMainRobot: public cUserRobot
{
public:
	cMainRobot(const string &nick, nSocket::cServerDC *server = NULL):cUserRobot(nick,server){};
	virtual bool ReceiveMsg(nSocket::cConnDC *conn, nProtocol::cMessageDC *msg);
};

namespace nPlugin {
	class cVHPlugin;
};

class cPluginRobot : public cUserRobot
{
public:
	cPluginRobot(const string &nick, nPlugin::cVHPlugin *pi, nSocket::cServerDC *server = NULL);
	nPlugin::cVHPlugin *mPlugin;
	virtual bool ReceiveMsg(nSocket::cConnDC *conn, nProtocol::cMessageDC *msg);
};

}; // namespace nVerliHub

#endif
