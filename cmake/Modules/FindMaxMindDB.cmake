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

IF(MAXMINDDB_INCLUDE_DIR AND MAXMINDDB_LIBRARIES)
	SET(MAXMINDDB_FIND_QUIETLY TRUE)
ENDIF(MAXMINDDB_INCLUDE_DIR AND MAXMINDDB_LIBRARIES)

find_path(MAXMINDDB_INCLUDE_DIR maxminddb.h
	/usr/include
	/usr/local/include
)

find_library(MAXMINDDB_LIBRARIES
	NAMES
		maxminddb
		libmaxminddb
	PATHS
		/usr/lib
		/usr/local/lib
)

IF(MAXMINDDB_INCLUDE_DIR AND MAXMINDDB_LIBRARIES)
	SET(MAXMINDDB_FOUND TRUE)
ELSE(MAXMINDDB_INCLUDE_DIR AND MAXMINDDB_LIBRARIES)
	SET(MAXMINDDB_FOUND FALSE)
ENDIF(MAXMINDDB_INCLUDE_DIR AND MAXMINDDB_LIBRARIES)

IF(MAXMINDDB_FOUND)
	IF(NOT MAXMINDDB_FIND_QUIETLY)
		MESSAGE(STATUS "[ OK ] Found MaxMindDB: ${MAXMINDDB_INCLUDE_DIR}, ${MAXMINDDB_LIBRARIES}")
	ENDIF(NOT MAXMINDDB_FIND_QUIETLY)
ELSE(MAXMINDDB_FOUND)
	IF(MaxMindDB_FIND_REQUIRED)
		MESSAGE(FATAL_ERROR "[ ER ] MaxMindDB headers or libraries not found, please install them via your package manager or compile from source: https://github.com/maxmind/libmaxminddb/")
	ENDIF(MaxMindDB_FIND_REQUIRED)
ENDIF(MAXMINDDB_FOUND)

mark_as_advanced(MAXMINDDB_INCLUDE_DIR MAXMINDDB_LIBRARIES)
