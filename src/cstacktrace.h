/*
	Copyright (C) 2008 Timo Bingmann, http://idlebox.net/
	Copyright (C) 2014 RoLex, webmaster at feardc dot net

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

#ifndef CSTACKTRACE_H
#define CSTACKTRACE_H

#include <stdio.h>
#include <stdlib.h>
#include <execinfo.h>
#include <cxxabi.h>

static inline void print_stacktrace(FILE *out = stderr, unsigned int max_frames = 63) // this functions prints a demangled stack backtrace of the caller function to out file
{
	fprintf(out, "Stack trace:\n");
	void* addrlist[max_frames + 1]; // storage array for stack trace address data
	int addrlen = backtrace(addrlist, sizeof(addrlist) / sizeof(void*)); // retrieve current stack addresses

	if (addrlen == 0) {
		fprintf(out, "  <empty, possibly corrupt>\n");
		return;
	}

	char** symbollist = backtrace_symbols(addrlist, addrlen); // resolve addresses into strings containing filename(function+address)
	size_t funcnamesize = 256; // allocate string which will be filled with the demangled function name
	char* funcname = (char*)malloc(funcnamesize);

	for (int i = 1; i < addrlen; i++) { // iterate over the returned symbol lines, skip the first, it is the address of this function
		char *begin_name = 0, *begin_offset = 0, *end_offset = 0;

		for (char *p = symbollist[i]; *p; ++p) { // find parentheses and +address offset surrounding the mangled name: module(function+0x15c) [0x8048a6d]
			if (*p == '(')
				begin_name = p;
			else if (*p == '+')
				begin_offset = p;
			else if ((*p == ')') && begin_offset) {
				end_offset = p;
				break;
			}
		}

		if (begin_name && begin_offset && end_offset && (begin_name < begin_offset)) {
			*begin_name++ = '\0';
			*begin_offset++ = '\0';
			*end_offset = '\0';
			int status; // mangled name is now in [begin_name, begin_offset] and caller offset in [begin_offset, end_offset], apply __cxa_demangle
			char* ret = abi::__cxa_demangle(begin_name, funcname, &funcnamesize, &status);

			if (status == 0) {
				funcname = ret; // use possibly reallocated string
				fprintf(out, "  %s: %s +%s\n", symbollist[i], funcname, begin_offset);
			} else // demangling failed, output function name as a c function with no arguments
				fprintf(out, "  %s: %s() +%s\n", symbollist[i], begin_name, begin_offset);
		} else // could not parse the line, print the whole line
			fprintf(out, "  %s\n", symbollist[i]);
	}

	free(funcname);
	free(symbollist);
}

#endif // CSTACKTRACE_H
