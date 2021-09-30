#include "sceneobject.h"

SceneObject::SceneObject(size_t numChildren) :
    vsg::Inherit<vsg::Group, SceneObject>(numChildren)
{
}

SceneObject::SceneObject(vsg::ref_ptr<vsg::Node> loaded, const QFileInfo &file) :
    vsg::Inherit<vsg::Group, SceneObject>(1)

{
    addChild(loaded);
    setValue(META_NAME, file.baseName().toStdString());
    path = file.absoluteFilePath();
}

SceneObject::SceneObject(vsg::Allocator* allocator, size_t numChildren) :
    vsg::Inherit<vsg::Group, SceneObject>(allocator, numChildren)
{
}

SceneObject::~SceneObject()
{
}


