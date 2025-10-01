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

SET(PYTHON_PI_VERSION_MAJOR 3)
SET(PYTHON_PI_VERSION_MINOR 0)
SET(PYTHON_PI_VERSION_PATCH 0)
SET(PYTHON_PI_VERSION_TWEAK 0)
SET(PYTHON_PI_VERSION "${PYTHON_PI_VERSION_MAJOR}.${PYTHON_PI_VERSION_MINOR}.${PYTHON_PI_VERSION_PATCH}.${PYTHON_PI_VERSION_TWEAK}")

ADD_DEFINITIONS(-DPYTHON_PI_VERSION=\"${PYTHON_PI_VERSION}\")
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

find_package(Python3 REQUIRED COMPONENTS Interpreter Development)
INCLUDE_DIRECTORIES(${Python3_INCLUDE_DIRS})

SET(PYTHON_HDRS
	cconsole.h
	cpipython.h
	cpythoninterpreter.h
	wrapper.h
)

SET(PYTHON_SRCS
	cconsole.cpp
	cpythoninterpreter.cpp
)

SET(PYTHON_WRAPPER_SRCS
	wrapper.cpp
)

IF(CMAKE_COMPILER_IS_GNUCC) # prevent strict aliasing warning
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-strict-aliasing")
ENDIF(CMAKE_COMPILER_IS_GNUCC)

CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/cpipython.cpp" "${CMAKE_CURRENT_BINARY_DIR}/cpipython.cpp" @ONLY)
LIST(APPEND PYTHON_SRCS "${CMAKE_CURRENT_BINARY_DIR}/cpipython.cpp")

ADD_LIBRARY(libpython_pi MODULE ${PYTHON_SRCS})
SET_TARGET_PROPERTIES(libpython_pi PROPERTIES OUTPUT_NAME "python_pi")
TARGET_LINK_LIBRARIES(libpython_pi ${DL_LIBRARIES} libverlihub_so)

ADD_LIBRARY(libvh_python_wrapper MODULE ${PYTHON_WRAPPER_SRCS})
SET_TARGET_PROPERTIES(libvh_python_wrapper PROPERTIES OUTPUT_NAME "vh_python_wrapper")
target_link_libraries(libvh_python_wrapper ${Python3_LIBRARIES})

INSTALL(TARGETS libpython_pi libvh_python_wrapper LIBRARY DESTINATION ${PLUGINDIR})

# Find GTest (optional, quiet mode)
find_package(GTest QUIET)
if(GTEST_FOUND)
    include_directories(${GTEST_INCLUDE_DIRS})

    # Test executable
    add_executable(test_python_wrapper tests/test_wrapper.cpp)
    target_link_libraries(test_python_wrapper ${GTEST_LIBRARIES} pthread libvh_python_wrapper)

    # Enable testing
    enable_testing()
    add_test(NAME PythonWrapperTests COMMAND test_python_wrapper)
else()
    message(STATUS "GTest not found; skipping test build")
endif()