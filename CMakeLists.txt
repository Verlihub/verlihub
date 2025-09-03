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

cmake_minimum_required(VERSION 3.16...3.50)
#cmake_policy(VERSION 3.16)

PROJECT(VERLIHUB)
SET(PROJECT_NAME "verlihub")

SET(VERLIHUB_VERSION_MAJOR 1) # note: these must be numbers
SET(VERLIHUB_VERSION_MINOR 6)
SET(VERLIHUB_VERSION_PATCH 1)
SET(VERLIHUB_VERSION_TWEAK 0)
SET(VERLIHUB_VERSION "${VERLIHUB_VERSION_MAJOR}.${VERLIHUB_VERSION_MINOR}.${VERLIHUB_VERSION_PATCH}.${VERLIHUB_VERSION_TWEAK}")

message(STATUS)
message(STATUS "[ ?? ] You can change build options with: -D{OPTION}=ON|OFF")
message(STATUS "[ ?? ] You can enable debug build with: -DDEFINE_DEBUG=ON")
message(STATUS "[ ?? ] You can disable all plugins with: -DWITH_PLUGINS=OFF")
message(STATUS "[ ?? ] You can build TLS proxy library with: -DUSE_TLS_PROXY=ON")
#message(STATUS "[ ?? ] You can disable SSL secured connects with: -DUSE_SSL_CONNECTS=OFF")
message(STATUS "[ !! ] NOTE: Plugins other than LUA are now DEPRECATED")
message(STATUS "[ ?? ] You can enable or disable specific plugin with: -DWITH_{NAME}=[ON/OFF]")
message(STATUS "[ ?? ] Plugin names: LUA PYTHON PERL FORBID CHATROOM IPLOG ISP MESSENGER STATS REPLACER FLOODPROT")
message(STATUS "[ ?? ] If you get errors related to autosprintf, please try with: -DUSE_CUSTOM_AUTOSPRINTF=ON")
message(STATUS)

SET(OK_PLUGINS_COUNT 0)
SET(ALL_PLUGINS_COUNT 12)

SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules")
OPTION(INSTALL_CMAKE_FILES "Install *.cmake files into CMake root" ON)

if(INSTALL_CMAKE_FILES)
	install(FILES "${CMAKE_SOURCE_DIR}/cmake/Modules/FindVerlihub.cmake" DESTINATION "${CMAKE_ROOT}/Modules")
endif(INSTALL_CMAKE_FILES)

INCLUDE(CheckIncludeFiles)
INCLUDE(MacroLogFeature)
INCLUDE(VerlihubMacro)

# ----------------------------------------------------------------------------------------------------
# detect os

#IF(${CMAKE_SYSTEM_NAME} MATCHES "Windows") # detect os
	#ADD_DEFINITIONS(-D_WIN32 1) # WIN32 flag already exists
#ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Windows")

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
	# RPATH setting from https://cmake.org/Wiki/CMake_RPATH_handling#Mac_OS_X_and_the_RPATH (should use cmake >= 2.8.12)
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
	SET(CMAKE_FIND_ROOT_PATH ${CMAKE_FIND_ROOT_PATH} /usr/local/opt /usr/local/opt/gettext /usr/local/opt/perl) # workaround for homebrew path
	SET(GETTEXT_EXEC_PATH "/usr/local/opt/gettext")
	SET(OPENSSL_ROOT_DIR "/usr/local/opt/openssl")
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

IF(EXISTS "/etc/alpine-release")
	ADD_DEFINITIONS(-DHAVE_ALPINE)
	SET(HAVE_ALPINE TRUE)
ENDIF(EXISTS "/etc/alpine-release")

# ----------------------------------------------------------------------------------------------------

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
	SET(PLUGINDIR ${LIBDIR})
ENDIF(NOT PLUGINDIR)

# ----------------------------------------------------------------------------------------------------
# dependencies

message(STATUS "[ .. ] Looking for dependencies")
message(STATUS)

Find_Package(OpenSSL 1.1.0 REQUIRED)
Find_Package(Crypt REQUIRED)
Find_Package(MySQL REQUIRED)
Find_Package(ZLIB REQUIRED)
Find_Package(MaxMindDB REQUIRED)
#Find_Package(Iconv REQUIRED)
Find_Package(ICU REQUIRED)
Find_Package(DL REQUIRED)
Find_Package(Pcre REQUIRED)
Find_Package(Threads REQUIRED)
Find_Package(Gettext REQUIRED)
Find_Package(Gettext-devel REQUIRED)
Find_Package(Libintl REQUIRED)
Find_Library(GETTEXT_ASPRINTF_LIBRARY asprintf DOC "Gettext asprintf library")

