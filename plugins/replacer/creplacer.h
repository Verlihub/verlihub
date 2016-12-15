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

#ifndef NDIRECTCONNECTCREPLACER_H
#define NDIRECTCONNECTCREPLACER_H

#include "creplacerworker.h"
#include <vector>
#include "src/cconfmysql.h"
#include "src/cconndc.h"
#include "src/cserverdc.h"

using std::vector;
namespace nVerliHub {
	namespace nReplacePlugin {
/**
the vector of triggers, with load, reload, save functions..
@author Daniel Muller
changes by Pralcio
*/

class cReplacer : public nConfig::cConfMySQL
{
public:
	cReplacer(cServerDC *server);
	~cReplacer();
	void Empty();
	int LoadAll();
	void CreateTable(void);
	int Size(){ return mData.size();}
	void PrepareNew();
	void DeleteLast();

	int AddReplacer(cReplacerWorker &);
	void DelReplacer(cReplacerWorker &);

	string ReplacerParser(const string & str, nSocket::cConnDC * conn);

	cReplacerWorker * operator[](int );
private:
	typedef vector<cReplacerWorker *> tDataType;
	tDataType mData;
	// a model of a replacer worker
	cReplacerWorker mModel;

	nSocket::cServerDC *mS;
};

class cReplaceCfg : public nConfig::cConfigBase
{
public:
	cReplaceCfg(nSocket::cServerDC *);

	nSocket::cServerDC *mS;
	virtual int Load();
	virtual int Save();
};
	}; // namespace nReplacePlugin
}; // namespace nVerliHub

#endif
