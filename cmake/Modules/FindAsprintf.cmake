#	Copyright (C) 2003-2005 Daniel Muller, dan at verliba dot cz
#	Copyright (C) 2006-2025 Verlihub Team, info at verlihub dot net
#
#	Verlihub is free software; You can redistribute it
#	and modify it under the terms of the GNU General
#	Public License as published by the Free Software
#	Foundation, either version 3 of the license, or at
#	your option any later version.
#
#	Verlihub is distributed in the hope that it will be
#	useful, but without any warranty, without even the
#	implied warranty of merchantability or fitness for
#	a particular purpose. See the GNU General Public
#	License for more details.
#
#	Please see https://www.gnu.org/licenses/ for a copy
#	of the GNU General Public License.

find_library(GETTEXT_ASPRINTF_LIBRARY asprintf)

if(GETTEXT_ASPRINTF_LIBRARY)
	message(STATUS "[ OK ] Found asprintf library: ${GETTEXT_ASPRINTF_LIBRARY}")
else(GETTEXT_ASPRINTF_LIBRARY)
	message(FATAL_ERROR "[ ER ] asprintf library not found, install it via your package manager or compile from source, else try with: -DUSE_CUSTOM_AUTOSPRINTF=ON")
endif(GETTEXT_ASPRINTF_LIBRARY)

# end of file
