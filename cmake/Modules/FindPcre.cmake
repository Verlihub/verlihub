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

IF(PCRE_INCLUDE_DIR AND PCRE_LIBRARIES)
	SET(PCRE_FIND_QUIETLY TRUE)
ENDIF(PCRE_INCLUDE_DIR AND PCRE_LIBRARIES)

FIND_PATH(PCRE_INCLUDE_DIR pcre.h
	/usr/include
	/usr/local/include
)

FIND_LIBRARY(PCRE_PCRE_LIBRARY NAMES pcre
	PATHS
		/usr/lib
		/usr/local/lib
)

FIND_LIBRARY(PCRE_PCREPOSIX_LIBRARY NAMES pcreposix
	PATHS
		/usr/lib
		/usr/local/lib
)

set(PCRE_LIBRARIES ${PCRE_PCRE_LIBRARY} ${PCRE_PCREPOSIX_LIBRARY} CACHE STRING "The libraries needed to use PCRE")

IF(PCRE_INCLUDE_DIR AND PCRE_LIBRARIES)
	SET(PCRE_FOUND TRUE)
ENDIF(PCRE_INCLUDE_DIR AND PCRE_LIBRARIES)

IF(PCRE_FOUND)
	IF(NOT Pcre_FIND_QUIETLY)
		MESSAGE(STATUS "[ OK ] Found PCRE: ${PCRE_INCLUDE_DIR}, ${PCRE_LIBRARIES}")
	ENDIF(NOT Pcre_FIND_QUIETLY)
ELSE(PCRE_FOUND)
	IF(Pcre_FIND_REQUIRED)
		MESSAGE(FATAL_ERROR "[ ER ] PCRE headers or libraries not found, please install them via your package manager or compile from source: https://ftp.pcre.org/pub/pcre/")
	ENDIF(Pcre_FIND_REQUIRED)
ENDIF(PCRE_FOUND)

MARK_AS_ADVANCED(PCRE_INCLUDE_DIR PCRE_LIBRARIES PCRE_PCREPOSIX_LIBRARY PCRE_PCRE_LIBRARY)
