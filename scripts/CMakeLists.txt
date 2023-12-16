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

SET(SCRIPT_FILES
	vh_daemon
	vh_lib
	vh_gui
	vh
	vhm
	vh_regimporter
	vh_migration_0.9.8eto1.0
)

# Autotool variables that will be replaced
SET(prefix ${CMAKE_INSTALL_PREFIX})
SET(exec_prefix ${CMAKE_INSTALL_PREFIX})
SET(libdir ${CMAKE_INSTALL_PREFIX}/${LIBDIR})
SET(plugindir ${CMAKE_INSTALL_PREFIX}/${PLUGINDIR})
SET(bindir ${CMAKE_INSTALL_PREFIX}/bin)
SET(datadir ${CMAKE_INSTALL_PREFIX}/share/${PROJECT_NAME})

FOREACH(VH_SCRIPT ${SCRIPT_FILES})
	MESSAGE(STATUS "Generating ${VH_SCRIPT}")
	CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/scripts/${VH_SCRIPT}.in ${CMAKE_BINARY_DIR}/scripts/${VH_SCRIPT} IMMEDIATE @ONLY)
	LIST(APPEND GENERATED_SCRIPTS ${VERLIHUB_BINARY_DIR}/scripts/${VH_SCRIPT})
ENDFOREACH(VH_SCRIPT ${SCRIPT_FILES})

INSTALL(
	FILES ${GENERATED_SCRIPTS}
	DESTINATION bin
	PERMISSIONS
		OWNER_READ OWNER_EXECUTE
		GROUP_READ GROUP_EXECUTE
	WORLD_READ WORLD_EXECUTE
)
