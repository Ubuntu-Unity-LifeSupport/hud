project(hud C CXX)
cmake_minimum_required(VERSION 3.10)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" "${CMAKE_MODULE_PATH}")

set(PACKAGE ${CMAKE_PROJECT_NAME})
set(GETTEXT_PACKAGE ${CMAKE_PROJECT_NAME})

option(ENABLE_DOCUMENTATION "Enable documentation." OFF)
option(ENABLE_TESTS "Enable tests." ON)
option(ENABLE_SCALABILITY_TESTS "Additional scalability tests that are potentially very slow to run." OFF)
option(LOCAL_INSTALL "Support local installation." OFF)
option(ENABLE_BAMF "Enable building for BAMF." ON)

# Trick the H10enable_coverage script into enabling coverage by including the text below:
# CMAKE_BUILD_TYPE coverage

find_package(PkgConfig REQUIRED)
include(GNUInstallDirs)
include(CheckIncludeFile)
include(CheckFunctionExists)
include(UseGlibGeneration)
include(UseGdbusCodegen)
include(UseConstantBuilder)
find_package(GSettings)
include(UseVala)

# Workaround for libexecdir on debian
if (EXISTS "/etc/debian_version") 
  set(CMAKE_INSTALL_LIBEXECDIR ${CMAKE_INSTALL_LIBDIR})
  set(CMAKE_INSTALL_FULL_LIBEXECDIR "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBEXECDIR}")
endif()

set(HUD_QUERY_XML ${CMAKE_CURRENT_SOURCE_DIR}/data/com.canonical.hud.query.xml)
set(HUD_SERVICE_XML ${CMAKE_CURRENT_SOURCE_DIR}/data/com.canonical.hud.xml)
set(HUD_APP_XML ${CMAKE_CURRENT_SOURCE_DIR}/data/com.canonical.hud.Application.xml)

set(WINDOW_STACK_XML "${CMAKE_CURRENT_SOURCE_DIR}/data/com.canonical.Unity.WindowStack.xml")
set(APPMENU_REGISTRAR_XML "${CMAKE_CURRENT_SOURCE_DIR}/data/com.canonical.AppMenu.Registrar.xml")

set(BAMF_XML "${CMAKE_CURRENT_SOURCE_DIR}/data/org.ayatana.bamf.xml")
set(BAMF_VIEW_XML "${CMAKE_CURRENT_SOURCE_DIR}/data/org.ayatana.bamf.view.xml")

