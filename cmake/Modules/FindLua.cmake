#	Copyright (C) 2003-2005 Daniel Muller, dan at verliba dot cz
#	Copyright (C) 2006-2026 Verlihub Team, info at verlihub dot net
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

IF(LUA_LIBRARIES)
	SET(LUA_FOUND TRUE)

ELSE(LUA_LIBRARIES)
	FIND_PATH(LUA_INCLUDE_DIR
		NAMES
			lua.h
			lualib.h
		PATHS
			/usr/include
			/usr/local/include
			/usr/pkg/include
		PATH_SUFFIXES
			lua
			lua/5.5
			lua/5.4
			lua/5.3
			lua/5.2
			lua5.5
			lua5.4
			lua5.3
			lua5.2
	)

	IF(LUA_INCLUDE_DIR)
		MESSAGE(STATUS "Found Lua headers: ${LUA_INCLUDE_DIR}")
	ENDIF(LUA_INCLUDE_DIR)

	FIND_LIBRARY(LUA_LIBRARIES
		NAMES
			lua
			liblua
		PATHS
			/usr/lib
			/usr/local/lib
			/usr/pkg/lib
		PATH_SUFFIXES
			lua
			lua/5.5
			lua/5.4
			lua/5.3
			lua/5.2
			lua5.5
			lua5.4
			lua5.3
			lua5.2
	)

	IF(LUA_LIBRARIES)
		MESSAGE(STATUS "Found Lua libraries: ${LUA_LIBRARIES}")
	ENDIF(LUA_LIBRARIES)

	IF(LUA_INCLUDE_DIR AND LUA_LIBRARIES)
		SET(LUA_FOUND TRUE)
		INCLUDE(FindPackageHandleStandardArgs)
		FIND_PACKAGE_HANDLE_STANDARD_ARGS(Lua DEFAULT_MSG LUA_LIBRARIES LUA_INCLUDE_DIR)
		MARK_AS_ADVANCED(LUA_LIBRARIES LUA_INCLUDE_DIR)
		MESSAGE(STATUS "Detected Lua version: ${LUA_VERSION_STRING}")

	ELSE(LUA_INCLUDE_DIR AND LUA_LIBRARIES)
		SET(LUA_FOUND FALSE)
	ENDIF(LUA_INCLUDE_DIR AND LUA_LIBRARIES)
ENDIF(LUA_LIBRARIES)
