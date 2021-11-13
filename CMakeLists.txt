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
#    src/sorter.ui
#    src/sorter.h
#    src/sorter.cpp
    src/metainf.h
#    src/metainf.cpp
    src/StartDialog.h
    src/StartDialog.cpp
    src/StartDialog.ui
    src/DatabaseManager.cpp
    src/DatabaseManager.h
    src/undo-redo.cpp
    src/undo-redo.h
    src/Manipulator.h
    src/Manipulator.cpp
    src/TilesVisitor.cpp
    src/TilesVisitor.h
    src/sceneobjects.h
    src/sceneobjects.cpp
    src/tilessorter.cpp
    src/tilessorter.h
    src/LambdaVisitor.cpp
    src/LambdaVisitor.h
    src/ObjectModel.h
    src/ObjectModel.cpp
    src/SceneModel.h
    src/SceneModel.cpp
    src/ParentVisitor.cpp
    src/ParentVisitor.h
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
find_package(Qt6 COMPONENTS Concurrent REQUIRED)

find_package(vsg REQUIRED)
find_package(vsgXchange REQUIRED)
find_package(vsgQt REQUIRED)

if(TARGET assimp::assimp)
  get_property(assimp_INTERFACE_INCLUDE_DIRECTORIES
               TARGET assimp::assimp
               PROPERTY INTERFACE_INCLUDE_DIRECTORIES)
  if(assimp_INTERFACE_INCLUDE_DIRECTORIES MATCHES "/usr/lib/include")
    string(REPLACE "/usr/lib/include" "/usr/include" assimp_INTERFACE_INCLUDE_DIRECTORIES "${assimp_INTERFACE_INCLUDE_DIRECTORIES}")
    set_property(TARGET assimp::assimp
                 PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                 "${assimp_INTERFACE_INCLUDE_DIRECTORIES}")
    get_property(assimp_LOCATION_RELEASE
                 TARGET assimp::assimp
                 PROPERTY LOCATION_RELEASE)
    set_property(TARGET assimp::assimp
                 PROPERTY IMPORTED_LOCATION
                 "${assimp_LOCATION_RELEASE}")
  endif()
endif()

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

add_executable(editor ${SOURCES})

target_compile_definitions(editor PRIVATE VK_USE_PLATFORM_XCB_KHR)

target_compile_definitions(editor PRIVATE META_NAME="MetaName" TILE_PATH="Path" ORGANIZATION_NAME="RRS" ORGANIZATION_DOMAIN="rusrailsim.org" APPLICATION_NAME="editor")

target_link_libraries(editor vsgQt::vsgQt vsg::vsg vsgXchange::vsgXchange Qt6::Concurrent)
