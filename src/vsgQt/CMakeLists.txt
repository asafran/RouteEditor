
# create and install export header which contains export macros for libraries
#
# available arguments:
#
#    <target>        pattern for generating macro names (<target>_DECLSPEC)
#                    and install pathes (include/<target>/Export.h)
#
# In public c++ headers the generated file must be included with
#
#    #include <<target>/Export.h>
#
# and public classes be decorated with
#
#    class <target>_DECLSPEC <classname> ...
#
# @TODO: move to vsg

set(SOURCES
    KeyboardMap.cpp
    ProxySurface.cpp
    ProxyWindow.cpp
    ViewerWindow.cpp
)

set(HEADERS
    ../../include/vsgQt/KeyboardMap.h
    ../../include/vsgQt/ProxySurface.h
    ../../include/vsgQt/ProxyWindow.h
    ../../include/vsgQt/ViewerWindow.h
)

# shared mode is automatically choosed by setting BUILD_SHARED_LIBS=ON
# POSITION_INDEPENDENT_CODE ON is set be default in shared mode
add_library(vsgQt ${SOURCES} ${HEADERS})

target_include_directories(vsgQt
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
)
target_link_libraries(vsgQt
    PUBLIC
        Qt6::Widgets
        vsg::vsg
)

if(vsgXchange_FOUND)
    target_link_libraries(vsgQt PRIVATE vsgXchange::vsgXchange)
endif()
