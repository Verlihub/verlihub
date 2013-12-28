# - Find where dlopen and friends are located.
# DL_FOUND - system has dynamic linking interface available
# DL_INCLUDE_DIR - where dlfcn.h is located.
# DL_LIBRARIES - libraries needed to use dlopen

INCLUDE(CheckFunctionExists)

IF(DL_INCLUDE_DIR AND DL_LIBRARIES)
	SET(DL_FOUND TRUE)
ELSE(DL_INCLUDE_DIR AND DL_LIBRARIES)
	FIND_PATH(DL_INCLUDE_DIR NAMES dlfcn.h)
	FIND_LIBRARY(DL_LIBRARIES NAMES dl)
	IF(DL_LIBRARIES)
		SET(DL_FOUND TRUE)
	ELSE(DL_LIBRARIES)
		check_function_exists(dlopen DL_FOUND)
		# If dlopen can be found without linking in dl then dlopen is part
		# of libc, so don't need to link extra libs.
		SET(DL_LIBRARIES "")
	ENDIF(DL_LIBRARIES)
ENDIF(DL_INCLUDE_DIR AND DL_LIBRARIES)

IF(DL_FOUND)
	IF(NOT DL_FIND_QUIETLY)
		MESSAGE(STATUS "Found dlopen")
	ENDIF(NOT DL_FIND_QUIETLY)
ELSE(DL_FOUND)
	IF(DL_FIND_REQUIRED)
		MESSAGE(FATAL_ERROR "dlopen not found.")
	ENDIF(DL_FIND_REQUIRED)
ENDIF(DL_FOUND)

mark_as_advanced(DL_LIBRARIES)
