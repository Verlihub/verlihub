/**************************************************************************
*   Copyright (C) 2003 by Dan Muller                                      *
*   dan at verliba.cz                                                     *
*                                                                         *
*   Copyright (C) 2011 by Shurik                                          *
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
#ifndef NSCRIPTSCPERLINTERPRETER_H
#define NSCRIPTSCPERLINTERPRETER_H

#define PERL_CALL "vh::VH__Call__Function"


namespace nPerl
{
#include <EXTERN.h>               /* from the Perl distribution     */
#include <perl.h>                 /* from the Perl distribution     */
};
using namespace nPerl;

namespace nVerliHub {
	namespace nPerlPlugin {

/** \brief a wrapper for perl calling ; a c++ wrapper for the perl XS API

  *
  * Now suppose we have more than one interpreter instance running at the same time.
  * This is feasible, but only if you used the Configure option -Dusemultiplicity or
  * the options -Dusethreads -Duseithreads when building Perl. By default, enabling
  * one of these Configure options sets the per-interpreter global variable
  * PL_perl_destruct_level to 1, so that thorough cleaning is automatic.
  *
  * Using -Dusethreads -Duseithreads rather than -Dusemultiplicity is more appropriate
  * if you intend to run multiple interpreters concurrently in different threads,
  * because it enables support for linking in the thread libraries of your system
  * with the interpreter.
  *
  * @author Daniel Muller
  */
class cPerlInterpreter
{
public:
	std::string mScriptName;

	cPerlInterpreter();
	virtual ~cPerlInterpreter();

	/** \brief calling this ensures that this interpreter is gonna be used in next perl call

	   Note the calls to PERL_SET_CONTEXT(). These are necessary to initialize the global state
	   that tracks which interpreter is the "current" one on the particular process or thread
	   that may be running it. It should always be used if you have more than one interpreter
	   and are making perl API calls on both interpreters in an interleaved fashion.

	   cPerlInterpreter calls itself this function in many most of cases
	*/
	void SetMyContext();

	/** parse arguments like for a command line of perl */
	int Parse(int argc, char *argv[]);

	bool CallArgv(const char *Function, char * Args [] );
        void ReportPerlError(char *error);

protected:
	PerlInterpreter *mPerl;
};

	}; // namespace nPerlPlugin
}; // namespace nVerlihub

#endif
