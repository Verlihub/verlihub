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

if(MYSQL_INCLUDE_DIR AND MYSQL_LIBRARIES)
	set(MYSQL_FOUND TRUE)
else(MYSQL_INCLUDE_DIR AND MYSQL_LIBRARIES)
	find_path(MYSQL_INCLUDE_DIR mysql.h
		/usr/include/mysql
		/usr/local/include/mysql
		/usr/pkg/include/mysql
		/usr/local/pkg/include/mysql
		$ENV{ProgramFiles}/MySQL/*/include
		$ENV{SystemDrive}/MySQL/*/include
	)

	#if(WIN32 AND MSVC)
		#find_library(MYSQL_LIBRARIES NAMES libmysql
			#PATHS
				#$ENV{ProgramFiles}/MySQL/*/lib/opt
				#$ENV{SystemDrive}/MySQL/*/lib/opt
		#)
	#else(WIN32 AND MSVC)
		find_library(MYSQL_LIBRARIES NAMES mysqlclient
			PATHS
				/usr/lib/mysql
				/usr/local/lib/mysql
				/usr/pkg/lib/mysql
				/usr/local/pkg/lib/mysql
		)
	#endif(WIN32 AND MSVC)

	if(MYSQL_INCLUDE_DIR AND MYSQL_LIBRARIES)
		set(MYSQL_FOUND TRUE)
	else(MYSQL_INCLUDE_DIR AND MYSQL_LIBRARIES)
		set(MYSQL_FOUND FALSE)
	endif(MYSQL_INCLUDE_DIR AND MYSQL_LIBRARIES)

	mark_as_advanced(MYSQL_INCLUDE_DIR MYSQL_LIBRARIES)
endif(MYSQL_INCLUDE_DIR AND MYSQL_LIBRARIES)

IF(MYSQL_FOUND)
	IF(NOT MySQL_FIND_QUIETLY)
		MESSAGE(STATUS "[ OK ] Found MySQL: ${MYSQL_INCLUDE_DIR}, ${MYSQL_LIBRARIES}")
	ENDIF(NOT MySQL_FIND_QUIETLY)
ELSE(MYSQL_FOUND)
	IF(MySQL_FIND_REQUIRED)
		MESSAGE(FATAL_ERROR "[ ER ] MySQL headers or libraries or binaries not found, please install them via your package manager or compile from source: https://dev.mysql.com/downloads/mysql/")
	ENDIF(MySQL_FIND_REQUIRED)
ENDIF(MYSQL_FOUND)
