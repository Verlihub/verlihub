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

#ifndef NDIRECTCONNECT_NPLUGINCVHPLUGIN_H
#define NDIRECTCONNECT_NPLUGINCVHPLUGIN_H

#include "cpluginbase.h"
#include "cusercollection.h"

using namespace std;

namespace  nVerliHub {
	//using nPlugin::cPluginBase;

	namespace nProtocol {
		class cMessageDC;
	};

	namespace nTables {
		class cBan;
		class cRegUserInfo;
	};

	using namespace nProtocol;
	using namespace nTables;

	namespace nSocket {
		class cServerDC;
		class cConnDC;
	};

	//using nSocket::cConnDC;
	class cDCTag;
	class cUser;
	class cUserRobot;
	class cPluginRobot;

namespace nPlugin {

//using cServerDC;
//using cConnDC;

class cPluginUserData
{
	public:
		cPluginUserData()
		{
			mxUser = NULL;
		}
	private:
		string Nick; // in case user is already left this is still valid
		cUser *mxUser;
};

/** \brief The Verlihub's plugin class
  *
  * derive your plugins from this one
  *
  * @author Daniel Muller
*/

class cVHPlugin : public cPluginBase
{
public:
	/// constructor
	cVHPlugin();
	/// destructor
	virtual ~cVHPlugin();

	////////// Genral VH plugin's interface
	virtual bool SupportsScripts(){ return false;}
	virtual bool SupportsMultipleScripts(){ return false;}
	// to load the main single script if only one is supported
	virtual bool LoadScript(const string &filename, ostream &os);
	// to load another extra script if mutiple are supported
	virtual bool AddScript(const string &filename, ostream &os);
	virtual bool IsScriptLoaded(const string &filename){return false;}
	virtual bool UnLoadScript(const string &filename){return false;}
	virtual bool GetFirstScript(string &filename){return false;}
	virtual bool GetNextScript(string &filename){return false;}

	//! This method is called when a plugin is being loaded
	/*!
	\param server Verlihub server that is loading this plugin
	 */
	virtual void OnLoad(nSocket::cServerDC* server){mServer = server;}

	/*
	* Event handler function that is called when there is a new incoming connection.
	* Use RegisterCallBack("VH_OnNewConn") to register it. This event can be discarded.
	* conn = The new connection.
	* return = Return false to close the connection otherwise return true.
	*/
	virtual bool OnNewConn(nSocket::cConnDC* conn) {return true;}

	//! Event handler function that is called when a connection is closed
	/*!
	 * 	Use RegisterCallBack("VH_OnCloseConn") to register it. This event cannot be discardable.
	\param conn The connection is going to be closed
	 */
	virtual bool OnCloseConn(nSocket::cConnDC* conn){ return true; }

	/*
	* Extended event handler function that is called when a connection is closed.
	* Use RegisterCallBack("VH_OnCloseConnEx") to register it. This event isnt discardable.
	* conn = The connection to be closed.
	*/
	virtual bool OnCloseConnEx(nSocket::cConnDC* conn) {return true;}

	//! Event handler function that is called when an unknown message is sent from a connection
	/*!
	 * 	Use RegisterCallBack("VH_OnUnknownMsg") to register it. This event cannot be discardable.
	\param conn The pointer to the connection that sent the message
	\param msg The pointer to cMessageDC object
	 */
	virtual bool OnUnknownMsg(nSocket::cConnDC* conn, nProtocol::cMessageDC * msg){ return true; }

	/*
	* Event handler function that is called when an unparsed message is sent from a connection.
	* Use RegisterCallBack("VH_OnUnparsedMsg") to register it. This event isnt discardable.
	* conn = The pointer to the connection that sent the message.
	* msg = The pointer to cMessageDC object.
	*/
	//virtual bool OnUnparsedMsg(nSocket::cConnDC *conn, nProtocol::cMessageDC *msg) {return true;}

