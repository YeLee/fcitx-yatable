include_directories(${FCITX4_FCITX_INCLUDE_DIRS})
include_directories(${FCITX4_FCITX_CONFIG_INCLUDE_DIRS})
include_directories(${FCITX4_FCITX_UTILS_INCLUDE_DIRS})
include_directories(${YaTable_INCLUDE_DIR})

set(fcitx_yatable_sources fcitx-yatable.c fcitx-yatable_session.c)

fcitx_add_addon_full(yatable
                     DESC
                     EXTRA_DESC yatable.desc
                     SOURCES ${fcitx_yatable_sources}
                     LINK_LIBS ${YaTable_LIBRARIES})

