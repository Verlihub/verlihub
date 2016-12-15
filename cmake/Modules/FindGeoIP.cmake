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

# - Find GeoIP
# Find the GeoIP includes and library
# This module defines
#  GEOIP_INCLUDE_DIR, where to find geoip.h
#  GEOIP_LIBRARIES, the libraries needed to use GeoIP.
#  GEOIP_FOUND, If false, do not try to use GeoIP.

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
