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

IF(CRYPT_LIBRARIES)
	SET(CRYPT_FOUND TRUE)
ELSE(CRYPT_LIBRARIES)
	IF(HAVE_OPENBSD)
		find_library(CRYPT_LIBRARIES NAMES gcrypt
			PATHS
				/usr/lib
				/usr/local/lib
		)
	ELSEIF(HAVE_APPLE)
		find_library(CRYPT_LIBRARIES NAMES gcrypt
			PATHS
				/usr/lib
				/usr/local/lib
				/usr/local/lib/libgcrypt/lib
		)
	ELSE()
		find_library(CRYPT_LIBRARIES NAMES crypt
			PATHS
				/usr/lib
				/usr/local/lib
		)
	ENDIF()

	IF(CRYPT_LIBRARIES)
		SET(CRYPT_FOUND TRUE)
		MESSAGE(STATUS "[ OK ] Found Crypt: ${CRYPT_LIBRARIES}")
	ELSE(CRYPT_LIBRARIES)
		SET(CRYPT_FOUND FALSE)
		MESSAGE(FATAL_ERROR "[ ER ] Crypt ? not found, please install ? via your package manager or compile from source: ?")
	ENDIF(CRYPT_LIBRARIES)
	
	mark_as_advanced(CRYPT_LIBRARIES)
ENDIF(CRYPT_LIBRARIES)
