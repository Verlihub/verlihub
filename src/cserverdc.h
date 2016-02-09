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

#ifndef CSERVERDC_H
#define CSERVERDC_H
#include "casyncsocketserver.h"
#include "cmysql.h"
#if defined _WIN32
#include <winsock2.h>
#endif
#include <fstream>
#include "cuser.h"
#include "cmessagedc.h"
#include "cconfigbase.h"
#include "cdcconf.h"
#include "cdbconf.h"
#include "cdcproto.h"
#include "csetuplist.h"
#include "ctempfunctionbase.h"
#ifdef HAVE_LIBGEOIP
#include "cgeoip.h"
#endif
#include "cusercollection.h"
#include "cvhpluginmgr.h"
#include "cmeanfrequency.h"
#include "cworkerthread.h"
#include "czlib.h"
#include "cconndc.h"

#define USER_ZONES 6

using namespace std;

namespace nVerliHub {
#ifdef HAVE_LIBGEOIP
	using nUtils::cGeoIP;
#endif
	using namespace nPlugin;
	using namespace nThread;
	using namespace nSocket;

	using nMySQL::cMySQL;
	namespace nEnums {

		typedef enum
		{
			eVN_OK,
			eVN_CHARS,
			eVN_SHORT,
			eVN_LONG,
			eVN_USED,
			eVN_BANNED,
			eVN_PREFIX,
			eVN_NOT_REGED_OP,
			//eVN_RESERVED
		} tVAL_NICK;

		typedef enum
		{
			eVI_OK,
			eVI_UNKNOWN,
			eVI_PRIVATE,
			eVI_BAN,
			eVI_T_BAN,
			eVI_BAN_RANGE,
			eVI_BAN_HOST
		} tVAL_IP;

		typedef enum
		{
			eMA_PROCEED,
			eMA_LIMITED,
			eMA_LATER,
			eMA_WARNING,
			eMA_IGNORE,
			eMA_HANGUP,
			eMA_HANGUP1,
			eMA_TBAN,
			eMA_ERROR

		} tMsgAct;

		typedef enum
		{
			eSL_NORMAL,		// normal mode
			eSL_PROGRESSIVE,	// nearing capacity
			eSL_CAPACITY,		// resource limits reached
			eSL_RECOVERY,		// refusing new actions while we try to recover
			eSL_SYSTEM_DOWN		// this actually never happens ;o) errrm yes it does its called a lockup!
		} tSysLoad;

		enum
		{
			eULO_NONE = 0,
			eULO_GETINFO = 1 // optimize_userlist = true
		};

		enum
		{
			eCR_DEFAULT = 0,  // default value, means not closed or unknown reason
			eCR_INVALID_USER, // bad nick, banned nick, ip or whatever
			eCR_KICKED, // user was kicked
			eCR_FORCEMOVE, // operator redirect command
			eCR_QUIT, // user quits himself
			eCR_HUB_LOAD, // critical hub load, no new users accepted
			eCR_TIMEOUT, // some kind of timeout
			eCR_TO_ANYACTION, // user did nothing for too long time
			eCR_USERLIMIT, // user limit exceeded for this user
			eCR_SHARE_LIMIT, // min or max share limit
			eCR_TAG_NONE, // no tags in description, or badly parsed
			eCR_TAG_INVALID, // tags not validated, slots, hubs, limiter, version, etc
			eCR_PASSWORD, // wrong password
			eCR_LOGIN_ERR, // error in login sequence
			eCR_SYNTAX, // syntax error in some message
			eCR_INVALID_KEY, // lock2key is invalid
			eCR_RECONNECT, // too fast reconnect
			eCR_CLONE, // clone detected
			eCR_SELF, // same user connects twice
			eCR_BADNICK, // bad nick, already used, too short, etc
			eCR_NOREDIR // do not redirect, special reason
		};

		enum
		{
			eKI_CLOSE = 1,
			eKI_WHY = 2,
			eKI_PM = 4,
			eKI_BAN = 8,
			eKI_DROP = 16
		};

		typedef enum
		{
			ePFA_CHAT,
			ePFA_PRIV,
			ePFA_MCTO,
			ePFA_LAST
		} tProtoFloodAllItems;
	}; // namespace nEnums

