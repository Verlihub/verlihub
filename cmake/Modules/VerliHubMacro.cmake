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

# Add Verlihub plugin (e.g., for Verlihub.Lua). Use as:
#
#   add_plugin(pluginname [srcdir])
#
# where pluginname is the name of the plugin (e.g., Lua or Python),
# srcdir is the subdirectory containing plugin sources.

# This file defines the add plugin macro.
#
# ADD_PLUGIN(VAR PLUGIN [SRCDIR])
#   Add a plugin so it can be compiled
#   VAR : TRUE or FALSE, indicating whether the plugin must be compiled
#   PLUGIN: name of the plugin, e.g. "lua"
#   SRCDIR: source file directory

# Example:
#
# INCLUDE(VerliHubMacro)
#
# FIND_PACKAGE(LUA51)
# ADD_PLUGIN(LUA51_FOUND "lua")

MACRO(ADD_PLUGIN _var _name) # _src
	SET(_src ${ARGV2})
	IF (NOT ${_src} MATCHES ".*")
		SET(_src ${_name})
	ENDIF (NOT ${_src} MATCHES ".*")
	STRING(TOUPPER ${_name} PLUGIN_OPTION)
	
	OPTION(WITH_${PLUGIN_OPTION} "Whether to build the ${_name} plugin or not" TRUE)
	IF(${_var} AND WITH_${PLUGIN_OPTION})
		MESSAGE(STATUS "---- Will build ${_name} plugin")
		ADD_SUBDIRECTORY(${_src})
		MATH(EXPR OK_PLUGINS_COUNT "${OK_PLUGINS_COUNT} + 1")
		SET(OK_PLUGINS_COUNT "${OK_PLUGINS_COUNT}" PARENT_SCOPE)
	ELSEIF(WITH_${PLUGIN_OPTION})
		MESSAGE(STATUS "---- Cannot build ${_name} plugin (unsatisfied dependencies)")
	ELSE(${_var} AND WITH_${PLUGIN_OPTION})
		MESSAGE(STATUS "---- Cannot build ${_name} plugin (user specified -DWITH_${PLUGIN_OPTION}=OFF)")
	ENDIF(${_var} AND WITH_${PLUGIN_OPTION})
ENDMACRO(ADD_PLUGIN)
