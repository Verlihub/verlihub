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

SET(PLUG_LUA_VERSION_MAJOR 1)
SET(PLUG_LUA_VERSION_MINOR 8)
SET(PLUG_LUA_VERSION_PATCH 0)
SET(PLUG_LUA_VERSION_TWEAK 1)
SET(PLUG_LUA_VERSION "${PLUG_LUA_VERSION_MAJOR}.${PLUG_LUA_VERSION_MINOR}.${PLUG_LUA_VERSION_PATCH}.${PLUG_LUA_VERSION_TWEAK}")

ADD_DEFINITIONS(-DLUA_PI_VERSION=\"${PLUG_LUA_VERSION}\")
INCLUDE_DIRECTORIES(${LUA_INCLUDE_DIR})

SET(LUA_HDRS
	callbacks.h
	cconsole.h
	cluainterpreter.h
	cpilua.h
)

SET(LUA_SRCS
	callbacks.cpp
	cconsole.cpp
	cluainterpreter.cpp
	cpilua.cpp
)

ADD_LIBRARY(liblua_pi MODULE ${LUA_SRCS})
SET_TARGET_PROPERTIES(liblua_pi PROPERTIES OUTPUT_NAME "lua_pi")
TARGET_LINK_LIBRARIES(liblua_pi ${LUA_LIBRARIES} libverlihub)

INSTALL(TARGETS liblua_pi LIBRARY DESTINATION ${PLUGINDIR})
