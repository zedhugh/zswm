cmake_minimum_required(VERSION 3.24)

project(zswm VERSION 2.0 LANGUAGES C)

add_compile_definitions(LOG_FILE="/home/zedhugh/zswm-log.txt")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(BINARY "zswm")
add_executable(${BINARY} zswm.c utils.c event.c)

find_package(X11 REQUIRED)
target_link_libraries(${BINARY} X11 X11-xcb xcb xcb-icccm xcb-keysyms xcb-util xcb-xinerama)

find_package(PkgConfig REQUIRED)

pkg_check_modules(Cairo cairo REQUIRED)
if(Cairo_FOUND)
  foreach(DIR IN LISTS Cairo_INCLUDE_DIRS)
    target_include_directories(${BINARY} PRIVATE ${DIR})
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
    target_include_directories(${BINARY} PRIVATE ${DIR})
  endforeach()

  foreach(LIB IN LISTS Pango_LIBRARIES)
    target_link_libraries(${BINARY} ${LIB})
  endforeach()
endif()

pkg_check_modules(Pango_Cairo pangocairo REQUIRED)
if(Pango_Cairo_FOUND)
  foreach(DIR IN LISTS Pango_Cairo_INCLUDE_DIRS)
    target_include_directories(${BINARY} PRIVATE ${DIR})
  endforeach()

  foreach(LIB IN LISTS Pango_Cairo_LIBRARIES)
    target_link_libraries(${BINARY} ${LIB})
  endforeach()
endif()
