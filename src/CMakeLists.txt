project(POG2WHY)

find_package(Qt5Core)
find_package(Qt5Concurrent)
find_package(Qt5Xml)

set(POG2WHY_HEADERS
    typingContext.h
    why.h
    )

set(POG2WHY_SRC
    typingContext.cpp
    why.cpp
    )


include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${POFILES_SOURCE_DIR}
    )

add_library(POG2WHYLIB
    ${POG2WHY_SRC}
    ${POG2WHY_HEADERS}
    )

target_link_libraries(POG2WHYLIB POGLIB Qt5::Core Qt5::Xml)

add_executable(pog2why
    MACOSX_BUNDLE
    pog2why.cpp
    )

target_link_libraries(pog2why POG2WHYLIB POGLIB Qt5::Core Qt5::Xml)

if(APPLE)
    install(TARGETS pog2why
        RUNTIME DESTINATION ${install_prefix}bbin/${type}
        BUNDLE  DESTINATION ${install_prefix}bbin/${type}
        )
else(APPLE)
    install(TARGETS pog2why
        RUNTIME DESTINATION ${install_prefix}bbin/${type}
        )
endif(APPLE)
