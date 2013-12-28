# - Find Gettext-devel
# Find the Gettext includes and library
# This module defines
#  GETTEXT_DEVEL_INCLUDE_DIR, where to find geoip.h
#  GETTEXT_ASPRINTF_LIBRARY, asprintf library.
#  GETTEXT_DEVEL_FOUND, If false, do not try to use GeoIP.
#

SET(GETTEXT_DEVEL_FOUND TRUE)

find_path(GETTEXT_DEVEL_INCLUDE_DIR libintl.h
	/usr/local/include
	/usr/include
	DOC "Path to gettext include directory"
)

MARK_AS_ADVANCED(GETTEXT_DEVEL_INCLUDE_DIR)

IF(NOT GETTEXT_DEVEL_INCLUDE_DIR)
	SET(GETTEXT_DEVEL_FOUND FALSE)
ENDIF(NOT GETTEXT_DEVEL_INCLUDE_DIR)

# Find asprintf

FIND_LIBRARY(GETTEXT_ASPRINTF_LIBRARY
	asprintf
	/usr/lib
	/usr/local/lib
	DOC "Gettext asprintf library"
)

MARK_AS_ADVANCED(GETTEXT_ASPRINTF_LIBRARY)

IF(NOT GETTEXT_ASPRINTF_LIBRARY)
	SET(GETTEXT_DEVEL_FOUND 0)
ENDIF(NOT GETTEXT_ASPRINTF_LIBRARY)
 
# Check gettext function
check_function_exists(gettext HAVE_GETTEXT)

IF(GETTEXT_DEVEL_FOUND)
	IF(NOT Gettext-devel_FIND_QUIETLY)
		MESSAGE(STATUS "Found Gettext-devel")
	ENDIF(NOT Gettext-devel_FIND_QUIETLY)
ELSE(GETTEXT_DEVEL_FOUND)
	IF(Gettext-devel_FIND_REQUIRED)
		MESSAGE(FATAL_ERROR "Could NOT find Gettext-devel. Please install gettext-devel package")
	ENDIF(Gettext-devel_FIND_REQUIRED)
ENDIF(GETTEXT_DEVEL_FOUND)
