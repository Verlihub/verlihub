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

MACRO(ADD_PLUGIN _var _name) # _src
	SET(_src ${ARGV2})

	IF(NOT ${_src} MATCHES ".*")
		SET(_src ${_name})
	ENDIF(NOT ${_src} MATCHES ".*")

	STRING(TOUPPER ${_name} PLUGIN_OPTION)
	OPTION(WITH_${PLUGIN_OPTION} "Build ${PLUGIN_OPTION} plugin?" TRUE)

	IF(${_var} AND WITH_${PLUGIN_OPTION})
		MESSAGE(STATUS "[ OK ] Building ${PLUGIN_OPTION} plugin")
		ADD_SUBDIRECTORY(${_src})
		MATH(EXPR OK_PLUGINS_COUNT "${OK_PLUGINS_COUNT} + 1")
		SET(OK_PLUGINS_COUNT "${OK_PLUGINS_COUNT}" PARENT_SCOPE)
	ELSEIF(WITH_${PLUGIN_OPTION})
		MESSAGE(STATUS "[ ER ] Not building ${PLUGIN_OPTION} plugin due to unsatisfied dependencies")
	ELSE(${_var} AND WITH_${PLUGIN_OPTION})
		MESSAGE(STATUS "[ OK ] Not building ${PLUGIN_OPTION} plugin due to user specification: -DWITH_${PLUGIN_OPTION}=OFF")
	ENDIF(${_var} AND WITH_${PLUGIN_OPTION})
ENDMACRO(ADD_PLUGIN)
