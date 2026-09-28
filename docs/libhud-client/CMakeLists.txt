
find_package(GtkDoc)

if(GTKDOC_FOUND)
  get_target_property(_hud_client_output_name hud-client OUTPUT_NAME)
  include(UseGtkDoc)
  gtk_doc_add_module(libhud-client "${CMAKE_SOURCE_DIR}/libhud-client"
    XML
      libhud-client-docs.xml
    SUFFIXES
      h c
    IGNOREHEADERS
      HudClient.h
      HudToolbarModel.h
      action-muxer.h
      connection-private.h
      hud-client.h
      query-iface.h
      service-iface.h
      toolbar-items.h
    LDFLAGS
      -L${CMAKE_CURRENT_BINARY_DIR}/../../libhud-client
      -l${_hud_client_output_name}
    LDPATH 
      ${CMAKE_CURRENT_BINARY_DIR}/../../libhud-client
    DEPENDS
      hud-client
  )

  install(
    DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/libhud-client/html/"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/gtk-doc/html/libhud-client" 
  )
  
else()
  message(STATUS "gtkdoc was not found.")
endif()
