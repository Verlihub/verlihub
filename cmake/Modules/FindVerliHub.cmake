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

# - Find Verlihub
# Find Verlihub and VerliApi libraries and includes.
# Once done this will define
#
#  VERLIHUB_INCLUDE_DIRS   - where to find cserverdc.h, etc.
#  VERLIHUB_LIBRARIES      - List of libraries when using Verlihub.
#  VERLIHUB_PLUGIN_DIR     - Where plugins must be stored
#  VERLIHUB_FOUND          - True if Verlihub found.
#
#  VERLIHUB_VERSION_STRING - The version of Verlihub found
#  VERLIHUB_VERSION_MAJOR  - The major version of Verlihub
#  VERLIHUB_VERSION_MINOR  - The minor version of Verlihub
#  VERLIHUB_VERSION_PATCH  - The patch version of Verlihub
#  VERLIHUB_VERSION_TWEAK  - The tweak version of Verlihub

SET(VERLIHUB_FOUND 0)

find_program(VERLIHUB_CONFIG verlihub_config
	/usr/local/verlihub/bin/
	/usr/local/bin/
	/usr/bin/
)

if(VERLIHUB_CONFIG)
	IF(NOT VerliHub_FIND_QUIETLY)
		MESSAGE(STATUS "Using verlihub_config: ${VERLIHUB_CONFIG}")
	ENDIF(NOT VerliHub_FIND_QUIETLY)
	# Get include directories
	exec_program(${VERLIHUB_CONFIG} ARGS --include OUTPUT_VARIABLE MY_TMP)
	string(REGEX REPLACE "-I([^ ]*) ?" "\\1;" MY_TMP "${MY_TMP}")
	SET(VERLIHUB_ADD_INCLUDE_PATH ${MY_TMP} CACHE "Verlihub include paths" PATH)

	# Get libraries
	exec_program(${VERLIHUB_CONFIG} ARGS --libs OUTPUT_VARIABLE MY_TMP )
	SET(VERLIHUB_ADD_LIBRARIES "")
	string(REGEX MATCHALL "-l[^ ]*" VERLIHUB_LIB_LIST "${MY_TMP}")
	string(REGEX REPLACE "-l([^;]*)" "\\1" VERLIHUB_ADD_LIBRARIES "${VERLIHUB_LIB_LIST}")
	#string(REGEX REPLACE ";" " " VERLIHUB_ADD_LIBRARIES "${VERLIHUB_ADD_LIBRARIES}")

	set(VERLIHUB_ADD_LIBRARIES_PATH "")
	string(REGEX MATCHALL "-L[^ ]*" VERLIHUB_LIBDIR_LIST "${MY_TMP}")
	foreach(LIB ${VERLIHUB_LIBDIR_LIST})
		string(REGEX REPLACE "[ ]*-L([^ ]*)" "\\1" LIB "${LIB}")
		list(APPEND VERLIHUB_ADD_LIBRARIES_PATH "${LIB}")
	endforeach(LIB ${VERLIHUB_LIBDIR_LIST})
	
	# Get verlihub version
	exec_program(${VERLIHUB_CONFIG} ARGS --version OUTPUT_VARIABLE MY_TMP)
	string(REGEX MATCH "(([0-9]+)\\.([0-9]+)\\.([0-9]+)\\.([0-9]+))" MY_TMP "${MY_TMP}")

	SET(VERLIHUB_VERSION_MAJOR ${CMAKE_MATCH_2})
	SET(VERLIHUB_VERSION_MINOR ${CMAKE_MATCH_3})
	SET(VERLIHUB_VERSION_PATCH ${CMAKE_MATCH_4})
	SET(VERLIHUB_VERSION_TWEAK ${CMAKE_MATCH_5})
	SET(VERLIHUB_VERSION_STRING ${MY_TMP})
	MESSAGE(STATUS "Verlihub version: ${VERLIHUB_VERSION_STRING}")

	# Get plugins directory
	exec_program(${VERLIHUB_CONFIG} ARGS --plugins OUTPUT_VARIABLE VERLIHUB_PLUGIN_DIR)

else(VERLIHUB_CONFIG)
	IF(VerliHub_FIND_REQUIRED)
		MESSAGE(FATAL_ERROR "verlihub_config not found. Please install Verlihub or specify the path to Verlihub with -DWITH_VERLIHUB=/path/to/verlihub")
	ENDIF(VerliHub_FIND_REQUIRED)
endif(VERLIHUB_CONFIG)

#LIST(REMOVE_DUPLICATES ${VERLIHUB_ADD_INCLUDE_PATH})
SET(VERLIHUB_INCLUDE_DIRS "${VERLIHUB_ADD_INCLUDE_PATH}")

find_path(VERLIHUB_INCLUDE_DIR
	NAMES
	cserverdc.h
	PATHS
	${VERLIHUB_ADD_INCLUDE_PATH}
	/usr/include
	/usr/include/verlihub
	/usr/local/include
	/usr/local/include/verlihub
	/usr/local/verlihub/include
	DOC
	"Specify the directory containing cserverdc.h."
)

foreach(LIB ${VERLIHUB_ADD_LIBRARIES})
	find_library(FOUND${LIB}
		${LIB}
		PATHS
		${VERLIHUB_ADD_LIBRARIES_PATH}
		/usr/lib
		/usr/lib/verlihub
		/usr/local/lib
		/usr/local/lib/verlihub
		/usr/local/verlihub/lib
		DOC "Specify the location of the verlihub libraries."
	)

	IF(FOUND${LIB})
		LIST(APPEND VERLIHUB_LIBRARIES ${FOUND${LIB}})
	ELSE(FOUND${LIB})
		IF(VerliHub_FIND_REQUIRED)
			MESSAGE(FATAL_ERROR "${LIB} library not found. Please install Verlihub")
		ENDIF(VerliHub_FIND_REQUIRED)
	ENDIF(FOUND${LIB})
endforeach(LIB ${VERLIHUB_ADD_LIBRARIES})

IF(VERLIHUB_INCLUDE_DIR AND VERLIHUB_LIBRARIES)
	SET(VERLIHUB_FOUND TRUE)
ELSE(VERLIHUB_INCLUDE_DIR AND VERLIHUB_LIBRARIES)
	SET(VERLIHUB_FOUND FALSE)
ENDIF(VERLIHUB_INCLUDE_DIR AND VERLIHUB_LIBRARIES)

IF(VERLIHUB_FOUND)
	IF(NOT VerliHub_FIND_QUIETLY)
		MESSAGE(STATUS "Found Verlihub: ${VERLIHUB_INCLUDE_DIR}, ${VERLIHUB_LIBRARIES}")
	ENDIF(NOT VerliHub_FIND_QUIETLY)
ELSE(VERLIHUB_FOUND)
	IF(VerliHub_FIND_REQUIRED)
		MESSAGE(FATAL_ERROR "Verlihub not found. Please install Verlihub")
	ENDIF(VerliHub_FIND_REQUIRED)
ENDIF(VERLIHUB_FOUND)

mark_as_advanced(VERLIHUB_INCLUDE_DIR VERLIHUB_LIBRARIES)
