#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <string>
#include <QStringList>

namespace app {

static constexpr const char*const PARENT = "Parent";
static constexpr const char*const COMPILER = "CompileTraversal";
static constexpr const char*const WIREFRAME = "stdWireframe";
static constexpr const char*const PROP = "MetaProp";
static constexpr const char*const NAME = "Name";
static constexpr const char*const TOPOLOGY = "Topology";
static const QStringList FORMATS = { "*.vsgt", "*.vsgb", "*.osgt", "*.osgb", "*.dae", "*.fbx", "*.obj", "*.gltf", "*.glb"};

}

#endif // CONSTANTS_H
