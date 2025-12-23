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

SET(PYTHON_PI_VERSION_MAJOR 1)
SET(PYTHON_PI_VERSION_MINOR 7)
SET(PYTHON_PI_VERSION_PATCH 0)
SET(PYTHON_PI_VERSION_TWEAK 0)
SET(PYTHON_PI_VERSION "${PYTHON_PI_VERSION_MAJOR}.${PYTHON_PI_VERSION_MINOR}.${PYTHON_PI_VERSION_PATCH}.${PYTHON_PI_VERSION_TWEAK}")

ADD_DEFINITIONS(-DPYTHON_PI_VERSION=\"${PYTHON_PI_VERSION}\")
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

# Advanced option: Use single interpreter instead of sub-interpreters
# WARNING: This mode has security implications - scripts can access each other's data
# Only enable if you need complex threading/daemon threads and accept the risks
option(PYTHON_USE_SINGLE_INTERPRETER 
    "Use single Python interpreter (allows threading but scripts can see each other)" 
    OFF)

# Option to build Python plugin tests (requires GTest, GMock, and optionally libcurl)
option(BUILD_PYTHON_TESTS
    "Build Python plugin unit and integration tests"
    ON)

if(PYTHON_USE_SINGLE_INTERPRETER)
    message(STATUS "[ !! ] Python single interpreter mode ENABLED")
    message(STATUS "       WARNING: Scripts can access each other's data")
    message(STATUS "       This mode enables threading but has security implications")
    add_definitions(-DPYTHON_SINGLE_INTERPRETER)
else()
    message(STATUS "[ OK ] Python sub-interpreter mode (default, secure isolation)")
endif()

find_package(Python3 REQUIRED COMPONENTS Interpreter Development)
INCLUDE_DIRECTORIES(${Python3_INCLUDE_DIRS})

# Find RapidJSON for JSON parsing (header-only library)
# RapidJSON is REQUIRED for Python plugin to function correctly
find_path(RAPIDJSON_INCLUDE_DIR
    NAMES rapidjson/rapidjson.h
    PATHS /usr/include /usr/local/include
)

if(RAPIDJSON_INCLUDE_DIR)
    include_directories(${RAPIDJSON_INCLUDE_DIR})
    message(STATUS "[ OK ] RapidJSON found at ${RAPIDJSON_INCLUDE_DIR}")
    add_definitions(-DHAVE_RAPIDJSON)
else()
    message(FATAL_ERROR "[ !! ] RapidJSON not found - REQUIRED for Python plugin\n"
            "Please install RapidJSON:\n"
            "  Ubuntu/Debian: sudo apt-get install rapidjson-dev\n"
            "  Fedora/RHEL:   sudo dnf install rapidjson-devel\n"
            "  macOS:         brew install rapidjson\n"
            "  Or download from: https://github.com/Tencent/rapidjson\n"
            "To build without Python plugin, use: -DWITH_PYTHON=OFF")
endif()

SET(PYTHON_HDRS
	cconsole.h
	cpipython.h
	cpythoninterpreter.h
	wrapper.h
	json_marshal.h
)

SET(PYTHON_WRAPPER_SRCS
	wrapper.cpp
	json_marshal.cpp
)

SET(PYTHON_CORE_SRCS
	cconsole.cpp
	cpythoninterpreter.cpp
)

IF(CMAKE_COMPILER_IS_GNUCC) # prevent strict aliasing warning
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-strict-aliasing")
ENDIF(CMAKE_COMPILER_IS_GNUCC)

# Build the wrapper library first (needed by both plugin and tests)
ADD_LIBRARY(libvh_python_wrapper SHARED ${PYTHON_WRAPPER_SRCS})
SET_TARGET_PROPERTIES(libvh_python_wrapper PROPERTIES OUTPUT_NAME "vh_python_wrapper")
target_link_libraries(libvh_python_wrapper ${Python3_LIBRARIES})

# Configure and add cpipython.cpp to core sources
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/cpipython.cpp" "${CMAKE_CURRENT_BINARY_DIR}/cpipython.cpp" @ONLY)
LIST(APPEND PYTHON_CORE_SRCS "${CMAKE_CURRENT_BINARY_DIR}/cpipython.cpp")

# Create a SHARED library with all plugin code (can be linked by tests)
ADD_LIBRARY(libpython_pi_core SHARED ${PYTHON_CORE_SRCS})
SET_TARGET_PROPERTIES(libpython_pi_core PROPERTIES OUTPUT_NAME "python_pi_core")
TARGET_LINK_LIBRARIES(libpython_pi_core ${DL_LIBRARIES} libverlihub_so libvh_python_wrapper ${Python3_LIBRARIES})

# Create the MODULE that Verlihub loads at runtime (thin wrapper around the SHARED library)
# This is just a stub that re-exports symbols from the shared library
ADD_LIBRARY(libpython_pi MODULE ${PYTHON_CORE_SRCS})
SET_TARGET_PROPERTIES(libpython_pi PROPERTIES OUTPUT_NAME "python_pi")
TARGET_LINK_LIBRARIES(libpython_pi ${DL_LIBRARIES} libverlihub_so libvh_python_wrapper ${Python3_LIBRARIES})

INSTALL(TARGETS libpython_pi libvh_python_wrapper LIBRARY DESTINATION ${PLUGINDIR})

# Only build tests if requested (enabled by default)
if(BUILD_PYTHON_TESTS)
    # Find GTest using modern CMake approach
    find_package(GTest)

    if(GTest_FOUND)
        # Also check for gmock
        find_path(GMOCK_INCLUDE_DIR gmock/gmock.h PATHS /usr/include /usr/src/googletest/googlemock/include)
        
        # Check for libcurl (needed for API stress test)
        find_package(CURL)
        
        if(GMOCK_INCLUDE_DIR)
        include_directories(${GTEST_INCLUDE_DIRS} ${GMOCK_INCLUDE_DIR})
        
        # Test executable for wrapper only
        add_executable(test_python_wrapper tests/test_wrapper.cpp)
        target_link_libraries(test_python_wrapper GTest::gtest GTest::gtest_main pthread libverlihub_so -Wl,--no-as-needed libpython_pi_core -Wl,--as-needed libvh_python_wrapper vhapi_so)
        target_compile_definitions(test_python_wrapper PRIVATE BUILD_DIR="${CMAKE_BINARY_DIR}")

        add_executable(test_python_wrapper_stress tests/test_stress_wrapper.cpp)
        target_link_libraries(test_python_wrapper_stress test_utils GTest::gtest GTest::gtest_main pthread libverlihub_so -Wl,--no-as-needed libpython_pi_core -Wl,--as-needed libvh_python_wrapper vhapi_so)
        target_compile_definitions(test_python_wrapper_stress PRIVATE BUILD_DIR="${CMAKE_BINARY_DIR}")

        # Integration test - now works with the SHARED library
        add_executable(test_python_plugin_integration tests/test_python_plugin_integration.cpp)
        target_link_libraries(test_python_plugin_integration test_utils GTest::gtest GTest::gtest_main pthread libverlihub_so vhapi_so libpython_pi_core libvh_python_wrapper)
        # Pass build directory to test for config file location
        target_compile_definitions(test_python_plugin_integration PRIVATE BUILD_DIR="${CMAKE_BINARY_DIR}")

        # Advanced type marshaling tests
        add_executable(test_python_advanced_types tests/test_python_advanced_types.cpp)
        target_link_libraries(test_python_advanced_types GTest::gtest GTest::gtest_main pthread libverlihub_so -Wl,--no-as-needed libpython_pi_core -Wl,--as-needed libvh_python_wrapper vhapi_so)
        target_compile_definitions(test_python_advanced_types PRIVATE BUILD_DIR="${CMAKE_BINARY_DIR}" SOURCE_DIR="${CMAKE_SOURCE_DIR}")

        # Test utilities library (for memory tracking)
        add_library(test_utils STATIC tests/test_utils.cpp)
        target_include_directories(test_utils PUBLIC tests)

        # vh Module tests - restored Python bidirectional API
        add_executable(test_vh_module tests/test_vh_module.cpp)
        target_link_libraries(test_vh_module test_utils GTest::gtest GTest::gtest_main pthread libverlihub_so -Wl,--no-as-needed libpython_pi_core -Wl,--as-needed libvh_python_wrapper vhapi_so)
        target_compile_definitions(test_vh_module PRIVATE BUILD_DIR="${CMAKE_BINARY_DIR}")
        
        # Dynamic registration tests
        add_executable(test_dynamic_registration tests/test_dynamic_registration.cpp)
        target_link_libraries(test_dynamic_registration GTest::gtest GTest::gtest_main pthread libverlihub_so -Wl,--no-as-needed libpython_pi_core -Wl,--as-needed libvh_python_wrapper vhapi_so)
        target_compile_definitions(test_dynamic_registration PRIVATE BUILD_DIR="${CMAKE_BINARY_DIR}")

        # JSON marshaling tests
        add_executable(test_json_marshal tests/test_json_marshal.cpp)
        target_link_libraries(test_json_marshal GTest::gtest GTest::gtest_main pthread libverlihub_so -Wl,--no-as-needed libpython_pi_core -Wl,--as-needed libvh_python_wrapper vhapi_so)
        target_compile_definitions(test_json_marshal PRIVATE BUILD_DIR="${CMAKE_BINARY_DIR}")

        # JSON integration tests - comprehensive nested structure tests
        add_executable(test_json_integration tests/test_json_integration.cpp)
        target_link_libraries(test_json_integration GTest::gtest GTest::gtest_main pthread libverlihub_so -Wl,--no-as-needed libpython_pi_core -Wl,--as-needed libvh_python_wrapper vhapi_so)
        target_compile_definitions(test_json_integration PRIVATE BUILD_DIR="${CMAKE_BINARY_DIR}")

        # Sets and tuples marshaling tests
        add_executable(test_sets_tuples tests/test_sets_tuples.cpp)
        target_link_libraries(test_sets_tuples GTest::gtest GTest::gtest_main pthread libverlihub_so -Wl,--no-as-needed libpython_pi_core -Wl,--as-needed libvh_python_wrapper vhapi_so)
        target_compile_definitions(test_sets_tuples PRIVATE BUILD_DIR="${CMAKE_BINARY_DIR}")

        # Hub API stress test - tests FastAPI server with concurrent message processing
        if(CURL_FOUND)
            add_executable(test_hub_api_stress tests/test_hub_api_stress.cpp)
            target_link_libraries(test_hub_api_stress test_utils GTest::gtest GTest::gtest_main pthread libverlihub_so vhapi_so libpython_pi_core libvh_python_wrapper ${CURL_LIBRARIES})
            target_compile_definitions(test_hub_api_stress PRIVATE BUILD_DIR="${CMAKE_BINARY_DIR}" SOURCE_DIR="${CMAKE_SOURCE_DIR}")
            target_include_directories(test_hub_api_stress PRIVATE ${CURL_INCLUDE_DIRS})
            message(STATUS "[ OK ] libcurl found, building hub_api stress test")
        else()
            message(STATUS "[ !! ] libcurl not found; skipping hub_api stress test (install libcurl4-openssl-dev)")
        endif()

        # Single interpreter mode tests (only built when option is enabled)
        if(PYTHON_USE_SINGLE_INTERPRETER)
            add_executable(test_single_interpreter tests/test_single_interpreter.cpp)
            target_link_libraries(test_single_interpreter GTest::gtest GTest::gtest_main pthread libverlihub_so -Wl,--no-as-needed libpython_pi_core -Wl,--as-needed libvh_python_wrapper vhapi_so)
            target_compile_definitions(test_single_interpreter PRIVATE BUILD_DIR="${CMAKE_BINARY_DIR}")
            add_test(NAME SingleInterpreterTests COMMAND test_single_interpreter)
        endif()

        # Add tests to CTest (enable_testing is called in root CMakeLists.txt)
        add_test(NAME PythonWrapperTests COMMAND test_python_wrapper)
        add_test(NAME PythonWrapperStressTests COMMAND test_python_wrapper_stress)
        # Integration test exercises full TreatMsg pathway through Python plugin
        add_test(NAME PythonPluginIntegrationTests COMMAND test_python_plugin_integration)
        # Advanced types test exercises list/dict marshaling
        add_test(NAME PythonAdvancedTypesTests COMMAND test_python_advanced_types)
        # vh Module test - validates restored bidirectional API
        add_test(NAME VHModuleTests COMMAND test_vh_module)
        # Dynamic registration test - validates runtime function registration
        add_test(NAME DynamicRegistrationTests COMMAND test_dynamic_registration)
        # JSON marshaling test - validates RapidJSON integration
        add_test(NAME JsonMarshalTests COMMAND test_json_marshal)
        # JSON integration test - validates complex nested structures
        add_test(NAME JsonIntegrationTests COMMAND test_json_integration)
        # Sets and tuples test - validates Python set/tuple marshaling
        add_test(NAME SetsTuplesTests COMMAND test_sets_tuples)
        
        # Hub API stress test - validates concurrent API calls with message processing
        if(CURL_FOUND)
            add_test(NAME HubApiStressTests COMMAND test_hub_api_stress)
        endif()
        
        message(STATUS "[ OK ] GTest and GMock found, building Python plugin tests")
    else()
        message(STATUS "[ !! ] GMock not found; skipping integration test build")
    endif()
else()
    message(STATUS "[ !! ] GTest not found; skipping test build")
endif()
else()
    message(STATUS "[ .. ] Python plugin tests disabled (use -DBUILD_PYTHON_TESTS=ON to enable)")
endif()