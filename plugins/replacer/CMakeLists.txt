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

SET(REPLACER_VERSION_MAJOR 1)
SET(REPLACER_VERSION_MINOR 3)
SET(REPLACER_VERSION_PATCH 0)
SET(REPLACER_VERSION_TWEAK 0)
SET(REPLACER_VERSION "${REPLACER_VERSION_MAJOR}.${REPLACER_VERSION_MINOR}.${REPLACER_VERSION_PATCH}.${REPLACER_VERSION_TWEAK}")

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
ADD_DEFINITIONS(-DREPLACER_VERSION="${REPLACER_VERSION}")

SET(REPLACER_HDRS
	cconsole.h
	cpireplace.h
	creplacer.h
	creplacerworker.h
)

SET(REPLACER_SRCS
	cconsole.cpp
	cpireplace.cpp
	creplacer.cpp
	creplacerworker.cpp
)

ADD_LIBRARY(libreplacer_pi MODULE ${REPLACER_SRCS})
SET_TARGET_PROPERTIES(libreplacer_pi PROPERTIES OUTPUT_NAME "replacer_pi")
TARGET_LINK_LIBRARIES(libreplacer_pi libverlihub)

INSTALL(TARGETS libreplacer_pi LIBRARY DESTINATION ${PLUGINDIR})