	//! Event handler function that is called when a parsed message is received
	/*!
	 * 	Use RegisterCallBack("VH_OnParsedMsgAny") to register it. This event can be discardable.
	 * 	Please notice that this method is slighty different from OnParsedMsg* methods because
	 * 	this one is called before the message is processed as a protocol message
	\return Return false to ignore the parsed message otherwise return true.
	\param conn The pointer to the connection that sent the message
	\param msg The pointer to cMessageDC object
	 */
	virtual bool OnParsedMsgAny(nSocket::cConnDC* conn, nProtocol::cMessageDC *msg){ return true; }

	/*
	* Event handler function that is called when a parsed message is received, but before mpUser is created.
	* Use RegisterCallBack("VH_OnParsedMsgAnyEx") to register it. This event can be discarded.
	* return = Return false to ignore the parsed message and drop user connection, otherwise return true.
	* conn = The pointer to the connection that sent the message.
	* msg = The pointer to cMessageDC object.
	*/
	virtual bool OnParsedMsgAnyEx(nSocket::cConnDC *conn, nProtocol::cMessageDC *msg) {return true;}

	/*
		Event handler function that is called when $Supports message is received.
		Use RegisterCallBack("VH_OnParsedMsgSupports") to register it.
		This event is discardable.
	*/
	virtual bool OnParsedMsgSupports(nSocket::cConnDC *conn, nProtocol::cMessageDC *msg, string *back) { return true; }

	/*
		Event handler function that is called when $MyHubURL message is received.
		Use RegisterCallBack("VH_OnParsedMsgMyHubURL") to register it.
		This event is discardable.
	*/
	virtual bool OnParsedMsgMyHubURL(nSocket::cConnDC *conn, nProtocol::cMessageDC *msg) { return true; }

	/*
		Event handler function that is called when $ExtJSON message is received.
		Use RegisterCallBack("VH_OnParsedMsgExtJSON") to register it.
		This event is discardable.
	*/
	virtual bool OnParsedMsgExtJSON(nSocket::cConnDC *conn, nProtocol::cMessageDC *msg) { return true; }

	/*
	* Event handler function that is called when $BotINFO message is received.
	* Use RegisterCallBack("VH_OnParsedMsgBotINFO") to register it. This event can be discarded.
	* conn = The pointer to the connection that sent the message.
	* msg = The pointer to cMessageDC object.
	*/
	virtual bool OnParsedMsgBotINFO(nSocket::cConnDC *conn, nProtocol::cMessageDC *msg) {return true;}

	/*
	* Event handler function that is called when $Version message is received.
	* Use RegisterCallBack("VH_OnParsedMsgVersion") to register it. This event can be discarded.
	* conn = The pointer to the connection that sent the message.
	* msg = The pointer to cMessageDC object.
	*/
	virtual bool OnParsedMsgVersion(nSocket::cConnDC *conn, nProtocol::cMessageDC *msg) {return true;}

	//! Event handler function that is called when $ValidateNick message is received
	/*!
	 * 	Use RegisterCallBack("VH_OnParsedMsgValidateNick") to register it. This event can be discardable.
	\return Return false to ignore the protocol message otherwise return true.
	\param conn The pointer to the connection that sent the message
	\param msg The pointer to cMessageDC object
	 */
	virtual bool OnParsedMsgValidateNick(nSocket::cConnDC*, nProtocol::cMessageDC *){ return true; }

	//! Event handler function that is called when $MyPass message is received
	/*!
	 * 	Use RegisterCallBack("VH_OnParsedMsgValidateNick") to register it. This event can be discardable.
	\return	Return false to ignore the protocol message otherwise return true.
	\param conn The pointer to the connection that sent the message
	\param msg The pointer to cMessageDC object
	\todo Add it in cdcproto.cpp
	 */
	virtual bool OnParsedMsgMyPass(nSocket::cConnDC*, nProtocol::cMessageDC *){ return true; }

