project(POG2WHY)
message(STATUS "Processing ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt")

find_package(Qt6 REQUIRED COMPONENTS Core Xml)


set(POG2WHY_SOURCES
    typingContext.h typingContext.cpp
    why.h why.cpp
    pog2why.cpp
    )

include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${BASTLIB_SOURCE_DIR}
    ${POG_SOURCE_DIR}
    ${tinyxml2_SOURCE_DIR}
    )

add_executable(pog2why ${POG2WHY_SOURCES})

target_link_libraries(pog2why -Bstatic POGLIB tinyxml2::tinyxml2 -Bdynamic Qt6::Core Qt6::Xml)
