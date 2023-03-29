cmake_minimum_required(VERSION 3.24)

project(zswm VERSION 2.0 LANGUAGES C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(BINARY "zswm")
add_executable(${BINARY} zswm.c utils.c event.c)

find_package(Freetype REQUIRED)
target_link_libraries(${BINARY} ${FREETYPE_LIBRARIES})
target_include_directories(${BINARY} PRIVATE ${FREETYPE_INCLUDE_DIRS})

find_package(Fontconfig REQUIRED)
target_link_libraries(${BINARY} ${Fontconfig_LIBRARIES})
target_include_directories(${BINARY} PRIVATE ${Fontconfig_INCLUDE_DIRS})

find_package(X11 REQUIRED)
target_link_libraries(${BINARY} X11 X11-xcb Xft xcb xcb-icccm xcb-keysyms xcb-util xcb-xinerama)


add_custom_target(copy_compile_commands
  DEPENDS ${CMAKE_BINARY_DIR}/compile_commands.json)

if (CMAKE_EXPORT_COMPILE_COMMANDS)
  add_custom_command(
    TARGET copy_compile_commands
    PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_CURRENT_BINARY_DIR}/compile_commands.json
    ${PROJECT_SOURCE_DIR}/compile_commands.json
  )
  # add_executable(${BINARY} DEPENDS aa)
endif()