	//! Event handler function that is called when $MyINFO message is received
	/*!
	 * 	Use RegisterCallBack("VH_OnParsedMsgMyINFO") to register it. This event can be discardable.
	\return	Return false to ignore the protocol message otherwise return true.
	\param conn The pointer to the connection that sent the message
	\param msg The pointer to cMessageDC object
	 */
	virtual bool OnParsedMsgMyINFO(nSocket::cConnDC* , cMessageDC *){ return true; }

	/* Event handler function that is called when first $MyINFO message is received.
	* Use RegisterCallBack("VH_OnFirstMyINFO") to register it. This event can be discarded.
	* return = Return false to ignore the parsed message, otherwise return true.
	* conn = The pointer to the connection that sent the message.
	* msg = The pointer to cMessageDC object.
	*/
	virtual bool OnFirstMyINFO(nSocket::cConnDC*, cMessageDC*){ return true; }

	//! Event handler function that is called when $Search message is received
	/*!
	 * 	Use RegisterCallBack("VH_OnParsedMsgSearch") to register it. This event can be discardable.
	\return	Return false to ignore the protocol message otherwise return true.
	\param conn The pointer to the connection that sent the message
	\param msg The pointer to cMessageDC object
	 */
	virtual bool OnParsedMsgSearch(nSocket::cConnDC* , cMessageDC *){ return true; }

	//! Event handler function that is called when $SR message is received
	/*!
	 * 	Use RegisterCallBack("VH_OnParsedMsgSR") to register it. This event can be discardable.
	\return Return false to ignore the protocol message otherwise return true.
	\param conn The pointer to the connection that sent the message
	\param msg The pointer to cMessageDC object
	\todo Add it in cdcproto.cpp
	 */
	virtual bool OnParsedMsgSR(nSocket::cConnDC* , cMessageDC *){ return true; }

	//! Event handler function that is called when a chat message is received
	/*!
	 * 	Use RegisterCallBack("VH_OnParsedMsgChat") to register it. This event can be discardable.
	\return Return false to ignore the protocol message and not to send it to all users, otherwise return true.
	\param conn The pointer to the connection that sent the message
	\param msg The pointer to cMessageDC object
	 */
	virtual bool OnParsedMsgChat(nSocket::cConnDC* , cMessageDC *){ return true; }

	//! Event handler function that is called when a pm message ($To) is received
	/*!
	 * 	Use RegisterCallBack("VH_OnParsedMsgPM") to register it. This event can be discardable.
	\return Return false to ignore the protocol message and not to send it to all users, otherwise return true.
	\param conn The pointer to the connection that sent the message
	\param msg The pointer to cMessageDC object
	 */
	virtual bool OnParsedMsgPM(nSocket::cConnDC* , cMessageDC *){ return true; }

	/* Event handler function that is called when private mainchat message is received.
	* Use RegisterCallBack("VH_OnParsedMsgMCTo") to register it. This event can be discarded.
	* return = Return false to ignore the parsed message, otherwise return true.
	* conn = The pointer to the connection that sent the message.
	* msg = The pointer to cMessageDC object.
	*/
	virtual bool OnParsedMsgMCTo(nSocket::cConnDC*, cMessageDC *){ return true; }

	//! Event handler function that is called when $ConnectToMe message is received
	/*!
	 * 	Use RegisterCallBack("VH_OnParsedMsgConnectToMe") to register it. This event can be discardable.
	\return Return false to ignore the protocol message and not to send it, otherwise return true.
	\param conn The pointer to the connection that sent the message
	\param msg The pointer to cMessageDC object
	 */
	virtual bool OnParsedMsgConnectToMe(nSocket::cConnDC* , cMessageDC *){ return true; }

	//! Event handler function that is called when $RevConnectToMe message is received
	/*!
	 * 	Use RegisterCallBack("VH_OnParsedMsgRevConnectToMe") to register it. This event can be discardable.
	\return Return false to ignore the protocol message and not to send it, otherwise return true.
	\param conn The pointer to the connection that sent the message
	\param msg The pointer to cMessageDC object
	 */
	virtual bool OnParsedMsgRevConnectToMe(nSocket::cConnDC* , cMessageDC *){ return true; }

