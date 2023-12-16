#	Copyright (C) 2003-2005 Daniel Muller, dan at verliba dot cz
#	Copyright (C) 2006-2024 Verlihub Team, info at verlihub dot net
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
# Generate make_time.h
ADD_CUSTOM_COMMAND(
	OUTPUT make_time.h
	COMMAND ${BASH_PATH} "echo \"#define VERLIHUB_MAKE_TIME \"`date \"+%s\"`" > make_time.h
	DEPENDS
	${VERLIHUB_SRCS}

)
ADD_CUSTOM_TARGET(make_time.h ALL)
LIST(APPEND ${VERLIHUB_HDRS} make_time.h)
# ----------------------------------------------------------------------------------------------------

#ADD_EXECUTABLE(verlihub ${VERLIHUB_SRCS} verlihub.cpp)
#target_link_libraries(verlihub ${VERLIHUB_LIBS})
ADD_LIBRARY(libverlihub SHARED ${VERLIHUB_SRCS})
SET_TARGET_PROPERTIES(libverlihub PROPERTIES OUTPUT_NAME "verlihub")
#SET_TARGET_PROPERTIES(libverlihub PROPERTIES VERSION ${VERLIHUB_VERSION} SOVERSION ${VERLIHUB_SOVERSION})

target_link_libraries(libverlihub ${VERLIHUB_LIBS}) # pthread dl execinfo

ADD_LIBRARY(vhapi SHARED "script_api.cpp")
#SET_TARGET_PROPERTIES(vhapi PROPERTIES VERSION ${VERLIHUB_VERSION} SOVERSION ${VERLIHUB_SOVERSION})
TARGET_LINK_LIBRARIES(vhapi libverlihub)

# Link libverlihub against verlihub executable
ADD_EXECUTABLE(verlihub verlihub.cpp)
TARGET_LINK_LIBRARIES(verlihub libverlihub)

# ----------------------------------------------------------------------------------------------------
# Generate verlihub_config
MESSAGE(STATUS "[ OK ] Generating configuration file: ${CMAKE_BINARY_DIR}/verlihub_config")
STRING(REPLACE ";" " " VERLIHUB_LIBS "${VERLIHUB_LIBS}")
CONFIGURE_FILE("${CMAKE_SOURCE_DIR}/cmake/verlihub_config.cm" ${CMAKE_BINARY_DIR}/verlihub_config)
# ----------------------------------------------------------------------------------------------------

#TODO: Add -rdynamic for backtracking
INSTALL(TARGETS vhapi libverlihub verlihub
	RUNTIME DESTINATION bin
        LIBRARY DESTINATION ${LIBDIR}
)

INSTALL(
	FILES ${VERLIHUB_BINARY_DIR}/verlihub_config
	DESTINATION bin PERMISSIONS OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE
	WORLD_READ WORLD_EXECUTE
)

INSTALL(FILES ${VERLIHUB_HDRS} ${VERLIHUB_BINARY_DIR}/dirsettings.h DESTINATION include/${PROJECT_NAME})
