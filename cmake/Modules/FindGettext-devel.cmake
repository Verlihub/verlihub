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

# todo: cmake 3.16 has FindGettext.cmake, really need this?

SET(GETTEXT_DEVEL_FOUND TRUE)

find_path(GETTEXT_DEVEL_INCLUDE_DIR libintl.h
	/usr/include
	/usr/local/include
)

MARK_AS_ADVANCED(GETTEXT_DEVEL_INCLUDE_DIR)

IF(NOT GETTEXT_DEVEL_INCLUDE_DIR)
	SET(GETTEXT_DEVEL_FOUND FALSE)
ENDIF(NOT GETTEXT_DEVEL_INCLUDE_DIR)

check_function_exists(gettext HAVE_GETTEXT) # check gettext function

IF(GETTEXT_DEVEL_FOUND)
	IF(NOT Gettext-devel_FIND_QUIETLY)
		MESSAGE(STATUS "[ OK ] Found GetText: ${GETTEXT_DEVEL_INCLUDE_DIR}")
	ENDIF(NOT Gettext-devel_FIND_QUIETLY)
ELSE(GETTEXT_DEVEL_FOUND)
	IF(Gettext-devel_FIND_REQUIRED)
		MESSAGE(FATAL_ERROR "[ ER ] GetText headers not found, please install them via your package manager or compile from source: http://ftp.gnu.org/pub/gnu/gettext/")
	ENDIF(Gettext-devel_FIND_REQUIRED)
ENDIF(GETTEXT_DEVEL_FOUND)
