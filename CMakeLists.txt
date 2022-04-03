cmake_minimum_required(VERSION 3.7)

project(RouteEditor
        VERSION 0.0.0
        DESCRIPTION "Route editor for RRS based on VirtualPlanetBuilder databases"
        LANGUAGES CXX C
)

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
    src/SignalManager.h
    src/SignalManager.cpp
    src/SignalManager.ui
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
    src/StageDialog.cpp
    src/StageDialog.h
    src/StageDialog.ui
    src/undo-redo.h
    src/undo-redo.cpp
    src/InterlockDialog.h
    src/InterlockDialog.cpp
    src/InterlockDialog.ui
    src/main.cpp
)

# Change the default build type to Release
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif(NOT CMAKE_BUILD_TYPE)

# find the vsg
if (VULKAN_SDK)
    set(ENV{VULKAN_SDK} ${VULKAN_SDK})
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 COMPONENTS Widgets REQUIRED)
find_package(Qt6 COMPONENTS Xml REQUIRED)
#find_package(Qt6 COMPONENTS Concurrent REQUIRED)

find_package(vsg 0.2.9 REQUIRED)
find_package(vsgXchange REQUIRED)
find_package(vsgQt REQUIRED)

# set the use of C++17 globally as all examples require it
set(CMAKE_CXX_STANDARD 17)

# add clobber build target to clear all the non git registered files/directories
add_custom_target(clobber
    COMMAND git clean -d -f -x
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

#add_subdirectory(RRSConv)

add_executable(editor ${SOURCES})

add_subdirectory(tools)

add_subdirectory(objects)

target_compile_definitions(editor PRIVATE VK_USE_PLATFORM_XCB_KHR)

target_compile_definitions(editor PRIVATE ORGANIZATION_NAME="RRS" ORGANIZATION_DOMAIN="rusrailsim.org" APPLICATION_NAME="editor")

target_link_libraries(editor objects vsgQt::vsgQt vsg::vsg vsgXchange::vsgXchange)
