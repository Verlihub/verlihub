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

IF(SYSINFO_LIBRARY)

	SET(SYSINFO_FOUND TRUE)

ELSE(SYSINFO_LIBRARY)

	FIND_PATH(SYSINFO_INCLUDE_DIR
		NAMES
			sys/sysinfo.h
		PATHS
			/usr/include
			/usr/local/include
			/opt/local/include
	)

	FIND_LIBRARY(SYSINFO_LIBRARY
		NAMES
			sysinfo
			libsysinfo
		PATHS
			/usr/lib
			/usr/local/lib
			/opt/local/lib
	)

	IF(SYSINFO_INCLUDE_DIR AND SYSINFO_LIBRARY)

		SET(SYSINFO_FOUND TRUE)
		INCLUDE(FindPackageHandleStandardArgs)
		FIND_PACKAGE_HANDLE_STANDARD_ARGS(SysInfo DEFAULT_MSG SYSINFO_LIBRARY SYSINFO_INCLUDE_DIR)
		MARK_AS_ADVANCED(SYSINFO_INCLUDE_DIR SYSINFO_LIBRARY)

	ELSE(SYSINFO_INCLUDE_DIR AND SYSINFO_LIBRARY)

		SET(SYSINFO_FOUND FALSE)

	ENDIF(SYSINFO_INCLUDE_DIR AND SYSINFO_LIBRARY)

ENDIF(SYSINFO_LIBRARY)
