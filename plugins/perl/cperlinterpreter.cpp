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

#include <iostream>
#include "cperlinterpreter.h"

using namespace std;

EXTERN_C void xs_init (pTHX);
EXTERN_C void boot_DynaLoader (pTHX_ CV* cv);
//EXTERN_C void boot_VHPerlExt (pTHX_ CV* cv);

namespace nScripts
{

cPerlInterpreter::cPerlInterpreter()
{
	mPerl = perl_alloc();
	if (!mPerl) throw "No Memory for Perl";
	SetMyContext();
	perl_construct(mPerl);
}

cPerlInterpreter::~cPerlInterpreter()
{
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
	SetMyContext();
	int result = perl_parse(mPerl,  xs_init, argc, argv, NULL);
	PL_exit_flags |= PERL_EXIT_DESTRUCT_END;
	return result;
}

int cPerlInterpreter::CallArgv(const char *Function, int Flags, char * Args [] )
{
	SetMyContext();
	int n = call_argv(Function, Flags , Args );
	return n;
}

};

EXTERN_C void xs_init(pTHX)
{
	char *file = __FILE__;
	/* DynaLoader is a special case */
	newXS("DynaLoader::boot_DynaLoader", boot_DynaLoader, file);
	//newXS("VHPerlExt::bootstrap", boot_VHPerlExt, file);
}