	namespace nTables{
		class cConnTypes;
		class cBanList;
		class cUnBanList;
		class cPenaltyList;
		class cRegList;
		class cKickList;
		class cDCConf;
	};

	namespace nSocket {
		class cConnDC;
		class cDCConnFactory;
	};

	//using namespace nConfig;
	using namespace nUtils;
	using namespace nProtocol;
	//using namespace ::nTables;
	//using namespace ::nPlugin;
	using nTables::cDCConf;

	class cUser;
	class cUserRobot;
	class cChatRoom;
	class cDCConsole;

	namespace nSocket {

/**A Direct Connect Verlihub server
  *@author Daniel Muller
  * the one ;)
  */
class cServerDC : public cAsyncSocketServer
{
	friend class nSocket::cConnDC;
	friend class nSocket::cDCConnFactory;
	friend class nVerliHub::cDCConsole;
	friend class nProtocol::cDCProto;
	friend class nVerliHub::cDCConf;
	friend class nTables::cRegList;
	friend class nTables::cDCBanList;
	friend class nVerliHub::cUser;
	public:
		// Path to VerliHub config folder
		string mConfigBaseDir;
		// Database configuration
		cDBConf mDBConf;
		// MySQL database connection
		cMySQL mMySQL;
		// VerliHub configuration
		cDCConf mC;
		// Setup loader
		cSetupList mSetupList;
		// Protocol message handler
		cDCProto mP;
		// Console managment
		class cDCConsole *mCo;
		// User registration and reglist handler
		class cRegList *mR;
		// Penalities and temp rights handler
		cPenaltyList *mPenList;
		// Banlist
		cBanList *mBanList;
		// Unbanlist
		cUnBanList *mUnBanList;
		// Kick list
		cKickList *mKickList;
		// OpChat room
		cChatRoom *mOpChat;
		// Connection types handler
		cConnTypes *mConnTypes;
		// ZLib compression class
		cZLib *mZLib;
		// Process name
		string mExecPath;
		string mBadNickNmdcChars, mBadNickOwnChars; // forbidden nick characters

		/**
		* Base class constructor.
		* @param CfgBase Path to VerliHub configuration folder.
		* @param ExecPath Process name.
		*/
		cServerDC(string CfgBase = string("./.verlihub"), const string &ExecPath= "");

		/**
		* Class destructor.
		*/
		virtual ~cServerDC();

		/**
		* Add a robot to the robot and user lists.
		*
		* User is automatically added to other lists like passive, active or op list.
		* Robot user is also able to receive hub events that has been registered and manage them.
		* @see AddToList()
		* @param usr The robot to add.
		* @return True if robot is added or false otherwise.
		*/
		bool AddRobot(cUserRobot *robot);

		/**
		* Add an user to userlist.
		*
		* User is automatically added to other lists like passive, active or op list.
		* @param usr The user to add.
		* @return True if user is added or false otherwise.
		*/
		bool AddToList(cUser *usr);

		/*
			This method is a forwarder for ScriptAPI::OnScriptCommand
		*/
		void OnScriptCommand(string *cmd, string *data, string *plug, string *script);

		/*
			This method is a forwarder for ScriptAPI::OnScriptQuery
		*/
		void OnScriptQuery(string *cmd, string *data, string *recipient, string *sender, ScriptResponses *responses);

		/*
			This method is a forwarder for ScriptAPI::OnOpChatMessage
		*/
		bool OnOpChatMessage(string *nick, string *data);

		/*
			This method is a forwarder for ScriptAPI::OnPublicBotMessage
		*/
		bool OnPublicBotMessage(string *nick, string *data, int min_class, int max_class);

		/*
			This method is a forwarder for ScriptAPI::OnUnLoad
		*/
		bool OnUnLoad(long code);

		/**
		* This method tells the server what the server can receive and what actions to perform depending on hub health.
		* It is a kind of message filter.
		* Note that:
		* If user is not in the userlist, he can send key, validate nick, mypass, version, getnicklist and myinfo.
		* If user is in the userlist, he can do everything except sending key, validate nick, mypass and version.
		* If hub health is critical, user may be banned or just disconnected.
		* @param msg Type of received protocol message.
		* @param conn The user connection.
		* @return The action to do or what the user can send.
		*/
		tMsgAct Filter(nEnums::tDCMsg msg, cConnDC * conn);

