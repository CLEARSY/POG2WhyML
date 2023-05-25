project(POG2WhyML)

set(POG2WhyML_HEADERS
    typingContext.h
    version.h
    WhyML.h
    )

set(POG2WHY_SRC
    typingContext.cpp
    WhyML.cpp
    )


include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${POFILES_SOURCE_DIR}
    ${AST_SOURCE_DIR}
    ${POG_SOURCE_DIR}
    ${tinyxml2_SOURCE_DIR}
    )

add_library(POG2WHYLIB
    ${POG2WHY_SRC}
    ${POG2WHY_HEADERS}
    )

target_link_libraries(POG2WHYLIB POGLIB TINYXML2LIB)

add_executable(POG2WhyML
    POG2WhyML.cpp
    )

target_link_libraries(POG2WhyML POG2WHYLIB POGLIB)
