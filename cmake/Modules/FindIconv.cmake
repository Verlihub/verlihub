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

# todo: this is no longer used, really need it? else cmake 3.16 has FindIconv.cmake

include(CheckCXXSourceCompiles)

IF(ICONV_INCLUDE_DIR AND ICONV_LIBRARIES)
	SET(ICONV_FIND_QUIETLY TRUE)
ENDIF(ICONV_INCLUDE_DIR AND ICONV_LIBRARIES)

FIND_PATH(ICONV_INCLUDE_DIR iconv.h
	/usr/include
	/usr/local/include
)

FIND_LIBRARY(ICONV_LIBRARIES
	NAMES
		iconv
		libiconv
		libiconv-2
		c
	PATHS
		/usr/include
		/usr/local/include
)

IF(ICONV_INCLUDE_DIR AND ICONV_LIBRARIES)
	SET(ICONV_FOUND TRUE)
ENDIF(ICONV_INCLUDE_DIR AND ICONV_LIBRARIES)

IF(ICONV_FOUND)
	IF(NOT ICONV_FIND_QUIETLY)
		MESSAGE(STATUS "[ OK ] Found IConv: ${ICONV_INCLUDE_DIR}, ${ICONV_LIBRARIES}")
	ENDIF(NOT ICONV_FIND_QUIETLY)
ELSE(ICONV_FOUND)
	IF(Iconv_FIND_REQUIRED)
		MESSAGE(FATAL_ERROR "[ ER ] IConv headers or libraries not found, please install them via your package manager or compile from source: https://ftp.gnu.org/pub/gnu/libiconv/")
	ENDIF(Iconv_FIND_REQUIRED)
ENDIF(ICONV_FOUND)

MARK_AS_ADVANCED(ICONV_INCLUDE_DIR ICONV_LIBRARIES)
