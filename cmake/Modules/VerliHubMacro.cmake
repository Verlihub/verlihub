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
		MESSAGE(STATUS "Building ${_name} plugin")
		ADD_SUBDIRECTORY(${_src})
	ELSE(${_var} AND WITH_${PLUGIN_OPTION})
		MESSAGE(STATUS "Not building ${_name} plugin")
	ENDIF(${_var} AND WITH_${PLUGIN_OPTION})
ENDMACRO(ADD_PLUGIN)
