#	Copyright (C) 2003-2005 Daniel Muller, dan at verliba dot cz
#	Copyright (C) 2006-2026 Verlihub Team, info at verlihub dot net
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

# ----------------------------------------------------------------------------------------------------
# version

cmake_minimum_required(VERSION 3.16...3.50)
#cmake_policy(VERSION 3.16)

PROJECT(VERLIHUB)
SET(PROJECT_NAME "verlihub")

SET(VERLIHUB_VERSION_MAJOR 1) # note: these must be numbers
SET(VERLIHUB_VERSION_MINOR 7)
SET(VERLIHUB_VERSION_PATCH 0)
SET(VERLIHUB_VERSION_TWEAK 0)
SET(VERLIHUB_VERSION "${VERLIHUB_VERSION_MAJOR}.${VERLIHUB_VERSION_MINOR}.${VERLIHUB_VERSION_PATCH}.${VERLIHUB_VERSION_TWEAK}")

# ----------------------------------------------------------------------------------------------------

# ----------------------------------------------------------------------------------------------------
# header

message(STATUS)
message(STATUS "[ ?? ] You can change build options with: -D{OPTION}=ON|OFF")
message(STATUS "[ ?? ] You can enable debug build with: -DDEFINE_DEBUG=ON")
message(STATUS "[ ?? ] You can disable all plugins with: -DWITH_PLUGINS=OFF")
message(STATUS "[ ?? ] You can build without TLS support with: -DUSE_TLS=OFF")
message(STATUS "[ ?? ] You can disable Lua plugin with: -DWITH_LUA=OFF")
message(STATUS "[ ?? ] You can use poll (select on Windows) instead of epoll connection chooser with: -DUSE_EPOLL=OFF")
message(STATUS "[ ?? ] If you get errors related to autosprintf, please try with: -DUSE_CUSTOM_AUTOSPRINTF=ON")
message(STATUS)

# ----------------------------------------------------------------------------------------------------

# ----------------------------------------------------------------------------------------------------
# defaults

if(WIN32) # note: it would be installed into cmake folder of msys2, outside of install prefix, use find_package(Verlihub CONFIG) there
	OPTION(INSTALL_CMAKE_FILES "Install FindVerlihub.cmake into CMake root" OFF)
else()
	OPTION(INSTALL_CMAKE_FILES "Install FindVerlihub.cmake into CMake root" ON)
endif()
option(USE_CUSTOM_AUTOSPRINTF "Build using custom internal autosprintf implementation" OFF)
OPTION(DEFINE_DEBUG "Build using debugging code" OFF)
option(USE_TLS "Build with TLS support using OpenSSL" ON)
option(USE_EPOLL "Use epoll connection chooser when available, else poll, on Windows bundled wepoll, else select" ON)

# ----------------------------------------------------------------------------------------------------

# ----------------------------------------------------------------------------------------------------
# prepare

SET(PLUGINS_LIST "")
SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules")

if(INSTALL_CMAKE_FILES)
	install(FILES "${CMAKE_SOURCE_DIR}/cmake/Modules/FindVerlihub.cmake" DESTINATION "${CMAKE_ROOT}/Modules")
endif(INSTALL_CMAKE_FILES)

INCLUDE(CheckIncludeFiles)
INCLUDE(MacroLogFeature)
INCLUDE(VerlihubMacro)

# ----------------------------------------------------------------------------------------------------

# ----------------------------------------------------------------------------------------------------
# system

IF(WIN32) # note: also when building with mingw from msys2 shell
	ADD_DEFINITIONS(-DHAVE_WINDOWS -D_WIN32_WINNT=0x0601 -DWIN32_LEAN_AND_MEAN -DNOMINMAX -DNOGDI) # note: windows 7 and later, small windows headers, no min and max macros that break std::min and std::max, no gdi macros like DEFAULT_CHARSET
	ADD_DEFINITIONS(-DFD_SETSIZE=4096) # note: windows select handles only 64 sockets by default, it must be set before winsock2.h everywhere
	ADD_DEFINITIONS(-D_POSIX_THREAD_SAFE_FUNCTIONS=200112L) # note: mingw time.h gives localtime_r and gmtime_r only with this
	SET(HAVE_WINDOWS TRUE)
ENDIF(WIN32)

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
	SET(CMAKE_FIND_ROOT_PATH ${CMAKE_FIND_ROOT_PATH} /usr/local/opt /usr/local/opt/gettext) # workaround for homebrew path
	SET(GETTEXT_EXEC_PATH "/usr/local/opt/gettext")
	SET(OPENSSL_ROOT_DIR "/usr/local/opt/openssl")
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

IF(EXISTS "/etc/alpine-release")
	ADD_DEFINITIONS(-DHAVE_ALPINE)
	SET(HAVE_ALPINE TRUE)
