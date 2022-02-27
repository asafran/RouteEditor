find_package(Boost REQUIRED)
find_package(TBB REQUIRED)

set(SOURCES
    src/LambdaVisitor.h
    src/ParentVisitor.cpp
    src/ParentVisitor.h
    src/Constants.h
    src/interlocking.h
    src/interlocking.cpp
    src/stmodels.h
    src/stmodels.cpp
    src/lightanimation.h
    src/lightanimation.cpp
    src/tools.h
)

set(SPLINE
    spline_library/splines/uniform_cr_spline.h
    spline_library/splines/cubic_hermite_spline.h
    spline_library/utils/arclength.h
    spline_library/utils/splineinverter.h
    spline_library/utils/spline_common.h
)

find_package(Qt6 COMPONENTS Widgets REQUIRED)

add_library(spline ${SPLINE})

target_include_directories(spline PUBLIC spline_library)

add_library(tools ${SOURCES})

target_include_directories(tools PUBLIC src)

target_link_libraries(tools spline objects vsg::vsg Qt6::Widgets)
