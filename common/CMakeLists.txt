
###########################
# Hud Common
###########################

set(HUD_COMMON_SOURCES
  AppstackModel.cpp
  Action.cpp
  ActionGroup.cpp
  Description.cpp
  DBusTypes.cpp
  HudDee.cpp
  MenuModel.cpp
  NameObject.cpp
  ResultsModel.cpp
  Suggestion.cpp
  WindowInfo.cpp
)

set_source_files_properties(
  ${WINDOW_STACK_XML}
  PROPERTIES
  INCLUDE "common/WindowInfo.h"
)

qt5_add_dbus_interface(
  HUD_COMMON_SOURCES
  ${WINDOW_STACK_XML}
  WindowStackInterface
)

set_source_files_properties(
  ${HUD_APP_XML}
  PROPERTIES
  INCLUDE "common/DBusTypes.h"
)

qt5_add_dbus_interface(
  HUD_COMMON_SOURCES
  ${HUD_APP_XML}
  ApplicationInterface
)

add_library(hud-common
  STATIC
  ${HUD_COMMON_SOURCES}
)

qt5_use_modules(
	hud-common
	Core
	DBus
)

target_link_libraries(
	hud-common
	${GLIB2_LIBRARIES}
	${DEE_LIBRARIES}
)

