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

INCLUDE_DIRECTORIES(${VERLIHUB_BINARY_DIR} ${VERLIHUB_SOURCE_DIR}/src)

SET(VERLIHUB_HDRS
	casyncconn.h
	casyncsocketserver.h
	cban.h
	cbanlist.h
	ccallbacklist.h
	cchatconsole.h
	ccommand.h
	ccommandcollection.h
	cconfigbase.h
	cconfigfile.h
	cconfigitembase.h
	cconfmysql.h
	cconnbase.h
	cconnchoose.h
	cconnepoll.h
	cconndc.h
	cconnpoll.h
	cconnselect.h
	cconntypes.h
	ccustomredirect.h
	ccustomredirects.h
	cdbconf.h
	cdcclient.h
	cdcclients.h
	cdccommand.h
	cdcconf.h
	cdcconsole.h
	cdcproto.h
	cdctag.h
	cfreqlimiter.h
	chttpconn.h
	cicuconvert.h
	cmaxminddb.h
	cinfoserver.h
	cinterpolexp.h
	ckick.h
	ckicklist.h
	clog.h
	cmeanfrequency.h
	cmessagedc.h
	cmysql.h
	cobj.h
	cpcre.h
	cpenaltylist.h
	cpluginbase.h
	cpluginloader.h
	cpluginmanager.h
	cprotocol.h
	cprotocommand.h
	cquery.h
	creglist.h
	creguserinfo.h
	cserverdc.h
	csetuplist.h
	ctempfunctionbase.h
	ctime.h
	ctimeout.h
	ctrigger.h
	ctriggers.h
	cuser.h
	cusercollection.h
	cvhplugin.h
	cvhpluginmgr.h
	cworkerpool.h
	cwakeup.h
	csocketcancel.h
	csocketapi.h
	ctlssession.h
	cmysqlworker.h
	czlib.h
	gettext.h
	i18n.h
	script_api.h
	stdafx.h # ?
	stringutils.h
	tcache.h
	tchashlistmap.h
	thasharray.h
	tlistconsole.h
	tlistplugin.h
	tmysqlmemoryhash.h
	tmysqlmemorylist.h
	tmysqlmemoryordlist.h
	tpluginbase.h
)

SET(VERLIHUB_SRCS
	casyncconn.cpp
	casyncsocketserver.cpp
	cban.cpp
	cbanlist.cpp
	ccallbacklist.cpp
	cchatconsole.cpp
	ccommand.cpp
	ccommandcollection.cpp
	cconfigbase.cpp
	cconfigfile.cpp
	cconfigitembase.cpp
	cconfmysql.cpp
	cconnchoose.cpp
	cconnepoll.cpp
	cconndc.cpp
	cconnpoll.cpp
	cconnselect.cpp
	cconntypes.cpp
	ccustomredirect.cpp
	ccustomredirects.cpp
	cdbconf.cpp
	cdcclient.cpp
	cdcclients.cpp
	cdccommand.cpp
	cdcconf.cpp
	cdcconsole.cpp
	cdcproto.cpp
	cdctag.cpp
	cfreqlimiter.cpp
	chttpconn.cpp
	cicuconvert.cpp
	cmaxminddb.cpp
	cinfoserver.cpp
	cinterpolexp.cpp
	ckick.cpp
	ckicklist.cpp
	clog.cpp
	cmeanfrequency.cpp
	cmessagedc.cpp
	cmysql.cpp
	cobj.cpp
	cpcre.cpp
	cpenaltylist.cpp
	cpluginbase.cpp
	cpluginloader.cpp
	cpluginmanager.cpp
	cprotocol.cpp
	cprotocommand.cpp
	cquery.cpp
	creglist.cpp
	creguserinfo.cpp
	cserverdc.cpp
	csetuplist.cpp
	ctempfunctionbase.cpp
	ctime.cpp
	ctimeout.cpp
	ctrigger.cpp
	ctriggers.cpp
	cuser.cpp
	cusercollection.cpp
	cvhplugin.cpp
	cvhpluginmgr.cpp
	cworkerpool.cpp
	cwakeup.cpp
	csocketcancel.cpp
	csocketapi.cpp
	ctlssession.cpp
	cmysqlworker.cpp
	czlib.cpp
	i18n.cpp
	script_api.cpp
	stdafx.cpp # ?
	stringutils.cpp
	thasharray.cpp
	tpluginbase.cpp
	#verlihub.cpp
)

if(HAVE_WEPOLL)
	LIST(APPEND VERLIHUB_SRCS wepoll/wepoll.c) # note: bundled epoll for windows
	LIST(APPEND VERLIHUB_HDRS wepoll/wepoll.h)
endif(HAVE_WEPOLL)

add_library(libverlihub_so SHARED ${VERLIHUB_SRCS})
set_target_properties(libverlihub_so PROPERTIES OUTPUT_NAME ${PROJECT_NAME})

target_link_libraries(libverlihub_so ${VERLIHUB_LIBS}) # pthread dl execinfo