		/**
		* This method is called every period.
		* It flushes messages in queue, updates hub health (aka frequency),
		* updates bandwidth usage stats, clean temp ban, run hublist registration
		* and reload hub configuration.
		* @param now Current time.
		* @return False if callbacks fails or true otherwise.
		*/
		virtual int OnTimer(cTime &now);

		/**
		* Start the socket and listen on ports.
		* @param OverrideDefaultPort The port to override.
		* @return The result of the socket operation.
		*/
		virtual int StartListening(int OverrideDefaultPort=0);

		/*
			kick user and perform different actions based of flags
		*/
		void DCKickNick(ostream *use_os, cUser *op, const string &nick, const string &why, int flags);

		/*
			Send a private message to user from other user or hub security.
			text - The message to send.
			conn - The user connection.
			from - The sender nick, if it is not specified hub security nick is used.
			nick - The message nick, if it is not specified hub security nick is used.
			return - Number greater than zero if message is sent.
		*/
		int DCPrivateHS(const string &text, cConnDC *conn, string *from = NULL, string *nick = NULL);

		/**
		* Send a message in mainchat for the given connection.
		* @param from Sender nickname.
		* @param msg The message to send.
		* @param conn The connection of the recipient.
		* @return Zero if connection is not valid or one on success.
		*/
		int DCPublic(const string &from, const string &msg,class cConnDC *conn);

		/**
		* Send a message in mainchat for the given connection as hub security.
		*
		* This is the same of calling cServerDC::DCPublic(mC.hub_security, msg, conn);
		* @param msg The message to send.
		* @param conn The connection of the recipient.
		* @return Zero if connection is not valid or one on success.
		*/
		int DCPublicHS(const string &text, cConnDC *conn);

		/**
		* Send a public message to all users as hub security.
		* @param text  The message to send
		*/
		void DCPublicHSToAll(const string &text, bool delay = true);

		/**
		* Send a message in mainchat to everyone.
		*
		* This methos also allows to restrict the recipients on their classes.
		*
		* @param from Sender nickname.
		* @param msg The message to send.
		* @param min_class Minimum class (default to 0).
		* @param max_class Maximum class (default to 10).
		* @return Always one.
		*/
		int DCPublicToAll(const string &from, const string &txt, int min_class = 0, int max_class = 10, bool delay = true);

		/**
		* Remove a robot from lists.
		* @see RemoveNick()
		* @param usr The robot to remove.
		* @return True if the robot is removed or false otherwise.
		*/
		bool DelRobot(cUserRobot *robot);

		/**
		* Register the hub to the given hublist.
		* @param host The address of hublist.
		* @param port The port.
		* @param NickForReply The nickname of the user to send the result.
		* @return Always one.
		*/
		int DoRegisterInHublist(string host, unsigned int port, string reply);

		/**
		* Register the hub to the given hublist.
		*
		* This method does the same thing of DoRegisterInHublist() but it is asynchronous.
		* @param host The address of hublist.
		* @param port The port.
		* @param conn The user connection to send the result.
		* @return One if the operation can be added and processed by thread or zero otherwise.
		*/
		int RegisterInHublist(string host, unsigned int port, cConnDC *conn);

		/**
		* Report an user to opchat.
		* @param conn User connection.
		* @param Msg The message to report.
		* @param ToMain Send report to mainchat.
		*/
		void ReportUserToOpchat(cConnDC *, const string &Msg, bool ToMain = false);

		/*
		* Send host headers to user.
		* conn = User connection.
		* where = Appearance destination, 1 - on login, 2 - on connection.
		*/
		void SendHeaders(cConnDC *, unsigned int where);

		/**
		* Remove an user from lists.
		* @param usr The user to remove.
		* @return True if the user is removed or false otherwise.
		*/
		bool RemoveNick(cUser *);

		/**
		* Save a file.
		*
		* Use %[CFG] variable in your path if you want to store the file in VerliHub config folder.
		* For example: %[CFG]/plugin.cnf.
		* @param file The filename.
		* @param text The content of the file.
		* @return One is the file has been saved or zero if the file cannot be created.
		*/
		int SaveFile(const string &file, const string &text);

