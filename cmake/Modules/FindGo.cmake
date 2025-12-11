#	Copyright (C) 2003-2005 Daniel Muller, dan at verliba dot cz
#	Copyright (C) 2006-2025 Verlihub Team, info at verlihub dot net
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

find_program(GOLANG_PATH go
	HINTS
		ENV GOROOT
	PATHS
		/usr
		/usr/local
		/var/snap
	PATH_SUFFIXES
		bin
)

if(GOLANG_PATH)
	execute_process(COMMAND ${GOLANG_PATH} version OUTPUT_VARIABLE GOLANG_VERSION)
	string(STRIP "${GOLANG_VERSION}" GOLANG_VERSION)
	message(STATUS "[ OK ] Found Go binary: ${GOLANG_PATH} (${GOLANG_VERSION})")
else(GOLANG_PATH)
	message(FATAL_ERROR "[ ER ] Go not found, please install it via your package manager or compile from source: https://go.dev/")
endif(GOLANG_PATH)

# end of file
