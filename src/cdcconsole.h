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

#ifndef CDCCONSOLE_H
#define CDCCONSOLE_H
#include <string>
#include "ctriggers.h"
#include "ccustomredirects.h"
#include "cdcclients.h"
#include "cobj.h"
#include "cban.h"
#include "ccommandcollection.h"
#include "cdccommand.h"
#include "cinfoserver.h"
#include "tlistconsole.h"
#include "cconntypes.h"

using namespace std;

namespace nVerliHub {
	namespace nTables {
		class cTriggers;
		class cTriggerConsole;
		class cRedirects;
		class cDCClients;
		class cRedirectConsole;
		class cDCClientConsole;
	}
	namespace nSocket {
		class cConnDC;
		class cServerDC;
	};
	using namespace nTables;
	using namespace nUtils;
	using namespace nCmdr;

/**
	 * cDCConsole class. VerliHub console and command interpreter for users' and operators' commands.
	 * Triggers are not handled by this class.
	 *
	 * @author Daniel Muller
	 * @version 1.1
*/

class cDCConsole : public cDCConsoleBase
{
public:
	/**
	* Class constructor.
	* @param s Pointer to VerliHub server.
	*/
	cDCConsole(nSocket::cServerDC *s);

	/**
	* Class destructor.
	*/
	virtual ~cDCConsole();

	/**
	* Handle operator's commands like !restart, !quit, etc.
	* In case the command does not exist, it is passed to Trigger console
	* @param command The command to run.
	* @param conn The user's connection that wants to run the command.
	* @return Zero if an error occurred, 1 otherwise.
	*/
	virtual int OpCommand(const string &, nSocket::cConnDC*);

	/**
	* Handle user's commands like +myinfo, +chat, +report, etc.
	* In case the command does not exist, it is passed to Trigger console
	* @param command The command to run.
	* @param conn The user's connection that wants to run the command.
	 @return Zero if an error occurred, 1 otherwise.
	*/
	virtual int UsrCommand(const string & , nSocket::cConnDC * );

	/**
	* Handle !getip or !gi hub command.
	* This command sends to the user his IP address.
	* @param cmd_line The stream. Not used.
	* @param conn Pointer to user's connection which to send the result message.
	* @return Always 1.
	*/
	int  CmdGetip(istringstream &, nSocket::cConnDC *);

	/**
	* Handle !gethost <user1> <user2>.. or !gh hub command.
	* These commands send users' hostname.
	* @param cmd_line The stream that contains the list of users.
	* @param conn Pointer to user's connection which to send the result message.
	* @return Always 1.
	*/
	int CmdGethost(istringstream & , nSocket::cConnDC * );

	/**
	* Handle !getinfo <user1> <user2>.. or !gi hub command.
	* These commands send users' information like Country Code, IP address and host.
	* @param cmd_line The stream that contains the list of users.
	* @param conn Pointer to user's connection which to send the result message.
	* @return Zero if an error occurred, 1 otherwise.
	*/
	int CmdGetinfo(istringstream &cmd_line , nSocket::cConnDC *conn );

	/**
	* Handle !quit or !restart hub command.
	* These commands shutdown or restart the hub.
	* @param cmd_line The stream. Not used.
	* @param conn Pointer to user's connection which to send the result message.
	* @return Always 1.
	*/
	int CmdQuit(istringstream &, nSocket::cConnDC * conn,int code);

	/**
	* Handle !help hub command.
	* This command sends the available help message depending on user class.
	* Command is handled by Trigger console.
	* @param cmd_line The stream. Not used.
	* @param conn Pointer to user's connection which to send the result message.
	* @return Always 1.
	*/
	int CmdHelp(istringstream & cmd_line, nSocket::cConnDC * conn);

	/**
	* Handle !ccbroadcast <CC list> <message> or !ccbc  hub command.
	* This command sends a message to all users that belong to a Country in CC list.
	* @param cmd_line The stream that contains the country code list and the message.
	* @param conn Pointer to user's connection which to send the result message.
	* @return Always 1.
	*/
	int CmdCCBroadcast(istringstream &cmd_line, nSocket::cConnDC *conn, int cl_min, int cl_max);

	/**
	* Handle +password <password> hub command.
	* This command is used by the user to set his password when he has been registered for the first time.
	* @param cmd_line The stream that contains the password.
	* @param conn Pointer to user's connection which to send the result message.
	* @return 0 if an error occurred, 1 otherwise.
	*/
	int CmdRegMyPasswd(istringstream & cmd_line, nSocket::cConnDC * conn);

	/**
	* Handle +info  hub command.
	* This command sends to the user information about himself and the hub.
	* @param cmd_line The stream. Not used.
	* @param conn Pointer to user's connection which to send the result message.
	* @return 0 if an error occurred, 1 otherwise.
	*/

	int CmdUInfo(istringstream & cmd_line, nSocket::cConnDC * conn);
	int CmdRInfo(istringstream & cmd_line, nSocket::cConnDC * conn);

