
find_package(GtkDoc)

if(GTKDOC_FOUND)
  get_target_property(_hud_output_name hud OUTPUT_NAME)
  include(UseGtkDoc)
  gtk_doc_add_module(libhud "${CMAKE_SOURCE_DIR}/libhud"
    XML
      libhud-docs.xml
    SUFFIXES
      h c
    IGNOREHEADERS
      hud.h
      app-iface.h
      marshal.h
      service-iface.h
    LDFLAGS
      -L${CMAKE_CURRENT_BINARY_DIR}/../../libhud
      -l${_hud_output_name}
    LDPATH 
      ${CMAKE_CURRENT_BINARY_DIR}/../../libhud
    DEPENDS
      hud
  )

  install(
    DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/libhud/html/"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/gtk-doc/html/libhud" 
  )
  
else()
  message(STATUS "gtkdoc was not found.")
endif()
