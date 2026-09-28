
add_definitions(
	-pedantic
	-Wall
	-Wextra
)

add_subdirectory(libhud)
add_subdirectory(libhud-client)
add_subdirectory(qtgmenu)
add_subdirectory(service)
add_subdirectory(window-stack-bridge)
