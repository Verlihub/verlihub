#	Copyright (C) 2003-2005 Daniel Muller, dan at verliba dot cz
#	Copyright (C) 2006-2024 Verlihub Team, info at verlihub dot net
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
#	Please see http://www.gnu.org/licenses/ for a copy
#	of the GNU General Public License.

# todo: cmake 3.16 has FindIntl.cmake, really need this?

if(LIBINTL_INCLUDE_DIR AND LIBINTL_LIB_FOUND)
	set(Libintl_FIND_QUIETLY TRUE)
endif(LIBINTL_INCLUDE_DIR AND LIBINTL_LIB_FOUND)

find_path(LIBINTL_INCLUDE_DIR libintl.h)
set(LIBINTL_LIB_FOUND FALSE)

if(LIBINTL_INCLUDE_DIR)
	include(CheckFunctionExists)
	check_function_exists(dgettext LIBINTL_LIBC_HAS_DGETTEXT)

	if(NOT LIBINTL_LIBC_HAS_DGETTEXT OR HAVE_ALPINE)
		find_library(LIBINTL_LIBRARIES NAMES intl libintl)

		if(LIBINTL_LIBRARIES)
			set(LIBINTL_LIB_FOUND TRUE)
		endif(LIBINTL_LIBRARIES)
	else(LIBINTL_LIBC_HAS_DGETTEXT)
		set(LIBINTL_LIBRARIES)
		set(LIBINTL_LIB_FOUND TRUE)
	endif(NOT LIBINTL_LIBC_HAS_DGETTEXT OR HAVE_ALPINE)
endif(LIBINTL_INCLUDE_DIR)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Libintl DEFAULT_MSG LIBINTL_INCLUDE_DIR LIBINTL_LIB_FOUND)
mark_as_advanced(LIBINTL_INCLUDE_DIR LIBINTL_LIBRARIES LIBINTL_LIBC_HAS_DGETTEXT LIBINTL_LIB_FOUND)
