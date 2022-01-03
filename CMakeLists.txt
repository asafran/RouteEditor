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
    src/AddDialog.ui
    src/AddDialog.cpp
    src/AddDialog.h
    src/ObjectPropertiesEditor.h
    src/ObjectPropertiesEditor.cpp
    src/ObjectPropertiesEditor.ui
#    src/typecase.h
    #src/MouseHandler.h
    src/SceneObjectVisitor.h
    src/SceneObjectVisitor.cpp
    src/ContentManager.h
    src/ContentManager.cpp
    src/ContentManager.ui
    src/tool.h
    src/tool.cpp
    #src/Selector.cpp
    #src/Selector.h
    src/StartDialog.h
    src/StartDialog.cpp
    src/StartDialog.ui
    src/DatabaseManager.cpp
    src/DatabaseManager.h
    src/undo-redo.cpp
    src/undo-redo.h
    src/Manipulator.h
    src/Manipulator.cpp
    src/TilesSorter.cpp
    src/TilesSorter.h
    #src/TilesVisitor.cpp
    #src/TilesVisitor.h
    #src/TrajectoryModel.h
    #src/TrajectoryModel.cpp
    src/SceneModel.h
    src/SceneModel.cpp
    src/main.cpp
)

set(TOOLS
    tools/sceneobjects.h
    tools/sceneobjects.cpp
    #tools/trackobjects.h
    #tools/trackobjects.cpp
    #tools/connector.h
    #tools/connector.cpp
    tools/topology.h
    tools/topology.cpp
    tools/trajectory.h
    tools/trajectory.cpp
    #tools/LambdaVisitor.cpp
    #tools/vehicle-controller.h
    #tools/vehicle-controller.cpp
    #tools/MouseMovable.cpp
    #tools/MouseMovable.h
    tools/LambdaVisitor.h
    tools/ParentVisitor.cpp
    tools/ParentVisitor.h

)

set(SPLINE
    spline_library/splines/uniform_cr_spline.h
    spline_library/splines/cubic_hermite_spline.h
    spline_library/utils/arclength.h
    spline_library/utils/splineinverter.h
    spline_library/utils/spline_common.h
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

find_package(vsg REQUIRED)
find_package(vsgXchange REQUIRED)
find_package(vsgQt REQUIRED)
find_package(Boost REQUIRED)
find_package(TBB REQUIRED)

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

target_include_directories(editor PRIVATE tools)

add_library(tools ${TOOLS})

add_library(spline ${SPLINE})

target_include_directories(spline PUBLIC spline_library)

target_compile_definitions(editor PRIVATE VK_USE_PLATFORM_XCB_KHR)

target_compile_definitions(editor PRIVATE META_NAME="MetaName" META_PROPERTY="MetaProp" TOPOLOGY_KEY="Topology" TOPOLOGY_CHILD=1 ORGANIZATION_NAME="RRS" ORGANIZATION_DOMAIN="rusrailsim.org" APPLICATION_NAME="editor")

target_compile_definitions(tools PRIVATE META_NAME="MetaName" META_PROPERTY="MetaProp" TOPOLOGY_KEY="Topology")

target_link_libraries(tools spline Qt6::Core TBB::tbb)

target_link_libraries(editor tools vsgQt::vsgQt vsg::vsg vsgXchange::vsgXchange)
