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

cmake_minimum_required(VERSION 2.8)
PROJECT(VERLIHUB)

message(STATUS)
message(STATUS "You can change build options using -D{OPTION}=ON|OFF on the command line.")
message(STATUS "-DDEFINE_DEBUG=ON turns on debug build. -DWITH_PLUGINS=OFF disables plugins.")
message(STATUS "You can also disable selected plugins using -DWITH_{NAME}=OFF, where {NAME}")
message(STATUS "is one of: LUA PYTHON PERL FORBID CHATROOM IPLOG ISP MESSENGER STATS REPLACER FLOODPROT")
message(STATUS "If you get errors related to autosprintf, try -DUSE_CUSTOM_AUTOSPRINTF=ON")
message(STATUS)

SET(VERLIHUB_VERSION_MAJOR 1)
SET(VERLIHUB_VERSION_MINOR 0)
SET(VERLIHUB_VERSION_PATCH 2)
SET(VERLIHUB_VERSION_TWEAK 2)
SET(VERLIHUB_VERSION "${VERLIHUB_VERSION_MAJOR}.${VERLIHUB_VERSION_MINOR}.${VERLIHUB_VERSION_PATCH}.${VERLIHUB_VERSION_TWEAK}")
#SET(VERLIHUB_SOVERSION "${VERLIHUB_VERSION_MAJOR}.${VERLIHUB_VERSION_MINOR}.${VERLIHUB_VERSION_PATCH}")
SET(PROJECT_NAME "verlihub")
SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake/Modules)

SET(OK_PLUGINS_COUNT 0)
SET(ALL_PLUGINS_COUNT 12)

INCLUDE(CheckIncludeFiles)
INCLUDE(MacroLogFeature)
INCLUDE(VerliHubMacro)
OPTION(INSTALL_CMAKE_FILES "Install the *.cmake files into the CMake root" ON)

# Detect OS
IF(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	ADD_DEFINITIONS(-D_WIN32 1) # WIN32 flag already exists
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Windows")

IF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	ADD_DEFINITIONS(-DHAVE_LINUX)
	SET(HAVE_LINUX TRUE)
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

IF(${CMAKE_SYSTEM_NAME} MATCHES "OpenBSD")
	ADD_DEFINITIONS(-DHAVE_OPENBSD)
	SET(HAVE_OPENBSD TRUE)
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "OpenBSD")

IF(${CMAKE_SYSTEM_NAME} MATCHES "NetBSD")
	ADD_DEFINITIONS(-DHAVE_NETBSD)
	SET(HAVE_NETBSD TRUE)
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "NetBSD")

IF(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
	ADD_DEFINITIONS(-DHAVE_FREEBSD)
	SET(HAVE_FREEBSD TRUE)
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")

IF(${CMAKE_SYSTEM_NAME} MATCHES "BSD")
	ADD_DEFINITIONS(-DHAVE_BSD)
	SET(HAVE_BSD TRUE)
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "BSD")

IF(${CMAKE_SYSTEM_NAME} MATCHES "SunOS")
	ADD_DEFINITIONS(-DHAVE_SOLARIS)
	SET(HAVE_SOLARIS TRUE)
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "SunOS")

IF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	ADD_DEFINITIONS(-DHAVE_APPLE) # APPLE flag already exists
	SET(HAVE_APPLE TRUE)
# RPATH setting from https://cmake.org/Wiki/CMake_RPATH_handling#Mac_OS_X_and_the_RPATH (should use cmake >=2.8.12):
	set(CMAKE_MACOSX_RPATH ON)
	set(CMAKE_SKIP_BUILD_RPATH FALSE)
	set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
	set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
	set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
	list(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/lib" isSystemDir)
	if("${isSystemDir}" STREQUAL "-1")
		set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
	endif()
	set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} -undefined warning -flat_namespace")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_FLAGS} -undefined warning -flat_namespace")
# Workaround for homebrew path:
	SET(CMAKE_FIND_ROOT_PATH ${CMAKE_FIND_ROOT_PATH} /usr/local/opt /usr/local/opt/gettext /usr/local/opt/perl)
	SET(GETTEXT_EXEC_PATH "/usr/local/opt/gettext" )
	SET(OPENSSL_ROOT_DIR "/usr/local/opt/openssl" )
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

IF(LIB_INSTALL_DIR)
        SET(LIBDIR ${LIB_INSTALL_DIR})