ENDIF(EXISTS "/etc/alpine-release")

# ----------------------------------------------------------------------------------------------------

# ----------------------------------------------------------------------------------------------------
# paths

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

# ----------------------------------------------------------------------------------------------------
# dependencies

message(STATUS "[ .. ] Looking for dependencies")
message(STATUS)

Find_Package(OpenSSL 1.1.0 REQUIRED)
IF(NOT WIN32) # note: windows has no crypt, openssl does the same there
	Find_Package(Crypt REQUIRED)
ENDIF(NOT WIN32)

Find_Package(MySQL REQUIRED)
Find_Package(ZLIB REQUIRED)
Find_Package(MaxMindDB REQUIRED)
Find_Package(ICU COMPONENTS data i18n uc io REQUIRED)
Find_Package(DL REQUIRED) # note: on windows this is dlfcn library
Find_Package(Pcre2 REQUIRED)
Find_Package(Threads REQUIRED)
Find_Package(Gettext REQUIRED)
Find_Package(Libintl REQUIRED)

if(USE_CUSTOM_AUTOSPRINTF)
	message(STATUS "[ OK ] Using custom internal autosprintf implementation")
else(USE_CUSTOM_AUTOSPRINTF)
	find_package(Asprintf REQUIRED)
endif(USE_CUSTOM_AUTOSPRINTF)

IF(HAVE_BSD OR HAVE_ALPINE)
	Find_Package(ExecInfo REQUIRED)
ENDIF(HAVE_BSD OR HAVE_ALPINE)

IF(HAVE_BSD)
	Find_Package(SysInfo REQUIRED)
ENDIF(HAVE_BSD)

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

IF(DEFINE_DEBUG)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g") # -Wextra
	SET(CMAKE_BUILD_TYPE "DEBUG")
ELSE(DEFINE_DEBUG)
	SET(CMAKE_BUILD_TYPE "RELEASE")
ENDIF(DEFINE_DEBUG)

if(USE_CUSTOM_AUTOSPRINTF)
	add_definitions(-DUSE_CUSTOM_AUTOSPRINTF=1)
endif(USE_CUSTOM_AUTOSPRINTF)

string(STRIP "${CMAKE_CXX_FLAGS}" CMAKE_CXX_FLAGS)

# ----------------------------------------------------------------------------------------------------

# ----------------------------------------------------------------------------------------------------
# includes

SET(VERLIHUB_LIBS "")

if(OPENSSL_LIBRARIES)
	set(VERLIHUB_LIBS "${VERLIHUB_LIBS} ${OPENSSL_LIBRARIES}")
endif(OPENSSL_LIBRARIES)

if(CRYPT_LIBRARIES)
	set(VERLIHUB_LIBS "${VERLIHUB_LIBS} ${CRYPT_LIBRARIES}")
endif(CRYPT_LIBRARIES)

if(MYSQL_LIBRARIES)
	set(VERLIHUB_LIBS "${VERLIHUB_LIBS} ${MYSQL_LIBRARIES}")
endif(MYSQL_LIBRARIES)

if(MAXMINDDB_LIBRARIES)
	set(VERLIHUB_LIBS "${VERLIHUB_LIBS} ${MAXMINDDB_LIBRARIES}")
endif(MAXMINDDB_LIBRARIES)

if(ICU_LIBRARIES)
	set(VERLIHUB_LIBS "${VERLIHUB_LIBS} ${ICU_LIBRARIES}")
endif(ICU_LIBRARIES)

if(ZLIB_LIBRARIES)
	set(VERLIHUB_LIBS "${VERLIHUB_LIBS} ${ZLIB_LIBRARIES}")
endif(ZLIB_LIBRARIES)

if(PCRE2_LIBRARIES)
	set(VERLIHUB_LIBS "${VERLIHUB_LIBS} ${PCRE2_LIBRARIES}")
endif(PCRE2_LIBRARIES)

if(CMAKE_THREAD_LIBS_INIT)
	set(VERLIHUB_LIBS "${VERLIHUB_LIBS} ${CMAKE_THREAD_LIBS_INIT}")
endif(CMAKE_THREAD_LIBS_INIT)

if(LIBINTL_LIBRARIES)
	set(VERLIHUB_LIBS "${VERLIHUB_LIBS} ${LIBINTL_LIBRARIES}")
endif(LIBINTL_LIBRARIES)

if(GETTEXT_ASPRINTF_LIBRARY)
	set(VERLIHUB_LIBS "${VERLIHUB_LIBS} ${GETTEXT_ASPRINTF_LIBRARY}")
endif(GETTEXT_ASPRINTF_LIBRARY)

