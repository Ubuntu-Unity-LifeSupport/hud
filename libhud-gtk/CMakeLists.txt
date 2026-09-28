
set(HUD_GTK_INCLUDE_DIRS
${CMAKE_CURRENT_SOURCE_DIR}
${CMAKE_CURRENT_BINARY_DIR}
)

set(HUD_GTK_INCLUDE_DIRS
${HUD_GTK_INCLUDE_DIRS}
PARENT_SCOPE
)

include_directories(${HUD_INCLUDE_DIRS})
include_directories(${HUD_GTK_INCLUDE_DIRS})
include_directories(${GTK3_INCLUDE_DIRS})

#add_definitions( -DHUD_COMPILATION=1 )

###########################
# Version Information
###########################

set(API_VERSION 1)
set(ABI_VERSION 1)

###########################
# Lib Hud Gtk
###########################

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden")

set(HUD_GTK_MAIN_HEADERS
hud-gtk.h
)

set(HUD_GTK_HEADERS
manager.h
${HUD_HEADERS}
)

set(HUD_GTK_SOURCES
manager.c
)

add_library(hud-gtk SHARED ${HUD_GTK_SOURCES})

set_target_properties(hud-gtk PROPERTIES
  VERSION ${API_VERSION}.0.0
  SOVERSION ${ABI_VERSION}
  OUTPUT_NAME "hud-gtk"
)

target_link_libraries(hud-gtk
hud
${GLIB2_LIBRARIES}
${GOBJECT2_LIBRARIES}
${GTK3_LIBRARIES}
-Wl,--no-undefined
)

###########################
# Pkg Config
###########################

set(HUD_GTK_PC "hud-gtk-${API_VERSION}.pc")
set(apiversion "${API_VERSION}")
set(libdir "${CMAKE_INSTALL_FULL_LIBDIR}")
set(includedir "${CMAKE_INSTALL_FULL_INCLUDEDIR}")
set(VERSION "${ABI_VERSION}")

configure_file("hud-gtk.pc.in" "${CMAKE_CURRENT_BINARY_DIR}/${HUD_GTK_PC}" @ONLY)

###########################
# Installation
###########################

install(
  FILES ${HUD_GTK_MAIN_HEADERS}
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/libhud-gtk-${API_VERSION}"
)

install(
  FILES ${HUD_GTK_HEADERS}
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/libhud-gtk-${API_VERSION}/libhud-gtk"
)

install(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/${HUD_GTK_PC}"
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig"
)

install(
  TARGETS hud-gtk
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)