		/**
		* Send data to all users that are in userlist.
		*
		* This methos also allows to restrict the recipients on their classes.
		* @param str The data to send.
		* @param cm Minimium class.
		* @param CM Maximum class.
		*/
		void SendToAll(string &str, int cm,int cM);

		/**
		* Send data to all users that are in userlist and belongs to the specified class range.
		*
		* Message to send is built in this way: start+nick+end.
		* This methos also allows to restrict the recipients on their classes.
		* @param start The data to send.
		* @param end The data to send.
		* @param cm Minimium class.
		* @param CM Maximum class.
		* @return The number of users that receives the message.
		*/
		int SendToAllWithNick(const string &start,const string &end, int cm,int cM);

		// replace user specific variables and send data to all users within specified class range
		int SendToAllWithNickVars(const string &start, const string &end, int cm, int cM);

		// replace user specific variables and send data to all users within specified class range
		int SendToAllNoNickVars(const string &msg, int cm, int cM);

		/**
		* Send data to all users that are in userlist and belongs to the specified class range and country.
		*
		* Message to send is built in this way: start+nick+end.
		* This methos also allows to restrict the recipients on their classes.
		* @param start The data to send.
		* @param end The data to send.
		* @param cm Minimium class.
		* @param CM Maximum class.
		* @param cc_zone Country code.
		* @return The number of users that receives the message.
		*/
		int SendToAllWithNickCC(const string &start,const string &end, int cm,int cM, const string &cc_zone);

		/*
			Replace user specific variables and send data to all users within specified class and country ranges.
		*/

		int SendToAllWithNickCCVars(const string &start, const string &end, int cm, int cM, const string &cc_zone);

		/*
			send conditional search request with filters to all users
				conn: sender connection
				data: search command
				passive: search mode flag
				tth: tth search flag
				return: send count
		*/
		unsigned int SearchToAll(cConnDC *conn, string &data, bool passive, bool tth);

		/*
			ExtJSON collector
		*/
		unsigned int CollectExtJSON(string &dest, cConnDC *conn);

		/**
		* Notify all users of a new user.
		*
		* The following operations are done in order:
		* 1. Send $Hello to hello users.
		* 2. Send MyInfo to all.
		* 3. Update OpList if the user is an operator.
		* @param user The new user.
		* @return Always true.
		*/
		bool ShowUserToAll(cUser *user);

		/*
			convert string to time in seconds
		*/
		unsigned int Str2Period(const string &s, ostream &err);

		/*
			Check if nick is valid, length, characters, prefix, ban, etc.
		*/
		tVAL_NICK ValidateNick(cConnDC *conn, const string &nick, string &more);

		/**
		* Check if the user is allowed to enter the hub.
		*
		* The following operations are done in order:
		* 1. Check if the nick is valid.
		* 2. Check if the user is banned or kicked.
		* 3. Verify if nickname  has a valid prefix.
		* If validation fails the connection is closed and not valid anymore.
		* @param conn User connection.
		* @param nick The nickname of the user.
		* @return Zero if an error occurs or one otherwise.
		*/
		int ValidateUser(cConnDC *conn, const string &nick, int &closeReason);

		/**
		* Return the list of the users that belongs to a country.
		* @param CC The country code.
		* @param dest The string where to store the users.
		* @param separator The seperator to use to split the user inside destination string.
		* @return The number of found users.
		*/
		unsigned int WhoCC(const string &cc, string &dest, const string &sep);

		// return list of users with specific city
		unsigned int WhoCity(const string &city, string &dest, const string &sep);

		// return list of users with specific hub port number
		unsigned int WhoHubPort(unsigned int port, string &dest, const string &sep);

		// return list of users with specific partial hub url
		unsigned int WhoHubURL(const string &url, string &dest, const string &sep);

		// return list of users with specific ip address range
		unsigned int WhoIP(unsigned long ip_min, unsigned long ip_max, string &dest, const string &sep, bool exact = true);

		// return number of users with class <= 1 with specific ip address
		unsigned int CntConnIP(const string &ip);

		// clone detection
		bool CheckUserClone(cConnDC *conn, string &clone);