if(EXECINFO_LIBRARY)
	IF(HAVE_BSD OR HAVE_ALPINE)
		SET(VERLIHUB_LIBS "${VERLIHUB_LIBS} ${EXECINFO_LIBRARY}")
	ENDIF(HAVE_BSD OR HAVE_ALPINE)
endif(EXECINFO_LIBRARY)

if(SYSINFO_LIBRARY)
	IF(HAVE_BSD)
		SET(VERLIHUB_LIBS "${VERLIHUB_LIBS} ${SYSINFO_LIBRARY}")
	ENDIF(HAVE_BSD)
endif(SYSINFO_LIBRARY)

if(DL_LIBRARIES) # note: newer glibc has it built in, but windows needs it
	set(VERLIHUB_LIBS "${VERLIHUB_LIBS} ${DL_LIBRARIES}")
endif(DL_LIBRARIES)

IF(WIN32)
	SET(VERLIHUB_LIBS "${VERLIHUB_LIBS} ws2_32 dbghelp") # note: sockets and stack backtrace
ENDIF(WIN32)

string(STRIP "${VERLIHUB_LIBS}" VERLIHUB_LIBS)
STRING(REPLACE " " ";" VERLIHUB_LIBS "${VERLIHUB_LIBS}")
STRING(REPLACE ";;" ";" VERLIHUB_LIBS "${VERLIHUB_LIBS}")
SET(VERLIHUB_INCLUDE_DIRS "")

if(OPENSSL_INCLUDE_DIR)
	SET(VERLIHUB_INCLUDE_DIRS "${VERLIHUB_INCLUDE_DIRS} -I${OPENSSL_INCLUDE_DIR}")
	INCLUDE_DIRECTORIES(${OPENSSL_INCLUDE_DIR})
endif(OPENSSL_INCLUDE_DIR)

if(MYSQL_INCLUDE_DIR)
	SET(VERLIHUB_INCLUDE_DIRS "${VERLIHUB_INCLUDE_DIRS} -I${MYSQL_INCLUDE_DIR}")
	INCLUDE_DIRECTORIES(${MYSQL_INCLUDE_DIR})
endif(MYSQL_INCLUDE_DIR)

if(MAXMINDDB_INCLUDE_DIR)
	SET(VERLIHUB_INCLUDE_DIRS "${VERLIHUB_INCLUDE_DIRS} -I${MAXMINDDB_INCLUDE_DIR}")
	INCLUDE_DIRECTORIES(${MAXMINDDB_INCLUDE_DIR})
endif(MAXMINDDB_INCLUDE_DIR)

if(ICU_INCLUDE_DIR)
	SET(VERLIHUB_INCLUDE_DIRS "${VERLIHUB_INCLUDE_DIRS} -I${ICU_INCLUDE_DIR}")
	INCLUDE_DIRECTORIES(${ICU_INCLUDE_DIR})
endif(ICU_INCLUDE_DIR)

if(ZLIB_INCLUDE_DIRS)
	SET(VERLIHUB_INCLUDE_DIRS "${VERLIHUB_INCLUDE_DIRS} -I${ZLIB_INCLUDE_DIRS}")
	INCLUDE_DIRECTORIES(${ZLIB_INCLUDE_DIRS})
endif(ZLIB_INCLUDE_DIRS)

if(PCRE2_INCLUDE_DIR)
	SET(VERLIHUB_INCLUDE_DIRS "${VERLIHUB_INCLUDE_DIRS} -I${PCRE2_INCLUDE_DIR}")
	INCLUDE_DIRECTORIES(${PCRE2_INCLUDE_DIR})
endif(PCRE2_INCLUDE_DIR)

if(LIBINTL_INCLUDE_DIR)
	SET(VERLIHUB_INCLUDE_DIRS "${VERLIHUB_INCLUDE_DIRS} -I${LIBINTL_INCLUDE_DIR}")
	INCLUDE_DIRECTORIES(${LIBINTL_INCLUDE_DIR})
endif(LIBINTL_INCLUDE_DIR)

IF(HAVE_BSD)
	if(EXECINFO_INCLUDE_DIR)
		SET(VERLIHUB_INCLUDE_DIRS "${VERLIHUB_INCLUDE_DIRS} -I${EXECINFO_INCLUDE_DIR}")
		INCLUDE_DIRECTORIES(${EXECINFO_INCLUDE_DIR})
	endif(EXECINFO_INCLUDE_DIR)

	if(SYSINFO_INCLUDE_DIR)
		SET(VERLIHUB_INCLUDE_DIRS "${VERLIHUB_INCLUDE_DIRS} -I${SYSINFO_INCLUDE_DIR}")
		INCLUDE_DIRECTORIES(${SYSINFO_INCLUDE_DIR})
	endif(SYSINFO_INCLUDE_DIR)
