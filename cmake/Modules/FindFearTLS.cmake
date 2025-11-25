#	Copyright (C) 2024-2025 FearDC, webmaster at feardc dot net
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

find_library(FEARTLS_PATH
	NAMES
		FearTLS
		libFearTLS
	PATHS
		feartls
)

if(FEARTLS_PATH)
	message(STATUS "[ OK ] Found FearTLS library: ${FEARTLS_PATH}")
else(FEARTLS_PATH)
	message(FATAL_ERROR "[ ER ] FearTLS library not found, please make sure that you have latest Verlihub source: https://github.com/verlihub/verlihub/")
endif(FEARTLS_PATH)

# end of file
