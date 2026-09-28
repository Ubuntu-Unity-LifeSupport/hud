
###########################
# Dbus Interfaces
###########################

install(
FILES
com.canonical.hud.query.xml
com.canonical.hud.xml
DESTINATION ${DBUSIFACEDIR}
)

###########################
# Dbus Services
###########################

set(HUD_SERVICE
"${CMAKE_CURRENT_BINARY_DIR}/com.canonical.hud.service"
)

set(pkglibexecdir "${CMAKE_INSTALL_FULL_LIBEXECDIR}/hud")
configure_file("com.canonical.hud.service.in"
  "${HUD_SERVICE}"
  @ONLY
)

install(
FILES
"${HUD_SERVICE}"
DESTINATION ${DBUSSERVICEDIR}
)

###########################
# Systemd Unit
###########################

# where to install
pkg_get_variable(SYSTEMD_USER_DIR systemd systemduserunitdir)
message (STATUS "${SYSTEMD_USER_DIR} is the systemd user unit file install dir")

set(pkglibexecdir "${CMAKE_INSTALL_FULL_LIBEXECDIR}/hud")
configure_file("hud.service.in"
  "hud.service"
  @ONLY
)

install(
FILES "${CMAKE_CURRENT_BINARY_DIR}/hud.service"
DESTINATION "${SYSTEMD_USER_DIR}"
)

###########################
# Window Stack Bridge
###########################

configure_file(
  "window-stack-bridge.service.in"
  "window-stack-bridge.service"
  @ONLY
)

install(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/window-stack-bridge.service"
  DESTINATION "${SYSTEMD_USER_DIR}"
)

###########################
# GSettings
###########################

add_schema ("com.canonical.indicator.appmenu.hud.gschema.xml")
add_schema ("com.canonical.indicator.appmenu.hud.search.gschema.xml")

###########################
# Desktop files
###########################

install(
  FILES hud-gui.desktop
  DESTINATION "${CMAKE_INSTALL_DATADIR}/applications"
)