ENDIF(LIB_INSTALL_DIR)

IF(NOT LIBDIR)
        SET(LIBDIR "lib")
ENDIF(NOT LIBDIR)

IF(PLUGIN_INSTALL_DIR)
        SET(PLUGINDIR ${PLUGIN_INSTALL_DIR})
ENDIF(PLUGIN_INSTALL_DIR)

IF(NOT PLUGINDIR)
        SET(PLUGINDIR "${LIBDIR}")
ENDIF(NOT PLUGINDIR)

message (STATUS "Library directory: ${CMAKE_INSTALL_PREFIX}/${LIBDIR}")

# Find dependences
Find_Package(OpenSSL REQUIRED)
Find_Package(Crypt REQUIRED)
Find_Package(MySQL REQUIRED)
Find_Package(ZLIB REQUIRED)
Find_Package(GeoIP)

IF(GEOIP_FOUND)
	ADD_DEFINITIONS(-DHAVE_LIBGEOIP)
ELSE()
	SET(GEOIP_INCLUDE_DIR "")
	SET(GEOIP_LIBRARIES "")
ENDIF(GEOIP_FOUND)

MACRO_LOG_FEATURE(GEOIP_FOUND "geoip" "Geolocalization support library" "http://www.maxmind.com/geoip/api/c.shtml")

Find_Package(DL REQUIRED)
Find_Package(Pcre REQUIRED)
Find_Package(Threads REQUIRED)
Find_Package(Gettext REQUIRED)
Find_Package(Gettext-devel REQUIRED)
Find_Package(Libintl REQUIRED)

FIND_LIBRARY(GETTEXT_ASPRINTF_LIBRARY asprintf DOC "Gettext asprintf library")

# Use cmake -DUSE_CUSTOM_AUTOSPRINTF=ON to use our own autosprintf
OPTION(USE_CUSTOM_AUTOSPRINTF "Build using custom internal autosprintf implementation" OFF)
IF(USE_CUSTOM_AUTOSPRINTF)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DUSE_CUSTOM_AUTOSPRINTF")
	SET(GETTEXT_ASPRINTF_LIBRARY "")
	MESSAGE(STATUS "Using custom internal autosprintf implementation")
ELSE(USE_CUSTOM_AUTOSPRINTF)
	IF(GETTEXT_ASPRINTF_LIBRARY)
		MESSAGE(STATUS "Found asprintf library: ${GETTEXT_ASPRINTF_LIBRARY}")
	ELSE(GETTEXT_ASPRINTF_LIBRARY)
		MESSAGE(FATAL_ERROR "Could NOT find asprintf library. Please install it or run cmake with the option -DUSE_CUSTOM_AUTOSPRINTF=ON")
	ENDIF(GETTEXT_ASPRINTF_LIBRARY)
ENDIF(USE_CUSTOM_AUTOSPRINTF)

IF(HAVE_BSD)
	#Find_Package(Iconv REQUIRED)
	Find_Package(ExecInfo REQUIRED)
ENDIF(HAVE_BSD)

