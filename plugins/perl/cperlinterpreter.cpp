/**************************************************************************
*   Copyright (C) 2003 by Dan Muller                                      *
*   dan at verliba.cz                                                     *
*                                                                         *
*   Copyright (C) 2011-2013 by Shurik                                     *
*   shurik at sbin.ru                                                     *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
*   This program is distributed in the hope that it will be useful,       *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
*   GNU General Public License for more details.                          *
*                                                                         *
*   You should have received a copy of the GNU General Public License     *
*   along with this program; if not, write to the                         *
*   Free Software Foundation, Inc.,                                       *
*   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
***************************************************************************/
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

cPerlInterpreter::cPerlInterpreter()
{
	mPerl = perl_alloc();
	if (!mPerl) throw "No Memory for Perl";
	SetMyContext();
	perl_construct(mPerl);
}

cPerlInterpreter::~cPerlInterpreter()
{
	PerlInterpreter *my_perl = mPerl;
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
int cPerlInterpreter::Parse(int argc, char *argv[])
{
	PerlInterpreter *my_perl = mPerl;
	SetMyContext();
	int result = perl_parse(mPerl, xs_init, argc, argv, NULL);
	PL_exit_flags |= PERL_EXIT_DESTRUCT_END;
	mScriptName = argv[1];
	
	char *args[] = { (char *)"Main", NULL };
	CallArgv(PERL_CALL, args);
	return result;
}

bool cPerlInterpreter::CallArgv(const char *Function, char * Args [] )
{
	PerlInterpreter *my_perl = mPerl;
	SetMyContext();
	dSP;
	int n;
	bool ret = true;
	ENTER;
	SAVETMPS;
	n = call_argv(Function, G_EVAL|G_SCALAR , Args );
	SPAGAIN;
	if(SvTRUE(ERRSV)) {
	  STRLEN n_a;
	  ReportPerlError(SvPV(ERRSV, n_a));
	} else if(n==1) {
	  ret = POPi;
	} else {
	  cerr << "Call " << Function << ": expected 1 return value, but " << n << " returned" << endl;
	}
	FREETMPS;
	LEAVE;
	return ret;
}

void cPerlInterpreter::ReportPerlError(char * error)
{
	string error2 = "[ Perl ERROR ] ";
	error2.append(error);
	cServerDC * server = cServerDC::sCurrentServer;
	if(server) SendPMToAll( (char *) error2.c_str(), (char *) server->mC.hub_security.c_str(), 3, 10);
}

	}; // namespace nPerlPlugin
}; // namespace nVerlihub

EXTERN_C void xs_init(pTHX)
{
	std::string file = __FILE__;
	/* DynaLoader is a special case */
	newXS("DynaLoader::boot_DynaLoader", boot_DynaLoader, file.c_str());
}


