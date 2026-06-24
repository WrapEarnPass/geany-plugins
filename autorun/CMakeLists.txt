cmake_minimum_required(VERSION 3.27)

set(CMAKE_EXPORT_COMPILE_COMMANDS on)
list(INSERT CMAKE_MODULE_PATH 0 "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

project(
  autorun
  VERSION 0.0.1
  LANGUAGES C
  DESCRIPTION "Geany action interceptor plugin")

# the `pkg_check_modules` function is created with this call
find_package(PkgConfig REQUIRED)
# these calls create special `PkgConfig::<MODULE>` variables
pkg_check_modules(geany REQUIRED IMPORTED_TARGET GLOBAL geany)

add_library(${PROJECT_NAME} SHARED src/plugin_main.c src/autorun.c src/utils.c
                                   src/spawn.c src/menu.c src/project.c)
# prevent cmake from prefixing the shared library
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
target_link_libraries(${PROJECT_NAME} PRIVATE PkgConfig::geany)
# install to users bin
install(TARGETS ${PROJECT_NAME} DESTINATION plugins)
