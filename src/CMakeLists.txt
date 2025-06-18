cmake_minimum_required(VERSION 3.24)

set(SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(BINARY "zswm")
add_compile_definitions(LOG_FILE="$ENV{HOME}/zswm-log.txt")

# add flags
add_compile_options(-g)
add_compile_options(-Wall)
# add_compile_options(-fsanitize=address)

project(${BINARY} VERSION 2.0 LANGUAGES C)

add_executable(${BINARY} zswm.c utils.c draw.c event.c window.c xcursor.c res.c)
set_property(TARGET ${BINARY} PROPERTY C_STANDARD 17)
target_include_directories(${BINARY} PRIVATE ${SOURCE_DIR})

add_subdirectory(status)
target_link_libraries(${BINARY} ${LIB_STATUS_NAME})
target_include_directories(${BINARY} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/status)

find_package(X11 REQUIRED)
target_link_libraries(${BINARY} xcb xcb-cursor xcb-icccm xcb-keysyms xcb-util xcb-xfixes xcb-xinerama xcb-xrm)

find_package(PkgConfig REQUIRED)

pkg_check_modules(GLIB2 glib-2.0 REQUIRED)
if(GLIB2_FOUND)
  target_include_directories(${BINARY} SYSTEM PUBLIC ${GLIB2_INCLUDE_DIRS})

  target_link_libraries(${BINARY} ${GLIB2_LIBRARIES})
endif()

pkg_check_modules(Cairo cairo REQUIRED)
if(Cairo_FOUND)
  target_include_directories(${BINARY} SYSTEM PUBLIC ${Cairo_INCLUDE_DIRS})
  target_link_libraries(${BINARY} ${Cairo_LIBRARIES})
endif()

pkg_check_modules(Cairo_XCB cairo-xcb REQUIRED)
if(Cairo_XCB_FOUND)
  target_link_libraries(${BINARY} ${Cairo_XCB_LIBRARIES})
endif()

pkg_check_modules(Pango pango REQUIRED)
if(Pango_FOUND)
  target_include_directories(${BINARY} SYSTEM PUBLIC ${Pango_INCLUDE_DIRS})
  target_link_libraries(${BINARY} ${Pango_LIBRARIES})
endif()

pkg_check_modules(Pango_Cairo pangocairo REQUIRED)
if(Pango_Cairo_FOUND)
  target_include_directories(${BINARY} SYSTEM PUBLIC ${Pango_Cairo_INCLUDE_DIRS})
  target_link_libraries(${BINARY} ${Pango_Cairo_LIBRARIES})
endif()

set(ASSETS_DIR "${CMAKE_SOURCE_DIR}/icons")
set(NET_DOWN_ICON "${ASSETS_DIR}/net_down.png")
set(NET_UP_ICON "${ASSETS_DIR}/net_up.png")
set(SPEAKER_ICON "${ASSETS_DIR}/spkr.png")
set(CPU_ICON "${ASSETS_DIR}/cpu.png")
set(MEMORY_ICON "${ASSETS_DIR}/mem.png")
set(CLOCK_ICON "${ASSETS_DIR}/clock.png")
configure_file("${CMAKE_SOURCE_DIR}/assets.h.in" "${CMAKE_BINARY_DIR}/assets.h" @ONLY)
target_include_directories(${BINARY} PRIVATE ${CMAKE_BINARY_DIR})
