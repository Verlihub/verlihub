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

IF(NOT _macroLogFeatureAlreadyIncluded)
	SET(_file ${VERLIHUB_BINARY_DIR}/EnabledFeatures.txt)

	IF(EXISTS ${_file})
		FILE(REMOVE ${_file})
	ENDIF(EXISTS ${_file})

	SET(_file ${VERLIHUB_BINARY_DIR}/DisabledFeatures.txt)

	IF(EXISTS ${_file})
		FILE(REMOVE ${_file})
	ENDIF(EXISTS ${_file})

	SET(_macroLogFeatureAlreadyIncluded TRUE)
ENDIF(NOT _macroLogFeatureAlreadyIncluded)

MACRO(MACRO_LOG_FEATURE _var _package _description) # _url _minvers _comments
	SET(_minvers "${ARGV4}")
	SET(_url "${ARGV3}")
	SET(_comments "${ARGV5}")

	IF(${_var})
		SET(_LOGFILENAME ${VERLIHUB_BINARY_DIR}/EnabledFeatures.txt)
	ELSE(${_var})
		SET(_LOGFILENAME ${VERLIHUB_BINARY_DIR}/DisabledFeatures.txt)
	ENDIF(${_var})

	SET(_logtext "+ ${_package}")

	IF(${_minvers} MATCHES ".*")
		SET(_logtext "${_logtext} (>=${_minvers})")
	ENDIF(${_minvers} MATCHES ".*")

	SET(_logtext "${_logtext}: ${_description}")

	IF(${_url} MATCHES ".*")
		SET(_logtext "${_logtext} <${_url}>")
	ENDIF(${_url} MATCHES ".*")

	STRING(TOUPPER ${_package} FEATURE_NAME)
	SET(_logtext "${_logtext} (WITH_${FEATURE_NAME})")

	IF(NOT ${_var})
		IF(${_comments} MATCHES ".*")
			SET(_logtext "${_logtext}: ${_comments}")
		ENDIF(${_comments} MATCHES ".*")
	ENDIF(NOT ${_var})

	FILE(APPEND "${_LOGFILENAME}" "${_logtext}\n")
ENDMACRO(MACRO_LOG_FEATURE)

MACRO(MACRO_DISPLAY_FEATURE_LOG)
	SET(_summary "\n")
	SET(_elist 0)
	SET(_file ${VERLIHUB_BINARY_DIR}/EnabledFeatures.txt)

	IF(EXISTS ${_file})
		SET(_elist 1)
		FILE(READ ${_file} _enabled)
		FILE(REMOVE ${_file})
		SET(_summary "${_summary}-----------------------------------------------------------------------------\n[ OK ] Following external packages were located on your system, this installation will have the extra features provided by these packages:\n${_enabled}")
	ENDIF(EXISTS ${_file})

	SET(_dlist 0)
	SET(_file ${VERLIHUB_BINARY_DIR}/DisabledFeatures.txt)

	IF(EXISTS ${_file})
		SET(_dlist 1)
		FILE(READ ${_file} _disabled)
		FILE(REMOVE ${_file})
		SET(_summary "${_summary}-----------------------------------------------------------------------------\n[ ER ] Following optional packages could not be located on your system, consider installing them to enable more features from this software:\n${_disabled}")
	ELSE(EXISTS ${_file})
		IF(${_elist})
			SET(_summary "${_summary}Congratulations, all external packages have been found:\n")
		ENDIF(${_elist})
	ENDIF(EXISTS ${_file})

	IF(${_elist} OR ${_dlist})
		SET(_summary "${_summary}-----------------------------------------------------------------------------\n")
	ENDIF(${_elist} OR ${_dlist})

	MESSAGE(STATUS "${_summary}")
ENDMACRO(MACRO_DISPLAY_FEATURE_LOG)