		// protocol flood from all
		unsigned int mProtoFloodAllCounts[nEnums::ePFA_LAST];
		cTime mProtoFloodAllTimes[nEnums::ePFA_LAST];
		bool mProtoFloodAllLocks[nEnums::ePFA_LAST];
		bool CheckProtoFloodAll(cConnDC *conn, cMessageDC *msg, int type);

		// helper functions
		char* SysLoadName();
		char* UserClassName(nEnums::tUserCl ucl);
		bool CheckPortNumber(unsigned int port);
		string EraseNewLines(const string &src);

		// The buffer that holds data to send to all
		string mSendAllBuf;
		// Static pointer to this class
		static cServerDC *sCurrentServer;
		// System load indicator
		nEnums::tSysLoad mSysLoad;
		// Last op that used the broadcast function
		string LastBCNick;
		// String containing all myinfos in a row
		string mNickInfoString;
		// Network output log
		ofstream mNetOutLog;
		// Hublist registration thread

		#ifndef _WIN32 // todo: implement worker thread on windows
			cWorkerThread mHublistReg;
		#endif

		// traffic frequency for all zones
		cMeanFrequency<unsigned long, 10> mUploadZone[USER_ZONES + 1];
		cMeanFrequency<unsigned long, 10> mDownloadZone;

		// temporary functions
		typedef int tDC_MsgFunc(cMessageDC * msg, cConnDC * conn);
		typedef vector<cTempFunctionBase *> tTmpFunc;
		typedef tTmpFunc::iterator tTFIt;

		// List of temporary functions
		tTmpFunc mTmpFunc;

		typedef cUserCollection::tHashType tUserHash;
		// Userlist of all user
		cCompositeUserCollection mUserList;
		// Userlist of users not logged in yet
		cUserCollection mInProgresUsers;

		// Oplist
		cCompositeUserCollection mOpList;
		// List of users in opchat
		cUserCollection mOpchatList;
		// List of active users
		cUserCollection mActiveUsers;
		// List of passive users
		cUserCollection mPassiveUsers;
		// List of hello users
		cUserCollection mHelloUsers;
		// List of users allowed to talk
		cUserCollection mChatUsers;
		// List of bots
		cUserCollection mRobotList;

#ifdef HAVE_LIBGEOIP
		// GeoIp object for country code support
		static cGeoIP sGeoIP;
#endif

		// prevent stack trace on core dump
		static bool mStackTrace;

		// stack trace
		void DoStackTrace();

		// ctm2hub
		struct sCtmToHubItem
		{
			//string mNick; // todo: make use of these
			//string mIP;
			//string mCC;
			//int mPort;
			//int mServ;
			string mRef;
			bool mUniq;
		};

		typedef vector<sCtmToHubItem*> tCtmToHubList;
		tCtmToHubList mCtmToHubList;

		struct sCtmToHubConf
		{
			cTime mTime;
			cTime mLast;
			bool mStart;
			unsigned long mNew;
		};

		sCtmToHubConf mCtmToHubConf;

		void CtmToHubAddItem(cConnDC *conn, const string &ref);
		int CtmToHubRefererList(string &list);
		void CtmToHubClearList();

		// Reloads configuration and all caches.
		void Reload();

protected: // Protected methods
	/**
	* This method is called when user is logged in.
	*
	* The following operations are done in order:
	* 1. Send MOTD trigger.
	* 2. Verify if the user has to change his password.
	* 3. Send hub topic, information about himself and welcome message if any is set.
	* @see DoUserLogin(), BeginUserLogin()
	* @param conn User connection.
	*/
	void AfterUserLogin(cConnDC *conn);

	/**
	* Return if the server is able to accept a new incoming connection.
	* @return True if connection is accepted or false otherwise.
	*/
	bool AllowNewConn();

	/**
	* This method is called before the user is logged in.
	*
	* This method performs few checks on the user before adding him to userlist.
	* It also checks for duplicated nick and establish when to send nicklist (before or after login).
	* @param conn User connection.
	* @return Return true if the user can login and connection is allowed; otherwise return false.
	*/
	bool BeginUserLogin(cConnDC *conn);

