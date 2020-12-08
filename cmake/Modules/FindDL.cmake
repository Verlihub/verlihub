#	Copyright (C) 2003-2005 Daniel Muller, dan at verliba dot cz
#	Copyright (C) 2006-2021 Verlihub Team, info at verlihub dot net
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

INCLUDE(CheckFunctionExists)

IF(DL_INCLUDE_DIR AND DL_LIBRARIES)
	SET(DL_FOUND TRUE)
ELSE(DL_INCLUDE_DIR AND DL_LIBRARIES)
	FIND_PATH(DL_INCLUDE_DIR NAMES dlfcn.h)
	FIND_LIBRARY(DL_LIBRARIES NAMES dl)

	IF(DL_LIBRARIES)
		SET(DL_FOUND TRUE)
	ELSE(DL_LIBRARIES)
		check_function_exists(dlopen DL_FOUND) # if dlopen can be found without linking in dl then dlopen is part of libc so dont need to link extra libs
		SET(DL_LIBRARIES "")
	ENDIF(DL_LIBRARIES)
ENDIF(DL_INCLUDE_DIR AND DL_LIBRARIES)

IF(DL_FOUND)
	IF(NOT DL_FIND_QUIETLY)
		MESSAGE(STATUS "[ OK ] Found DL: ${DL_INCLUDE_DIR}, ${DL_LIBRARIES}")
	ENDIF(NOT DL_FIND_QUIETLY)
ELSE(DL_FOUND)
	IF(DL_FIND_REQUIRED)
		MESSAGE(FATAL_ERROR "[ ER ] DL ? not found, please install ? via your package manager or compile from source: ?")
	ENDIF(DL_FIND_REQUIRED)
ENDIF(DL_FOUND)

mark_as_advanced(DL_LIBRARIES)
