file(STRINGS LANGS LOCALES)
# Merge POT files
add_custom_command(
	OUTPUT "${CMAKE_BINARY_DIR}/po/verlihub.pot"
	COMMAND msgcat "${CMAKE_CURRENT_SOURCE_DIR}/verlihub_core.pot" "${CMAKE_CURRENT_SOURCE_DIR}/verlihub_plugins.pot" --no-wrap --use-first --output "${CMAKE_BINARY_DIR}/po/verlihub.pot"
)
SET(POT_FILE "${CMAKE_BINARY_DIR}/po/verlihub.pot")
# Merge and compile PO files
FOREACH(LOCALE ${LOCALES})
	add_custom_command(
		OUTPUT "${CMAKE_BINARY_DIR}/po/${LOCALE}.po"
		COMMAND msgcat "${CMAKE_CURRENT_SOURCE_DIR}/core_${LOCALE}.po" "${CMAKE_CURRENT_SOURCE_DIR}/plugins_${LOCALE}.po" --no-wrap --use-first --output "${CMAKE_BINARY_DIR}/po/${LOCALE}.po"
	)
	LIST(APPEND PO_FILES "${CMAKE_BINARY_DIR}/po/${LOCALE}.po")
ENDFOREACH(LOCALE)

GETTEXT_CREATE_TRANSLATIONS(${POT_FILE} ALL ${PO_FILES})
