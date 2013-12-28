/*
	Copyright (C) 2003-2005 Daniel Muller, dan at verliba dot cz
	Copyright (C) 2006-2014 Verlihub Project, devs at verlihub-project dot org

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
#include <string>
#include "cpcre.h"
#include "cprotocol.h"

using namespace std;

namespace nVerliHub {
	namespace nTables{ class cConnType; };
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
	virtual ~cDCProto(){};

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
	* Check if the message is a command and pass it to the console.
	* msg = The message.
	* conn = User connection.
	* flag = Message is sent in PM or MC.
	* return = 1 if the message is a command otherwise 0.
	*/
	int ParseForCommands(const string &, nSocket::cConnDC *, int);

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
	int DC_UserIP(cMessageDC * msg, nSocket::cConnDC * conn);
	/** Treat the DC message in a appropriate way */
	int DC_MyINFO(cMessageDC * msg, nSocket::cConnDC * conn);
	/** Treat the DC message in a appropriate way */
	int DC_Kick(cMessageDC * msg, nSocket::cConnDC * conn);
	/** Treat the DC message in a appropriate way */
	int DC_OpForceMove(cMessageDC * msg, nSocket::cConnDC * conn);
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

 public:
	/**
	* Check if a message is valid (max length and max lines per message).
	* If message is not valid, a proper message describing the problem is sent to the user.
	* @param text The message.
	* @param conn User connection.
	* @return True if the message is valid or false otherwise.
	*/
	static bool CheckChatMsg(const string &text, nSocket::cConnDC *conn);

	/**
	* Create a message to send in mainchat.
	* @param dest String to store the result.
	* @param nick The sender.
	* @param text The message.
	*/
	static void Create_Chat(string &dest, const string&nick,const string &text);

	/**
	* Create protocol message for hub name ($HubName).
	* Hub topic, if not empty, is also appended after hub name.
	* @param dest String to store the result.
	* @param name Hub name.
	* @param topice Hub topic.
	*/
	static void Create_HubName(string &dest, string &name, string &topic);

	/**
	* Create MyINFO string ($MyINFO protocol message).
	* @param dest String to store MyINFO.
	* @param nick Nickname.
	* @param desc Description.
	* @param speed Speed.
	* @param mail E-Mail address.
	* @param share Share in bytes.
	*/
	static void Create_MyINFO(string &dest, const string&nick, const string &desc, const string&speed, const string &mail, const string &share);

	/**
	* Create a private message ($To protocol message).
	* @param dest String to store private message.
	* @param from The sender.
	* @param to The recipient.
	* @param sign The sender.
	* @param text The message.
	*/
	static void Create_PM(string &dest,const string &from, const string &to, const string &sign, const string &text);

	/**
	* Create a private message that should be sent to everyone ($To protocol message).
	* @param start Destination filled with first part of the protocol message ($To: ).
	* @param end Destination that contains the rest of the protocol message.
	* @param from The sender.
	* @param sign The sender.
	* @param text The message.
	*/
	static void Create_PMForBroadcast(string &start,string &end, const string &from, const string &sign, const string &text);

	/**
	* Create quit protocol message ($Quit).
	* @param dest String to store quit message.
	* @param nick The nick.
	*/
	static void Create_Quit(string &dest, const string&nick);

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
	int DCE_Supports(cMessageDC * msg, nSocket::cConnDC * conn);

	/**
	* Send hub topic to an user.
	* @param msg Not used.
	* @param conn User connection.
	* @return Always 0.
	*/
	int DCO_GetTopic(cMessageDC * msg, nSocket::cConnDC * conn);

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
	static bool isLanIP(string);

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

	const string &GetMyInfo(cUserBase * User, int ForClass);
	void Append_MyInfoList(string &dest, const string &MyINFO, const string &MyINFO_basic, bool DoBasic);
	static void UnEscapeChars(const string &, string &, bool WithDCN = false);
	static void UnEscapeChars(const string &, char *, int &len ,bool WithDCN = false);
	static bool CheckIP(nSocket::cConnDC * conn, string &ip);

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
