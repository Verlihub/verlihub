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

#ifndef NDIRECTCONNECTCFORBIDDEN_H
#define NDIRECTCONNECTCFORBIDDEN_H

#include <string>
#include "src/cpcre.h"
#include <vector>
#include "src/tlistplugin.h"

using namespace std;

namespace nVerliHub {
	namespace nSocket {
		class cConnDC;
		class cServerDC;
	};
	namespace nForbidPlugin {
		class cpiForbid;
/**
a trigger command ...
user defined string that triggers given action

@author Daniel Muller
*/

class cForbiddenWorker
{
public:
	cForbiddenWorker();
	virtual ~cForbiddenWorker();
	int DoIt(const string &cmd_line, nSocket::cConnDC *conn, nSocket::cServerDC *serv, int mask);
	bool CheckMsg(const string &msg);
	bool PrepareRegex();
	// the forbidden word
	string mWord;
	// the checking mask
	int mCheckMask;
	// affected class
	int mAfClass;

	string mReason;

	enum {eCHECK_CHAT = 1, eCHECK_PM = 2, eNOTIFY_OPS = 4};

	virtual void OnLoad();
	friend ostream &operator << (ostream &, cForbiddenWorker &);
private:
	nUtils::cPCRE *mpRegex;
};

typedef tList4Plugin<cForbiddenWorker, cpiForbid> tForbiddenBase;

/**
the vector of triggers, with load, reload, save functions..

@author Daniel Muller
*/
class cForbidden : public tForbiddenBase
{
public:
	cForbidden(cVHPlugin *pi);
	virtual ~cForbidden(){};
	virtual void AddFields();
	virtual bool CompareDataKey(const cForbiddenWorker &D1, const cForbiddenWorker &D2);

	int ForbiddenParser(const string & str, nSocket::cConnDC * conn, int mask);
	int CheckRepeat(const string & , int);
	int CheckUppercasePercent(const string & , int);
};

class cForbidCfg : public nConfig::cConfigBase
{
public:
	cForbidCfg(nSocket::cServerDC *);
	int max_upcase_percent;
	int max_repeat_char;
	int max_class_dest;

	nSocket::cServerDC *mS;
	virtual int Load();
	virtual int Save();
};

	}; // namespace nForbidPlugin
}; // namespace nVerliHub

#endif
