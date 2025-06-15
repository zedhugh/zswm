set(LIB_STATUS "zswm-status")
set(LIB_STATUS_NAME ${LIB_STATUS} PARENT_SCOPE)

add_library(${LIB_STATUS} STATIC pulse.c net.c)
target_include_directories(${LIB_STATUS} PUBLIC include)

target_link_libraries(${LIB_STATUS} m)

find_package(PkgConfig REQUIRED)

pkg_check_modules(PulseMainloopGlib libpulse-mainloop-glib REQUIRED)
if (PulseMainloopGlib_FOUND)
  target_include_directories(${LIB_STATUS} SYSTEM PUBLIC ${PulseMainloopGlib_INCLUDE_DIRS})
  target_link_libraries(${LIB_STATUS} ${PulseMainloopGlib_LIBRARIES})
endif()
