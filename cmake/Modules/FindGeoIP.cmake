# - Find GeoIP
# Find the GeoIP includes and library
# This module defines
#  GEOIP_INCLUDE_DIR, where to find geoip.h
#  GEOIP_LIBRARIES, the libraries needed to use GeoIP.
#  GEOIP_FOUND, If false, do not try to use GeoIP.
#
	
IF(GEOIP_INCLUDE_DIR AND GEOIP_LIBRARIES)
	# Already in cache, be silent
	SET(GeoIP_FIND_QUIETLY TRUE)
ENDIF(GEOIP_INCLUDE_DIR AND GEOIP_LIBRARIES)

find_path(GEOIP_INCLUDE_DIR GeoIP.h /usr/include /usr/local/include)
find_library(GEOIP_LIBRARIES NAMES GeoIP PATHS
	/usr/lib
	/usr/local/lib
)

IF(GEOIP_INCLUDE_DIR AND GEOIP_LIBRARIES)
	SET(GEOIP_FOUND TRUE)
ELSE(GEOIP_INCLUDE_DIR AND GEOIP_LIBRARIES)
	SET(GEOIP_FOUND FALSE)
ENDIF(GEOIP_INCLUDE_DIR AND GEOIP_LIBRARIES)

IF(GEOIP_FOUND)
	IF(NOT GeoIP_FIND_QUIETLY)
		MESSAGE(STATUS "Found GeoIP: ${GEOIP_INCLUDE_DIR}, ${GEOIP_LIBRARIES}")
	ENDIF(NOT GeoIP_FIND_QUIETLY)
ELSE(GEOIP_FOUND)
	IF(GeoIP_FIND_REQUIRED)
		MESSAGE(FATAL_ERROR "GeoIP not found. Please download and install it http://www.maxmind.com/download/geoip/api/c/")
	ENDIF(GeoIP_FIND_REQUIRED)
ENDIF(GEOIP_FOUND)

mark_as_advanced(GEOIP_INCLUDE_DIR GEOIP_LIBRARIES)