set(HUD_TOP_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(HUD_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")

add_definitions( -DDATADIR="${CMAKE_INSTALL_FULL_DATADIR}" )
add_definitions( -DGNOMELOCALEDIR="${CMAKE_INSTALL_FULL_DATADIR}/locale" )
add_definitions( -DGETTEXT_PACKAGE="${GETTEXT_PACKAGE}" )
add_definitions( -DLIBEXECDIR="${CMAKE_INSTALL_FULL_LIBEXECDIR}" )

pkg_check_modules(GLIB2 REQUIRED glib-2.0)
include_directories(${GLIB2_INCLUDE_DIRS})

pkg_check_modules(GOBJECT2 REQUIRED gobject-2.0)
include_directories(${GOBJECT2_INCLUDE_DIRS})

pkg_check_modules(GIO2 REQUIRED gio-2.0)
include_directories(${GIO2_INCLUDE_DIRS})

pkg_check_modules(GIO_UNIX2 REQUIRED gio-unix-2.0)
include_directories(${GIO_UNIX2_INCLUDE_DIRS})

pkg_check_modules(DEE REQUIRED dee-1.0)
include_directories(${DEE_INCLUDE_DIRS})

pkg_check_modules(DBUSMENU_GLIB REQUIRED dbusmenu-glib-0.4)
include_directories(${DBUSMENU_GLIB_INCLUDE_DIRS})

pkg_search_module(GTK3 REQUIRED gtk+-3.0)
include_directories(${GTK3_INCLUDE_DIRS})

pkg_check_modules(COLUMBUS REQUIRED libcolumbus)
include_directories(${COLUMBUS_INCLUDE_DIRS})

find_package(Qt5Core REQUIRED)
include_directories(${Qt5Core_INCLUDE_DIRS})

find_package(Qt5Widgets REQUIRED)
include_directories(${Qt5Widgets_INCLUDE_DIRS})

find_package(Qt5DBus REQUIRED)
include_directories(${Qt5DBus_INCLUDE_DIRS})

find_package(Qt5Sql REQUIRED)
include_directories(${Qt5Sql_INCLUDE_DIRS})

pkg_check_modules(DEE_QT REQUIRED libdee-qt5)
include_directories(${DEE_QT_INCLUDE_DIRS})

pkg_check_modules(GSETTINGS_QT REQUIRED gsettings-qt REQUIRED)
include_directories(${GSETTINGS_QT_INCLUDE_DIRS})

pkg_check_modules(DBUSMENU REQUIRED dbusmenu-qt5)
include_directories(${DBUSMENU_INCLUDE_DIRS})

find_package(Qt5Test REQUIRED)
include_directories(${Qt5Test_INCLUDE_DIRS})

pkg_check_modules(QTDBUSTEST REQUIRED libqtdbustest-1 REQUIRED)
include_directories(${QTDBUSTEST_INCLUDE_DIRS})

pkg_check_modules(QTDBUSMOCK REQUIRED libqtdbusmock-1 REQUIRED)
include_directories(${QTDBUSMOCK_INCLUDE_DIRS})

find_package(Vala 0.12)

find_package(GObjectIntrospection 0.9.12)

check_include_file("signal.h" HAVE_SIGNAL_H)
if(${HAVE_SIGNAL_H})
  set(CMAKE_REQUIRED_INCLUDES ${CMAKE_REQUIRED_INCLUDES} "signal.h")
  check_function_exists(kill HAVE_KILL)
endif()

if(${LOCAL_INSTALL})
  set(DBUSSERVICEDIR "${CMAKE_INSTALL_DATADIR}/dbus-1/services/")
else()
  EXEC_PROGRAM(${PKG_CONFIG_EXECUTABLE} ARGS dbus-1 --variable session_bus_services_dir OUTPUT_VARIABLE DBUSSERVICEDIR )
endif()
message("Installing DBus services to ${DBUSSERVICEDIR}")

if(${LOCAL_INSTALL})
  set(DBUSIFACEDIR "${CMAKE_INSTALL_DATADIR}/dbus-1/interfaces/")
else()
  EXEC_PROGRAM(${PKG_CONFIG_EXECUTABLE} ARGS dbus-1 --variable interfaces_dir OUTPUT_VARIABLE DBUSIFACEDIR )
endif()
message("Installing DBus interfaces to ${DBUSIFACEDIR}")

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_definitions(
	-DQT_NO_SIGNALS_SLOTS_KEYWORDS=1
)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11 -fPIC")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -fno-permissive")

include(GenerateExportHeader)

add_subdirectory(data)
add_subdirectory(common)
add_subdirectory(libhud-client)
add_subdirectory(libhud)
add_subdirectory(libhud-gtk)
add_subdirectory(libqtgmenu)
add_subdirectory(service)
add_subdirectory(tools)
add_subdirectory(window-stack-bridge)

if(${ENABLE_TESTS})
	enable_testing()
	ADD_CUSTOM_TARGET(
		check
		${CMAKE_CTEST_COMMAND} --force-new-ctest-process --output-on-failure
	)
	add_subdirectory(tests)

	find_package(CoverageReport)
	enable_coverage_report(
		TARGETS
			hud-generated
			hud
			hud-cli
			hud-cli-appstack
			hud-cli-param
			hud-cli-toolbar
			hud-client-generated
			hud-client
			hud-common
			hud-gtk
			hud-service
			hud-service-exec
			qtgmenu
			window-stack-bridge
			window-stack-bridge-bin
			dbusmenu-json-loader
			hud-test-interface
			qtgmenu-test-app
			test-integration-tests
			test-libhud-application
			test-libhud-client-unit-tests
			test-libhud-unit-tests
			test-menu-input-model-deep
			test-menu-input-model-shortcuts
			test-menu-input-model-simple
			test-menu-input-model-toolbar-all
			test-menu-input-model-toolbar-dynamic
			test-menu-input-model-toolbar-undo
			test-menu-input-model-toolbar-unknown
			test-qtgmenu-unit-tests
			test-service-unit-tests
			test-utils
			test-window-stack-bridge-unit-tests
		FILTER
			${CMAKE_SOURCE_DIR}/tests/*
			${CMAKE_BINARY_DIR}/*
		TESTS
			dbusmenu-json-loader
			hud-test-interface
			qtgmenu-test-app
			test-integration-tests
			test-libhud-application
			test-libhud-client-unit-tests
			test-libhud-unit-tests
			test-menu-input-model-deep
			test-menu-input-model-shortcuts
			test-menu-input-model-simple
			test-menu-input-model-toolbar-all
			test-menu-input-model-toolbar-dynamic
			test-menu-input-model-toolbar-undo
			test-menu-input-model-toolbar-unknown
			test-qtgmenu-unit-tests
			test-service-unit-tests
			test-utils
			test-window-stack-bridge-unit-tests
	)
endif()


add_subdirectory(tools-vala)
if(${ENABLE_DOCUMENTATION})
  add_subdirectory(docs)
endif()
