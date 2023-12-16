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

# todo: cmake 3.16 has FindICU.cmake, really need this?

find_package(PkgConfig QUIET)

function(icudebug _varname)
	if(ICU_DEBUG)
		message("${_varname} = ${${_varname}}")
	endif(ICU_DEBUG)
endfunction(icudebug)

set(IcuComponents)

macro(declare_icu_component _NAME) # <icu component name> <library name 1> ... <library name N>
	list(APPEND IcuComponents ${_NAME})
	set("IcuComponents_${_NAME}" ${ARGN})
endmacro(declare_icu_component)


#if(WIN32)
	#declare_icu_component(data icudt)
	#declare_icu_component(i18n icuin) # internationalization library
#else()
	declare_icu_component(data icudata)
	declare_icu_component(i18n icui18n) # internationalization library
#endif ()

declare_icu_component(uc icuuc) # common and data libraries
declare_icu_component(io icuio) # stream and i/o library
#declare_icu_component(le icule) # layout library
#declare_icu_component(-le-hb icu-le-hb) # layout library
#declare_icu_component(lx iculx) # paragraph layout library

set(ICU_FOUND TRUE)
set(ICU_LIBRARIES)
set(ICU_DYN_LIBRARIES)
set(ICU_INCLUDE_DIRS)
set(ICU_DEFINITIONS)

foreach(_icu_component ${IcuComponents})
	string(TOUPPER "${_icu_component}" _icu_upper_component)
	set("ICU_${_icu_upper_component}_FOUND" FALSE) # may be done in the declare_icu_component macro
endforeach(_icu_component)

if(NOT IcuComponents) # check components, uc required at least
	set(IcuComponents uc)
else()
	list(APPEND IcuComponents uc)
	list(REMOVE_DUPLICATES IcuComponents)

	foreach(_icu_component ${IcuComponents})
		if(NOT DEFINED "IcuComponents_${_icu_component}")
			message(FATAL_ERROR "Unknown ICU component: ${_icu_component}")
		endif()
	endforeach(_icu_component)
endif()

find_path(ICU_INCLUDE_DIRS # includes
	NAMES
		unicode/utypes.h
	HINTS
		${ICU_INCLUDE_PATH}
	DOC
		"Include directories for ICU"
)

if(PKG_CONFIG_FOUND) # check dependencies
	set(_components_dup ${IcuComponents})

	foreach(_icu_component ${_components_dup})
		pkg_check_modules(PC_ICU "icu-${_icu_component}" QUIET)

		if(PC_ICU_FOUND)
			foreach(_pc_icu_library ${PC_ICU_LIBRARIES})
				string(REGEX REPLACE "^icu" "" _pc_stripped_icu_library ${_pc_icu_library})
				list(APPEND IcuComponents ${_pc_stripped_icu_library})
			endforeach(_pc_icu_library)
		endif(PC_ICU_FOUND)
	endforeach(_icu_component)

	list(REMOVE_DUPLICATES IcuComponents)
endif(PKG_CONFIG_FOUND)

foreach(_icu_component ${IcuComponents}) # check libraries
	find_library(_icu_lib_${_icu_component}
		NAMES
			${IcuComponents_${_icu_component}}
		HINTS
			${ICU_LIBRARIES_PATH}
		DOC
			"Libraries for ICU"
	)

	string(TOUPPER "${_icu_component}" _icu_upper_component)

	if(_icu_lib STREQUAL _icu_lib-NOTFOUND)
		set("ICU_${_icu_upper_component}_FOUND" FALSE)
		set(ICU_FOUND FALSE)
	else()
		#if (WIN32)
			#file(GLOB dylib "${ICU_DYNLIB_PATH}/${IcuComponents_${_icu_component}}*.dll")
			#list(LENGTH dylib length)

			#if (length GREATER 0)
				#list(GET dylib 0 dylib)
				#list(APPEND ICU_DYN_LIBRARIES ${dylib})
				#set("ICU_${_icu_upper_component}_FOUND" TRUE)
				#list(APPEND ICU_LIBRARIES ${_icu_lib_${_icu_component}})
			#else()
				#set("ICU_${_icu_upper_component}_FOUND" FALSE)
				#set(ICU_FOUND FALSE)
			#endif()
		#else()
			set("ICU_${_icu_upper_component}_FOUND" TRUE)
			list(APPEND ICU_LIBRARIES ${_icu_lib_${_icu_component}})
		#endif()
	endif()
