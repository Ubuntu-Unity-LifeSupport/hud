set(CMAKE_AUTOMOC OFF)
find_package(GMock REQUIRED)
set(CMAKE_AUTOMOC ON)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${GMOCK_INCLUDE_DIRS})
include_directories(${GTEST_INCLUDE_DIRS})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

add_definitions(-DHUD_SERVICE_BINARY="${CMAKE_BINARY_DIR}/service/hud-service")

add_definitions(-DTEST_LOCALEDIR="${CMAKE_CURRENT_BINARY_DIR}/data")
set(TEST_DATADIR "${CMAKE_CURRENT_SOURCE_DIR}/data")
add_definitions(-DTEST_DATADIR="${TEST_DATADIR}")

add_definitions(-DDATA_DIR="${DATA_DIR}")

set(HUD_TEST_XML "${TEST_DATADIR}/com.canonical.hud.test.xml")

#######################
# Test Fixtures
#######################

add_definitions(-DJSON_SIMPLE="${TEST_DATADIR}/test-menu-input-simple.json")
add_definitions(-DJSON_SHORTCUTS="${TEST_DATADIR}/test-menu-input-shortcuts.json")
add_definitions(-DJSON_SOURCE_ONE="${TEST_DATADIR}/test-app-indicator-source-one.json")
add_definitions(-DJSON_SOURCE_TWO="${TEST_DATADIR}/test-app-indicator-source-two.json")
add_definitions(-DJSON_SOURCE_SESSION="${TEST_DATADIR}/test-indicator-source-session.json")
add_definitions(-DJSON_SOURCE_DATETIME="${TEST_DATADIR}/test-indicator-source-datetime.json")
add_definitions(-DJSON_SOURCE_SOUND="${TEST_DATADIR}/test-indicator-source-sound.json")
add_definitions(-DJSON_SOURCE="${TEST_DATADIR}/test-source.json")

add_definitions(-DDBUSMENU_JSON_LOADER="${CMAKE_CURRENT_BINARY_DIR}/menus/dbusmenu-json-loader")

add_definitions(-DMODEL_DEEP="${CMAKE_CURRENT_BINARY_DIR}/menus/test-menu-input-model-deep")
add_definitions(-DMODEL_SIMPLE="${CMAKE_CURRENT_BINARY_DIR}/menus/test-menu-input-model-simple")
add_definitions(-DMODEL_SHORTCUTS="${CMAKE_CURRENT_BINARY_DIR}/menus/test-menu-input-model-shortcuts")
add_definitions(-DMODEL_TOOLBAR_UNDO="${CMAKE_CURRENT_BINARY_DIR}/menus/test-menu-input-model-toolbar-undo")
add_definitions(-DMODEL_TOOLBAR_UNKNOWN="${CMAKE_CURRENT_BINARY_DIR}/menus/test-menu-input-model-toolbar-unknown")
add_definitions(-DMODEL_TOOLBAR_ALL="${CMAKE_CURRENT_BINARY_DIR}/menus/test-menu-input-model-toolbar-all")
add_definitions(-DMODEL_TOOLBAR_DYNAMIC="${CMAKE_CURRENT_BINARY_DIR}/menus/test-menu-input-model-toolbar-dynamic")

add_definitions(-DMODEL_LIBHUD="${CMAKE_CURRENT_BINARY_DIR}/menus/test-libhud-application")

########################
# Tests
########################

set(TEST_RUNNER "${CMAKE_CURRENT_SOURCE_DIR}/run-under-xvfb.sh")

function(add_hud_test NAME)
	add_test(
		${NAME}
		${TEST_RUNNER} qdbus-simple-test-runner "${CMAKE_CURRENT_BINARY_DIR}/${NAME}"
	)
	
	set_tests_properties(
		${NAME}
		PROPERTIES ENVIRONMENT "GTEST_OUTPUT=xml:${CMAKE_CURRENT_BINARY_DIR}/"
	)
endfunction()

add_subdirectory(integration)
add_subdirectory(menus)
add_subdirectory(testapps)
add_subdirectory(testutils)
add_subdirectory(unit)
