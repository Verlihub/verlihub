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

Find_Package(Perl)

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
  "${CMAKE_CURRENT_SOURCE_DIR}/typemap"
  "${CMAKE_CURRENT_BINARY_DIR}/typemap" COPYONLY)

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
