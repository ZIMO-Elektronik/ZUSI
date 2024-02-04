cmake_minimum_required(VERSION 3.25 FATAL_ERROR)
include(FetchContent)

FetchContent_Declare(
  CMakeModules
  GIT_REPOSITORY https://github.com/ZIMO-Elektronik/CMakeModules
  GIT_TAG v0.1.2
  SOURCE_DIR ${CMAKE_BINARY_DIR}/CMakeModules)
FetchContent_MakeAvailable(CMakeModules)

version_from_git()
project(
  ZUSI
  VERSION ${VERSION_FROM_GIT}
  LANGUAGES CXX)

file(GLOB_RECURSE SRC src/*.cpp)
add_library(ZUSI STATIC ${SRC})
add_library(ZUSI::ZUSI ALIAS ZUSI)

target_compile_features(ZUSI PUBLIC cxx_std_23)

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  target_include_directories(
    ZUSI
    INTERFACE include
    PRIVATE include/zusi)
else()
  target_include_directories(
    ZUSI SYSTEM
    INTERFACE include
    PRIVATE include/zusi)
endif()

target_common_warnings(ZUSI PRIVATE)

if(NOT TARGET Microsoft.GSL::GSL)
  cpmaddpackage("gh:microsoft/GSL@4.0.0")
endif()

target_link_libraries(ZUSI PRIVATE Microsoft.GSL::GSL)

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  include(CTest)
endif()

if(BUILD_TESTING
   AND CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME
   AND CMAKE_SYSTEM_NAME STREQUAL CMAKE_HOST_SYSTEM_NAME)
  add_subdirectory(examples)
  add_subdirectory(tests)
endif()