IF(HAVE_BSD OR HAVE_ALPINE)
	Find_Package(ExecInfo REQUIRED)
ENDIF(HAVE_BSD OR HAVE_ALPINE)

IF(HAVE_BSD)
	Find_Package(SysInfo REQUIRED)
	#Find_Package(LibKVM) # optional
ENDIF(HAVE_BSD)

if(USE_TLS_PROXY)
	find_package(Go REQUIRED)
endif(USE_TLS_PROXY)

IF(UNIX)
	FIND_PROGRAM(BASH_PATH bash
		PATHS
			/usr/bin
			/usr/local/bin
	)

	IF(NOT BASH_PATH)
		MESSAGE(FATAL_ERROR "[ ER ] Bash shell not found, please install it via your package manager or compile from source: https://ftp.gnu.org/gnu/bash/")
	ENDIF(NOT BASH_PATH)
ENDIF(UNIX)

# ----------------------------------------------------------------------------------------------------

# ----------------------------------------------------------------------------------------------------
# flags

SET(CMAKE_CXX_STANDARD 17) # note: required by icu 75+
SET(CMAKE_CXX_STANDARD_REQUIRED ON)
SET(CMAKE_CXX_EXTENSIONS OFF)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DU_USING_ICU_NAMESPACE=1")

IF(HAVE_BSD)
	SET(CMAKE_CXX_COMPILER "clang++")
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
ELSE(HAVE_BSD)
	set(CMAKE_CXX_COMPILER "g++")
	set(CMAKE_C_COMPILER "gcc")
ENDIF(HAVE_BSD)

IF(HAVE_ALPINE)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated")
ENDIF(HAVE_ALPINE)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -O3 -Wno-overloaded-virtual") # todo: -Woverloaded-virtual -pthread

OPTION(DEFINE_DEBUG "Build using debugging code" OFF)

IF(DEFINE_DEBUG)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g") # -Wextra
	SET(CMAKE_BUILD_TYPE "DEBUG")
ELSE(DEFINE_DEBUG)
	SET(CMAKE_BUILD_TYPE "RELEASE")
ENDIF(DEFINE_DEBUG)

SET(VERLIHUB_USE_LIBS "-lverlihub -lvhapi")

# ----------------------------------------------------------------------------------------------------

# ----------------------------------------------------------------------------------------------------
# autosprintf

OPTION(USE_CUSTOM_AUTOSPRINTF "Build using custom internal autosprintf implementation" OFF)

IF(USE_CUSTOM_AUTOSPRINTF)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DUSE_CUSTOM_AUTOSPRINTF")
	SET(GETTEXT_ASPRINTF_LIBRARY "")
	MESSAGE(STATUS "[ OK ] Using custom internal autosprintf implementation")
ELSE(USE_CUSTOM_AUTOSPRINTF)
	IF(GETTEXT_ASPRINTF_LIBRARY)
		MESSAGE(STATUS "[ OK ] Found asprintf library: ${GETTEXT_ASPRINTF_LIBRARY}")
	ELSE(GETTEXT_ASPRINTF_LIBRARY)
		MESSAGE(FATAL_ERROR "[ ER ] asprintf library not found, please install it via your package manager or compile from source, else please try with: -DUSE_CUSTOM_AUTOSPRINTF=ON")
	ENDIF(GETTEXT_ASPRINTF_LIBRARY)
ENDIF(USE_CUSTOM_AUTOSPRINTF)

# ----------------------------------------------------------------------------------------------------

# ----------------------------------------------------------------------------------------------------
# tls proxy

option(USE_TLS_PROXY "Build TLS proxy library" OFF)

if(USE_TLS_PROXY)
	add_definitions(-DUSE_TLS_PROXY=1)
	set(VERLIHUB_USE_LIBS "${VERLIHUB_USE_LIBS} -lvhproxy")
endif(USE_TLS_PROXY)

#ADD_DEFINITIONS(-DUSE_SSL_CONNECTS)
#OPTION(USE_SSL_CONNECTS "Use SSL secured connects" ON) # use cmake -DUSE_SSL_CONNECTS=OFF to disable ssl

# ----------------------------------------------------------------------------------------------------

# ----------------------------------------------------------------------------------------------------
# includes

SET(VERLIHUB_LIBS
	${OPENSSL_LIBRARIES}
	${CRYPT_LIBRARIES}
	${MYSQL_LIBRARIES}
	${MAXMINDDB_LIBRARIES}
	${ICU_LIBRARIES}
	${ZLIB_LIBRARIES}
	${PCRE_LIBRARIES}
	${CMAKE_THREAD_LIBS_INIT}
	${GETTEXT_ASPRINTF_LIBRARY}
	${LIBINTL_LIBRARIES}
	#${ICONV_LIBRARIES}
)

