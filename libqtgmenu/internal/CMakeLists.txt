set(
    QTGMENU_INTERNAL_SRC
    QtGActionGroup.cpp
    QtGMenuImporterPrivate.cpp
    QtGMenuUtils.cpp
    QtGMenuModel.cpp
)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_library(
    qtgmenu-internal
    ${QTGMENU_INTERNAL_SRC}
)

target_link_libraries(
    qtgmenu-internal
    ${GIO2_LIBRARIES}
)

qt5_use_modules(
    qtgmenu-internal
    Core
    DBus
    Widgets
)
