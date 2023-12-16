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

#ifndef NPLUGINTLISTPLUGIN_H
#define NPLUGINTLISTPLUGIN_H
#include "cvhplugin.h"
#include "tmysqlmemorylist.h"
#include "tmysqlmemoryhash.h"
#include "tmysqlmemoryordlist.h"
#include "cserverdc.h"

namespace nVerliHub {
	using namespace nUtils;
	namespace nPlugin {


template <class DATA_TYPE, class PLUGIN_TYPE>
class tList4Plugin : public tMySQLMemoryList<DATA_TYPE, PLUGIN_TYPE>
{
public:
	tList4Plugin(cVHPlugin *pi, const string & TableName):
		nConfig::tMySQLMemoryList<DATA_TYPE,PLUGIN_TYPE>(pi->mServer->mMySQL, (PLUGIN_TYPE*)pi, TableName)
	{}
	virtual ~tList4Plugin() {}
};

template <class DATA_TYPE, class PLUGIN_TYPE>
class tHash4Plugin : public nConfig::tMySQLMemoryHash<DATA_TYPE, PLUGIN_TYPE>
{
public:
	tHash4Plugin(cVHPlugin *pi, const string & TableName):
		nConfig::tMySQLMemoryHash<DATA_TYPE,PLUGIN_TYPE>(pi->mServer->mMySQL, (PLUGIN_TYPE*)pi, TableName)
	{}
	virtual ~tHash4Plugin() {}
};


template <class DATA_TYPE, class PLUGIN_TYPE>
class tOrdList4Plugin : public nConfig::tMySQLMemoryOrdList<DATA_TYPE, PLUGIN_TYPE>
{
public:
	tOrdList4Plugin(cVHPlugin *pi, const string & TableName, const string &DbOrder):
		nConfig::tMySQLMemoryOrdList<DATA_TYPE,PLUGIN_TYPE>(pi->mServer->mMySQL, (PLUGIN_TYPE*)pi, TableName, DbOrder)
	{}
	virtual ~tOrdList4Plugin() {}
};

template <class LIST_TYPE, class CONSOLE_TYPE>
class tpiListPlugin : public cVHPlugin
{
public:
	tpiListPlugin() : mConsole(this), mList(NULL) {}
	virtual ~tpiListPlugin() {
		if (mList != NULL) delete mList;
		mList = NULL;
	}

	virtual bool RegisterAll() {
		RegisterCallBack("VH_OnUserCommand");
		// is VH_OnUserCommand really called?
		// @todo: add VH_OnOperatorCommand
		return true;
	}

	virtual bool OnUserCommand(cConnDC *conn, string *str) {
		return !(mConsole.DoCommand(*str, conn));
	}

	virtual void OnLoad(cServerDC *server){
		cVHPlugin::OnLoad(server);

		mList = new LIST_TYPE(this);
		mList->OnStart();
	}
	CONSOLE_TYPE mConsole;
	LIST_TYPE* mList;
};
};
};

#endif