	//! Event handler function that is called when tag is parsed and validated
	/*!
	 * 	Use RegisterCallBack("VH_OnValidateTag") to register it. This event can be discardable.
	\return Return false to close the user connection, otherwise return true.
	\param conn The pointer to the connection that sent the message
	\param msg The pointer to cDCTag object
	 */
	virtual bool OnValidateTag(nSocket::cConnDC* , cDCTag *){ return true; }

	/*
	* Event handler function that is called when an operator command is received.
	* Use RegisterCallBack("VH_OnOperatorCommand") to register it. This event can be discarded.
	* return = Return false to ignore the command, otherwise return true.
	* conn = The pointer to the connection that sent the message.
	* cmd = The command.
	*/
	virtual bool OnOperatorCommand(nSocket::cConnDC *conn, string *cmd) {return true;}

	/*
	* Event handler function that is called when an user command is received.
	* Use RegisterCallBack("VH_OnUserCommand") to register it. This event can be discarded.
	* return = Return false to ignore the command, otherwise return true.
	* conn = The pointer to the connection that sent the message.
	* cmd = The command.
	*/
	virtual bool OnUserCommand(nSocket::cConnDC *conn, string *cmd) {return true;}

	/*
	* Event handler function that is called when a hub command is received.
	* Use RegisterCallBack("VH_OnHubCommand") to register it. This event can be discarded.
	* return = Return false to ignore the command, otherwise return true.
	* conn = The pointer to the connection that sent the message.
	* cmd = The command.
	* op = Command is user or operator.
	* pm = Command is sent to PM or MC.
	*/
	virtual bool OnHubCommand(nSocket::cConnDC *conn, string *cmd, int op, int pm) {return true;}

	//! Event handler function that is called when an operator kicks an user
	/*!
	 * 	Use RegisterCallBack("VH_OnOperatorKicks") to register it. This event can be discardable.
	\return Return false to ignore the kick, otherwise return true.
	\param Op The operator that did the kick
	\param User The kicked user
	\param Reason The reason of the kick
	 */

	virtual bool OnOperatorKicks(cUser* OP, cUser *User , string *Reason){ return true; }

	/*
		event handler function called when operator drops user
		use RegisterCallBack("VH_OnOperatorDrops") to register it
		this event is discardable
	*/
	virtual bool OnOperatorDrops(cUser *op, cUser *user, string *why) { return true; }

	/*
		event handler called before user login is complete
		it allows you to call any nick based plugin functions
		use RegisterCallBack("VH_OnUserInList") to register it
		this event is discardable
	*/
	virtual bool OnUserInList(cUser *user) { return true; }

	/*
		event handler called when user login is complete
		use RegisterCallBack("VH_OnUserLogin") to register it
		this event is discardable
	*/
	virtual bool OnUserLogin(cUser *user) { return true; }

	/*
		event handler called when user logout is complete
		use RegisterCallBack("VH_OnUserLogout") to register it
		this event is discardable
	*/
	virtual bool OnUserLogout(cUser *user) { return true; }

	/*
		event handler called when clone count is too low, but atleast one
		use RegisterCallBack("VH_OnCloneCountLow") to register it
		this event is discardable
	*/
	virtual bool OnCloneCountLow(cUser *user, string nick, int count) { return true; }

	/*
		event handler called on bad password from user
		use RegisterCallBack("VH_OnBadPass") to register it
		this event is partially discardable
		it allows you to add own ban instead of built in
	*/
	virtual bool OnBadPass(cUser *user) { return true; }

	/*
	* Event handler function that is called when a new ban is about to be created.
	* Use RegisterCallBack("VH_OnNewBan") to register it. This event can be discarded.
	* user = The operator.
	* ban = cBan object.
	*/
	virtual bool OnNewBan(cUser* User, nTables::cBan * Ban) {return true;}

