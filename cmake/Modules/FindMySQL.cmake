# - Find MySQL
# Find the MySQL includes and client library
# This module defines
#  MYSQL_INCLUDE_DIR, where to find mysql.h
#  MYSQL_LIBRARIES, the libraries needed to use MySQL.
#  MYSQL_FOUND, If false, do not try to use MySQL.
#
# Copyright (c) 2006, Jaroslaw Staniek, <js@iidea.pl>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

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

	if(WIN32 AND MSVC)
		find_library(MYSQL_LIBRARIES NAMES libmysql PATHS
			$ENV{ProgramFiles}/MySQL/*/lib/opt
			$ENV{SystemDrive}/MySQL/*/lib/opt
		)
	else(WIN32 AND MSVC)
		find_library(MYSQL_LIBRARIES NAMES mysqlclient PATHS
			/usr/lib/mysql
			/usr/local/lib/mysql
			/usr/pkg/lib/mysql
			/usr/local/pkg/lib/mysql
		)
	endif(WIN32 AND MSVC)

	if(MYSQL_INCLUDE_DIR AND MYSQL_LIBRARIES)
		set(MYSQL_FOUND TRUE)
	else(MYSQL_INCLUDE_DIR AND MYSQL_LIBRARIES)
		set(MYSQL_FOUND FALSE)
	endif(MYSQL_INCLUDE_DIR AND MYSQL_LIBRARIES)

	mark_as_advanced(MYSQL_INCLUDE_DIR MYSQL_LIBRARIES)
endif(MYSQL_INCLUDE_DIR AND MYSQL_LIBRARIES)

IF(MYSQL_FOUND)
	IF(NOT MySQL_FIND_QUIETLY)
		MESSAGE(STATUS "Found MySQL: ${MYSQL_INCLUDE_DIR}, ${MYSQL_LIBRARIES}")
	ENDIF(NOT MySQL_FIND_QUIETLY)
ELSE(MYSQL_FOUND)
	IF(MySQL_FIND_REQUIRED)
		MESSAGE(FATAL_ERROR "Could NOT find MySQL. Please install MySQL server and MySQL devel packages.")
	ENDIF(MySQL_FIND_REQUIRED)
ENDIF(MYSQL_FOUND)
