cmake_minimum_required(VERSION 3.24)

set(SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(BINARY "zswm")
add_compile_definitions(LOG_FILE="$ENV{HOME}/zswm-log.txt")

# add flags
add_compile_options(-g)
add_compile_options(-Wall)
# add_compile_options(-fsanitize=address)

project(zswm VERSION 2.0 LANGUAGES C)

add_executable(${BINARY} zswm.c utils.c draw.c event.c window.c xcursor.c)
target_include_directories(${BINARY} PRIVATE ${SOURCE_DIR})

find_package(X11 REQUIRED)
target_link_libraries(${BINARY} xcb xcb-cursor xcb-icccm xcb-keysyms xcb-util xcb-xfixes xcb-xinerama)

find_package(PkgConfig REQUIRED)

pkg_check_modules(Cairo cairo REQUIRED)
if(Cairo_FOUND)
  foreach(DIR IN LISTS Cairo_INCLUDE_DIRS)
    target_include_directories(${BINARY} SYSTEM PUBLIC ${DIR})
  endforeach()

  foreach(LIB IN LISTS Cairo_LIBRARIES)
    target_link_libraries(${BINARY} ${LIB})
  endforeach()
endif()

pkg_check_modules(Cairo_XCB cairo-xcb REQUIRED)
if(Cairo_XCB_FOUND)
  foreach(LIB IN LISTS Cairo_XCB_LIBRARIES)
    target_link_libraries(${BINARY} ${LIB})
  endforeach()
endif()

pkg_check_modules(Pango pango REQUIRED)
if(Pango_FOUND)
  foreach(DIR IN LISTS Pango_INCLUDE_DIRS)
    target_include_directories(${BINARY} SYSTEM PUBLIC ${DIR})
  endforeach()

  foreach(LIB IN LISTS Pango_LIBRARIES)
    target_link_libraries(${BINARY} ${LIB})
  endforeach()
endif()

pkg_check_modules(Pango_Cairo pangocairo REQUIRED)
if(Pango_Cairo_FOUND)
  foreach(DIR IN LISTS Pango_Cairo_INCLUDE_DIRS)
    target_include_directories(${BINARY} SYSTEM PUBLIC ${DIR})
  endforeach()

  foreach(LIB IN LISTS Pango_Cairo_LIBRARIES)
    target_link_libraries(${BINARY} ${LIB})
  endforeach()
endif()
