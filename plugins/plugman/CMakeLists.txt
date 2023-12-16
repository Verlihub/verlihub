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

SET(PLUGMAN_VERSION_MAJOR 1)
SET(PLUGMAN_VERSION_MINOR 5)
SET(PLUGMAN_VERSION_PATCH 0)
SET(PLUGMAN_VERSION_TWEAK 0)
SET(PLUGMAN_VERSION "${PLUGMAN_VERSION_MAJOR}.${PLUGMAN_VERSION_MINOR}.${PLUGMAN_VERSION_PATCH}.${PLUGMAN_VERSION_TWEAK}")

INCLUDE_DIRECTORIES(${VERLIHUB_BINARY_DIR} ${VERLIHUB_SOURCE_DIR}/src)

ADD_DEFINITIONS(-DPLUGMAN_VERSION="${PLUGMAN_VERSION}")

SET(PLUGMAN_SRCS
	cconsole.cpp
	cpiplug.cpp
	cplugs.cpp
)

SET(PLUGMAN_HDRS
	cconsole.h
	cpiplug.h
	cplugs.h
)

ADD_LIBRARY(plug_pi MODULE ${PLUGMAN_SRCS})
target_link_libraries(plug_pi libverlihub)
INSTALL(TARGETS plug_pi LIBRARY DESTINATION ${PLUGINDIR})
