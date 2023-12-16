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

IF(EXECINFO_LIBRARY)
	SET(EXECINFO_FOUND TRUE)
ELSE(EXECINFO_LIBRARY)
	FIND_PATH(EXECINFO_INCLUDE_DIR execinfo.h
		PATHS
			/usr/include
			/usr/local/include
			/usr/pkg/include
	)

	FIND_LIBRARY(EXECINFO_LIBRARY NAMES execinfo
		PATHS
			/usr/lib
			/usr/local/lib
			/usr/pkg/lib
	)

	IF(EXECINFO_INCLUDE_DIR AND EXECINFO_LIBRARY)
		SET(EXECINFO_FOUND TRUE)
		INCLUDE(FindPackageHandleStandardArgs)
		FIND_PACKAGE_HANDLE_STANDARD_ARGS(ExecInfo DEFAULT_MSG EXECINFO_LIBRARY EXECINFO_INCLUDE_DIR)
		MARK_AS_ADVANCED(EXECINFO_LIBRARY EXECINFO_INCLUDE_DIR)
	ELSE(EXECINFO_INCLUDE_DIR AND EXECINFO_LIBRARY)
		SET(EXECINFO_FOUND FALSE)
	ENDIF(EXECINFO_INCLUDE_DIR AND EXECINFO_LIBRARY)
ENDIF(EXECINFO_LIBRARY)
