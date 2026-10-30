/*
	Copyright (C) 2003-2005 Daniel Muller, dan at verliba dot cz
	Copyright (C) 2006-2026 Verlihub Team, info at verlihub dot net

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

#ifndef CPILUA_H
#define CPILUA_H

#include "cluainterpreter.h"
#include "callbacks.h"
#include "cconsole.h"
#include "cquery.h"
#include "src/cconndc.h"
#include "src/cvhplugin.h"
#include <vector>
#include <algorithm>

#define LUA_PI_IDENTIFIER "Lua"

using std::vector;
namespace nVerliHub {
	namespace nLuaPlugin {

class cpiLua : public nPlugin::cVHPlugin
{
public:
	cpiLua();
	virtual ~cpiLua();
	virtual void OnLoad(cServerDC *);
	virtual bool RegisterAll();
	virtual bool OnNewConn(nSocket::cConnDC *);
	virtual bool OnCloseConn(nSocket::cConnDC *);
	virtual bool OnCloseConnEx(nSocket::cConnDC *);
	virtual bool OnParsedMsgChat(nSocket::cConnDC *, nProtocol::cMessageDC *);
	virtual bool OnParsedMsgPM(nSocket::cConnDC *, nProtocol::cMessageDC *);
	virtual bool OnParsedMsgMCTo(nSocket::cConnDC *, nProtocol::cMessageDC *);
	virtual bool OnParsedMsgSearch(nSocket::cConnDC *, nProtocol::cMessageDC *);
	virtual bool OnParsedMsgSR(nSocket::cConnDC *, nProtocol::cMessageDC *);
	virtual bool OnParsedMsgMyINFO(nSocket::cConnDC *, nProtocol::cMessageDC *);
	virtual bool OnFirstMyINFO(nSocket::cConnDC *, nProtocol::cMessageDC *);
	virtual bool OnParsedMsgValidateNick(nSocket::cConnDC *, nProtocol::cMessageDC *);
	virtual bool OnParsedMsgAny(nSocket::cConnDC *, nProtocol::cMessageDC *);
	virtual bool OnParsedMsgAnyEx(nSocket::cConnDC *, nProtocol::cMessageDC *);
	virtual bool OnParsedMsgSupports(nSocket::cConnDC *, nProtocol::cMessageDC *, string *);
	virtual bool OnParsedMsgMyHubURL(nSocket::cConnDC *, nProtocol::cMessageDC *);
	virtual bool OnParsedMsgExtJSON(nSocket::cConnDC *, nProtocol::cMessageDC *);
	virtual bool OnParsedMsgBotINFO(nSocket::cConnDC *, nProtocol::cMessageDC *);
	virtual bool OnParsedMsgVersion(nSocket::cConnDC *, nProtocol::cMessageDC *);
	virtual bool OnParsedMsgMyPass(nSocket::cConnDC *, nProtocol::cMessageDC *);
	virtual bool OnParsedMsgConnectToMe(nSocket::cConnDC *, nProtocol::cMessageDC *);
	virtual bool OnParsedMsgRevConnectToMe(nSocket::cConnDC *, nProtocol::cMessageDC *);
	virtual bool OnUnknownMsg(nSocket::cConnDC *, nProtocol::cMessageDC *);
	//virtual bool OnUnparsedMsg(nSocket::cConnDC *, nProtocol::cMessageDC *);
	virtual bool OnOperatorCommand(nSocket::cConnDC *, string *);
	virtual bool OnUserCommand(nSocket::cConnDC *, string *);
	virtual bool OnHubCommand(nSocket::cConnDC *, string *, int, int);
	virtual bool OnOperatorKicks(cUser *, cUser *, string *);
	virtual bool OnOperatorDrops(cUser *, cUser *, string *);
	virtual bool OnValidateTag(nSocket::cConnDC *, cDCTag *);
	virtual bool OnUserInList(cUser *user);
	virtual bool OnUserLogin(cUser *user);
	virtual bool OnUserLogout(cUser *user);
	virtual bool OnCloneCountLow(cUser *, string, int);
	virtual bool OnTimer(__int64);
	virtual bool OnNewReg(cUser *, string, int);
	virtual bool OnDelReg(cUser *, string, int);
	virtual bool OnUpdateClass(cUser *, string, int, int);
	virtual bool OnBadPass(cUser *user);
	virtual bool OnNewBan(cUser *, cBan *);
	virtual bool OnUnBan(cUser *, string nick, string op, string reason);
	virtual bool OnSetConfig(cUser *user, string *conf, string *var, string *val_new, string *val_old, int val_type);
	virtual bool OnScriptCommand(string *cmd, string *data, string *plug, string *script);
	virtual bool OnCtmToHub(cConnDC *conn, string *ref);
	virtual bool OnOpChatMessage(string *nick, string *data);
	virtual bool OnPublicBotMessage(string *nick, string *data, int min_class, int max_class);
	virtual bool OnUnLoad(long code);

	bool AutoLoad();
	void DelBots(cLuaInterpreter *li); // note: removes bots that script added from hub
	bool CallAll(const char *, const char *[], cConnDC *conn = NULL);
	unsigned int Size() { return mLua.size(); }
	void SetLogLevel(int level);
	void SetErrClass(int eclass);
	void ReportLuaError(const string &err);

	void Empty()
	{
		tvLuaInterpreter::iterator it;

		for (it = mLua.begin(); it != mLua.end(); ++it) {
			if ((*it) != NULL) {
				delete *it;
				(*it) = NULL;
			}
		}

		mLua.clear();
		EmptyTrash();
	}

	void AddData(cLuaInterpreter *ip)
	{
		mLua.push_back(ip);
	}

	cLuaInterpreter * operator[](unsigned int i)
	{
		if (i >= Size())
			return NULL;
		else
			return mLua[i];
	}

	/*
		script can load, unload or reload scripts from inside its own call, for example with VH:ParseCommand, also itself
		note: unloaded script is removed from list at once, so it gets no more calls, but it is deleted only when no script is running, because its lua state may still run
	*/
	void CallStart()
	{
		mCallDepth++;
	}

	void CallEnd()
	{
		if (mCallDepth)
			mCallDepth--;
	}

	bool HasScript(const cLuaInterpreter *li) const
	{
		return (std::find(mLua.begin(), mLua.end(), li) != mLua.end());
	}

	void DelScript(cLuaInterpreter *li)
	{
		if (!li)
			return;

		tvLuaInterpreter::iterator it = std::find(mLua.begin(), mLua.end(), li);

		if (it == mLua.end()) // note: already unloaded, for example by its own UnLoad(), it must not be deleted twice
			return;

		mLua.erase(it);
		DelBots(li); // note: bots of script leave hub at once, also when script itself is deleted later, so reloaded script can add them again

		if (mCallDepth) // note: deleted on next timer
			mTrash.push_back(li);
		else
			delete li;
	}

	void EmptyTrash()
	{
		tvLuaInterpreter list; // note: closing state runs finalizers of script, they could unload too
		list.swap(mTrash);

		for (tvLuaInterpreter::iterator it = list.begin(); it != list.end(); ++it)
			delete *it;
	}

	cConsole mConsole;
	nMySQL::cQuery *mQuery;
	typedef vector<cLuaInterpreter*> tvLuaInterpreter;
	tvLuaInterpreter mLua;
	tvLuaInterpreter mTrash; // note: unloaded scripts that were still running, see DelScript()
	unsigned int mCallDepth = 0; // note: script calls in progress
	string mScriptDir;
	static int log_level;
	static int err_class;
	static cServerDC *server;
	static cpiLua *me;
};

	}; // namespace nLuaPlugin
}; // namespace nVerliHub

#endif
