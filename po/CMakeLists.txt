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

# note: translation files are utf-8, same as transifex gives them, hub converts translations to charset of its locale at runtime, for example ru_RU.CP1251
# note: musl libc of alpine can not convert at runtime, there translations are converted to codepage of their language when they are built
option(LOCALE_CONVERT "Convert translations to codepage of their language at build time, when gettext of system can not convert them at runtime" ${HAVE_ALPINE})

set(LOCALE_CODEPAGES # note: codepage of each language for LOCALE_CONVERT, language that is not here stays utf-8
	bg_BG CP1251
	cs_CZ CP1250
	de_DE CP1252
	es_ES CP1252
	fr_FR CP1252
	hu_HU CP1250
	it_IT CP1252
	nl_NL CP1252
	pl_PL CP1250
	ro_RO CP1250
	ru_RU CP1251
	sk_SK CP1250
	sv_SE CP1252
	tr_TR CP1254
	uk_UA CP1251
	zh_CN GB18030
)

find_program(MSGCAT msgcat
	/usr/local/bin
	/usr/bin
	${GETTEXT_EXEC_PATH}
	)

find_program(MSGCONV msgconv
	/usr/local/bin
	/usr/bin
	${GETTEXT_EXEC_PATH}
	)

find_program(XGETTEXT xgettext
	/usr/local/bin
	/usr/bin
	${GETTEXT_EXEC_PATH}
	)

if(LOCALE_CONVERT AND NOT MSGCONV)
	message(FATAL_ERROR "[ ER ] msgconv of gettext not found, it is required by LOCALE_CONVERT")
endif()

# merge pot files
add_custom_command(
	OUTPUT "${CMAKE_BINARY_DIR}/po/verlihub.pot"
	COMMAND ${MSGCAT} "${CMAKE_CURRENT_SOURCE_DIR}/verlihub_core.pot" "${CMAKE_CURRENT_SOURCE_DIR}/verlihub_plugins.pot" --no-wrap --use-first --output "${CMAKE_BINARY_DIR}/po/verlihub.pot"
	DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/verlihub_core.pot" "${CMAKE_CURRENT_SOURCE_DIR}/verlihub_plugins.pot"
)

set(POT_FILE "${CMAKE_BINARY_DIR}/po/verlihub.pot")
file(GLOB CORE_PO_FILES "${CMAKE_CURRENT_SOURCE_DIR}/core_*.po") # note: languages are found from files, so new language from transifex needs no change here

# merge and compile po files
foreach(CORE_PO ${CORE_PO_FILES})
	get_filename_component(LOCALE "${CORE_PO}" NAME_WE)
	string(REGEX REPLACE "^core_" "" LOCALE "${LOCALE}")
	set(LOCALE_FILES "${CORE_PO}")

	if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/plugins_${LOCALE}.po")
		list(APPEND LOCALE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/plugins_${LOCALE}.po")
	endif()

	set(LOCALE_PO "${CMAKE_BINARY_DIR}/po/${LOCALE}.po")
	set(LOCALE_CODEPAGE "")

	if(LOCALE_CONVERT)
		list(FIND LOCALE_CODEPAGES "${LOCALE}" LOCALE_INDEX)

		if(NOT LOCALE_INDEX EQUAL -1)
			math(EXPR LOCALE_INDEX "${LOCALE_INDEX} + 1")
			list(GET LOCALE_CODEPAGES ${LOCALE_INDEX} LOCALE_CODEPAGE)
		endif()
	endif()

	if(LOCALE_CODEPAGE)
		add_custom_command(
			OUTPUT "${LOCALE_PO}"
			COMMAND ${MSGCAT} ${LOCALE_FILES} --no-wrap --use-first --output "${CMAKE_BINARY_DIR}/po/${LOCALE}.utf8.po"
			COMMAND ${MSGCONV} --to-code=${LOCALE_CODEPAGE} --no-wrap --output-file "${LOCALE_PO}" "${CMAKE_BINARY_DIR}/po/${LOCALE}.utf8.po"
			DEPENDS ${LOCALE_FILES}
		)
	else()
		add_custom_command(
			OUTPUT "${LOCALE_PO}"
			COMMAND ${MSGCAT} ${LOCALE_FILES} --no-wrap --use-first --output "${LOCALE_PO}"
			DEPENDS ${LOCALE_FILES}
		)
	endif()

	list(APPEND PO_FILES "${LOCALE_PO}")
endforeach(CORE_PO)

GETTEXT_CREATE_TRANSLATIONS(${POT_FILE} ALL ${PO_FILES})

# extract translation templates from source, run it after changing texts: make pot, mingw32-make pot on windows
# note: transifex reads both templates from github and sends translations back as utf-8 po files, see transifex.yml

if(XGETTEXT)
	file(GLOB CORE_SOURCES RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_SOURCE_DIR}/src/*.cpp" "${CMAKE_SOURCE_DIR}/src/*.h")
	file(GLOB_RECURSE PLUGIN_SOURCES RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_SOURCE_DIR}/plugins/*.cpp" "${CMAKE_SOURCE_DIR}/plugins/*.h")
	list(SORT CORE_SOURCES)
	list(SORT PLUGIN_SOURCES)
	set(XGETTEXT_ARGS --language=C++ --from-code=UTF-8 --keyword=_ --no-wrap --sort-by-file --package-name=Verlihub --msgid-bugs-address=info@verlihub.net)

	add_custom_target(pot
		COMMAND ${XGETTEXT} ${XGETTEXT_ARGS} --output=verlihub_core.pot ${CORE_SOURCES}
		COMMAND ${XGETTEXT} ${XGETTEXT_ARGS} --output=verlihub_plugins.pot ${PLUGIN_SOURCES}
		WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
		COMMENT "Extracting translation templates into ${CMAKE_CURRENT_SOURCE_DIR}"
		VERBATIM
	)
else()
	message(STATUS "[ .. ] xgettext not found, target pot to extract translation templates is not available")
endif()