ENDIF(HAVE_BSD)

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR} "${CMAKE_SOURCE_DIR}/src" ${CMAKE_BINARY_DIR})
string(STRIP "${VERLIHUB_INCLUDE_DIRS}" VERLIHUB_INCLUDE_DIRS)

# ----------------------------------------------------------------------------------------------------

# ----------------------------------------------------------------------------------------------------
# headers

CHECK_INCLUDE_FILES(errno.h HAVE_ERRNO_H)
CHECK_INCLUDE_FILES(poll.h HAVE_SYS_POLL_H)

if(USE_EPOLL AND WIN32)
	SET(HAVE_WEPOLL TRUE) # note: windows has no epoll, bundled wepoll gives same api, see ThirdParty.md
elseif(USE_EPOLL)
	CHECK_INCLUDE_FILES(sys/epoll.h HAVE_SYS_EPOLL_H)
else()
	unset(HAVE_SYS_EPOLL_H CACHE) # note: result of earlier check is cached
endif()

if(HAVE_SYS_EPOLL_H)
	message(STATUS "[ OK ] Using epoll connection chooser")
elseif(HAVE_WEPOLL)
	message(STATUS "[ OK ] Using epoll connection chooser (bundled wepoll)")
elseif(HAVE_SYS_POLL_H)
	message(STATUS "[ OK ] Using poll connection chooser")
else()
	message(STATUS "[ OK ] Using select connection chooser")
endif()
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

ADD_SUBDIRECTORY(src)
ADD_SUBDIRECTORY(plugins)
ADD_SUBDIRECTORY(share)
ADD_SUBDIRECTORY(po)

IF(NOT WIN32) # note: man pages are not for windows
	ADD_SUBDIRECTORY(man)
ENDIF(NOT WIN32)

# ----------------------------------------------------------------------------------------------------
# todo: build a cpack driven installer package

#INCLUDE(InstallRequiredSystemLibraries)
#SET(CPACK_PACKAGE_NAME "verlihub")
#SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/License.md")
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
#SET(CPACK_DEBIAN_PACKAGE_DEPENDS "zlib, mysql-server-5.5, mysql-client-5.5, mmdb-bin, gettext, libssl, openssl, libpcre2-8-0")
#
#SET(CPACK_RPM_PACKAGE_LICENSE ${CPACK_RESOURCE_FILE_LICENSE})
#SET(CPACK_RPM_PACKAGE_URL "https://github.com/verlihub/")
#SET(CPACK_RPM_PACKAGE_GROUP "Application/Internet")
#SET(CPACK_RPM_PACKAGE_REQUIRES "zlib, openssl, mysql-server >= 5.5, mysql >= 5.5, maxminddb, gettext, pcre2")
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

# ----------------------------------------------------------------------------------------------------
# information

set(LIBS_INFO "${VERLIHUB_LIBS}")
string(REPLACE ";" "\n" LIBS_INFO "${LIBS_INFO}")

set(DIRS_INFO "${VERLIHUB_INCLUDE_DIRS}")
string(REPLACE " " "\n" DIRS_INFO "${DIRS_INFO}")

string(STRIP "${PLUGINS_LIST}" PLUGINS_LIST)

if(NOT PLUGINS_LIST)
	set(PLUGINS_LIST "<NONE>")
endif(NOT PLUGINS_LIST)

message(STATUS)
message(STATUS "========== Build information ==========")
message(STATUS)
message(STATUS "[ ** ] Used libraries:\n\n${LIBS_INFO}\n")
message(STATUS "[ ** ] Used includes:\n\n${DIRS_INFO}\n")
message(STATUS "[ ** ] CMake version: ${CMAKE_VERSION}")
message(STATUS "[ ** ] Compiler version: ${CMAKE_CXX_COMPILER} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "[ ** ] Compiler flags: ${CMAKE_CXX_FLAGS}")
message(STATUS "[ ** ] C++ standard version: ${CMAKE_CXX_STANDARD}")
message(STATUS "[ ** ] Verlihub version: ${VERLIHUB_VERSION}")
message(STATUS "[ ** ] Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "[ ** ] Installation prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "[ ** ] Building plugins: ${PLUGINS_LIST}")
if(USE_TLS)
	message(STATUS "[ ** ] TLS support: ON (OpenSSL ${OPENSSL_VERSION})")
else()
	message(STATUS "[ ** ] TLS support: OFF")
endif()
if(WIN32)
	message(STATUS "[ ** ] Now run \"mingw32-make\" and then \"mingw32-make install\"")
else()
	message(STATUS "[ ** ] Now run \"make\" as current user and \"make install\" as super user")
	message(STATUS "[ ** ] Then run \"ldconfig\" as super user to avoid shared library errors")
endif()
message(STATUS)
message(STATUS "================= End =================")
message(STATUS)

# ----------------------------------------------------------------------------------------------------

# end of file
