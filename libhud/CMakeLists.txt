
add_compiler_export_flags()

set(HUD_INCLUDE_DIRS
${CMAKE_CURRENT_SOURCE_DIR}
${CMAKE_CURRENT_BINARY_DIR}
)

set(HUD_INCLUDE_DIRS
${HUD_INCLUDE_DIRS}
PARENT_SCOPE
)

include_directories(${HUD_INCLUDE_DIRS})

# FIXME Would rather not include this
include_directories(${HUD_SRC_DIR})

add_definitions( -DHUD_COMPILATION=1 )

###########################
# Version Information
###########################

set(API_VERSION 2)
set(ABI_VERSION 2)

###########################
# Generated Lib Hud
###########################

set(HUD_GENERATED_HEADERS
app-iface.h
service-iface.h
marshal.h
)

set(HUD_GENERATED_SOURCES
)

add_gdbus_codegen(
OUTFILES HUD_GENERATED_SOURCES
NAME service-iface
PREFIX com.canonical.hud.
NAMESPACE _HudServiceIface
SERVICE_XML ${HUD_SERVICE_XML}
)

add_gdbus_codegen(
OUTFILES HUD_GENERATED_SOURCES
NAME app-iface
PREFIX com.canonical.hud.Application.
NAMESPACE _HudAppIface
SERVICE_XML ${HUD_APP_XML}
)

add_glib_marshal(HUD_GENERATED_SOURCES
marshal
_hud_marshal
marshal.h
)

add_library(hud-generated STATIC ${HUD_GENERATED_SOURCES})

target_link_libraries(hud-generated
${GLIB2_LIBRARIES}
${GOBJECT2_LIBRARIES}
${GIO2_LIBRARIES}
)

###########################
# Lib Hud
###########################

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden")

set(HUD_MAIN_HEADERS
hud.h
)

set(HUD_HEADERS
manager.h
action-publisher.h
${HUD_HEADERS}
)

set(HUD_SOURCES
manager.c
action-publisher.c
)

add_library(hud SHARED ${HUD_SOURCES})

set_target_properties(hud PROPERTIES
  VERSION ${API_VERSION}.0.0
  SOVERSION ${ABI_VERSION}
  OUTPUT_NAME "hud"
)

set_target_properties(
  hud
  PROPERTIES 
  COMPILE_FLAGS "-Wpedantic -Wall -Wextra"
)

set_target_properties(hud PROPERTIES LINK_FLAGS "${ldflags} -Wl,--version-script,${CMAKE_CURRENT_SOURCE_DIR}/libhud.map")
set_target_properties(hud PROPERTIES LINK_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/libhud.map")

target_link_libraries(hud
hud-generated
${GLIB2_LIBRARIES}
${GOBJECT2_LIBRARIES}
-Wl,--no-undefined
)

###########################
# Pkg Config
###########################

set(HUD_PC "hud-${API_VERSION}.pc")
set(apiversion "${API_VERSION}")
set(libdir "${CMAKE_INSTALL_FULL_LIBDIR}")
set(includedir "${CMAKE_INSTALL_FULL_INCLUDEDIR}")
set(VERSION "${ABI_VERSION}")

configure_file("hud.pc.in" "${CMAKE_CURRENT_BINARY_DIR}/${HUD_PC}" @ONLY)

###########################
# GObject Introspection
###########################

if(${INTROSPECTION_FOUND})
  include(UseGObjectIntrospection)
  set(INTROSPECTION_GIRS)

  set(_introspection_files ${HUD_SOURCES} ${HUD_HEADERS})
  set(Hud_2_gir "hud")
  set(Hud_2_gir_INCLUDES GObject-2.0)
  
  gir_get_cflags(_cflags)
  set(Hud_2_gir_CFLAGS ${_cflags})
  
  set(Hud_2_gir_LIBS hud)
  
  list_make_absolute(_abs_introspection_files _introspection_files "${CMAKE_CURRENT_SOURCE_DIR}/")
  set(Hud_2_gir_FILES ${_abs_introspection_files})
  set(Hud_2_gir_SCANNERFLAGS --c-include "hud.h")
  set(Hud_2_gir_EXPORT_PACKAGES "hud-${API_VERSION}")

  list(APPEND INTROSPECTION_GIRS Hud-2.gir)
  gir_add_introspections(INTROSPECTION_GIRS)
else()
  message("Not performing GIR as required tools not found")
endif()

###########################
# libhud Vala
###########################

if(${VALA_FOUND})
if(${INTROSPECTION_FOUND})

vapi_gen(hud-vapi
  INPUT
  	Hud-2.gir
  PACKAGES
    gio-2.0
  LIBRARY
    Hud-2
)

install(
  FILES
    ${hud-vapi_OUTPUT}
  DESTINATION
    "${CMAKE_INSTALL_DATADIR}/vala/vapi"
)

endif()
endif()

###########################
# Installation
###########################

install(
  FILES ${HUD_MAIN_HEADERS}
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/libhud-${API_VERSION}"
)

install(
  FILES ${HUD_HEADERS}
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/libhud-${API_VERSION}/libhud"
)

install(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/${HUD_PC}"
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig"
)

install(
  TARGETS hud
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