	/**
	* Close a connection and send a message with the reason of why connection has been closed.
	* It is also possible to specify a timeout in micro-second to wait before disconnecting
	* the user and a reason of disconnect that is used to determinate which redirect address to use.
	* @param conn User connection.
	* @param msg The reason to send to the user.
	* @param to_msec Micro-seconds to wait before disconnecting (default 4000).
	* @param Reason The reason used for redirect.
	*/
	void ConnCloseMsg(cConnDC *conn, const string &msg, int to_msec=4000, int Reason = eCR_DEFAULT);

	/**
	* Send hello message to the user. Example: $Hello foobar.
	* Extra information can be sent after hello message by passing
	* a pointer to a string containing the message.
	* @param nick The nickname of the user.
	* @param conn User connection.
	* @param info Extra information.
	*/
	int DCHello(const string & nick, cConnDC * conn, string *info=NULL);

	/**
	* This methos is called when user is going to be added to userlist.
	*
	* This method is called after BeginUserLogin().
	*
	* The following operations are done in order:
	* 1. Verify unique nick again because the user is not added to userlist yet.
	* 2. Apply user rights
	* 3. Notify all users of new user
	* It also checks for duplicated nick and establish when to send nicklist (before or after login).
	* @see BeginUserLogin(), AfterUserLogin()
	* @param conn User connection.
	*/
	void DoUserLogin(cConnDC *conn);

	/** create somehow a string to get line for given connection, ad return th pointer */
	/**
	*
	* @param conn User connection
	* @return Allocated string.
	*/
	virtual string * FactoryString(cAsyncConn * );

	/**
	* Check if timeout if expired (seconds resolution).
	* @param what cTime object to check.
	* @param int Timeout in seconds.
	* @return True if timer is expired or false otherwise.
	*/
	bool MinDelay(cTime &then, unsigned int min, bool update = false);

	/**
	* Check if timeout if expired (milli-seconds resolution).
	* @param what cTime object to check.
	* @param int Timeout in seconds.
	* @return True if timer is expired or false otherwise.
	*/
	bool MinDelayMS(cTime &then, unsigned long min, bool update = false);

	/**
	* This method is triggered when there is a new incoming connection.
	*
	* Send $Lock message and check if connection should be allowed
	* depending on hub health.
	* @param conn User connection.
	* @return A negative number if connection is refused or zero otherwise.
	*/
	int OnNewConn(cAsyncConn *);

	/**
	* This method is triggered when there is a new incoming message.
	*
	* Treat a message and pass it to the protocol parser.
	* @param conn User connection.
	* @param msg The message.
	*/
	void OnNewMessage(cAsyncConn * , string * );

	/**
	* Check if another user with same nick is already logged in.
	*
	* Eventually decide if old connection (the one of the user logged in)
	* shold be closed or not
	* @param conn User connection.
	* @return Return true if the user can login and connection is allowed; otherwise return false.
	*/
	bool VerifyUniqueNick(cConnDC *conn);

public:

	// protocol message counters
	unsigned int mProtoCount[nEnums::eDC_UNKNOWN + 2]; // last is ping

	// protocol total download = 0 and upload = 1
	unsigned __int64 mProtoTotal[2];

	// Usercount of zones (CC and IP-range zones)
	unsigned int mUserCount[USER_ZONES + 1];
	// Total number of users
	unsigned int mUserCountTot;
	// User peak
	unsigned int mUsersPeak;
	// Total share of the hub
	unsigned __int64 mTotalShare;
	// peak total share
	unsigned __int64 mTotalSharePeak;
	// cTime object when the hub was started
	cTime mStartTime;
	// Timer that deletes temp bans
	cTimeOut mSlowTimer;
	// Timer that periodically registers the hub to hublists
	cTimeOut mHublistTimer;
	// Timer that reloads hub configuration
	cTimeOut mReloadcfgTimer;
	// Plugin manager
	cVHPluginMgr mPluginManager;

private:

