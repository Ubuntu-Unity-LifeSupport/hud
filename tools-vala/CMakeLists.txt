
if(${VALA_FOUND})
if(${INTROSPECTION_FOUND})

set(HEADER_PATH "${CMAKE_CURRENT_BINARY_DIR}/hud-gtk.h")
set(SYMBOLS_PATH "${CMAKE_CURRENT_BINARY_DIR}/hud-gtk.def")

set(HUD_GTK_DATADIR "${CMAKE_INSTALL_FULL_DATADIR}/hud-gtk")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-deprecated-declarations")

###########################
# HUD Gtk Vala Generation
###########################

set(VALA_OPTIONS)
foreach(DIR ${HUD_CLIENT_INCLUDE_DIRS})
  list(APPEND VALA_OPTIONS "--vapidir" ${DIR})
endforeach()

vala_init (hud-gtk-vala
  PACKAGES
    dee-1.0
    gtk+-3.0
    HudClient-2
  OPTIONS
    ${VALA_OPTIONS}
  DEPENDS
  	hud-client-vapi
)

vala_add(hud-gtk-vala hud-gtk.vala)

vala_finish(hud-gtk-vala
  SOURCES
    project_VALA_SOURCES
  OUTPUTS
    project_VALA_C
  GENERATE_HEADER
    ${HEADER_PATH}
  GENERATE_SYMBOLS
    ${SYMBOLS_PATH}
)

set_source_files_properties(${project_VALA_SOURCES}
  PROPERTIES HEADER_FILE_ONLY TRUE)
set(project_SOURCES ${project_VALA_SOURCES} ${project_VALA_C} ${SYMBOLS_PATH})

###########################
# HUD Gtk Executable
###########################

include_directories(${HUD_CLIENT_INCLUDE_DIRS})

add_definitions(-DHUD_GTK_DATADIR="${HUD_GTK_DATADIR}")

add_executable (hud-gtk-exec ${project_SOURCES})

set_target_properties(hud-gtk-exec PROPERTIES OUTPUT_NAME "hud-gtk")

target_link_libraries(hud-gtk-exec
  hud-client
  ${GTK3_LIBRARIES}
  ${DEE_LIBRARIES}
)

###########################
# Installation
###########################

install(
  TARGETS hud-gtk-exec
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(
  FILES hud-gtk.ui
  DESTINATION ${HUD_GTK_DATADIR}
)

endif()
endif()
