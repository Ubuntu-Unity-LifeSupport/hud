cmake_minimum_required(VERSION 2.6)

find_program(MSCGEN_EXECUTABLE NAMES mscgen DOC "mscgen executable")
if(NOT MSCGEN_EXECUTABLE)
  message(STATUS "Excutable mscgen not found")
endif()

function(mscgen ID)
  if (MSCGEN_EXECUTABLE)
    set(_options INSTALL)
    set(_one_value INPUT_NAME OUTPUT_NAME OUTPUT_TYPE)
    cmake_parse_arguments (ARG "${_options}" "${_one_value}" "" ${ARGN})

    set(OUTPUT_TYPE "png")
    if(ARG_OUTPUT_TYPE)
      set(OUTPUT_TYPE ${ARG_OUTPUT_TYPE})
    endif()

    set(INPUT_NAME "${ID}.msc")
    if(ARG_INPUT_NAME)
      set(INPUT_NAME ${ARG_INPUT_NAME})
    endif()

    set(OUTPUT_NAME "${ID}.png")
    if(ARG_OUTPUT_NAME)
      set(OUTPUT_NAME ${ARG_OUTPUT_NAME})
    endif()

    add_custom_command(
      OUTPUT 
        ${OUTPUT_NAME}
      COMMAND
        ${MSCGEN_EXECUTABLE}
        -T ${OUTPUT_TYPE}
        -i "${CMAKE_CURRENT_SOURCE_DIR}/${INPUT_NAME}"
        -o "${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT_NAME}"
      DEPENDS
        ${INPUT_NAME}
    )

    add_custom_target(${ID} ALL
      DEPENDS ${OUTPUT_NAME}
    )

    if(ARG_INSTALL)
      install(
        FILES
          "${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT_NAME}"
        DESTINATION
          ${CMAKE_INSTALL_DOCDIR}
      )
    endif()
  endif()
endfunction(mscgen)
