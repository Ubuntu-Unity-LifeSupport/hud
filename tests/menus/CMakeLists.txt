
add_gdbus_codegen(
	OUTFILES HUD_TEST_INTERFACE_SOURCES
	NAME test-iface 
	PREFIX com.canonical.hud.test.
	NAMESPACE HudTest
	SERVICE_XML ${HUD_TEST_XML}
)

add_library(
	hud-test-interface STATIC
	${HUD_TEST_INTERFACE_SOURCES}
)

#######################
# Fake Touch App
#######################

set(TEST_LIBHUD_APPLICATION_SOURCES
	test-libhud-application.cpp
	FakeTouchApplication.cpp
)

qt5_add_dbus_adaptor(
	TEST_LIBHUD_APPLICATION_SOURCES
	${HUD_TEST_XML}
	FakeTouchApplication.h
	FakeTouchApplication
	TestAdaptor
)

add_executable(test-libhud-application
	${TEST_LIBHUD_APPLICATION_SOURCES}
)

target_link_libraries(test-libhud-application
	test-utils
	hud
)

#######################
# Dbus JSON Loader
#######################

pkg_check_modules(DBUSMENU_JSONLOADER REQUIRED dbusmenu-jsonloader-0.4)
include_directories(${DBUSMENU_JSONLOADER_INCLUDE_DIRS})

add_executable(dbusmenu-json-loader dbusmenu-json-loader.c)
target_link_libraries(dbusmenu-json-loader
${DBUSMENU_JSONLOADER_LIBRARIES}
)

#######################
# Menu input model deep
#######################

add_executable(test-menu-input-model-deep test-menu-input-model-deep.c)
target_link_libraries(test-menu-input-model-deep
  ${GOBJECT2_LIBRARIES}
  ${GIO2_LIBRARIES}
)

#########################
# Menu input model simple
#########################

add_executable(test-menu-input-model-simple test-menu-input-model-simple.c)
target_link_libraries(test-menu-input-model-simple
  ${GOBJECT2_LIBRARIES}
  ${GIO2_LIBRARIES}
)

############################
# Menu input model shortcuts
############################

add_executable(test-menu-input-model-shortcuts test-menu-input-model-shortcuts.c)
target_link_libraries(test-menu-input-model-shortcuts
  ${GOBJECT2_LIBRARIES}
  ${GIO2_LIBRARIES}
  hud-test-interface
)

####################################
# Menu input model toolbar undo item
####################################

add_executable(test-menu-input-model-toolbar-undo test-menu-input-model-toolbar-undo.c)
target_link_libraries(test-menu-input-model-toolbar-undo
  ${GOBJECT2_LIBRARIES}
  ${GIO2_LIBRARIES}
)

#######################################
# Menu input model toolbar unknown item
#######################################

add_executable(test-menu-input-model-toolbar-unknown test-menu-input-model-toolbar-unknown.c)
target_link_libraries(test-menu-input-model-toolbar-unknown
  ${GOBJECT2_LIBRARIES}
  ${GIO2_LIBRARIES}
)

####################################
# Menu input model toolbar all items
####################################

add_executable(test-menu-input-model-toolbar-all test-menu-input-model-toolbar-all.c)
target_link_libraries(test-menu-input-model-toolbar-all
  ${GOBJECT2_LIBRARIES}
  ${GIO2_LIBRARIES}
)

########################################
# Menu input model toolbar dynamic items
########################################

add_executable(test-menu-input-model-toolbar-dynamic test-menu-input-model-toolbar-dynamic.c)
target_link_libraries(test-menu-input-model-toolbar-dynamic
  ${GOBJECT2_LIBRARIES}
  ${GIO2_LIBRARIES}
)