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
	cmutex.h
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
	cthread.h
	cthreadwork.h
	ctime.h
	ctimeout.h
	ctrigger.h
	ctriggers.h
	cuser.h
	cusercollection.h
	cvhplugin.h
	cvhpluginmgr.h
	cworkerthread.h
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
	cmutex.cpp
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
	cthread.cpp
	cthreadwork.cpp
	ctime.cpp
	ctimeout.cpp
	ctrigger.cpp
	ctriggers.cpp
	cuser.cpp
	cusercollection.cpp
	cvhplugin.cpp
	cvhpluginmgr.cpp
	cworkerthread.cpp
	czlib.cpp
	i18n.cpp
	script_api.cpp
	stdafx.cpp # ?
	stringutils.cpp
	thasharray.cpp
	tpluginbase.cpp
	#verlihub.cpp
)

# ----------------------------------------------------------------------------------------------------
# tls proxy

if(USE_TLS_PROXY AND GOLANG_PATH)
	add_custom_command(
		OUTPUT "${CMAKE_BINARY_DIR}/tls-proxy/libvhproxy.a"
		DEPENDS "${CMAKE_SOURCE_DIR}/tls-proxy/lib/lib.go"
		COMMAND ${GOLANG_PATH} build -buildmode c-archive -o "${CMAKE_BINARY_DIR}/tls-proxy/libvhproxy.a" "${CMAKE_SOURCE_DIR}/tls-proxy/lib/lib.go"
		WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/tls-proxy"
		COMMENT "Building TLS proxy"
	)

	add_custom_target(libvhproxy_a DEPENDS "${CMAKE_BINARY_DIR}/tls-proxy/libvhproxy.a")

	add_custom_command(
		OUTPUT "${CMAKE_SOURCE_DIR}/src/libvhproxy.h"
		DEPENDS "${CMAKE_BINARY_DIR}/tls-proxy/libvhproxy.h"
		COMMAND cp "${CMAKE_BINARY_DIR}/tls-proxy/libvhproxy.h" "${CMAKE_SOURCE_DIR}/src/libvhproxy.h"
		COMMENT "Copying libvhproxy.h header"
	)

	add_custom_target(libvhproxy_h DEPENDS "${CMAKE_SOURCE_DIR}/src/libvhproxy.h")
	add_dependencies(libvhproxy_h libvhproxy_a)

	add_custom_command(
		OUTPUT "${CMAKE_SOURCE_DIR}/src/proxy_types.h"
		DEPENDS "${CMAKE_SOURCE_DIR}/tls-proxy/lib/proxy_types.h"
		COMMAND cp "${CMAKE_SOURCE_DIR}/tls-proxy/lib/proxy_types.h" "${CMAKE_SOURCE_DIR}/src/proxy_types.h"
		COMMENT "Copying proxy_types.h header"
	)

	add_custom_target(proxy_types_h DEPENDS "${CMAKE_SOURCE_DIR}/src/proxy_types.h")
	add_dependencies(proxy_types_h libvhproxy_a)

	list(APPEND ${VERLIHUB_HDRS} "libvhproxy.h" "proxy_types.h")
endif(USE_TLS_PROXY AND GOLANG_PATH)

# ----------------------------------------------------------------------------------------------------

add_library(libverlihub_so SHARED ${VERLIHUB_SRCS})
set_target_properties(libverlihub_so PROPERTIES OUTPUT_NAME ${PROJECT_NAME})
add_dependencies(libverlihub_so libvhproxy_a libvhproxy_h proxy_types_h)

if(USE_TLS_PROXY)
	add_library(vhproxy SHARED IMPORTED)
	set_target_properties(vhproxy PROPERTIES IMPORTED_LOCATION "${CMAKE_BINARY_DIR}/tls-proxy/libvhproxy.a")
	target_link_libraries(libverlihub_so vhproxy)
endif(USE_TLS_PROXY)

target_link_libraries(libverlihub_so ${VERLIHUB_LIBS}) # pthread dl execinfo

add_library(vhapi_so SHARED "script_api.cpp")
set_target_properties(vhapi_so PROPERTIES OUTPUT_NAME "vhapi")
target_link_libraries(vhapi_so libverlihub_so)

add_executable(verlihub "verlihub.cpp")
target_link_libraries(verlihub libverlihub_so)

# ----------------------------------------------------------------------------------------------------
# generate verlihub_config
MESSAGE(STATUS "[ .. ] Generating configuration executable: ${CMAKE_BINARY_DIR}/verlihub_config")
STRING(REPLACE ";" " " VERLIHUB_LIBS "${VERLIHUB_LIBS}")
set(VERLIHUB_USE_LIBS "${VERLIHUB_USE_LIBS} ${VERLIHUB_LIBS}")
CONFIGURE_FILE("${CMAKE_SOURCE_DIR}/cmake/verlihub_config.cm" "${CMAKE_BINARY_DIR}/verlihub_config")
# ----------------------------------------------------------------------------------------------------

#if(USE_TLS_PROXY)
##	#install(FILES "${CMAKE_BINARY_DIR}/tls-proxy/libvhproxy.a" LIBRARY DESTINATION "lib") # old cmake
#	install(IMPORTED_RUNTIME_ARTIFACTS vhproxy LIBRARY DESTINATION "lib")
#endif(USE_TLS_PROXY)

install(TARGETS vhapi_so libverlihub_so verlihub RUNTIME DESTINATION "bin" LIBRARY DESTINATION "lib")

INSTALL(
	FILES "${VERLIHUB_BINARY_DIR}/verlihub_config"
	DESTINATION "bin"
	PERMISSIONS OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE
	WORLD_READ WORLD_EXECUTE
)

INSTALL(FILES ${VERLIHUB_HDRS} "${VERLIHUB_BINARY_DIR}/dirsettings.h" DESTINATION "include/${PROJECT_NAME}")

# end of file
