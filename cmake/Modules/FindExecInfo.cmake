# Find execinfo, which, if not provided with libc/libSystem, provides functions like backtrace
# This module defines the following variables:
# LIBEXECINFO_FOUND - whether or not we found execinfo
# LIBEXECINFO_INCLUDE_DIR - include directory containing execinfo.h
# LIBEXECINFO_INCLUDE_DIRS - include directories, including dependencies
# https://github.com/CTSRD-TESLA/TESLA/blob/master/cmake/Modules/FindExecInfo.cmake

find_path(EXECINFO_INCLUDE_DIR execinfo.h)
find_library(EXECINFO_LIBRARY NAMES execinfo)
set(EXECINFO_INCLUDE_DIRS "${EXECINFO_INCLUDE_DIR}") # execinfo does not depend on anything
include (FindPackageHandleStandardArgs)
find_package_handle_standard_args(EXECINFO DEFAULT_MSG EXECINFO_LIBRARY EXECINFO_INCLUDE_DIR)
mark_as_advanced (EXECINFO_LIBRARY EXECINFO_INCLUDE_DIR)
