#	Copyright (C) 2003-2005 Daniel Muller, dan at verliba dot cz
#	Copyright (C) 2006-2017 Verlihub Team, info at verlihub dot net
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

# - Find Gettext-devel
# Find the Gettext includes and library
# This module defines
#  GETTEXT_DEVEL_INCLUDE_DIR, where to find geoip.h
#  GETTEXT_ASPRINTF_LIBRARY, asprintf library.
#  GETTEXT_DEVEL_FOUND, If false, do not try to use GeoIP.

SET(GETTEXT_DEVEL_FOUND TRUE)

find_path(GETTEXT_DEVEL_INCLUDE_DIR libintl.h
	/usr/local/include
	/usr/include
	DOC "Path to gettext include directory"
)

MARK_AS_ADVANCED(GETTEXT_DEVEL_INCLUDE_DIR)

IF(NOT GETTEXT_DEVEL_INCLUDE_DIR)
	SET(GETTEXT_DEVEL_FOUND FALSE)
ENDIF(NOT GETTEXT_DEVEL_INCLUDE_DIR)

# Check gettext function
check_function_exists(gettext HAVE_GETTEXT)

IF(GETTEXT_DEVEL_FOUND)
	IF(NOT Gettext-devel_FIND_QUIETLY)
		MESSAGE(STATUS "Found Gettext-devel")
	ENDIF(NOT Gettext-devel_FIND_QUIETLY)
ELSE(GETTEXT_DEVEL_FOUND)
	IF(Gettext-devel_FIND_REQUIRED)
		MESSAGE(FATAL_ERROR "Could NOT find Gettext-devel. Please install gettext-devel package")
	ENDIF(Gettext-devel_FIND_REQUIRED)
ENDIF(GETTEXT_DEVEL_FOUND)
