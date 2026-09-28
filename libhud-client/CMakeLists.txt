
add_compiler_export_flags()

set(HUD_CLIENT_INCLUDE_DIRS
${CMAKE_CURRENT_SOURCE_DIR}
${CMAKE_CURRENT_BINARY_DIR}
)

set(HUD_CLIENT_INCLUDE_DIRS
${HUD_CLIENT_INCLUDE_DIRS}
PARENT_SCOPE
)

include_directories(${HUD_CLIENT_INCLUDE_DIRS})

# FIXME would rather not include this dir
include_directories(${HUD_SRC_DIR})

add_definitions( -DHUD_CLIENT_COMPILATION=1 )

###########################
# Version Information
###########################

set(API_VERSION 2)
set(ABI_VERSION 2)

###########################
# Generated Hud Client
###########################

set(HUD_CLIENT_GENERATED_HEADERS
service-iface.h
query-iface.h
)

set(HUD_CLIENT_GENERATED_SOURCES
)

add_gdbus_codegen(
OUTFILES HUD_CLIENT_GENERATED_SOURCES
NAME service-iface
PREFIX com.canonical.hud.
NAMESPACE _HudService
SERVICE_XML ${HUD_SERVICE_XML}
)

add_gdbus_codegen(
OUTFILES HUD_CLIENT_GENERATED_SOURCES
NAME query-iface
PREFIX com.canonical.hud.query.
NAMESPACE _HudQuery
SERVICE_XML ${HUD_QUERY_XML}
)

add_library(hud-client-generated STATIC ${HUD_CLIENT_GENERATED_SOURCES})

target_link_libraries(hud-client-generated
${GLIB2_LIBRARIES}
${GOBJECT2_LIBRARIES}
${GIO2_LIBRARIES}
${DEE_LIBRARIES}
)

###########################
# Hud Client
###########################

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden")

set(HUD_CLIENT_MAIN_HEADERS
hud-client.h
)

set(HUD_CLIENT_HEADERS
connection.h
param.h
query.h
toolbar-items.h
${CMAKE_SOURCE_DIR}/common/query-columns.h
)

set(HUD_CLIENT_QT_HEADERS
HudClient.h
HudToolbarModel.h
)

set(HUD_CLIENT_SOURCES
action-muxer.c
connection.c
param.c
query.c
HudClient.cpp
HudToolbarModel.cpp
)


add_glib_enumtypes_t(HUD_CLIENT_SOURCES
enum-types
enum-types.h.in
enum-types.c.in
${HUD_CLIENT_HEADERS}
)
list(APPEND HUD_CLIENT_HEADERS ${CMAKE_CURRENT_BINARY_DIR}/enum-types.h)

add_library(hud-client SHARED ${HUD_CLIENT_SOURCES} connection-private.h)

set_target_properties(hud-client PROPERTIES
  VERSION ${API_VERSION}.0.0
  SOVERSION ${ABI_VERSION}
  OUTPUT_NAME "hud-client"
)

set_target_properties(
  hud-client
  PROPERTIES 
  COMPILE_FLAGS "-Wpedantic -Wall -Wextra"
)

target_link_libraries(hud-client
hud-client-generated
${GLIB2_LIBRARIES}
${GOBJECT2_LIBRARIES}
${DEE_QT_LIBRARIES}
-Wl,--no-undefined
)

set_target_properties(hud-client PROPERTIES LINK_FLAGS "${ldflags} -Wl,--version-script,${CMAKE_CURRENT_SOURCE_DIR}/libhud-client.map")
set_target_properties(hud-client PROPERTIES LINK_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/libhud-client.map")

qt5_use_modules(hud-client
  Core
)

###########################
# Pkg Config
###########################

set(HUD_CLIENT_PC "hud-client-${API_VERSION}.pc")
set(apiversion "${API_VERSION}")
set(libdir "${CMAKE_INSTALL_FULL_LIBDIR}")
set(includedir "${CMAKE_INSTALL_FULL_INCLUDEDIR}")
set(VERSION "${ABI_VERSION}")
configure_file("hud-client.pc.in" "${CMAKE_CURRENT_BINARY_DIR}/${HUD_CLIENT_PC}" @ONLY)

###########################
# GObject Introspection
###########################

if(${INTROSPECTION_FOUND})
  include(UseGObjectIntrospection)
  set(INTROSPECTION_GIRS)

  set(_introspection_files ${HUD_CLIENT_SOURCES} ${HUD_CLIENT_HEADERS})
  set(HudClient_2_gir "hud-client")
  set(HudClient_2_gir_INCLUDES GObject-2.0 Dee-1.0)
  
  gir_get_cflags(_cflags)
  set(HudClient_2_gir_CFLAGS ${_cflags})
  
  set(HudClient_2_gir_LIBS hud-client)
  
  list_make_absolute(_abs_introspection_files _introspection_files "${CMAKE_CURRENT_SOURCE_DIR}/")
  set(HudClient_2_gir_FILES ${_abs_introspection_files})
  set(HudClient_2_gir_SCANNERFLAGS --c-include "hud-client.h")
  set(HudClient_2_gir_EXPORT_PACKAGES "hud-client-${API_VERSION}")

  list(APPEND INTROSPECTION_GIRS HudClient-2.gir)
  gir_add_introspections(INTROSPECTION_GIRS)
else()
  message("Not performing GIR as required tools not found")
endif()

###########################
# libhud-client Vala
###########################

if(${VALA_FOUND})
if(${INTROSPECTION_FOUND})

vapi_gen(hud-client-vapi
  INPUT
  	HudClient-2.gir
  PACKAGES
    gio-2.0
    dee-1.0
  LIBRARY
    HudClient-2
)

install(
  FILES
    ${hud-client-vapi_OUTPUT}
  DESTINATION
    "${CMAKE_INSTALL_DATADIR}/vala/vapi"
)

endif()
endif()


###########################
# Installation
###########################

install(
  FILES ${HUD_CLIENT_MAIN_HEADERS}
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/libhud-client-${API_VERSION}"
)

install(
  FILES ${HUD_CLIENT_HEADERS} ${HUD_CLIENT_QT_HEADERS}
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/libhud-client-${API_VERSION}/libhud-client"
)

install(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/${HUD_CLIENT_PC}"
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig"
)

install(
  TARGETS hud-client
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
