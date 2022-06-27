project(ProgressBar)

cmake_minimum_required(VERSION 3.0)

set(CURSES_NEED_NCURSES TRUE)
find_package(Curses)
include_directories(${CURSES_INCLUDE_DIRS})

set(INCLUDE_INSTALL_DIR "include/" CACHE STRING "Include path")
set(LIB_INSTALL_DIR "lib/" CACHE STRING "Library path")
include(CMakePackageConfigHelpers)
configure_package_config_file(progressbarConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/progressbarConfig.cmake
  INSTALL_DESTINATION ${LIB_INSTALL_DIR}/progressbar/cmake
  PATH_VARS INCLUDE_INSTALL_DIR)
write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/progressbarConfigVersion.cmake
  VERSION 2.0.0
  COMPATIBILITY SameMajorVersion)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/progressbarConfig.cmake
              ${CMAKE_CURRENT_BINARY_DIR}/progressbarConfigVersion.cmake
        DESTINATION ${LIB_INSTALL_DIR}/progressbar/cmake)

include_directories(include/progressbar)

add_subdirectory(lib)
add_subdirectory(test)
