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
	MESSAGE(STATUS "Not building LUA plugin, to enable please use: -DWITH_LUA=ON")
ENDIF(WITH_LUA)

# python plugin, deprecated

OPTION(WITH_PYTHON "Whether to build PYTHON plugin or not" FALSE)
IF(WITH_PYTHON)
	SET(Python_ADDITIONAL_VERSIONS 2.7)
	Find_Package(PythonLibs 2.7)
	ADD_PLUGIN(PYTHONLIBS_FOUND python)
ELSE(WITH_PYTHON)
	MESSAGE(STATUS "Not building PYTHON plugin because DEPRECATED, to enable please use: -DWITH_PYTHON=ON")
ENDIF(WITH_PYTHON)

# perl plugin, deprecated

OPTION(WITH_PERL "Whether to build PERL plugin or not" FALSE)
IF(WITH_PERL)
	Find_Package(PerlLibs)

	IF(EXISTS "/etc/debian_version")
		Find_Library(PERLLIBS_SO perl)

		IF(PERLLIBS_FOUND AND PERLLIBS_SO)
			SET(PERLLIBS_FOUND TRUE)
		ELSE()
			SET(PERLLIBS_FOUND FALSE)
		ENDIF()
	ENDIF()

	ADD_PLUGIN(PERLLIBS_FOUND perl)
ELSE(WITH_PERL)
	MESSAGE(STATUS "Not building PERL plugin because DEPRECATED, to enable please use: -DWITH_PERL=ON")
ENDIF(WITH_PERL)

# other plugins, deprecated

OPTION(WITH_FORBID "Whether to build FORBID plugin or not" FALSE)
IF(WITH_FORBID)
	ADD_PLUGIN(ENABLE_PLUGIN forbid)
ELSE(WITH_FORBID)
	MESSAGE(STATUS "Not building FORBID plugin because DEPRECATED, to enable please use: -DWITH_FORBID=ON")
ENDIF(WITH_FORBID)

OPTION(WITH_CHATROOM "Whether to build CHATROOM plugin or not" FALSE)
IF(WITH_CHATROOM)
	ADD_PLUGIN(ENABLE_PLUGIN chatroom)
ELSE(WITH_CHATROOM)
	MESSAGE(STATUS "Not building CHATROOM plugin because DEPRECATED, to enable please use: -DWITH_CHATROOM=ON")
ENDIF(WITH_CHATROOM)

OPTION(WITH_IPLOG "Whether to build IPLOG plugin or not" FALSE)
IF(WITH_IPLOG)
	ADD_PLUGIN(ENABLE_PLUGIN iplog)
ELSE(WITH_IPLOG)
	MESSAGE(STATUS "Not building IPLOG plugin because DEPRECATED, to enable please use: -DWITH_IPLOG=ON")
ENDIF(WITH_IPLOG)

OPTION(WITH_ISP "Whether to build ISP plugin or not" FALSE)
IF(WITH_ISP)
	ADD_PLUGIN(ENABLE_PLUGIN isp)
ELSE(WITH_ISP)
	MESSAGE(STATUS "Not building ISP plugin because DEPRECATED, to enable please use: -DWITH_ISP=ON")
ENDIF(WITH_ISP)

OPTION(WITH_MESSENGER "Whether to build MESSENGER plugin or not" FALSE)
IF(WITH_MESSENGER)
	ADD_PLUGIN(ENABLE_PLUGIN messenger)
ELSE(WITH_MESSENGER)
	MESSAGE(STATUS "Not building MESSENGER plugin because DEPRECATED, to enable please use: -DWITH_MESSENGER=ON")
ENDIF(WITH_MESSENGER)

OPTION(WITH_STATS "Whether to build STATS plugin or not" FALSE)
IF(WITH_STATS)
	ADD_PLUGIN(ENABLE_PLUGIN stats)
ELSE(WITH_STATS)
	MESSAGE(STATUS "Not building STATS plugin because DEPRECATED, to enable please use: -DWITH_STATS=ON")
ENDIF(WITH_STATS)

OPTION(WITH_REPLACER "Whether to build REPLACER plugin or not" FALSE)
IF(WITH_REPLACER)
	ADD_PLUGIN(ENABLE_PLUGIN replacer)
ELSE(WITH_REPLACER)
	MESSAGE(STATUS "Not building REPLACER plugin because DEPRECATED, to enable please use: -DWITH_REPLACER=ON")
ENDIF(WITH_REPLACER)

OPTION(WITH_FLOODPROT "Whether to build FLOODPROT plugin or not" FALSE)
IF(WITH_FLOODPROT)
	ADD_PLUGIN(ENABLE_PLUGIN floodprot)
ELSE(WITH_FLOODPROT)
	MESSAGE(STATUS "Not building FLOODPROT plugin because DEPRECATED, to enable please use: -DWITH_FLOODPROT=ON")
ENDIF(WITH_FLOODPROT)

ELSE(WITH_PLUGINS)
	MESSAGE(STATUS "Will not build any plugins, to enable please use: -DWITH_PLUGINS=ON")
ENDIF(WITH_PLUGINS)
