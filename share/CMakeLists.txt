SET(SQL_FILES
	sql/default_conn_types.sql
	sql/default_file_trigger.sql
	sql/default_reglist.sql
	sql/default_custom_redirects.sql
	sql/default_dc_clients.sql
)

SET(CONFIG_FILES
	config/faq
	config/help_admin
	config/help_cheef
	config/help_master
	config/help_op
	config/help_reg
	config/help_usr
	config/help_vip
	config/motd
	config/rules
)

SET(prefix ${CMAKE_INSTALL_PREFIX})
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/sql/default_pi_plug.sql.in ${CMAKE_CURRENT_BINARY_DIR}/sql/default_pi_plug.sql)
SET(SQL_FILES ${SQL_FILES}  ${CMAKE_CURRENT_BINARY_DIR}/sql/default_pi_plug.sql)
INSTALL(FILES ${SQL_FILES} DESTINATION share/${PROJECT_NAME}/sql)
INSTALL(FILES ${CONFIG_FILES} DESTINATION share/${PROJECT_NAME}/config)
