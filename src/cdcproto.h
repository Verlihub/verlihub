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

#ifndef CDCPROTO_H
#define CDCPROTO_H

#include "cpcre.h"
#include "cprotocol.h"

#include <string>

using namespace std;

namespace nVerliHub {
	namespace nTables{
		class cConnType;
	};

	using namespace nTables;

	namespace nSocket {
		class cConnDC;
		class cServerDC;
	};

	class cUser;
	class cUserBase;

	// Protocol stuff
	namespace nProtocol {

//using cConnDC;

class cMessageDC;

/**
* Protocol message managment class.
*
* @author Daniel Muller
* @version 1.1
*/
class cDCProto : public cProtocol
{
	friend class nSocket::cServerDC;
 public:
	/**
	* Class constructor.
	* @param serv An instance of Direct Connect hub server.
	*/
	cDCProto(nSocket::cServerDC *serv);

	/**
	* Class destructor.
	*/
	virtual ~cDCProto();

	/**
	* Create the parser to process protocol messages.
	* @return serv An instance of the parser
	*/
	virtual cMessageParser *CreateParser();

	/**
	* Delete a parser.
	* @param parse The parser to delete.
	*/
	virtual void DeleteParser(cMessageParser *);

	/**
	* Send user and op lists to the user.
	* @param conn User connection.
	* @return A negative number if an error occurs or zero otherwise.
	*/
	int NickList(nSocket::cConnDC *);

	/*
	* check if the message is a command and pass it to the console
	* cmd - the command
	* conn - user connection
	* pm - command is sent in pm
	* return - 1 if the message is a command otherwise 0
	*/
	virtual int ParseForCommands(string &cmd, nSocket::cConnDC *conn, int pm);

	/**
	* Process a given protocol message that has been already parsed.
	* @param msg The parsed message.
	* @param conn User connection.
	* @return A negative number if an error occurs or zero otherwise.
	*/
	virtual int TreatMsg(cMessageParser *msg,  nSocket::cAsyncConn *conn);
protected:

	/**
	* Treat $ConnectToMe protocol message.
	* @param msg The parsed message.
	* @param conn User connection.
	* @return A negative number if an error occurs or zero otherwise.
	*/
	int DC_ConnectToMe(cMessageDC * msg, nSocket::cConnDC * conn);

	/**
	* Treat $MyPass protocol message.
	* @param msg The parsed message.
	* @param conn User connection.
	* @return A negative number if an error occurs or zero otherwise.
	*/
	int DC_MyPass(cMessageDC * msg, nSocket::cConnDC * conn);

	/**
	* Treat $Search protocol message.
	* @param msg The parsed message.
	* @param conn User connection.
	* @return A negative number if an error occurs or zero otherwise.
	*/
	int DC_Search(cMessageDC * msg, nSocket::cConnDC * conn);

	// short active and passive tth search commands
	int DC_SA(cMessageDC *msg, nSocket::cConnDC *conn);
	int DC_SP(cMessageDC *msg, nSocket::cConnDC *conn);

	/**
	* Treat $SR protocol message.
	* @param msg The parsed message.
	* @param conn User connection.
	* @return A negative number if an error occurs or zero otherwise.
	*/
	int DC_SR(cMessageDC * msg, nSocket::cConnDC * conn);

	/**
	* Treat $BotINFO protocol message.
	* If the user client sent BotINFO in support, send also $HubINFO.
	* @param msg The parsed message.
	* @param conn User connection.
	* @return A negative number if an error occurs or zero otherwise.
	*/
	int DCB_BotINFO(cMessageDC * msg, nSocket::cConnDC * conn);

	/**
	* Treat $WhoIP protocol message and send requested users' IP.
	* @param msg The parsed message.
	* @param conn User connection.
	* @return A negative number if an error occurs or zero otherwise.
	*/
	int DCO_WhoIP(cMessageDC * msg, nSocket::cConnDC * conn);

