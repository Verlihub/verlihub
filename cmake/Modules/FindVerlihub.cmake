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

SET(VERLIHUB_FOUND 0)

find_program(VERLIHUB_CONFIG verlihub_config
	/usr/local/verlihub/bin/
	/usr/local/bin/
	/usr/bin/
)

if(VERLIHUB_CONFIG)
	IF(NOT Verlihub_FIND_QUIETLY)
		MESSAGE(STATUS "[ OK ] Found Verlihub configuration: ${VERLIHUB_CONFIG}")
	ENDIF(NOT Verlihub_FIND_QUIETLY)

	exec_program(${VERLIHUB_CONFIG} ARGS --include OUTPUT_VARIABLE MY_TMP) # get include directories
	string(REGEX REPLACE "-I([^ ]*) ?" "\\1;" MY_TMP "${MY_TMP}")
	SET(VERLIHUB_ADD_INCLUDE_PATH ${MY_TMP} CACHE "Verlihub include paths" PATH)

	exec_program(${VERLIHUB_CONFIG} ARGS --libs OUTPUT_VARIABLE MY_TMP) # get libraries
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

	exec_program(${VERLIHUB_CONFIG} ARGS --version OUTPUT_VARIABLE MY_TMP) # get verlihub version
	string(REGEX MATCH "(([0-9]+)\\.([0-9]+)\\.([0-9]+)\\.([0-9]+))" MY_TMP "${MY_TMP}")

	SET(VERLIHUB_VERSION_MAJOR ${CMAKE_MATCH_2})
	SET(VERLIHUB_VERSION_MINOR ${CMAKE_MATCH_3})
	SET(VERLIHUB_VERSION_PATCH ${CMAKE_MATCH_4})
	SET(VERLIHUB_VERSION_TWEAK ${CMAKE_MATCH_5})
	SET(VERLIHUB_VERSION_STRING ${MY_TMP})
	MESSAGE(STATUS "[ OK ] Found Verlihub version: ${VERLIHUB_VERSION_STRING}")

	exec_program(${VERLIHUB_CONFIG} ARGS --plugins OUTPUT_VARIABLE VERLIHUB_PLUGIN_DIR) # get plugins directory
else(VERLIHUB_CONFIG)
	IF(Verlihub_FIND_REQUIRED)
		MESSAGE(FATAL_ERROR "[ ER ] Verlihub configuration not found, please configure it or specify path: -DWITH_VERLIHUB=/path/to/verlihub")
	ENDIF(Verlihub_FIND_REQUIRED)
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
		"Specify directory containing cserverdc.h"
)

foreach(LIB ${VERLIHUB_ADD_LIBRARIES})
	find_library(FOUND${LIB} ${LIB}
		PATHS
			${VERLIHUB_ADD_LIBRARIES_PATH}
			/usr/lib
			/usr/lib/verlihub
			/usr/local/lib
			/usr/local/lib/verlihub
			/usr/local/verlihub/lib
		DOC
			"Specify location of Verlihub libraries"
	)

	IF(FOUND${LIB})
		LIST(APPEND VERLIHUB_LIBRARIES ${FOUND${LIB}})
	ELSE(FOUND${LIB})
		IF(Verlihub_FIND_REQUIRED)
			MESSAGE(FATAL_ERROR "[ ER ] ${LIB} library not found, please compile it from source.")
		ENDIF(Verlihub_FIND_REQUIRED)
	ENDIF(FOUND${LIB})
endforeach(LIB ${VERLIHUB_ADD_LIBRARIES})

IF(VERLIHUB_INCLUDE_DIR AND VERLIHUB_LIBRARIES)
	SET(VERLIHUB_FOUND TRUE)
ELSE(VERLIHUB_INCLUDE_DIR AND VERLIHUB_LIBRARIES)
	SET(VERLIHUB_FOUND FALSE)
ENDIF(VERLIHUB_INCLUDE_DIR AND VERLIHUB_LIBRARIES)

IF(VERLIHUB_FOUND)
	IF(NOT Verlihub_FIND_QUIETLY)
		MESSAGE(STATUS "[ OK ] Found Verlihub: ${VERLIHUB_INCLUDE_DIR}, ${VERLIHUB_LIBRARIES}")
	ENDIF(NOT Verlihub_FIND_QUIETLY)
ELSE(VERLIHUB_FOUND)
	IF(Verlihub_FIND_REQUIRED)
		MESSAGE(FATAL_ERROR "[ ER ] Verlihub headers or libraries not found, please compile them from source.")
	ENDIF(Verlihub_FIND_REQUIRED)
ENDIF(VERLIHUB_FOUND)

mark_as_advanced(VERLIHUB_INCLUDE_DIR VERLIHUB_LIBRARIES)