	/*
	* Event handler function that is called when a ban is about to be removed.
	* Use RegisterCallBack("VH_OnUnBan") to register it. This event can be discarded.
	* user = The operator.
	* nick = The nick we are going to unban.
	* op = The operators nick.
	* reason = The reason of the unban.
	*/
	virtual bool OnUnBan(cUser* User, string nick, string op, string reason) {return true;}

	/*
		event handler function that is called when a config is set either by user or within a plugin
		use RegisterCallBack("VH_OnSetConfig") to register it
		this event can be discarded
	*/
	virtual bool OnSetConfig(cUser *user, string *conf, string *var, string *val_new, string *val_old, int val_type) { return true; }

	/*
	* Event handler function that is called when timer is called.
	* Use RegisterCallBack("VH_OnTimer") to register it. This event isnt discardable.
	* msec = Current system time in milliseconds.
	*/
	virtual bool OnTimer(__int64 msec) {return true;}

	/// Called when loading and  when it's the correct time to register for callbacks
	virtual bool RegisterAll() = 0;

	virtual bool AddRobot(cUserRobot *);
	virtual bool DelRobot(cUserRobot *);
	virtual cPluginRobot* NewRobot(const string &nick, const int clas, const string &info);

	/// robot events
	virtual bool RobotOnPM( cPluginRobot *, cMessageDC *, nSocket::cConnDC *){ return true;};
	// nick list extension
	//virtual bool OnCreateUserNickList (string *NickList) {return true;};
	//virtual bool OnCreateUserInfoList (string *InfoList) {return true;};
	//virtual bool OnCreateOpList (string *OpList) {return true;};

	/*
	* Event handler function that is called when an operator wants to delete a registered user.
	* Use RegisterCallBack("VH_OnDelReg") to register it. This event can be discarded.
	* user = The operator.
	* nick = The nickname of the registered user.
	* class = The users class.
	*/
	virtual bool OnDelReg(cUser* User, string mNick, int mClass) {return true;}

	/*
	* Event handler function that is called when an operator wants to register a new user.
	* Use RegisterCallBack("VH_OnNewReg") to register it. This event can be discarded.
	* user = The operator.
	* nick = The nickname of the registered user.
	* class = The users class.
	*/
	virtual bool OnNewReg(cUser* User, string mNick, int mClass) {return true;}

	/*
	* Event handler function that is called when an operator wants to update a registered users class.
	* Use RegisterCallBack("VH_OnDelReg") to register it. This event can be discarded.
	* user = The operator.
	* nick = The nickname of the registered user.
	* oldclass = The users old class.
	* newclass = The users new class.
	*/
	virtual bool OnUpdateClass(cUser* User, string mNick, int oldClass, int newClass) {return true;}

	/*
	* Event handler function that is called when a script sends a command to other scripts.
	* Use RegisterCallBack("VH_OnScriptCommand") to register it. This is not discardable.
	* cmd = Command ID.
	* data = Additional data.
	* plug = ID of plugin that makes the call.
	* script = ID of script that makes the call.
	*/
	virtual bool OnScriptCommand(string *cmd, string *data, string *plug, string *script) { return true; }

	// ctm2hub
	virtual bool OnCtmToHub(nSocket::cConnDC *conn, string *ref) { return true; }

	// opchat message
	virtual bool OnOpChatMessage(string *nick, string *data) { return true; }

	// public bot message
	virtual bool OnPublicBotMessage(string *nick, string *data, int min_class, int max_class) { return true; }

	// hub stop
	virtual bool OnUnLoad(long code) { return true; }

	/// per-user data of the plugin
	/*
	virtual cPluginUserData *GetPluginUserData( cUser * );
	virtual cPluginUserData *SetPluginUserData( cUser *, cPluginUserData *NewData );
	*/

	/// Pointer for the verlihub server
	nSocket::cServerDC *mServer;
	cUserCollection mRobots;
	//nUtils::tHashArray<cPluginUserData*> *mUserDataTable;
};

	}; // namespace nPlugins
}; // namespace nVerliHub

#endif