IF(HAVE_BSD OR HAVE_ALPINE)
	SET(VERLIHUB_LIBS "${VERLIHUB_LIBS} ${EXECINFO_LIBRARY}")
ENDIF(HAVE_BSD OR HAVE_ALPINE)

IF(HAVE_BSD)
	SET(VERLIHUB_LIBS "${VERLIHUB_LIBS} ${SYSINFO_LIBRARY}")

	#IF(HAVE_LIBKVM)
		#SET(VERLIHUB_LIBS "${VERLIHUB_LIBS} ${LIBKVM_LIBRARY}")
	#ENDIF(HAVE_LIBKVM)
ENDIF(HAVE_BSD)

SET(VERLIHUB_INCLUDE_DIRS
	"-I${OPENSSL_INCLUDE_DIR} -I${MYSQL_INCLUDE_DIR} -I${MAXMINDDB_INCLUDE_DIR} -I${ICU_INCLUDE_DIR} -I${ZLIB_INCLUDE_DIRS} -I${PCRE_INCLUDE_DIRS} -I${LIBINTL_INCLUDE_DIR}"
) # -I${ICONV_INCLUDE_DIR}

INCLUDE_DIRECTORIES(
	${OPENSSL_INCLUDE_DIR}
	${MYSQL_INCLUDE_DIR}
	${ZLIB_INCLUDE_DIR}
	${MAXMINDDB_INCLUDE_DIR}
	${ICU_INCLUDE_DIR}
	${PCRE_INCLUDE_DIR}
	${LIBINTL_INCLUDE_DIR}
	# ${ICONV_INCLUDE_DIR}
	${CMAKE_SOURCE_DIR}
	"${CMAKE_SOURCE_DIR}/src"
	${CMAKE_BINARY_DIR}
	#"${CMAKE_BINARY_DIR}/tls-proxy"
)

IF(HAVE_BSD)
	SET(VERLIHUB_INCLUDE_DIRS "${VERLIHUB_INCLUDE_DIRS} -I${EXECINFO_INCLUDE_DIR} -I${SYSINFO_INCLUDE_DIR}")
	INCLUDE_DIRECTORIES(${EXECINFO_INCLUDE_DIR} ${SYSINFO_INCLUDE_DIR})

	#IF(HAVE_LIBKVM)
		#SET(VERLIHUB_INCLUDE_DIRS "${VERLIHUB_INCLUDE_DIRS} -I${LIBKVM_INCLUDE_DIR}")
		#INCLUDE_DIRECTORIES(${LIBKVM_INCLUDE_DIR})
	#ENDIF(HAVE_LIBKVM)
ENDIF(HAVE_BSD)

# ----------------------------------------------------------------------------------------------------

# ----------------------------------------------------------------------------------------------------
# headers

CHECK_INCLUDE_FILES(errno.h HAVE_ERRNO_H)
CHECK_INCLUDE_FILES(poll.h HAVE_SYS_POLL_H)
CHECK_INCLUDE_FILES(getopt.h HAVE_GETOPT_H)
CHECK_INCLUDE_FILES(syslog.h HAVE_SYSLOG_H)

IF(HAVE_SYSLOG_H)
	ADD_DEFINITIONS(-DENABLE_SYSLOG)
ENDIF(HAVE_SYSLOG_H)

IF(NOT HAVE_GETOPT_H)
	MESSAGE(FATAL_ERROR "[ ER ] getopt.h header not found, it is required to build Verlihub")
ENDIF(NOT HAVE_GETOPT_H)

ADD_DEFINITIONS(-DHAVE_CONFIG_H)

# ----------------------------------------------------------------------------------------------------

# ----------------------------------------------------------------------------------------------------
# configuration

message(STATUS)

MESSAGE(STATUS "[ .. ] Generating configuration file: ${CMAKE_BINARY_DIR}/config.h")
CONFIGURE_FILE("${CMAKE_SOURCE_DIR}/cmake/config.h.cm" "${CMAKE_BINARY_DIR}/config.h")

MESSAGE(STATUS "[ .. ] Generating directory settings file: ${CMAKE_BINARY_DIR}/dirsettings.h")
CONFIGURE_FILE("${CMAKE_SOURCE_DIR}/cmake/dirsettings.h.cm" "${CMAKE_BINARY_DIR}/dirsettings.h")

MESSAGE(STATUS "[ .. ] Generating uninstall file: ${CMAKE_BINARY_DIR}/cmake_uninstall.cmake")
CONFIGURE_FILE("${CMAKE_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in" "${CMAKE_BINARY_DIR}/cmake_uninstall.cmake" IMMEDIATE @ONLY)
ADD_CUSTOM_TARGET(uninstall "${CMAKE_COMMAND}" -P "${PROJECT_BINARY_DIR}/cmake/cmake_uninstall.cmake")