	/**
	* Handle +myinfo  hub command.
	* This command returns the information about the user.
	* @param cmd_line The stream. Not used.
	* @param conn Pointer to user's connection which to send the result message.
	* @return Always 1.
	*/
	int CmdMyInfo(istringstream & cmd_line, nSocket::cConnDC * conn);

	/**
	* Handle +myip hub command.
	* This command returns the IP address of the user.
	* @param cmd_line The stream. Not used.
	* @param conn Pointer to user's connection which to send the result message.
	* @return Always 1.
	*/
	int CmdMyIp(istringstream & cmd_line, nSocket::cConnDC * conn);

	/**
	* Handle +me <message> hub command.
	* This command lets an user to talk in 3rd person.
	* @param cmd_line The stream that contains the message.
	* @param conn Pointer to user's connection which to send the result message.
	* @return 0 if an error occured or 1 otherwise.
	*/
	int CmdMe(istringstream & cmd_line, nSocket::cConnDC * conn);

	/*
	* handle +regme <password> and +unregme hub commands
	* command sends a report to operator chat in order to ask for registration or registers a user automatically if autoreg_class config is set properly
	* also allows user to delete his own account if found
	* cmd_line - parameter stream that contains password
	* conn - pointer to user connection to send the result message
	* unreg - user wans to delete his account
	* return - 0 if an error occured or 1 otherwise
	*/
	int CmdRegMe(istringstream &cmd_line, nSocket::cConnDC *conn, bool unreg = false);

	/**
	* Handle +kick <user> <reason>.
	* This command will kick an user with the given reason.
	* @param cmd_line The stream that contains user and the reason.
	* @param conn Pointer to user's connection which to send the result message.
	* @return Always 1.
	*/
	//int CmdKick(istringstream & cmd_line, nSocket::cConnDC * conn);

	/**
	* Handle +chat and +nochat hub command.
	* These two commands are used to talk in mainchat.
	* @param cmd_line The stream. Not Used.
	* @param conn Pointer to user's connection which to send the result message.
	* @param switchon. If set to true add the user to mChatUsers list that contains the users that can talk in mainchat. False value does the opposite.
	* @return 0 if the user does not exist or 1 otherwise.
	*/
	int CmdChat(istringstream & cmd_line, nSocket::cConnDC * conn, bool switchon);

	/**
	* Handle !hideme or !hm <class> hub command.
	* This command will hide any commands for users with class lower than <class>.
	* @param cmd_line The stream that contains the class.
	* @param conn Pointer to user's connection which to send the result message.
	* @return Always 1
	*/
	int CmdHideMe(istringstream & cmd_line, nSocket::cConnDC * conn);

	/**
	* Handle !ul(imit) <users> <time>  hub command.
	* This command will progressively increase the max allowed users in the hub in <time>. The time must be speficied in minutes; this value can be ommited and default value is 60 minutes.
	* @param cmd_line The stream that contains the number of users and the time.
	* @param conn Pointer to user's connection which to send the result message.
	* @return Always 1
	*/
	int CmdUserLimit(istringstream & cmd_line, nSocket::cConnDC * conn);

	/**
	* Handle !unhidekick <user> hub command.
	* This command will un-hide kick made by <user>, previously hidden by using !hidekick <user> command.
	* @param cmd_line The stream that contains the username.
	* @param conn Pointer to user's connection which to send the result message.
	* @return Always 1
	*/
	int CmdUnHideKick(istringstream &cmd_line, nSocket::cConnDC *conn);

	/**
	* Handle !hidekick <user> hub command.
	* This command will hide kick made by <user> until he reconnects to the hub.
	* @param cmd_line The stream that contains the username.
	* @param conn Pointer to user's connection which to send the result message.
	* @return Always 1
	*/
	int CmdHideKick(istringstream &cmd_line, nSocket::cConnDC *conn);

	/**
	* Handle !class <nick> <new_class> hub command.
	* This command will change temporarily the user's class.
	* @param cmd_line The stream that contains the data like the username and the class.
	* @param conn Pointer to user's connection which to send the result message.
	* @return Always 1
	*/
	int CmdClass(istringstream &cmd_line, nSocket::cConnDC *conn);


	/**
	* Handle !protect <user> <class>  hub command.
	* This command protects an user against another one with lower class than <class>.
	* @param cmd_line The stream that contains the data like the username to protect and the class.
	* @param conn Pointer to user's connection which to send the result message.
	* @return Always 1
	*/
	int CmdProtect(istringstream &cmd_line, nSocket::cConnDC *conn);

	/**
	* Handle !reload command to reload VerliHub cache like triggers, custom redirects, configuration and reglist.
	* @param cmd_line The stream. Not used
	* @param conn Pointer to user's connection which to send the result message.
	* @return Always 1
	*/
	int CmdReload (istringstream &cmd_line, nSocket::cConnDC *conn);

