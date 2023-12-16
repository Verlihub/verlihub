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


#ifdef USE_CUSTOM_AUTOSPRINTF

#include "i18n.h"
#include <stdio.h>
#include <stdarg.h>

const size_t my_autosprintf_buffer_size = 4096;

static char my_autosprintf_buffer[my_autosprintf_buffer_size] = { 0, };

std::string my_autosprintf_va(const char *format, va_list ap)
{
	if (!format || format[0] == '\0')
		return std::string();

	int res = vsnprintf(my_autosprintf_buffer, my_autosprintf_buffer_size, format, ap);
	return res > 0 ? std::string(my_autosprintf_buffer) : std::string();
}

std::string my_autosprintf(const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	std::string res = my_autosprintf_va(format, ap);
	va_end(ap);
	return res;
}


#endif
