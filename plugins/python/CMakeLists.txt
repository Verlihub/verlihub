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
SET(PYTHON_PI_VERSION_MINOR 6)
SET(PYTHON_PI_VERSION_PATCH 0)
SET(PYTHON_PI_VERSION_TWEAK 0)
SET(PYTHON_PI_VERSION "${PYTHON_PI_VERSION_MAJOR}.${PYTHON_PI_VERSION_MINOR}.${PYTHON_PI_VERSION_PATCH}.${PYTHON_PI_VERSION_TWEAK}")

ADD_DEFINITIONS(-DPYTHON_PI_VERSION=\"${PYTHON_PI_VERSION}\")
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

find_package(Python3 REQUIRED COMPONENTS Interpreter Development)
INCLUDE_DIRECTORIES(${Python3_INCLUDE_DIRS})

# Find RapidJSON for JSON parsing (header-only library)
find_path(RAPIDJSON_INCLUDE_DIR
    NAMES rapidjson/rapidjson.h
    PATHS /usr/include /usr/local/include
)

if(RAPIDJSON_INCLUDE_DIR)
    include_directories(${RAPIDJSON_INCLUDE_DIR})
    message(STATUS "[ OK ] RapidJSON found at ${RAPIDJSON_INCLUDE_DIR}")
    add_definitions(-DHAVE_RAPIDJSON)
else()
    message(WARNING "[ !! ] RapidJSON not found; advanced JSON features will use fallback")
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

# Find GTest using modern CMake approach
find_package(GTest)

if(GTest_FOUND)
    # Also check for gmock
    find_path(GMOCK_INCLUDE_DIR gmock/gmock.h PATHS /usr/include /usr/src/googletest/googlemock/include)
    
    if(GMOCK_INCLUDE_DIR)
        include_directories(${GTEST_INCLUDE_DIRS} ${GMOCK_INCLUDE_DIR})
        
        # Test executable for wrapper only
        add_executable(test_python_wrapper tests/test_wrapper.cpp)
        target_link_libraries(test_python_wrapper GTest::gtest GTest::gtest_main pthread libvh_python_wrapper)

        add_executable(test_python_wrapper_stress tests/test_stress_wrapper.cpp)
        target_link_libraries(test_python_wrapper_stress GTest::gtest GTest::gtest_main pthread libvh_python_wrapper)

        # Integration test - now works with the SHARED library
        add_executable(test_python_plugin_integration tests/test_python_plugin_integration.cpp)
        target_link_libraries(test_python_plugin_integration GTest::gtest GTest::gtest_main pthread libverlihub_so vhapi_so libpython_pi_core libvh_python_wrapper)
        # Pass build directory to test for config file location
        target_compile_definitions(test_python_plugin_integration PRIVATE BUILD_DIR="${CMAKE_BINARY_DIR}")

        # Phase 3 & 4: Advanced type marshaling tests
        add_executable(test_python_advanced_types tests/test_python_advanced_types.cpp)
        target_link_libraries(test_python_advanced_types GTest::gtest GTest::gtest_main pthread libvh_python_wrapper)

        # vh Module tests - restored Python bidirectional API
        add_executable(test_vh_module tests/test_vh_module.cpp)
        target_link_libraries(test_vh_module GTest::gtest GTest::gtest_main pthread libvh_python_wrapper)
        
        # Dimension 4: Dynamic registration tests
        add_executable(test_dynamic_registration tests/test_dynamic_registration.cpp)
        target_link_libraries(test_dynamic_registration GTest::gtest GTest::gtest_main pthread libvh_python_wrapper)

        # JSON marshaling tests
        add_executable(test_json_marshal tests/test_json_marshal.cpp)
        target_link_libraries(test_json_marshal GTest::gtest GTest::gtest_main pthread libvh_python_wrapper)

        # JSON integration tests - comprehensive nested structure tests
        add_executable(test_json_integration tests/test_json_integration.cpp)
        target_link_libraries(test_json_integration GTest::gtest GTest::gtest_main pthread libvh_python_wrapper)

        # Sets and tuples marshaling tests
        add_executable(test_sets_tuples tests/test_sets_tuples.cpp)
        target_link_libraries(test_sets_tuples GTest::gtest GTest::gtest_main pthread libvh_python_wrapper)

        # Add tests to CTest (enable_testing is called in root CMakeLists.txt)
        add_test(NAME PythonWrapperTests COMMAND test_python_wrapper)
        add_test(NAME PythonWrapperStressTests COMMAND test_python_wrapper_stress)
        # Integration test exercises full TreatMsg pathway through Python plugin
        add_test(NAME PythonPluginIntegrationTests COMMAND test_python_plugin_integration)
        # Advanced types test exercises Phase 3 list/dict marshaling
        add_test(NAME PythonAdvancedTypesTests COMMAND test_python_advanced_types)
        # vh Module test - validates restored bidirectional API
        add_test(NAME VHModuleTests COMMAND test_vh_module)
        # Dynamic registration test - validates Dimension 4
        add_test(NAME DynamicRegistrationTests COMMAND test_dynamic_registration)
        # JSON marshaling test - validates RapidJSON integration
        add_test(NAME JsonMarshalTests COMMAND test_json_marshal)
        # JSON integration test - validates complex nested structures
        add_test(NAME JsonIntegrationTests COMMAND test_json_integration)
        # Sets and tuples test - validates Python set/tuple marshaling
        add_test(NAME SetsTuplesTests COMMAND test_sets_tuples)
        
        message(STATUS "[ OK ] GTest and GMock found, building Python plugin tests")
    else()
        message(STATUS "[ !! ] GMock not found; skipping integration test build")
    endif()
else()
    message(STATUS "[ !! ] GTest not found; skipping test build")
endif()