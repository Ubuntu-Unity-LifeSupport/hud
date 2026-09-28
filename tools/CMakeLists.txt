
add_subdirectory(hudkeywords)

include_directories(${HUD_CLIENT_INCLUDE_DIRS})
include_directories(${HUD_SERVICE_INCLUDE_DIRS})

pkg_check_modules(PANGO REQUIRED pango)

include_directories(${PANGO_INCLUDE_DIRS})

###########################
# Hud CLI
###########################

add_executable(hud-cli hud-cli.c)

target_link_libraries(hud-cli
  hud-client
  ${PANGO_LIBRARIES}
)

install(
  TARGETS hud-cli
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

###########################
# Hud CLI Param
###########################

add_executable(hud-cli-param hud-cli-param.c)

target_link_libraries(hud-cli-param
  hud-client
  ${PANGO_LIBRARIES}
)

install(
  TARGETS hud-cli-param
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

###########################
# Hud CLI Appstack
###########################

add_executable(hud-cli-appstack hud-cli-appstack.c)

target_link_libraries(hud-cli-appstack
  hud-client
)

install(
  TARGETS hud-cli-appstack
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

###########################
# Hud CLI Toolbar
###########################

add_executable(hud-cli-toolbar hud-cli-toolbar.c)

target_link_libraries(hud-cli-toolbar
  hud-client
)

install(
  TARGETS hud-cli-toolbar
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)
