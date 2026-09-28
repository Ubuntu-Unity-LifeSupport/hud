find_program(PYTHON "python")

if (PYTHON)
  set(PYTHON_ROOT ${CMAKE_INSTALL_PREFIX})
  set(PYTHON_PREFIX ".")
  if(DEFINED DEBIAN_TARGET_DIR)
    set(PYTHON_ROOT ${DEBIAN_TARGET_DIR})
    set(PYTHON_PREFIX "usr")
	set(PYTHON_LAYOUT "--install-layout=deb")
  endif()

  set(SETUP_PY ${CMAKE_CURRENT_SOURCE_DIR}/setup.py)
  file(GLOB_RECURSE DEPS *.py)
  set(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/build")

  add_custom_command(
    OUTPUT
      ${OUTPUT}
    COMMAND
      ${PYTHON}
      ${SETUP_PY}
      build
      --build-base ${OUTPUT}
      --build-temp ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS
      ${DEPS}
    WORKING_DIRECTORY
      ${CMAKE_CURRENT_SOURCE_DIR}
  )

  add_custom_target(target ALL DEPENDS ${OUTPUT})
  
  install(CODE "
  execute_process(
    COMMAND
      ${PYTHON}
      ${SETUP_PY}
      build
      --build-base ${OUTPUT}
      --build-temp ${CMAKE_CURRENT_BINARY_DIR}
      install
      --root=${PYTHON_ROOT}
      --prefix=${PYTHON_PREFIX}
      ${PYTHON_LAYOUT}
    WORKING_DIRECTORY
      ${CMAKE_CURRENT_SOURCE_DIR} )
  ")
  
  add_test(hudkeywords-check ${PYTHON} ${SETUP_PY} test)
  set_tests_properties(hudkeywords-check PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
endif()