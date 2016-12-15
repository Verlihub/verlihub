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

SET(STATS_VERSION_MAJOR 1)
SET(STATS_VERSION_MINOR 3)
SET(STATS_VERSION_PATCH 0)
SET(STATS_VERSION_TWEAK 0)
SET(STATS_VERSION "${STATS_VERSION_MAJOR}.${STATS_VERSION_MINOR}.${STATS_VERSION_PATCH}.${STATS_VERSION_TWEAK}")

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
ADD_DEFINITIONS(-DSTATS_VERSION="${STATS_VERSION}")

SET(STATS_HDRS
	cpipisg.h
)

SET(STATS_SRCS
	cpipisg.cpp
)

ADD_LIBRARY(libpisg_pi MODULE ${STATS_SRCS})
SET_TARGET_PROPERTIES(libpisg_pi PROPERTIES OUTPUT_NAME "pisg_pi")

INSTALL(TARGETS libpisg_pi LIBRARY DESTINATION lib)
