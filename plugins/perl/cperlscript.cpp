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

#include "cperlscript.h"

namespace nScripts {


cPerlScript::cPerlScript()
{
	mPerl = perl_alloc();
	perl_construct(mPerl);
	//perl_parse(mPerl, NULL, argc, argv, (char **)NULL);
}

void cPerlScript::Run()
{
	perl_run(mPerl);
}

cPerlScript::~cPerlScript()
{
	perl_destruct(mPerl);
	perl_free(mPerl);
}


};