# ----------------------------------------------------------------------------------------------------

# ----------------------------------------------------------------------------------------------------
# directories

if(USE_TLS_PROXY)
	make_directory("${CMAKE_BINARY_DIR}/tls-proxy")
endif(USE_TLS_PROXY)

ADD_SUBDIRECTORY(src)
ADD_SUBDIRECTORY(plugins)
ADD_SUBDIRECTORY(share)
ADD_SUBDIRECTORY(scripts)
ADD_SUBDIRECTORY(po)
ADD_SUBDIRECTORY(man)

# ----------------------------------------------------------------------------------------------------
# todo: build a cpack driven installer package

#INCLUDE(InstallRequiredSystemLibraries)
#SET(CPACK_PACKAGE_NAME "verlihub")
#SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/COPYING")
#SET(CPACK_PACKAGE_DESCRIPTION "Verlihub Direct Connect Hub Server")
#SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Verlihub Direct Connect Hub Server")
#
#SET(CPACK_PACKAGE_CONTACT "info@verlihub.net")
#SET(CPACK_PACKAGE_VENDOR "Verlihub Team")
#SET(CPACK_PACKAGE_VERSION_MAJOR "${VERLIHUB_VERSION_MAJOR}")
#SET(CPACK_PACKAGE_VERSION_MINOR "${VERLIHUB_VERSION_MINOR}")
#SET(CPACK_PACKAGE_VERSION_PATCH "${VERLIHUB_VERSION_PATCH}")
#SET(CPACK_PACKAGE_VERSION_TWEAK "${VERLIHUB_VERSION_TWEAK}")
#SET(CPACK_PACKAGE_VERSION "${VERLIHUB_VERSION}")
#SET(CPACK_DEBIAN_PACKAGE_SECTION "net")
#SET(CPACK_DEBIAN_PACKAGE_DEPENDS "zlib, mysql-server-5.5, mysql-client-5.5, mmdb-bin, gettext, libssl, openssl, libpcre3")
#
#SET(CPACK_RPM_PACKAGE_LICENSE ${CPACK_RESOURCE_FILE_LICENSE})
#SET(CPACK_RPM_PACKAGE_URL "https://github.com/verlihub/")
#SET(CPACK_RPM_PACKAGE_GROUP "Application/Internet")
#SET(CPACK_RPM_PACKAGE_REQUIRES "zlib, openssl, mysql-server >= 5.5, mysql >= 5.5, maxminddb, gettext, pcre")
#
#IF(WITH_PYTHON)
#	SET(CPACK_DEBIAN_PACKAGE_DEPENDS "${CPACK_DEBIAN_PACKAGE_DEPENDS}, python (>= 2.7)")
#	SET(CPACK_RPM_PACKAGE_REQUIRES "${CPACK_RPM_PACKAGE_REQUIRES}, python2.7 >= 2.7")
#ENDIF(WITH_PYTHON)
#
#IF(WITH_LUA)
#	SET(CPACK_DEBIAN_PACKAGE_DEPENDS "${CPACK_DEBIAN_PACKAGE_DEPENDS}, lua (>= 5.2)")
#	SET(CPACK_RPM_PACKAGE_REQUIRES "${CPACK_RPM_PACKAGE_REQUIRES}, lua >= 5.2")
#ENDIF(WITH_LUA)

#SET(CPACK_GENERATOR "DEB;RPM;TBZ2")
#SET(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${VERLIHUB_VERSION}-${CMAKE_SYSTEM_PROCESSOR}")
#SET(CPACK_SET_DESTDIR ON)
#INCLUDE(CPack)

# ----------------------------------------------------------------------------------------------------

message(STATUS)
message(STATUS "========== Build information ==========")
message(STATUS)
MESSAGE(STATUS "[ ** ] Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "[ ** ] Hub version: ${VERLIHUB_VERSION}")
message(STATUS "[ ** ] CMake installation prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "[ ** ] Library directory: ${CMAKE_INSTALL_PREFIX}/${LIBDIR}")
message(STATUS "[ ** ] Building plugins: ${OK_PLUGINS_COUNT} of ${ALL_PLUGINS_COUNT}")
message(STATUS "[ ** ] Building TLS proxy: ${USE_TLS_PROXY}")
message(STATUS "[ ** ] Now run \"make\" as current user and \"make install\" as super user")
message(STATUS "[ ** ] Then run \"ldconfig\" as super user to avoid shared library errors")
message(STATUS)
message(STATUS "================= End =================")
message(STATUS)

# end of file
