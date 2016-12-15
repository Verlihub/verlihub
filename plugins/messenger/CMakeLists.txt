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

SET(MESSENGER_VERSION_MAJOR 1)
SET(MESSENGER_VERSION_MINOR 3)
SET(MESSENGER_VERSION_PATCH 0)
SET(MESSENGER_VERSION_TWEAK 0)
SET(MESSENGER_VERSION "${MESSENGER_VERSION_MAJOR}.${MESSENGER_VERSION_MINOR}.${MESSENGER_VERSION_PATCH}.${MESSENGER_VERSION_TWEAK}")

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
ADD_DEFINITIONS(-DMESSENGER_VERSION="${MESSENGER_VERSION}")

SET(MESSENGER_HDRS
	cconsole.h
	cmsglist.h
	cpimessanger.cpp
)

SET(MESSENGER_SRCS
	cconsole.cpp
	cmsglist.cpp
	cpimessanger.cpp
)

ADD_LIBRARY(libmessenger_pi MODULE ${MESSENGER_SRCS})
SET_TARGET_PROPERTIES(libmessenger_pi PROPERTIES OUTPUT_NAME "messenger_pi")
TARGET_LINK_LIBRARIES(libmessenger_pi libverlihub)

INSTALL(TARGETS libmessenger_pi LIBRARY DESTINATION ${PLUGINDIR})
