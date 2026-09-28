
find_package(GtkDoc)

if(GTKDOC_FOUND)
  get_target_property(_hud_output_name hud-gtk OUTPUT_NAME)
  include(UseGtkDoc)
  gtk_doc_add_module(libhud-gtk "${CMAKE_SOURCE_DIR}/libhud-gtk"
    XML
      libhud-gtk-docs.xml
    SUFFIXES
      h c
    IGNOREHEADERS
      hud-gtk.h
    LDFLAGS
      -L${CMAKE_CURRENT_BINARY_DIR}/../../libhud-gtk
      -l${_hud_output_name}
    LDPATH 
      ${CMAKE_CURRENT_BINARY_DIR}/../../libhud-gtk
    DEPENDS
      hud-gtk
  )

  install(
    DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/libhud-gtk/html/"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/gtk-doc/html/libhud-gtk" 
  )
  
else()
  message(STATUS "gtkdoc was not found.")
endif()
