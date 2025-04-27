/*
	Copyright (C) 2003-2005 Daniel Muller, dan at verliba dot cz
	Copyright (C) 2006-2025 Verlihub Team, info at verlihub dot net

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

#ifndef NSCRIPTSCPERLSCRIPT_H
#define NSCRIPTSCPERLSCRIPT_H

#include <EXTERN.h>               /* from the Perl distribution     */
#include <perl.h>                 /* from the Perl distribution     */

/** \brief script related calsses

blabla
*/
namespace nScripts {

/**
perl script wrapper

@author Daniel Muller
*/
class cPerlScript{
public:
	cPerlScript();
	~cPerlScript();
	void Run();
	PerlInterpreter *mPerl;
};

};

#endif
