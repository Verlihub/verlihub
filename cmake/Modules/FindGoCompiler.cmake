#	Copyright (C) 2003-2005 Daniel Muller, dan at verliba dot cz
#	Copyright (C) 2006-2020 Verlihub Team, info at verlihub dot net
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

if(NOT CMAKE_Go_COMPILER)
	if(NOT $ENV{GO_COMPILER} STREQUAL "")
		get_filename_component(CMAKE_Go_COMPILER_INIT $ENV{GO_COMPILER} PROGRAM PROGRAM_ARGS CMAKE_Go_FLAGS_ENV_INIT)

		if(CMAKE_Go_FLAGS_ENV_INIT)
			set(CMAKE_Go_COMPILER_ARG1 "${CMAKE_Go_FLAGS_ENV_INIT}" CACHE STRING "First argument to Go Compiler")
		endif()

		if(NOT EXISTS ${CMAKE_Go_COMPILER_INIT})
			message(SEND_ERROR "[ER] Could not find Go Compiler set in environment variable GO_COMPILER:\n\t$ENV{GO_COMPILER}")
		endif()
	endif()

	set(Go_BIN_PATH
		$ENV{GOPATH}
		$ENV{GOROOT}
		$ENV{GOROOT}/../bin
		$ENV{GO_COMPILER}
		/usr/bin
		/usr/local/bin
	)

	if(CMAKE_Go_COMPILER_INIT)
		set(CMAKE_Go_COMPILER ${CMAKE_Go_COMPILER_INIT} CACHE PATH "Go Compiler")
	else()
		find_program(CMAKE_Go_COMPILER
			NAMES go
			PATHS ${Go_BIN_PATH}
		)

		execute_process(COMMAND ${CMAKE_Go_COMPILER} version OUTPUT_VARIABLE CMAKE_Go_COMPILER_ID OUTPUT_STRIP_TRAILING_WHITESPACE)
		string(REPLACE "go version " "" CMAKE_Go_COMPILER_ID ${CMAKE_Go_COMPILER_ID})
		message(STATUS "[OK] Found Go Compiler version: ${CMAKE_Go_COMPILER_ID}")

		# TODO: Test for working Golang compiler, or complain...
		# message(STATUS "Check for working Go compiler: ${CMAKE_Go_COMPILER}")
	endif()
endif()

mark_as_advanced(CMAKE_Go_COMPILER)
set(CMAKE_Go_COMPILER_ENV_VAR "GO_COMPILER")

# end of file
