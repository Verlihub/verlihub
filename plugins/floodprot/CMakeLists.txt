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

SET(FLOODPROT_VERSION_MAJOR 1)
SET(FLOODPROT_VERSION_MINOR 5)
SET(FLOODPROT_VERSION_PATCH 0)
SET(FLOODPROT_VERSION_TWEAK 0)
SET(FLOODPROT_VERSION "${FLOODPROT_VERSION_MAJOR}.${FLOODPROT_VERSION_MINOR}.${FLOODPROT_VERSION_PATCH}.${FLOODPROT_VERSION_TWEAK}")

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
add_definitions(-DFLOODPROT_VERSION="${FLOODPROT_VERSION}")

SET(FLOODPROT_HDRS
	cfloodprotect.h
	cpifloodprot.h
)

SET(FLOODPROT_SRCS
	cfloodprotect.cpp
	cpifloodprot.cpp
)

ADD_LIBRARY(libfloodprot_pi MODULE ${FLOODPROT_SRCS})
SET_TARGET_PROPERTIES(libfloodprot_pi PROPERTIES OUTPUT_NAME "floodprot_pi")
TARGET_LINK_LIBRARIES(libfloodprot_pi libverlihub)

INSTALL(TARGETS libfloodprot_pi LIBRARY DESTINATION ${PLUGINDIR})