endforeach(_icu_component)

if(ICU_FOUND)
	list(REMOVE_DUPLICATES ICU_LIBRARIES)
	message(STATUS "[ OK ] Found ICU libraries:")

	foreach (item ${ICU_LIBRARIES})
		message("    [ OK ] ${item}")
	endforeach()

	#if(WIN32)
		#list(REMOVE_DUPLICATES ICU_DYN_LIBRARIES)
		#message(STATUS "[ OK ] Found ICU dynamic libraries:")

		#foreach (item ${ICU_DYN_LIBRARIES})
			#message("    [ OK ] ${item}")
		#endforeach()
	#endif()

	if(EXISTS "${ICU_INCLUDE_DIRS}/unicode/uvernum.h")
		file(READ "${ICU_INCLUDE_DIRS}/unicode/uvernum.h" _icu_contents)
	endif()

	string(REGEX REPLACE ".*# *define *U_ICU_VERSION_MAJOR_NUM *([0-9]+).*" "\\1" ICU_MAJOR_VERSION "${_icu_contents}")
	string(REGEX REPLACE ".*# *define *U_ICU_VERSION_MINOR_NUM *([0-9]+).*" "\\1" ICU_MINOR_VERSION "${_icu_contents}")
	string(REGEX REPLACE ".*# *define *U_ICU_VERSION_PATCHLEVEL_NUM *([0-9]+).*" "\\1" ICU_PATCH_VERSION "${_icu_contents}")
	set(ICU_VERSION "${ICU_MAJOR_VERSION}.${ICU_MINOR_VERSION}.${ICU_PATCH_VERSION}")
elseif(ICU_FIND_REQUIRED AND NOT ICU_FIND_QUIETLY)
	message(FATAL_ERROR "[ ER ] ICU libraries not found, please install them via your package manager or compile from source: http://site.icu-project.org/download/")
endif(ICU_FOUND)

if(ICU_INCLUDE_DIRS)
	include(FindPackageHandleStandardArgs)

	if(ICU_FIND_REQUIRED AND NOT ICU_FIND_QUIETLY)
		find_package_handle_standard_args(ICU REQUIRED_VARS ICU_LIBRARIES ICU_INCLUDE_DIRS VERSION_VAR ICU_VERSION)
	else()
		find_package_handle_standard_args(ICU "[ OK ] Found ICU" ICU_LIBRARIES ICU_INCLUDE_DIRS)
	endif()
else(ICU_INCLUDE_DIRS)
	if(ICU_FIND_REQUIRED AND NOT ICU_FIND_QUIETLY)
		message(FATAL_ERROR "[ ER ] ICU headers not found, please install them via your package manager or compile from source: http://site.icu-project.org/download/")
	endif()
endif(ICU_INCLUDE_DIRS)

mark_as_advanced(
	ICU_INCLUDE_DIRS
	ICU_LIBRARIES
	ICU_DEFINITIONS
	ICU_VERSION
	ICU_MAJOR_VERSION
	ICU_MINOR_VERSION
	ICU_PATCH_VERSION
)

icudebug("IcuComponents")
icudebug("ICU_FIND_REQUIRED")
icudebug("ICU_FIND_QUIETLY")
icudebug("ICU_FIND_VERSION")

icudebug("ICU_FOUND")
icudebug("ICU_UC_FOUND")
icudebug("ICU_I18N_FOUND")
icudebug("ICU_IO_FOUND")
icudebug("ICU_LE_FOUND")
icudebug("ICU_LX_FOUND")

icudebug("ICU_INCLUDE_DIRS")
icudebug("ICU_LIBRARIES")

icudebug("ICU_MAJOR_VERSION")
icudebug("ICU_MINOR_VERSION")
icudebug("ICU_PATCH_VERSION")
icudebug("ICU_VERSION")
