set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

set(SOURCES
    src/MainConvWindow.cpp
    src/MainConvWindow.h
    src/MainConvWindow.ui
    src/IntersectionHandler.h
    src/IntersectionHandler.cpp
    src/IntersectionHandler.ui
    src/AnimationModel.h
    src/AnimationModel.cpp
    src/main_conv.cpp
)

add_executable(converter ${SOURCES})

target_include_directories(converter PRIVATE ../tools)

target_compile_definitions(converter PRIVATE VK_USE_PLATFORM_XCB_KHR)

set_target_properties( converter
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}../bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}../bin"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}../bin"
)

target_link_libraries(converter tools vsgQt::vsgQt vsg::vsg vsgXchange::vsgXchange)
