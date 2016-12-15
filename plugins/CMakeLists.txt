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

# Plugman plugin

OPTION(WITH_PLUGINS "Whether to build any plugins or not" TRUE)
IF(WITH_PLUGINS)

SET(ENABLE_PLUGIN TRUE)

ADD_PLUGIN(ENABLE_PLUGIN plugman)

# Lua plugin

OPTION(WITH_LUA "Whether to build the lua plugin or not" TRUE)
IF(WITH_LUA)
	Find_Package(Lua52) # Lua 5.2
	#Find_Package(LuaSocket)

	IF(LUA52_FOUND)
		ADD_PLUGIN(LUA52_FOUND lua)
	ELSE(LUA52_FOUND)
		Find_Package(Lua51) # Lua 5.1
		#Find_Package(LuaSocket)
		ADD_PLUGIN(LUA51_FOUND lua)
	ENDIF(LUA52_FOUND)
ELSE(WITH_LUA)
	MESSAGE(STATUS "---- Cannot build lua plugin (user specified -DWITH_LUA=OFF)")
ENDIF(WITH_LUA)

# Python plugin

OPTION(WITH_PYTHON "Whether to build the python plugin or not" TRUE)
IF(WITH_PYTHON)
	SET(Python_ADDITIONAL_VERSIONS 2.7 2.6 2.5 2.4)
	Find_Package(PythonLibs)
	ADD_PLUGIN(PYTHONLIBS_FOUND python)
ELSE(WITH_PYTHON)
	MESSAGE(STATUS "---- Cannot build python plugin (user specified -DWITH_PYTHON=OFF)")
ENDIF(WITH_PYTHON)

# Perl plugin

OPTION(WITH_PERL "Whether to build the perl plugin or not" TRUE)
IF(WITH_PERL)
	Find_Package(PerlLibs)
	# Workaround for Debian: libperl-dev required
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
	MESSAGE(STATUS "---- Cannot build perl plugin (user specified -DWITH_PERL=OFF)")
ENDIF(WITH_PERL)

# Other plugins

ADD_PLUGIN(ENABLE_PLUGIN forbid)

ADD_PLUGIN(ENABLE_PLUGIN chatroom)

ADD_PLUGIN(ENABLE_PLUGIN iplog)

ADD_PLUGIN(ENABLE_PLUGIN isp)

ADD_PLUGIN(ENABLE_PLUGIN messenger)

ADD_PLUGIN(ENABLE_PLUGIN stats)

ADD_PLUGIN(ENABLE_PLUGIN replacer)

ADD_PLUGIN(ENABLE_PLUGIN floodprot)

ELSE(WITH_PLUGINS)
	MESSAGE(STATUS "---- Will not build any plugins (user specified -DWITH_PLUGINS=OFF)")
ENDIF(WITH_PLUGINS)
