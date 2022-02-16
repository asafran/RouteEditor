#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <string>
#include <QStringList>

namespace app {

static constexpr const char*const PARENT = "Parent";
static constexpr const char*const PROP = "MetaProp";
static constexpr const char*const NAME = "Name";
static constexpr const char*const TOPOLOGY = "Topology";
static const QStringList FORMATS = { "*.vsgt", "*.vsgb", "*.osgt", "*.osgb", "*.obj", "*.gltf", "*.glb"};

}

#endif // CONSTANTS_H
