find_package(Boost REQUIRED)

set(SOURCES
    src/sceneobjects.h
    src/sceneobjects.cpp
    src/topology.h
    src/topology.cpp
    src/trajectory.h
    src/trajectory.cpp
    src/signal.h
    src/signal.cpp
    tiny_obj_loader.h
)

add_library(objects ${SOURCES})

target_include_directories(objects PUBLIC src)

target_link_libraries(objects spline tools vsg::vsg vsgXchange::vsgXchange Qt6::Core Qt6::Concurrent)
