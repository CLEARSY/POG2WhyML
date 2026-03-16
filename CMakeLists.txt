cmake_minimum_required(VERSION 3.16)
project(POG2WhyML)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# set build type if not set
if(NOT CMAKE_BUILD_TYPE)
  message(STATUS "Setting build type to 'Release' as none was specified.")
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
endif()

if(${CMAKE_PROJECT_NAME} STREQUAL ${PROJECT_NAME})
  set(PROJECT_IS_TOP_LEVEL TRUE)
else()
  set(PROJECT_IS_TOP_LEVEL FALSE)
endif()


if(PROJECT_IS_TOP_LEVEL)
  set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake/modules)
  add_subdirectory(extern/tinyxml2)
  add_subdirectory(extern/BAST)
  add_subdirectory(extern/POGLIB)
endif(PROJECT_IS_TOP_LEVEL)

add_subdirectory(src)

if(PROJECT_IS_TOP_LEVEL)
  include (CTest)
  add_subdirectory(test)
endif(PROJECT_IS_TOP_LEVEL)
