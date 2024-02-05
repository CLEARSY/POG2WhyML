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

add_library(POG2WHYLIB
    ${POG2WHY_SOURCES}
    ${POG2WHY_HEADERS}
    )

target_link_libraries(POG2WHYLIB Qt5::Core Qt5::Xml POGLIB)

add_executable(pog2why
    pog2why.cpp
    )

target_link_libraries(pog2why -Bstatic POG2WHYLIB POGLIB -Bdynamic Qt5::Core Qt5::Xml)

# if(APPLE)
#     install(TARGETS pog2why
#         RUNTIME DESTINATION ${install_prefix}bbin/${type}
#         BUNDLE  DESTINATION ${install_prefix}bbin/${type}
#         )
# else(APPLE)
#     install(TARGETS pog2why
#         RUNTIME DESTINATION ${install_prefix}bbin/${type}
#         )
# endif(APPLE)
