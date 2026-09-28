
add_definitions(
	-pedantic
	-Wall
	-Wextra
)

###########################
# Lib Hud Service
###########################

set(HUD_SERVICE_LIB_SOURCES
  Application.cpp
  ApplicationImpl.cpp
  ApplicationList.cpp
  ApplicationListImpl.cpp
  Collector.cpp
  DBusMenuCollector.cpp
  DBusMenuWindowCollector.cpp
  Factory.cpp
  GMenuCollector.cpp
  GMenuWindowCollector.cpp
  HardCodedSearchSettings.cpp
  HudService.cpp
  HudServiceImpl.cpp
  Item.cpp
  ItemStore.cpp
  QGSettingsSearchSettings.cpp
  Query.cpp
  QueryImpl.cpp
  Result.cpp
  SearchSettings.cpp
  SignalHandler.cpp
  SqliteUsageTracker.cpp
  UsageTracker.cpp
  Voice.cpp
  VoiceImpl.cpp
  Window.cpp
  WindowImpl.cpp
  WindowContext.cpp
  WindowContextImpl.cpp
)

qt5_add_dbus_adaptor(
  HUD_SERVICE_LIB_SOURCES
  ${HUD_SERVICE_XML}
  service/HudServiceImpl.h
  hud::service::HudServiceImpl
  HudAdaptor
)

qt5_add_dbus_adaptor(
  HUD_SERVICE_LIB_SOURCES
  ${HUD_QUERY_XML}
  service/QueryImpl.h
  hud::service::QueryImpl
  QueryAdaptor
)

qt5_add_dbus_adaptor(
  HUD_SERVICE_LIB_SOURCES
  ${HUD_APP_XML}
  service/ApplicationImpl.h
  hud::service::ApplicationImpl
  ApplicationAdaptor
)

qt5_add_dbus_interface(
  HUD_SERVICE_LIB_SOURCES
  ${APPMENU_REGISTRAR_XML}
  AppmenuRegistrarInterface
)

add_library(hud-service
  STATIC
  ${HUD_SERVICE_LIB_SOURCES}
)

target_link_libraries(hud-service
  hud-common
  qtgmenu
  ${DBUSMENU_LIBRARIES}
  ${GOBJECT2_LIBRARIES}
  ${DEE_LIBRARIES}
  ${COLUMBUS_LIBRARIES}
  ${GSETTINGS_QT_LIBRARIES}
)

qt5_use_modules(
	hud-service
	Core
	DBus
	Widgets
	Sql
)

###########################
# Hud Service Executable
###########################

add_executable(hud-service-exec
  main.cpp
)

set_target_properties(hud-service-exec
  PROPERTIES
    OUTPUT_NAME "hud-service"
 )

target_link_libraries(hud-service-exec
  hud-service
)

qt5_use_modules(
	hud-service-exec
	Core
)

###########################
# Installation
###########################

install(
  TARGETS hud-service-exec
  RUNTIME DESTINATION "${CMAKE_INSTALL_FULL_LIBEXECDIR}/hud"
)