	struct sCallBacks
	{
		sCallBacks(cVHPluginMgr *mgr):
			mOnNewConn(mgr, "VH_OnNewConn", &cVHPlugin::OnNewConn),
			mOnCloseConn(mgr, "VH_OnCloseConn", &cVHPlugin::OnCloseConn),
			mOnCloseConnEx(mgr, "VH_OnCloseConnEx", &cVHPlugin::OnCloseConnEx),
			mOnUnknownMsg(mgr, "VH_OnUnknownMsg", &cVHPlugin::OnUnknownMsg),
			//mOnUnparsedMsg(mgr, "VH_OnUnparsedMsg", &cVHPlugin::OnUnparsedMsg),
			mOnParsedMsgSupports(mgr, "VH_OnParsedMsgSupports", &cVHPlugin::OnParsedMsgSupports),
			mOnParsedMsgMyHubURL(mgr, "VH_OnParsedMsgMyHubURL", &cVHPlugin::OnParsedMsgMyHubURL),
			mOnParsedMsgExtJSON(mgr, "VH_OnParsedMsgExtJSON", &cVHPlugin::OnParsedMsgExtJSON),
			mOnParsedMsgBotINFO(mgr, "VH_OnParsedMsgBotINFO", &cVHPlugin::OnParsedMsgBotINFO),
			mOnParsedMsgVersion(mgr, "VH_OnParsedMsgVersion", &cVHPlugin::OnParsedMsgVersion),
			mOnParsedMsgMyPass(mgr, "VH_OnParsedMsgMyPass", &cVHPlugin::OnParsedMsgMyPass),
			mOnParsedMsgAny(mgr, "VH_OnParsedMsgAny", &cVHPlugin::OnParsedMsgAny),
			mOnParsedMsgAnyEx(mgr, "VH_OnParsedMsgAnyEx", &cVHPlugin::OnParsedMsgAnyEx),
			mOnParsedMsgPM(mgr, "VH_OnParsedMsgPM", &cVHPlugin::OnParsedMsgPM),
			mOnParsedMsgMCTo(mgr, "VH_OnParsedMsgMCTo", &cVHPlugin::OnParsedMsgMCTo),
			mOnParsedMsgChat(mgr, "VH_OnParsedMsgChat", &cVHPlugin::OnParsedMsgChat),
			mOnParsedMsgSearch(mgr, "VH_OnParsedMsgSearch", &cVHPlugin::OnParsedMsgSearch),
			mOnParsedMsgMyINFO(mgr, "VH_OnParsedMsgMyINFO", &cVHPlugin::OnParsedMsgMyINFO),
			mOnFirstMyINFO(mgr, "VH_OnFirstMyINFO", &cVHPlugin::OnFirstMyINFO),
			mOnParsedMsgValidateNick(mgr, "VH_OnParsedMsgValidateNick", &cVHPlugin::OnParsedMsgValidateNick),
			mOnParsedMsgConnectToMe(mgr, "VH_OnParsedMsgConnectToMe", &cVHPlugin::OnParsedMsgConnectToMe),
			mOnParsedMsgRevConnectToMe(mgr, "VH_OnParsedMsgRevConnectToMe", &cVHPlugin::OnParsedMsgRevConnectToMe),
			mOnOperatorCommand(mgr, "VH_OnOperatorCommand", &cVHPlugin::OnOperatorCommand),
			mOnUserCommand(mgr, "VH_OnUserCommand", &cVHPlugin::OnUserCommand),
			mOnHubCommand(mgr, "VH_OnHubCommand", &cVHPlugin::OnHubCommand),
			mOnParsedMsgSR(mgr, "VH_OnParsedMsgSR", &cVHPlugin::OnParsedMsgSR),
			mOnOperatorKicks( mgr, "VH_OnOperatorKicks", &cVHPlugin::OnOperatorKicks),
			mOnOperatorDrops(mgr, "VH_OnOperatorDrops", &cVHPlugin::OnOperatorDrops),
			mOnUserLogin(mgr, "VH_OnUserLogin", &cVHPlugin::OnUserLogin),
			mOnUserLogout  (mgr, "VH_OnUserLogout",   &cVHPlugin::OnUserLogout ),
			mOnValidateTag(mgr, "VH_OnValidateTag", &cVHPlugin::OnValidateTag),
			mOnTimer(mgr, "VH_OnTimer", &cVHPlugin::OnTimer),
			mNickListNicks( mgr, "VH_OnCreateUserNickList", &cVHPlugin::OnCreateUserNickList),
			mNickListInfos( mgr, "VH_OnCreateUserInfoList", &cVHPlugin::OnCreateUserInfoList),
			mOpListNicks( mgr, "VH_OnCreateOpList", &cVHPlugin::OnCreateOpList),
			mOnNewReg(mgr, "VH_OnNewReg", &cVHPlugin::OnNewReg),
			mOnDelReg(mgr, "VH_OnDelReg", &cVHPlugin::OnDelReg),
			mOnUpdateClass(mgr, "VH_OnUpdateClass", &cVHPlugin::OnUpdateClass),
			mOnNewBan(mgr, "VH_OnNewBan", &cVHPlugin::OnNewBan),
			mOnUnBan(mgr, "VH_OnUnBan", &cVHPlugin::OnUnBan),
			mOnScriptCommand(mgr, "VH_OnScriptCommand", &cVHPlugin::OnScriptCommand),
			mOnScriptQuery(mgr, "VH_OnScriptQuery", &cVHPlugin::OnScriptQuery),
			mOnCtmToHub(mgr, "VH_OnCtmToHub", &cVHPlugin::OnCtmToHub),
			mOnOpChatMessage(mgr, "VH_OnOpChatMessage", &cVHPlugin::OnOpChatMessage),
			mOnPublicBotMessage(mgr, "VH_OnPublicBotMessage", &cVHPlugin::OnPublicBotMessage),
			mOnUnLoad(mgr, "VH_OnUnLoad", &cVHPlugin::OnUnLoad)
		{};