	/** Treat the DC message in a appropriate way */
	int DC_GetNickList(cMessageDC * msg, nSocket::cConnDC * conn);
	/** Treat the DC message in a appropriate way */
	int DC_GetINFO(cMessageDC * msg, nSocket::cConnDC * conn);
	int DCO_UserIP(cMessageDC * msg, nSocket::cConnDC * conn);
	/** Treat the DC message in a appropriate way */
	int DC_MyINFO(cMessageDC * msg, nSocket::cConnDC * conn);
	// $IN
	int DC_IN(cMessageDC *msg, nSocket::cConnDC *conn);
	// $ExtJSON
	int DC_ExtJSON(cMessageDC *msg, nSocket::cConnDC *conn);
	/** Treat the DC message in a appropriate way */
	int DCO_Kick(cMessageDC * msg, nSocket::cConnDC * conn);
	/** Treat the DC message in a appropriate way */
	int DCO_OpForceMove(cMessageDC * msg, nSocket::cConnDC * conn);
	/** Treat the DC message in a appropriate way */
	int DC_RevConnectToMe(cMessageDC * msg, nSocket::cConnDC * conn);
	/** Treat the DC message in a appropriate way */
	int DC_MultiConnectToMe(cMessageDC * msg, nSocket::cConnDC * conn);
	/** operator ban */
	int DCO_TempBan(cMessageDC * msg, nSocket::cConnDC * conn);
	/** operator unban */
	int DCO_UnBan(cMessageDC * msg, nSocket::cConnDC * conn);
	/** operator getbanlist */
	int DCO_GetBanList(cMessageDC * msg, nSocket::cConnDC * conn);

	/** operator set hub topic */
	int DCO_SetTopic(cMessageDC * msg, nSocket::cConnDC * conn);

	// unknown message
	int DCU_Unknown(cMessageDC *msg, nSocket::cConnDC *conn);

	// tls proxy
	int DCC_MyIP(cMessageDC *msg, nSocket::cConnDC *conn);

	// ctm2hub
	int DCC_MyNick(cMessageDC *msg, nSocket::cConnDC *conn);
	int DCC_Lock(cMessageDC *msg, nSocket::cConnDC *conn);
	void ParseReferer(const string &lock, string &ref, bool inlock = true);

 public:
	/**
	* Check if a message is valid (max length and max lines per message).
	* If message is not valid, a proper message describing the problem is sent to the user.
	* @param text The message.
	* @param conn User connection.
	* @return True if the message is valid or false otherwise.
	*/
	static bool CheckChatMsg(const string &text, nSocket::cConnDC *conn);

	// protocol creation helper functions, note, they all clear destination buffer before appending new data
	static void Create_Chat(string &dest, const string &nick, const string &text);
	static void Create_Me(string &dest, const string &nick, const string &text);
	static void Create_Lock(string &dest, const string &lock, const string &name, const string &vers);
	static void Create_HubName(string &dest, const string &name, const string &topic);
	static void Create_MyINFO(string &dest, const string &nick, const string &desc, const string &speed, const string &mail, const string &share);
	static void Create_PM(string &dest, const string &from, const string &to, const string &sign, const string &text);
	static void Create_PMForBroadcast(string &start, string &end, const string &from, const string &sign, const string &text);
	static void Create_Quit(string &dest, const string &nick);
	static void Create_ValidateDenide(string &dest, const string &nick);
	static void Create_BadNick(string &dest, const string &id, const string &par);
	static void Create_Hello(string &dest, const string &nick);
	static void Create_LogedIn(string &dest, const string &nick);
	static void Create_NickList(string &dest, const string &nick);
	static void Create_OpList(string &dest, const string &nick);
	static void Create_BotList(string &dest, const string &nick);
	static void Create_Key(string &dest, const string &key);
	static void Create_FailOver(string &dest, const string &addr);
	static void Create_ForceMove(string &dest, const string &addr, const bool clear);
	static void Create_HubTopic(string &dest, const string &topic);
	static void Create_ConnectToMe(string &dest, const string &nick, const string &addr, const string &port, const string &extra);
	static void Create_Search(string &dest, const string &addr, const string &lims, const string &spat);
	static void Create_Search(string &dest, const string &addr, const string &tth, const bool pas);
	static void Create_SA(string &dest, const string &tth, const string &addr);
	static void Create_SP(string &dest, const string &tth, const string &nick);
	static void Create_UserIP(string &dest, const string &list);
	static void Create_UserIP(string &dest, const string &nick, const string &addr);
	static void Create_GetPass(string &dest);
	static void Create_BadPass(string &dest);
	static void Create_GetHubURL(string &dest);
	static void Create_HubIsFull(string &dest);
	static void Create_Supports(string &dest, const string &flags);
	static void Create_NickRule(string &dest, const string &rules);
	static void Create_SearchRule(string &dest, const string &rules);
	static void Create_SetIcon(string &dest, const string &url, bool pipe = true);
	static void Create_SetLogo(string &dest, const string &url, bool pipe = true);
	static void Create_HubINFO(string &dest, const string &pars);

