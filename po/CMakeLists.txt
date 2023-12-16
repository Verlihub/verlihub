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

file(STRINGS LANGS LOCALES)

find_program(MSGCAT msgcat
	/usr/local/bin
	/usr/bin
	${GETTEXT_EXEC_PATH}
	)

# Merge POT files
add_custom_command(
	OUTPUT "${CMAKE_BINARY_DIR}/po/verlihub.pot"
	COMMAND ${MSGCAT} "${CMAKE_CURRENT_SOURCE_DIR}/verlihub_core.pot" "${CMAKE_CURRENT_SOURCE_DIR}/verlihub_plugins.pot" --no-wrap --use-first --output "${CMAKE_BINARY_DIR}/po/verlihub.pot"
)
SET(POT_FILE "${CMAKE_BINARY_DIR}/po/verlihub.pot")
# Merge and compile PO files
FOREACH(LOCALE ${LOCALES})
	add_custom_command(
		OUTPUT "${CMAKE_BINARY_DIR}/po/${LOCALE}.po"
		COMMAND ${MSGCAT} "${CMAKE_CURRENT_SOURCE_DIR}/core_${LOCALE}.po" "${CMAKE_CURRENT_SOURCE_DIR}/plugins_${LOCALE}.po" --no-wrap --use-first --output "${CMAKE_BINARY_DIR}/po/${LOCALE}.po"
	)
	LIST(APPEND PO_FILES "${CMAKE_BINARY_DIR}/po/${LOCALE}.po")
ENDFOREACH(LOCALE)

GETTEXT_CREATE_TRANSLATIONS(${POT_FILE} ALL ${PO_FILES})
