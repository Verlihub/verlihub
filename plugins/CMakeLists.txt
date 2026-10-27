#	Copyright (C) 2003-2005 Daniel Muller, dan at verliba dot cz
#	Copyright (C) 2006-2025 Verlihub Team, info at verlihub dot net
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
#	Please see https://www.gnu.org/licenses/ for a copy
#	of the GNU General Public License.

message(STATUS)
OPTION(WITH_PLUGINS "Whether to build any plugins or not" TRUE)

IF(WITH_PLUGINS)
SET(ENABLE_PLUGIN TRUE)

# plugman plugin, always enabled

ADD_PLUGIN(ENABLE_PLUGIN plugman)

# lua plugin

OPTION(WITH_LUA "Whether to build LUA plugin or not" TRUE)

IF(WITH_LUA)
	#SET(CMAKE_FIND_DEBUG_MODE 1) # todo: test only
	Find_Package(Lua 5.2 MODULE REQUIRED) # lua 5.2 and later, note: cmake 3.18 adds support for lua 5.4

	#IF(LUA_FOUND)
		#Find_Package(LuaSocket)
	#ENDIF(LUA_FOUND)

	ADD_PLUGIN(LUA_FOUND lua)

ELSE(WITH_LUA)
	MESSAGE(STATUS "[ !! ] NOT building LUA plugin, to enable please use: -DWITH_LUA=ON")
ENDIF(WITH_LUA)

ELSE(WITH_PLUGINS)
	MESSAGE(STATUS "[ !! ] NOT building any plugins, to enable please use: -DWITH_PLUGINS=ON")
ENDIF(WITH_PLUGINS)

message(STATUS)
