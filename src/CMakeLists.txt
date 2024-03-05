project(POG2WHY)
message(STATUS "Processing ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt")

if(NOT DEFINED QT5_FOUND)
  find_package(Qt5 REQUIRED COMPONENTS Core Xml)
endif()

set(POG2WHY_HEADERS
    typingContext.h
    why.h
    )

set(POG2WHY_SOURCES
    typingContext.cpp
    why.cpp
    )

include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${POFILES_SOURCE_DIR}
    ${BASTLIB_SOURCE_DIR}
    ${POG_SOURCE_DIR}
    )

add_library(POG2WHY_LIB STATIC ${POG2WHY_SOURCES} ${POG2WHY_HEADERS})
set_target_properties(POG2WHY_LIB PROPERTIES PREFIX "lib" OUTPUT_NAME "POG2Why")
target_link_libraries(POG2WHY_LIB POG_LIB Qt5::Core Qt5::Xml)

add_executable(pog2why pog2why.cpp)

target_link_libraries(pog2why -Bstatic POG2WHY_LIB POG_LIB -Bdynamic Qt5::Core Qt5::Xml)