add_library(vhapi_so SHARED "script_api.cpp")
set_target_properties(vhapi_so PROPERTIES OUTPUT_NAME "vhapi")
target_link_libraries(vhapi_so libverlihub_so)

add_executable(verlihub "verlihub.cpp")
target_link_libraries(verlihub libverlihub_so)

# ----------------------------------------------------------------------------------------------------
# generate verlihub_config

MESSAGE(STATUS "[ .. ] Generating configuration executable: ${CMAKE_BINARY_DIR}/verlihub_config")
STRING(REPLACE ";" " " VERLIHUB_USE_LIBS "${VERLIHUB_LIBS}")
string(STRIP "${VERLIHUB_USE_LIBS}" VERLIHUB_USE_LIBS)
CONFIGURE_FILE("${CMAKE_SOURCE_DIR}/cmake/verlihub_config.cm" "${CMAKE_BINARY_DIR}/verlihub_config")

# ----------------------------------------------------------------------------------------------------

# ----------------------------------------------------------------------------------------------------
# package for plugins built outside of verlihub source, found with find_package(Verlihub)

get_directory_property(VERLIHUB_EXPORT_DEFS COMPILE_DEFINITIONS) # note: verlihub headers depend on same defines and config.h, else plugin sees other class layout
list(APPEND VERLIHUB_EXPORT_DEFS U_USING_ICU_NAMESPACE=1) # note: this one is set in compiler flags

string(REGEX REPLACE " *-I([^ ]+)" "\\1;" VERLIHUB_EXPORT_INCS "${VERLIHUB_INCLUDE_DIRS}") # note: headers of libraries that verlihub headers include
list(FILTER VERLIHUB_EXPORT_INCS EXCLUDE REGEX "^$")

set_target_properties(libverlihub_so PROPERTIES EXPORT_NAME verlihub)
target_include_directories(libverlihub_so INTERFACE "$<INSTALL_INTERFACE:include/${PROJECT_NAME}>") # note: install interface only, build inside source is not changed

foreach(INC ${VERLIHUB_EXPORT_INCS})
	target_include_directories(libverlihub_so INTERFACE "$<INSTALL_INTERFACE:${INC}>")
endforeach(INC ${VERLIHUB_EXPORT_INCS})

foreach(DEF ${VERLIHUB_EXPORT_DEFS})
	target_compile_definitions(libverlihub_so INTERFACE "$<INSTALL_INTERFACE:${DEF}>")
endforeach(DEF ${VERLIHUB_EXPORT_DEFS})

SET(VERLIHUB_CMAKE_DIR "${LIBDIR}/cmake/Verlihub")
SET(VERLIHUB_HEADER_DIR "include/${PROJECT_NAME}")
include(CMakePackageConfigHelpers)
configure_package_config_file("${CMAKE_SOURCE_DIR}/cmake/VerlihubConfig.cmake.cm" "${CMAKE_BINARY_DIR}/VerlihubConfig.cmake" INSTALL_DESTINATION ${VERLIHUB_CMAKE_DIR} PATH_VARS VERLIHUB_HEADER_DIR PLUGINDIR)
write_basic_package_version_file("${CMAKE_BINARY_DIR}/VerlihubConfigVersion.cmake" VERSION ${VERLIHUB_VERSION} COMPATIBILITY SameMinorVersion)

# ----------------------------------------------------------------------------------------------------

install(TARGETS libverlihub_so EXPORT VerlihubTargets RUNTIME DESTINATION "bin" LIBRARY DESTINATION ${LIBDIR} ARCHIVE DESTINATION ${LIBDIR}) # note: on windows dll goes to bin and its import library to lib
install(TARGETS vhapi_so verlihub RUNTIME DESTINATION "bin" LIBRARY DESTINATION ${LIBDIR} ARCHIVE DESTINATION ${LIBDIR})
install(EXPORT VerlihubTargets NAMESPACE Verlihub:: DESTINATION ${VERLIHUB_CMAKE_DIR})
install(FILES "${CMAKE_BINARY_DIR}/VerlihubConfig.cmake" "${CMAKE_BINARY_DIR}/VerlihubConfigVersion.cmake" DESTINATION ${VERLIHUB_CMAKE_DIR})

INSTALL(
	FILES "${VERLIHUB_BINARY_DIR}/verlihub_config"
	DESTINATION "bin"
	PERMISSIONS OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE
	WORLD_READ WORLD_EXECUTE
)

if(HAVE_WEPOLL) # note: cconnepoll.h includes it from own folder
	LIST(REMOVE_ITEM VERLIHUB_HDRS wepoll/wepoll.h)
	INSTALL(FILES wepoll/wepoll.h DESTINATION "include/${PROJECT_NAME}/wepoll")
endif(HAVE_WEPOLL)

INSTALL(FILES ${VERLIHUB_HDRS} "${VERLIHUB_BINARY_DIR}/dirsettings.h" "${VERLIHUB_BINARY_DIR}/config.h" DESTINATION "include/${PROJECT_NAME}") # note: headers include config.h, it decides class layout

# end of file
