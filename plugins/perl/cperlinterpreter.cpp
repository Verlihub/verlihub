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

#include <iostream>
#include "src/cserverdc.h"
#include "src/script_api.h"
#include "cperlinterpreter.h"

using namespace std;

EXTERN_C void xs_init (pTHX);
EXTERN_C void boot_DynaLoader (pTHX_ CV* cv);

namespace nVerliHub {
	namespace nPerlPlugin {
		using namespace nSocket;

cPerlInterpreter::cPerlInterpreter():
	cObj("cPerlInterpreter")
{
	mPerl = perl_alloc();

	if (!mPerl)
		throw "No memory for Perl";

	my_perl = mPerl;
	SetMyContext();
	perl_construct(mPerl);
}

cPerlInterpreter::~cPerlInterpreter()
{
	//PerlInterpreter *my_perl = mPerl;
	SetMyContext();
	perl_destruct(mPerl);
	perl_free(mPerl);
}

/** \brief calling this ensures that this interpreter is gonna be used in next perl call

Note the calls to PERL_SET_CONTEXT(). These are necessary to initialize the global state
that tracks which interpreter is the "current" one on the particular process or thread
that may be running it. It should always be used if you have more than one interpreter
and are making perl API calls on both interpreters in an interleaved fashion.

cPerlInterpreter calls itself this function in many most of cases
*/
void cPerlInterpreter::SetMyContext()
{
	PERL_SET_CONTEXT((mPerl));
}

/** parse arguments like for a command line of perl */
int cPerlInterpreter::Parse(int argc, const char *argv[])
{
	//PerlInterpreter *my_perl = mPerl;
	SetMyContext();
	int result = perl_parse(mPerl, xs_init, argc, (char **)argv, NULL);
	PL_exit_flags |= PERL_EXIT_DESTRUCT_END;
	mScriptName = argv[1];
	if (!result)
	{
		const char *args[] = { "Main", NULL };
		CallArgv(PERL_CALL, args);
	}
	return result;
}

bool cPerlInterpreter::CallArgv(const char *Function, const char *Args[])
{
	//PerlInterpreter *my_perl = mPerl;
	SetMyContext();
	dSP;
	int n;
	bool ret = true;
	ENTER;
	SAVETMPS;
	n = call_argv(Function, G_EVAL|G_SCALAR , (char **)Args);
	SPAGAIN;
	if (SvTRUE(ERRSV)) {
	  STRLEN n_a;
	  ReportPerlError(SvPV(ERRSV, n_a));
	} else if (n == 1) {
	  ret = POPi;
	} else {
	  vhErr(1) << "Call " << Function << ": expected 1 return value, but " << n << " returned" << endl;
	}
	FREETMPS;
	LEAVE;
	return ret;
}

void cPerlInterpreter::ReportPerlError(const char * error)
{
	string error2 = "[ Perl ERROR ] ";
	error2.append(error);
	cServerDC * server = cServerDC::sCurrentServer;
	if(server) SendPMToAll(error2.c_str(), server->mC.hub_security.c_str(), 3, 10);
}

	}; // namespace nPerlPlugin
}; // namespace nVerlihub

EXTERN_C void xs_init(pTHX)
{
	std::string file = __FILE__;
	/* DynaLoader is a special case */
	newXS("DynaLoader::boot_DynaLoader", boot_DynaLoader, file.c_str());
}


