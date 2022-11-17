cmake_minimum_required(VERSION 3.7)

# build all examples into the bin directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

set(SOURCES
    src/MainWindow.cpp
    src/MainWindow.h
    src/MainWindow.ui
    src/ObjectPropertiesEditor.h
    src/ObjectPropertiesEditor.cpp
    src/ObjectPropertiesEditor.ui
    src/RailsPointEditor.h
    src/RailsPointEditor.cpp
    src/RailsPointEditor.ui
    src/ContentManager.h
    src/ContentManager.cpp
    src/ContentManager.ui
    #src/SignalManager.h
    #src/SignalManager.cpp
    #src/SignalManager.ui
    src/RouteCmdDialog.h
    src/RouteCmdDialog.cpp
    src/RouteCmdDialog.ui
    src/tool.h
    src/tool.cpp
    src/Painter.h
    src/Painter.cpp
    src/Painter.ui
    src/StartDialog.h
    src/StartDialog.cpp
    src/StartDialog.ui
    src/AddRails.cpp
    src/AddRails.ui
    src/AddRails.h
    src/DatabaseManager.cpp
    src/DatabaseManager.h
    src/Manipulator.h
    src/Manipulator.cpp
    src/TilesSorter.cpp
    src/TilesSorter.h
    src/SceneModel.h
    src/SceneModel.cpp
    src/PointsModel.h
    src/PointsModel.cpp
    src/SceneObjectVisitor.h
    src/SceneObjectVisitor.cpp
    src/undo-redo.h
    src/undo-redo.cpp
    src/InterlockDialog.h
    src/InterlockDialog.cpp
    src/InterlockDialog.ui
    src/main.cpp
)

if(WIN32)
    add_definitions(-DVK_USE_PLATFORM_WIN32_KHR -DNOMINMAX)
    set(MODE WIN32)
elseif(APPLE)
    add_definitions(-DVK_USE_PLATFORM_MACOS_MVK)
elseif(UNIX)
    add_definitions(-DVK_USE_PLATFORM_XCB_KHR)
else()
    add_definitions(-DVK_USE_PLATFORM_XLIB_KHR)
endif()

add_subdirectory(RRSConv)

add_executable(editor ${SOURCES})

target_compile_definitions(editor PRIVATE VK_USE_PLATFORM_XCB_KHR)

target_link_libraries(editor objects vsgQt::vsgQt vsg::vsg vsgXchange::vsgXchange)