		cVHCBL_Connection mOnNewConn;
		cVHCBL_Connection mOnCloseConn;
		cVHCBL_Connection mOnCloseConnEx;
		cVHCBL_Message mOnUnknownMsg;
		//cVHCBL_Message mOnUnparsedMsg;
		cVHCBL_ConnMsgStr mOnParsedMsgSupports;
		cVHCBL_Message mOnParsedMsgMyHubURL;
		cVHCBL_Message mOnParsedMsgExtJSON;
		cVHCBL_Message mOnParsedMsgBotINFO;
		cVHCBL_Message mOnParsedMsgVersion;
		cVHCBL_Message mOnParsedMsgMyPass;
		cVHCBL_Message mOnParsedMsgAny;
		cVHCBL_Message mOnParsedMsgAnyEx;
		cVHCBL_Message mOnParsedMsgPM;
		cVHCBL_Message mOnParsedMsgMCTo;
		cVHCBL_Message mOnParsedMsgChat;
		cVHCBL_Message mOnParsedMsgSearch;
		cVHCBL_Message mOnParsedMsgMyINFO;
		cVHCBL_Message mOnFirstMyINFO;
		cVHCBL_Message mOnParsedMsgValidateNick;
		cVHCBL_Message mOnParsedMsgConnectToMe;
		cVHCBL_Message mOnParsedMsgRevConnectToMe;
		cVHCBL_ConnText mOnOperatorCommand;
		cVHCBL_ConnText mOnUserCommand;
		cVHCBL_ConnTextIntInt mOnHubCommand;
		cVHCBL_Message mOnParsedMsgSR;
		cVHCBL_UsrUsrStr mOnOperatorKicks;
		cVHCBL_UsrUsrStr mOnOperatorDrops;
		cVHCBL_User mOnUserLogin;
		cVHCBL_User mOnUserLogout;
		cVHCBL_ConnTag mOnValidateTag;
		cVHCBL_Long mOnTimer;
		cVHCBL_String mNickListNicks;
		cVHCBL_String mNickListInfos;
		cVHCBL_String mOpListNicks;
		cVHCBL_UsrStrInt mOnNewReg;
		cVHCBL_UsrStrInt mOnDelReg;
		cVHCBL_UsrStrIntInt mOnUpdateClass;
		cVHCBL_UsrBan mOnNewBan;
		cVHCBL_UsrStrStrStr mOnUnBan;
		cVHCBL_StrStrStrStr mOnScriptCommand;
		cVHCBL_StrStrStrStrResponses mOnScriptQuery;
		cVHCBL_ConnText mOnCtmToHub;
		cVHCBL_Strings mOnOpChatMessage;
		cVHCBL_StrStrIntInt mOnPublicBotMessage;
		cVHCBL_Long mOnUnLoad;
	};

	sCallBacks mCallBacks; // structure that holds all callbacks
};

	}; // namespace nServer
}; // namespace nVerliHub

#endif
