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

SET(VERLIHUB_FOUND 0)

# note: newer verlihub installs package configuration, it also gives defines that verlihub headers depend on, so use it when found
SET(_VH_FIND_QUIETLY ${Verlihub_FIND_QUIETLY}) # note: inner find_package changes these
SET(_VH_FIND_REQUIRED ${Verlihub_FIND_REQUIRED})
find_package(Verlihub ${Verlihub_FIND_VERSION} CONFIG QUIET HINTS ${WITH_VERLIHUB})
SET(Verlihub_FIND_QUIETLY ${_VH_FIND_QUIETLY})
SET(Verlihub_FIND_REQUIRED ${_VH_FIND_REQUIRED})

if(Verlihub_FOUND)
	IF(NOT Verlihub_FIND_QUIETLY)
		MESSAGE(STATUS "[ OK ] Found Verlihub ${VERLIHUB_VERSION_STRING} package: ${Verlihub_DIR}")
	ENDIF(NOT Verlihub_FIND_QUIETLY)

	return()
endif(Verlihub_FOUND)

IF(Verlihub_FIND_REQUIRED) # note: verlihub_config script was removed, package configuration is installed instead
	MESSAGE(FATAL_ERROR "[ ER ] Verlihub package configuration not found, please install Verlihub 1.7 or newer, or specify path: -DCMAKE_PREFIX_PATH=/path/to/verlihub")
ENDIF(Verlihub_FIND_REQUIRED)
