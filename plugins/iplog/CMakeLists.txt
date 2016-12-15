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

SET(IPLOG_VERSION_MAJOR 1)
SET(IPLOG_VERSION_MINOR 2)
SET(IPLOG_VERSION_PATCH 0)
SET(IPLOG_VERSION_TWEAK 0)
SET(IPLOG_VERSION "${IPLOG_VERSION_MAJOR}.${IPLOG_VERSION_MINOR}.${IPLOG_VERSION_PATCH}.${IPLOG_VERSION_TWEAK}")

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
ADD_DEFINITIONS(-DIPLOG_VERSION="${IPLOG_VERSION}")

SET(IPLOG_HDRS
	cconsole.h
	ciplog.h
	cpiiplog.h
)

SET(IPLOG_SRCS
	cconsole.cpp
	ciplog.cpp
	cpiiplog.cpp
)

ADD_LIBRARY(libiplog_pi MODULE ${IPLOG_SRCS})
SET_TARGET_PROPERTIES(libiplog_pi PROPERTIES OUTPUT_NAME "iplog_pi")
TARGET_LINK_LIBRARIES(libiplog_pi libverlihub)

INSTALL(TARGETS libiplog_pi LIBRARY DESTINATION ${PLUGINDIR})
