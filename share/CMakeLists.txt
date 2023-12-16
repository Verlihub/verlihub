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

SET(SQL_FILES
	sql/default_conn_types.sql
	sql/default_file_trigger.sql
	sql/default_reglist.sql
	sql/default_custom_redirects.sql
	sql/default_client_list.sql
)

SET(CONFIG_FILES # default
	config/faq
	config/motd
	config/rules
	config/help_admin
	config/help_cheef
	config/help_master
	config/help_op
	config/help_reg
	config/help_usr
	config/help_vip
)

SET(CONFIG_FILES_RU # russian
	config/ru/help_admin
	config/ru/help_cheef
	config/ru/help_master
	config/ru/help_op
	config/ru/help_reg
	config/ru/help_usr
	config/ru/help_vip
)

SET(prefix ${CMAKE_INSTALL_PREFIX})
SET(plugindir "${CMAKE_INSTALL_PREFIX}/${PLUGINDIR}")
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/sql/default_pi_plug.sql.in ${CMAKE_CURRENT_BINARY_DIR}/sql/default_pi_plug.sql)
SET(SQL_FILES ${SQL_FILES}  ${CMAKE_CURRENT_BINARY_DIR}/sql/default_pi_plug.sql)
INSTALL(FILES ${SQL_FILES} DESTINATION share/${PROJECT_NAME}/sql)
INSTALL(FILES ${CONFIG_FILES} DESTINATION share/${PROJECT_NAME}/config)
INSTALL(FILES ${CONFIG_FILES_RU} DESTINATION share/${PROJECT_NAME}/config/ru)
