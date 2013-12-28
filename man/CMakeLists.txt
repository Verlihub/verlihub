file(GLOB manpages ${CMAKE_CURRENT_SOURCE_DIR}/*.1)
install(FILES ${manpages} DESTINATION share/man/man1)
