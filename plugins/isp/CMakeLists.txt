#	Copyright (C) 2003-2005 Daniel Muller, dan at verliba dot cz
#	Copyright (C) 2006-2017 Verlihub Team, info at verlihub dot net
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

SET(ISP_VERSION_MAJOR 1)
SET(ISP_VERSION_MINOR 2)
SET(ISP_VERSION_PATCH 0)
SET(ISP_VERSION_TWEAK 0)
SET(ISP_VERSION "${ISP_VERSION_MAJOR}.${ISP_VERSION_MINOR}.${ISP_VERSION_PATCH}.${ISP_VERSION_TWEAK}")

ADD_DEFINITIONS(-DISP_VERSION="${ISP_VERSION}")
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

SET(ISP_HDRS
	cconsole.h
	cisps.h
	cpiisp.h
)

SET(ISP_SRCS
	cconsole.cpp
	cisps.cpp
	cpiisp.cpp
)

ADD_LIBRARY(libisp_pi MODULE ${ISP_SRCS})
SET_TARGET_PROPERTIES(libisp_pi PROPERTIES OUTPUT_NAME "isp_pi")
TARGET_LINK_LIBRARIES(libisp_pi libverlihub)

INSTALL(TARGETS libisp_pi LIBRARY DESTINATION ${PLUGINDIR})
