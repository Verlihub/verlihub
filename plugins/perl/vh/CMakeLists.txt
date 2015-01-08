Find_Package(Perl REQUIRED)

EXECUTE_PROCESS(
  COMMAND
    ${PERL_EXECUTABLE} -MConfig -e "print \$Config{installsitearch}"
    OUTPUT_VARIABLE
      PERL_SITE_ARCH
    RESULT_VARIABLE
      PERL_SITE_ARCH_RESULT
)

GET_DIRECTORY_PROPERTY(DEFINITIONS DEFINITIONS)

CONFIGURE_FILE(
  "${CMAKE_CURRENT_SOURCE_DIR}/Makefile.PL.in"
  "${CMAKE_CURRENT_BINARY_DIR}/Makefile.PL")

CONFIGURE_FILE(
  "${CMAKE_CURRENT_SOURCE_DIR}/vh.pm"
  "${CMAKE_CURRENT_BINARY_DIR}/vh.pm" COPYONLY)

CONFIGURE_FILE(
  "${CMAKE_CURRENT_SOURCE_DIR}/vh.xs"
  "${CMAKE_CURRENT_BINARY_DIR}/vh.xs" COPYONLY)

CONFIGURE_FILE(
  "${CMAKE_CURRENT_SOURCE_DIR}/ppport.h"
  "${CMAKE_CURRENT_BINARY_DIR}/ppport.h" COPYONLY)

CONFIGURE_FILE(
  "${CMAKE_CURRENT_SOURCE_DIR}/const-xs.inc"
  "${CMAKE_CURRENT_BINARY_DIR}/const-xs.inc" COPYONLY)

CONFIGURE_FILE(
  "${CMAKE_CURRENT_SOURCE_DIR}/const-c.inc"
  "${CMAKE_CURRENT_BINARY_DIR}/const-c.inc" COPYONLY)

ADD_CUSTOM_COMMAND(
  OUTPUT
    Makefile.build
  COMMAND perl Makefile.PL
)

ADD_CUSTOM_COMMAND(
  OUTPUT
    blib/arch/auto/vh/vh.so
    blib/arch/auto/vh/vh.bs 
    lib/arch/auto/vh/autosplit.ix
  COMMAND make -f Makefile.build
  DEPENDS Makefile.build
  COMMENT "Building vh module")

ADD_CUSTOM_COMMAND(
  OUTPUT
    const.pm
  COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/gen-const" "${CMAKE_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/const.pm"
  DEPENDS gen-const
  COMMENT "Building vh::const module")

ADD_CUSTOM_TARGET(vh_make ALL DEPENDS blib/arch/auto/vh/vh.so)
ADD_CUSTOM_TARGET(vh_const_make ALL DEPENDS const.pm)

INSTALL(
  FILES
    ${CMAKE_CURRENT_BINARY_DIR}/blib/arch/auto/vh/vh.so
    ${CMAKE_CURRENT_BINARY_DIR}/blib/arch/auto/vh/vh.bs
    ${CMAKE_CURRENT_BINARY_DIR}/blib/lib/auto/vh/autosplit.ix
  DESTINATION
    ${PERL_SITE_ARCH}/auto/vh/)

INSTALL(
  FILES
    vh.pm
  DESTINATION
    ${PERL_SITE_ARCH})

INSTALL(
  FILES
    ${CMAKE_CURRENT_BINARY_DIR}/const.pm
  DESTINATION
    ${PERL_SITE_ARCH}/vh/)

