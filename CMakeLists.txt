cmake_minimum_required(VERSION 3.25 FATAL_ERROR)
include(FetchContent)

FetchContent_Declare(
  CMakeModules
  GIT_REPOSITORY "https://github.com/ZIMO-Elektronik/CMakeModules"
  GIT_TAG v0.9.4
  SOURCE_DIR ${CMAKE_BINARY_DIR}/CMakeModules)
FetchContent_MakeAvailable(CMakeModules)

version_from_git()
project(
  ZUSI
  VERSION ${VERSION_FROM_GIT}
  LANGUAGES CXX)

set(ZUSI_MAX_PACKET_SIZE
    263u
    CACHE STRING "Maximum size of a packet in bytes")
set(ZUSI_MAX_FEEDBACK_SIZE
    4u
    CACHE STRING "Maximum size of feedback in bytes")

file(GLOB_RECURSE SRC src/*.cpp)
add_library(ZUSI STATIC ${SRC})
add_library(ZUSI::ZUSI ALIAS ZUSI)

target_compile_features(ZUSI PUBLIC cxx_std_23)

target_compile_definitions(
  ZUSI PUBLIC ZUSI_MAX_PACKET_SIZE=${ZUSI_MAX_PACKET_SIZE}
              ZUSI_MAX_FEEDBACK_SIZE=${ZUSI_MAX_FEEDBACK_SIZE})

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
  cpmaddpackage("gh:microsoft/GSL@4.2.0")
endif()

if(NOT TARGET ZTL::ZTL)
  cpmaddpackage("gh:ZIMO-Elektronik/ZTL@0.20.0")
endif()

target_link_libraries(
  ZUSI
  PRIVATE Microsoft.GSL::GSL
  PUBLIC ZTL::ZTL)

if(PROJECT_IS_TOP_LEVEL)
  include(CTest)
  add_subdirectory(examples)
  file(
    DOWNLOAD
    "https://github.com/ZIMO-Elektronik/.github/raw/master/data/.clang-format"
    ${CMAKE_CURRENT_LIST_DIR}/.clang-format)
  file(GLOB_RECURSE SRC include/*.*pp src/*.*pp tests/*.*pp)
  add_clang_format_target(ZUSIFormat OPTIONS -i FILES ${SRC})
endif()

if(BUILD_TESTING
   AND PROJECT_IS_TOP_LEVEL
   AND CMAKE_SYSTEM_NAME STREQUAL CMAKE_HOST_SYSTEM_NAME)
  add_subdirectory(tests)
endif()
