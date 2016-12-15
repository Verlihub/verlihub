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

SET(FORBID_VERSION_MAJOR 1)
SET(FORBID_VERSION_MINOR 4)
SET(FORBID_VERSION_PATCH 0)
SET(FORBID_VERSION_TWEAK 0)
SET(FORBID_VERSION "${FORBID_VERSION_MAJOR}.${FORBID_VERSION_MINOR}.${FORBID_VERSION_PATCH}.${FORBID_VERSION_TWEAK}")

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
ADD_DEFINITIONS(-DFORBID_VERSION="${FORBID_VERSION}")

SET(FORBID_HDRS
	cconsole.h
	cforbidden.h
	cpiforbid.h
)

SET(FORBID_SRCS
	cconsole.cpp
	cforbidden.cpp
	cpiforbid.cpp
)

ADD_LIBRARY(libforbid_pi MODULE ${FORBID_SRCS})
SET_TARGET_PROPERTIES(libforbid_pi PROPERTIES OUTPUT_NAME "forbid_pi")
TARGET_LINK_LIBRARIES(libforbid_pi libverlihub)

INSTALL(TARGETS libforbid_pi LIBRARY DESTINATION ${PLUGINDIR})
