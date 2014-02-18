SET(SQLITE_BIN sqlite3)
SET(YATABLEDATAFILES "cangjie5" "wubi86")
SET(SHAREDIR "${FCITX4_PREFIX}/share/fcitx")

FOREACH(DATAFILE ${YATABLEDATAFILES})
    fcitx_translate_add_apply_source(
        "${DATAFILE}.conf.in" "${PROJECT_BINARY_DIR}/data/${DATAFILE}.conf" 
        )
    INSTALL(FILES  "${PROJECT_BINARY_DIR}/data/${DATAFILE}.conf" 
        DESTINATION "${SHAREDIR}/yatable")
    FILE(REMOVE "${PROJECT_BINARY_DIR}/data/${DATAFILE}")
    MESSAGE(STATUS "Generating DATABASE for ${DATAFILE} .")
    EXECUTE_PROCESS(COMMAND "${SQLITE_BIN}"
        "${PROJECT_BINARY_DIR}/data/${DATAFILE}"
        ".read ${PROJECT_SOURCE_DIR}/data/${DATAFILE}.txt")
    INSTALL(FILES  "${PROJECT_BINARY_DIR}/data/${DATAFILE}" 
        DESTINATION "${SHAREDIR}/yatable")
ENDFOREACH()

