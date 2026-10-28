#	Copyright (C) 2003-2005 Daniel Muller, dan at verliba dot cz
#	Copyright (C) 2006-2026 Verlihub Team, info at verlihub dot net
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

IF(PCRE2_INCLUDE_DIR AND PCRE2_LIBRARIES)
	SET(PCRE2_FIND_QUIETLY TRUE)
ENDIF(PCRE2_INCLUDE_DIR AND PCRE2_LIBRARIES)

FIND_PATH(PCRE2_INCLUDE_DIR pcre2.h
	/usr/include
	/usr/local/include
)

FIND_LIBRARY(PCRE2_8_LIBRARY NAMES pcre2-8 # note: library for 8 bit strings, verlihub uses only this one
	PATHS
		/usr/lib
		/usr/local/lib
)

set(PCRE2_LIBRARIES ${PCRE2_8_LIBRARY} CACHE STRING "The libraries needed to use PCRE2")

IF(PCRE2_INCLUDE_DIR AND PCRE2_8_LIBRARY)
	SET(PCRE2_FOUND TRUE)
ENDIF(PCRE2_INCLUDE_DIR AND PCRE2_8_LIBRARY)

IF(PCRE2_FOUND)
	IF(NOT Pcre2_FIND_QUIETLY)
		MESSAGE(STATUS "[ OK ] Found PCRE2: ${PCRE2_INCLUDE_DIR}, ${PCRE2_LIBRARIES}")
	ENDIF(NOT Pcre2_FIND_QUIETLY)
ELSE(PCRE2_FOUND)
	IF(Pcre2_FIND_REQUIRED)
		MESSAGE(FATAL_ERROR "[ ER ] PCRE2 headers or libraries not found, please install them via your package manager or compile from source: https://github.com/PCRE2Project/pcre2")
	ENDIF(Pcre2_FIND_REQUIRED)
ENDIF(PCRE2_FOUND)

MARK_AS_ADVANCED(PCRE2_INCLUDE_DIR PCRE2_LIBRARIES PCRE2_8_LIBRARY)
