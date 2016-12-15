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

SET(CHATROOM_VERSION_MAJOR 1)
SET(CHATROOM_VERSION_MINOR 2)
SET(CHATROOM_VERSION_PATCH 0)
SET(CHATROOM_VERSION_TWEAK 0)
SET(CHATROOM_VERSION "${CHATROOM_VERSION_MAJOR}.${CHATROOM_VERSION_MINOR}.${CHATROOM_VERSION_PATCH}.${CHATROOM_VERSION_TWEAK}")

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
add_definitions(-DCHATROOM_VERSION="${CHATROOM_VERSION}")

SET(CHATROOM_HDRS
	cconsole.h
	crooms.h
	cpichatroom.h
)

SET(CHATROOM_SRCS
	cconsole.cpp
	crooms.cpp
	cpichatroom.cpp
)

ADD_LIBRARY(libchatroom_pi MODULE ${CHATROOM_SRCS})
SET_TARGET_PROPERTIES(libchatroom_pi PROPERTIES OUTPUT_NAME "chatroom_pi")
TARGET_LINK_LIBRARIES(libchatroom_pi libverlihub)

INSTALL(TARGETS libchatroom_pi LIBRARY DESTINATION ${PLUGINDIR})
