
add_compiler_export_flags()

add_subdirectory(internal)

set(
    QTGMENU_SRC
    QtGMenuExporter.cpp
    QtGMenuImporter.cpp
)

add_library(
    qtgmenu
    ${QTGMENU_SRC}
)

qt5_use_modules(
    qtgmenu
    Core
    DBus
)

target_link_libraries(
    qtgmenu
    qtgmenu-internal
)
