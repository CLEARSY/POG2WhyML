project(POG2WHY)
message(STATUS "Processing ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt")

find_package(Qt6 REQUIRED COMPONENTS Core Xml)


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
    ${tinyxml2_SOURCE_DIR}
    )

add_library(POG2WHY_LIB STATIC ${POG2WHY_SOURCES} ${POG2WHY_HEADERS})
set_target_properties(POG2WHY_LIB PROPERTIES PREFIX "lib" OUTPUT_NAME "POG2Why")
target_link_libraries(POG2WHY_LIB POG_LIB Qt6::Core Qt6::Xml)

add_executable(pog2why pog2why.cpp)

target_link_libraries(pog2why -Bstatic POG2WHY_LIB POG_LIB tinyxml2::tinyxml2 -Bdynamic Qt6::Core Qt6::Xml)