	/**
	* Handle !commands or !cmds to show the list of available and register commands in VerliHub console.
	* @param cmd_line The stream. Not used
	* @param conn Pointer to user's connection which to send the list of command.
	* @return Always 1
	*/
	int CmdCmds (istringstream &cmd_line, nSocket::cConnDC *conn);

	/**
	* Handle !topic <msg> command to set the hub topic for the hub. The topic will be appended after the hub name and look like this: <HUB NAME> - <TOPIC>
	* @param cmd_line The stream the contains the topic.
	* @param conn Pointer to user's connection which set the hub topic. It is used to send error message.
	* @return Always 1
	*/
	int CmdTopic(istringstream & cmd_line, nSocket::cConnDC * conn);

	static cPCRE mIPRangeRex;
	static bool GetIPRange(const string &rang, unsigned long &frdr, unsigned long &todr);

	typedef cDCCommand::sDCCmdFunc cfDCCmdBase;
	typedef cDCCommand cDCCmdBase;

	enum { eCM_CMD, eCM_BAN, eCM_TEMPBAN, eCM_GAG, eCM_TRIGGER, eCM_CUSTOMREDIR, eCM_DCCLIENT, eCM_SET, eCM_REG, eCM_INFO, eCM_RAW, eCM_WHO, eCM_KICK, eCM_PLUG, eCM_REPORT, eCM_BROADCAST, eCM_CONNTYPE, eCM_TRIGGERS, eCM_GETCONFIG, eCM_CLEAN };

	// Pointr to VerliHub server
	nSocket::cServerDC *mServer;

	// Pointer to Trigger console to handle custom commands not defined here
	cTriggers *mTriggers;

	// Pointer to Redirect console to handle custom redirect commands
	cRedirects *mRedirects;

	// Pointer to Client console to handle custom client TAG
	cDCClients *mDCClients;
private:
	cCommandCollection mCmdr;
	cCommandCollection mUserCmdr;
	struct cfBan: cfDCCmdBase { virtual bool operator()(); } mFunBan;
	cDCCmdBase mCmdBan;
	struct cfTempBan: cfDCCmdBase { virtual bool operator()(); } mFunTempBan;
	cDCCmdBase mCmdTempBan;
	struct cfGag : cfDCCmdBase { virtual bool operator()(); } mFunGag;
	cDCCmdBase mCmdGag;
	struct cfTrigger : cfDCCmdBase { virtual bool operator()(); } mFunTrigger;
	cDCCmdBase mCmdTrigger;
	struct cfSetVar : cfDCCmdBase { virtual bool operator()(); } mFunSetVar;
	cDCCmdBase mCmdSetVar;
	struct cfRegUsr : cfDCCmdBase { virtual bool operator()(); } mFunRegUsr;
	cDCCmdBase mCmdRegUsr;
	struct cfRaw : cfDCCmdBase { virtual bool operator()(); } mFunRaw;
	cDCCmdBase mCmdRaw;
	struct cfCmd : cfDCCmdBase { virtual bool operator()(); } mFunCmd;
	cDCCmdBase mCmdCmd;
	struct cfWho : cfDCCmdBase { virtual bool operator()(); } mFunWho;
	cDCCmdBase mCmdWho;
	struct cfKick : cfDCCmdBase { virtual bool operator()(); } mFunKick;
	cDCCmdBase mCmdKick;
	struct cfInfo : cfDCCmdBase {
		virtual bool operator()();
		cInfoServer mInfoServer;
	} mFunInfo;
	cDCCmdBase mCmdInfo;
	cDCCmdBase mCmdPlug;
	struct cfPlug : cfDCCmdBase { virtual bool operator()(); } mFunPlug;
	struct cfReport : cfDCCmdBase { virtual bool operator()(); } mFunReport;
	cDCCmdBase mCmdReport;
	struct cfBc : cfDCCmdBase { virtual bool operator()(); } mFunBc;
	cDCCmdBase mCmdBc;

	/**
	 * Handle !getconfig or !gc hub command.
	 * These structure is used to return the list of available config variables with values.
	 */
	struct cfGetConfig : cfDCCmdBase { virtual bool operator()(); } mFunGetConfig;
	cDCCmdBase mCmdGetConfig;
	struct cfClean : cfDCCmdBase { virtual bool operator()(); } mFunClean;
	cDCCmdBase mCmdClean;

	// Redirect commands to other console
	struct cfRedirToConsole : cfDCCmdBase {
		virtual bool operator()();
		tConsoleBase<cDCConsole> *mConsole;
	} mFunRedirConnType, mFunRedirTrigger, mFunCustomRedir, mFunDCClient;
	cDCCmdBase mCmdRedirConnType;
	cDCCmdBase mCmdRedirTrigger;
	cDCCmdBase mCmdCustomRedir;
	cDCCmdBase mCmdDCClient;
	cConnTypeConsole mConnTypeConsole;
	cTriggerConsole *mTriggerConsole;
	cRedirectConsole *mRedirectConsole;
	cDCClientConsole *mDCClientConsole;
};

}; // namespace nVerliHub

#endif
