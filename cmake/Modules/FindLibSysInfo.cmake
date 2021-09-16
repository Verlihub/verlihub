#	Copyright (C) 2003-2005 Daniel Muller, dan at verliba dot cz
#	Copyright (C) 2006-2021 Verlihub Team, info at verlihub dot net
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

find_path(SYSINFO_INCLUDE_DIR
	NAMES sys/sysinfo.h
	PATHS
		/usr/include /usr/local/include /opt/local/include)

find_library(SYSINFO_LIBRARY
	NAMES sysinfo libsysinfo
	PATHS
		/usr/lib /usr/local/lib /opt/local/lib)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(sysinfo DEFAULT_MSG SYSINFO_LIBRARY SYSINFO_INCLUDE_DIR)
mark_as_advanced(SYSINFO_INCLUDE_DIR SYSINFO_LIBRARY)

if(SYSINFO_FOUND)
	set(SYSINFO_INCLUDE_DIRS ${SYSINFO_INCLUDE_DIR})
	set(SYSINFO_LIBRARIES ${SYSINFO_LIBRARY})
endif()