IF(HAVE_BSD)
	SET(VERLIHUB_LIBS ${OPENSSL_LIBRARIES} ${CRYPT_LIBRARIES} ${MYSQL_LIBRARIES} ${GEOIP_LIBRARIES} ${ZLIB_LIBRARIES} ${PCRE_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${GETTEXT_ASPRINTF_LIBRARY} ${LIBINTL_LIBRARIES} ${EXECINFO_LIBRARY}) #${ICONV_LIBRARIES}
ELSE(HAVE_BSD)
	SET(VERLIHUB_LIBS ${OPENSSL_LIBRARIES} ${CRYPT_LIBRARIES} ${MYSQL_LIBRARIES} ${GEOIP_LIBRARIES} ${ZLIB_LIBRARIES} ${PCRE_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${GETTEXT_ASPRINTF_LIBRARY} ${LIBINTL_LIBRARIES})
ENDIF(HAVE_BSD)

SET(VERLIHUB_SCRIPTS_LIBS "-lverlihub -lvhapi")

IF(HAVE_BSD)
	SET(VERLIHUB_INCLUDE_DIRS  "-I${OPENSSL_INCLUDE_DIR} -I${MYSQL_INCLUDE_DIR} -I${GEOIP_INCLUDE_DIR} -I${ZLIB_INCLUDE_DIRS} -I${PCRE_INCLUDE_DIRS} -I${LIBINTL_INCLUDE_DIR} -I${EXECINFO_INCLUDE_DIR}")
	INCLUDE_DIRECTORIES(${OPENSSL_INCLUDE_DIR} ${MYSQL_INCLUDE_DIR} ${ZLIB_INCLUDE_DIR} ${GEOIP_INCLUDE_DIR} ${PCRE_INCLUDE_DIR} ${LIBINTL_INCLUDE_DIR} ${EXECINFO_INCLUDE_DIR} ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/src ${CMAKE_BINARY_DIR})
ELSE(HAVE_BSD)
	SET(VERLIHUB_INCLUDE_DIRS  "-I${OPENSSL_INCLUDE_DIR} -I${MYSQL_INCLUDE_DIR} -I${GEOIP_INCLUDE_DIR} -I${ZLIB_INCLUDE_DIRS} -I${PCRE_INCLUDE_DIRS} -I${LIBINTL_INCLUDE_DIR}")
	INCLUDE_DIRECTORIES(${OPENSSL_INCLUDE_DIR} ${MYSQL_INCLUDE_DIR} ${ZLIB_INCLUDE_DIR} ${GEOIP_INCLUDE_DIR} ${PCRE_INCLUDE_DIR} ${LIBINTL_INCLUDE_DIR} ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/src ${CMAKE_BINARY_DIR})
ENDIF(HAVE_BSD)
# ----------------------------------------------------------------------------------------------------

CHECK_INCLUDE_FILES(errno.h HAVE_ERRNO_H)
CHECK_INCLUDE_FILES(sys/poll.h HAVE_SYS_POLL_H)
CHECK_INCLUDE_FILES(getopt.h HAVE_GETOPT_H)
CHECK_INCLUDE_FILES(syslog.h HAVE_SYSLOG_H)
IF(HAVE_SYSLOG_H)
	ADD_DEFINITIONS(-DENABLE_SYSLOG)
ENDIF(HAVE_SYSLOG_H)

IF(NOT HAVE_GETOPT_H)
	MESSAGE(FATAL_ERROR "getopt.h is required to build verlihub")
ENDIF(NOT HAVE_GETOPT_H)

#TODO: Fix creguserinfo.cpp for HAVE_LIBSSL and crypto stuff

# Use cmake -DDEFINE_DEBUG=ON to enable debug
OPTION(DEFINE_DEBUG "Build the project using debugging code" OFF)
IF(DEFINE_DEBUG)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -O3 -g")
	SET(CMAKE_BUILD_TYPE Debug)
	MESSAGE(STATUS "Build type is " ${CMAKE_BUILD_TYPE})
ENDIF(DEFINE_DEBUG)

ADD_DEFINITIONS(-DHAVE_CONFIG_H)

IF(UNIX)
	FIND_PROGRAM(BASH_PATH bash PATHS /usr/bin /usr/local/bin)
	IF(NOT BASH_PATH)
		MESSAGE(FATAL_ERROR "Bash shell not found. Please install Bourne Again SHell")
	ENDIF(NOT BASH_PATH)
	FIND_PROGRAM(PYTHON_PATH python PATHS /usr/bin /usr/local/bin)
	IF(NOT PYTHON_PATH)
		MESSAGE(WARNING "Python interpreter not found. You won't be able to run the integration test.")
	ENDIF(NOT PYTHON_PATH)
ENDIF(UNIX)

# ----------------------------------------------------------------------------------------------------
# Generate configure files
MESSAGE(STATUS "Generating config.h file")
CONFIGURE_FILE("${CMAKE_SOURCE_DIR}/cmake/config.h.cm" "${CMAKE_BINARY_DIR}/config.h")
MESSAGE(STATUS "Generating dirsettings.h file")
CONFIGURE_FILE("${CMAKE_SOURCE_DIR}/cmake/dirsettings.h.cm" "${CMAKE_BINARY_DIR}/dirsettings.h")
# ----------------------------------------------------------------------------------------------------

if(INSTALL_CMAKE_FILES)
	install(FILES "${CMAKE_SOURCE_DIR}/cmake/Modules/FindVerliHub.cmake" DESTINATION ${CMAKE_ROOT}/Modules)
endif()

# Create uninstall configuration file and uninstall target
CONFIGURE_FILE("${CMAKE_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in" "${CMAKE_BINARY_DIR}/cmake/cmake_uninstall.cmake" IMMEDIATE @ONLY)
ADD_CUSTOM_TARGET(uninstall "${CMAKE_COMMAND}" -P "${PROJECT_BINARY_DIR}/cmake/cmake_uninstall.cmake")
# ----------------------------------------------------------------------------------------------------

ADD_SUBDIRECTORY(src)
ADD_SUBDIRECTORY(plugins)
ADD_SUBDIRECTORY(share)
ADD_SUBDIRECTORY(scripts)
ADD_SUBDIRECTORY(po)
ADD_SUBDIRECTORY(man)

# ----------------------------------------------------------------------------------------------------
# Build a CPack driven installer package
INCLUDE(InstallRequiredSystemLibraries)
SET(CPACK_PACKAGE_NAME "verlihub")
SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/COPYING")
SET(CPACK_PACKAGE_DESCRIPTION "Verlihub Direct Connect Hub Server")
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Verlihub Direct Connect Hub Server")

SET(CPACK_PACKAGE_CONTACT "info@verlihub.net")
SET(CPACK_PACKAGE_VENDOR "Verlihub Team")
SET(CPACK_PACKAGE_VERSION_MAJOR "${VERLIHUB_VERSION_MAJOR}")
SET(CPACK_PACKAGE_VERSION_MINOR "${VERLIHUB_VERSION_MINOR}")
SET(CPACK_PACKAGE_VERSION_PATCH "${VERLIHUB_VERSION_PATCH}")
SET(CPACK_PACKAGE_VERSION_TWEAK "${VERLIHUB_VERSION_TWEAK}")
SET(CPACK_PACKAGE_VERSION "${VERLIHUB_VERSION}")
SET(CPACK_DEBIAN_PACKAGE_SECTION "net")
SET(CPACK_DEBIAN_PACKAGE_DEPENDS "zlib, mysql-server-5.5, mysql-client-5.5, geoip-bin, gettext, libssl, openssl, libpcre3")

SET(CPACK_RPM_PACKAGE_LICENSE ${CPACK_RESOURCE_FILE_LICENSE})
SET(CPACK_RPM_PACKAGE_URL "https://github.com/verlihub/")
SET(CPACK_RPM_PACKAGE_GROUP "Application/Internet")
set(CPACK_RPM_PACKAGE_REQUIRES "zlib, openssl, mysql-server >= 5.0, mysql >= 5.0, GeoIP, gettext, pcre")
IF(WITH_PYTHON)
	SET(CPACK_DEBIAN_PACKAGE_DEPENDS "${CPACK_DEBIAN_PACKAGE_DEPENDS}, python (>= 2.5)")
	SET(CPACK_RPM_PACKAGE_REQUIRES "${CPACK_RPM_PACKAGE_REQUIRES}, python2.6 >= 2.6")
ENDIF(WITH_PYTHON)
IF(WITH_LUA)
	SET(CPACK_DEBIAN_PACKAGE_DEPENDS "${CPACK_DEBIAN_PACKAGE_DEPENDS}, lua (>= 5.1)")
	SET(CPACK_RPM_PACKAGE_REQUIRES "${CPACK_RPM_PACKAGE_REQUIRES}, lua >= 5.1")
ENDIF(WITH_LUA)

SET(CPACK_GENERATOR "DEB;RPM;TBZ2")
SET(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${VERLIHUB_VERSION}-${CMAKE_SYSTEM_PROCESSOR}")
SET(CPACK_SET_DESTDIR ON)

INCLUDE(CPack)
# ----------------------------------------------------------------------------------------------------

message(STATUS)
message(STATUS "========== Verlihub Build Information ==========")
message(STATUS "Build Version: ${VERLIHUB_VERSION}")
message(STATUS "Install Prefix (CMAKE_INSTALL_PREFIX): ${CMAKE_INSTALL_PREFIX}")

IF(OK_PLUGINS_COUNT EQUAL ALL_PLUGINS_COUNT)
	message(STATUS "All ${ALL_PLUGINS_COUNT} plugins will be built.")
ELSEIF(OK_PLUGINS_COUNT EQUAL 0)
	message(STATUS "No plugins will be built.")
ELSE()
	message(STATUS "Only ${OK_PLUGINS_COUNT} from ${ALL_PLUGINS_COUNT} plugins will be built.")
ENDIF()

message(STATUS)
message(STATUS "To build and install Verlihub, run \"make\" and \"make install\"")
message(STATUS)
