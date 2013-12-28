# - Find libcrypt
# Find libcrypt that is provided by GNU C Library
# This module defines
#  CRYPT_LIBRARIES, the libraries needed to use Crypt.
#  CRYPT_FOUND, If false, do not try to use Crypt.
#
	
IF(CRYPT_LIBRARIES)
	SET(CRYPT_FOUND TRUE)
ELSE(CRYPT_LIBRARIES)
	IF(HAVE_OPENBSD)
		find_library(CRYPT_LIBRARIES NAMES gcrypt PATHS
			/usr/lib
			/usr/local/lib
		)
	ELSE(HAVE_OPENBSD)
		find_library(CRYPT_LIBRARIES NAMES crypt PATHS
			/usr/lib
			/usr/local/lib
		)
	ENDIF(HAVE_OPENBSD)

	IF(CRYPT_LIBRARIES)
		SET(CRYPT_FOUND TRUE)
		MESSAGE(STATUS "Found Crypt: ${CRYPT_LIBRARIES}")
	ELSE(CRYPT_LIBRARIES)
		SET(CRYPT_FOUND FALSE)
	ENDIF(CRYPT_LIBRARIES)
	
	mark_as_advanced(CRYPT_LIBRARIES)
ENDIF(CRYPT_LIBRARIES)
