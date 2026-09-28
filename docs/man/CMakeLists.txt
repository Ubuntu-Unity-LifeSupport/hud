
find_program(XSLTPROC_EXECUTABLE xsltproc)

macro(add_manpage name)
  add_custom_command(
    OUTPUT
      "${CMAKE_CURRENT_BINARY_DIR}/${name}.1"
    COMMAND
      ${XSLTPROC_EXECUTABLE}
      -nonet http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl
      "${CMAKE_CURRENT_SOURCE_DIR}/${name}.xml"
    DEPENDS
      "${CMAKE_CURRENT_SOURCE_DIR}/${name}.xml"
  )
  add_custom_target(${name}-man DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/${name}.1")
  add_dependencies(man-pages ${name}-man)
  install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/${name}.1"
    DESTINATION "${CMAKE_INSTALL_MANDIR}/man1"
  )
endmacro()

add_custom_target(man-pages ALL)

add_manpage(hud-cli)
add_manpage(hud-dump-application)
add_manpage(hud-list-applications)
add_manpage(hud-verify-app-info)