	/**
	* Treat mainchat messages.
	* @param msg The parsed message.
	* @param conn User connection.
	* @return A negative number if an error occurs or zero otherwise.
	*/
	int DC_Chat(cMessageDC * msg, nSocket::cConnDC * conn);

	/**
	* Treat $Key protocol message.
	* @param msg The parsed message.
	* @param conn User connection.
	* @return A negative number if an error occurs or zero otherwise.
	*/
	int DC_Key(cMessageDC * msg, nSocket::cConnDC * conn);

	/**
	* Treat $To protocol message.
	* Check also private message flood.
	* @param msg The parsed message.
	* @param conn User connection.
	* @return A negative number if an error occurs or zero otherwise.
	*/
	int DC_To(cMessageDC * msg, nSocket::cConnDC * conn);

	/*
	* Treat $MCTo: protocol message.
	* Check also private message flood.
	* msg = The parsed message.
	* conn = User connection.
	* return = A negative number if an error occurs or zero otherwise.
	*/
	int DC_MCTo(cMessageDC * msg, nSocket::cConnDC * conn);

	/**
	* Treat $ValidateNick protocol message.
	* @param msg The parsed message.
	* @param conn User connection.
	* @return A negative number if an error occurs or zero otherwise.
	*/
	int DC_ValidateNick(cMessageDC *msg, nSocket::cConnDC *conn);

	/**
	* Treat $Version protocol message.
	* @param msg The parsed message.
	* @param conn User connection.
	* @return A negative number if an error occurs or zero otherwise.
	*/
	int DC_Version(cMessageDC * msg, nSocket::cConnDC * conn);

	/**
	* Parse client's extensions and send $Support.
	* @param msg The parse message.
	* @param conn User connection.
	* @return Always 0.
	*/
	int DC_Supports(cMessageDC * msg, nSocket::cConnDC * conn);

	/*
		$MyHubURL <url>
	*/
	int DC_MyHubURL(cMessageDC *msg, nSocket::cConnDC *conn);

	/**
	* Send hub topic to an user.
	* @param msg Not used.
	* @param conn User connection.
	* @return Always 0.
	*/
	int DCO_GetTopic(cMessageDC * msg, nSocket::cConnDC * conn);

	// helper functions
	bool CheckUserLogin(nSocket::cConnDC *conn, cMessageDC *msg, bool inlist = true);
	bool CheckUserRights(nSocket::cConnDC *conn, cMessageDC *msg, bool cond);
	bool CheckProtoSyntax(nSocket::cConnDC *conn, cMessageDC *msg);
	bool CheckProtoLen(nSocket::cConnDC *conn, cMessageDC *msg);
	bool CheckUserNick(nSocket::cConnDC *conn, const string &nick);
	bool CheckCompatTLS(nSocket::cConnDC *one, nSocket::cConnDC *two);
	bool FindInSupports(const string &list, const string &flag);

	/**
	* Escape DC string.
	* @param msg The message to escape.
	* @param dest Result message.
	* @param WithDCN If true /%DCNXXX%/ is used instead of $#XXXX;
	*/
	static void EscapeChars(const string &, string &, bool WithDCN = false);

	/**
	* Escape DC string.
	* @param msg The message to escape.
	* @param len The length of the string
	* @param WithDCN If true /%DCNXXX%/ is used instead of $#XXXX;
	*/
	static void EscapeChars(const char *, int, string &, bool WithDCN = false);

	/**
	* Check if the IP belongs to private network.
	* @param ip IP address (DOT-notation).
	* @return True if the IP belongs to private network or false otherwise.
	*/
	static bool isLanIP(const string &ip);

	/**
	* Calculate the key from the given lock.
	* @param lock The lock.
	* @param fkey The result (key).
	*/
	static void Lock2Key(const string &lock, string &fkey);

	/**
	* Parse speed chunck and return a pointer to connection type object.
	* If it is not possible to get a connection type, a default object is returned
	* @param speed The speed.
	* @return Pointer to connection type object.
	*/
	cConnType *ParseSpeed(const string &speed);

	static void UnEscapeChars(const string &, string &, bool WithDCN = false);
	static void UnEscapeChars(const string &, char *, unsigned int &len, bool WithDCN = false);
	static bool CheckIP(nSocket::cConnDC *conn, const string &ip);

	// Message kick regex
	nUtils::cPCRE mKickChatPattern;

	// Ban regex
	nUtils::cPCRE mKickBanPattern;

	// Direct Connect hub server
	nSocket::cServerDC *mS;
};

	}; // namespace nProtocol
}; // namespace nVerliHub

#endif
