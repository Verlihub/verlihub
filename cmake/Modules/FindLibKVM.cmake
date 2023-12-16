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

# todo: this is no longer used, really need it?

IF(LIBKVM_LIBRARY)

	ADD_DEFINITIONS(-DHAVE_LIBKVM)
	SET(LIBKVM_FOUND TRUE)

ELSE(LIBKVM_LIBRARY)

	FIND_PATH(LIBKVM_INCLUDE_DIR NAMES kvm.h)
	FIND_LIBRARY(LIBKVM_LIBRARY NAMES kvm)

	IF(LIBKVM_INCLUDE_DIR AND LIBKVM_LIBRARY)

		SET(LIBKVM_FOUND TRUE)
		ADD_DEFINITIONS(-DHAVE_LIBKVM)
		INCLUDE(FindPackageHandleStandardArgs)
		FIND_PACKAGE_HANDLE_STANDARD_ARGS(LibKVM DEFAULT_MSG LIBKVM_LIBRARY LIBKVM_INCLUDE_DIR)
		MARK_AS_ADVANCED(LIBKVM_INCLUDE_DIR LIBKVM_LIBRARY)

	ELSE(LIBKVM_INCLUDE_DIR AND LIBKVM_LIBRARY)

		SET(LIBKVM_FOUND FALSE)

	ENDIF(LIBKVM_INCLUDE_DIR AND LIBKVM_LIBRARY)

ENDIF(LIBKVM_LIBRARY)