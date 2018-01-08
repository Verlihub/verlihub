#	Copyright (C) 2003-2005 Daniel Muller, dan at verliba dot cz
#	Copyright (C) 2006-2018 Verlihub Team, info at verlihub dot net
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

find_path(LUA_INCLUDE_DIR lua.h
	HINTS
		ENV LUA_DIR
	PATH_SUFFIXES
		include/lua52
		include/lua5.2
		include/lua-5.2
		include/lua
		include
	PATHS
		~/Library/Frameworks
		/Library/Frameworks
		#/usr/local
		#/usr
		/sw # Fink
		/opt/local # DarwinPorts
		/opt/csw # Blastwave
		/opt
)

find_library(LUA_LIBRARY
	NAMES
		lua52
		lua5.2
		lua-5.2
		lua
	HINTS
		ENV LUA_DIR
	PATH_SUFFIXES
		lib
		#lib64
	PATHS
		~/Library/Frameworks
		/Library/Frameworks
		#/usr/local
		#/usr
		/sw
		/opt/local
		/opt/csw
		/opt
)

if(LUA_LIBRARY)
	if(UNIX AND NOT APPLE AND NOT BEOS) # include the math library for unix
		find_library(LUA_MATH_LIBRARY m)
		set(LUA_LIBRARIES "${LUA_LIBRARY};${LUA_MATH_LIBRARY}" CACHE STRING "Lua Libraries")
	else() # for windows and mac dont need to explicitly include the math library
		set(LUA_LIBRARIES "${LUA_LIBRARY}" CACHE STRING "Lua Libraries")
	endif()
endif()

if(LUA_INCLUDE_DIR AND EXISTS "${LUA_INCLUDE_DIR}/lua.h")
	file(STRINGS "${LUA_INCLUDE_DIR}/lua.h" lua_version_str REGEX "^#define[ \t]+LUA_RELEASE[ \t]+\"Lua .+\"")
	string(REGEX REPLACE "^#define[ \t]+LUA_RELEASE[ \t]+\"Lua ([^\"]+)\".*" "\\1" LUA_VERSION_STRING "${lua_version_str}")
	unset(lua_version_str)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Lua52 REQUIRED_VARS LUA_LIBRARIES LUA_INCLUDE_DIR VERSION_VAR LUA_VERSION_STRING) # FOUND_VAR LUA52_FOUND
mark_as_advanced(LUA_INCLUDE_DIR LUA_LIBRARIES LUA_LIBRARY LUA_MATH_LIBRARY)
