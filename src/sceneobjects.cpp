#include "sceneobjects.h"

SceneObject::SceneObject(vsg::ref_ptr<vsg::Node> loaded, const QFileInfo &file, const vsg::dmat4& in_matrix)
    : vsg::Inherit<vsg::MatrixTransform, SceneObject>(in_matrix)
    , fileinfo(file)
{
    addChild(loaded);
    setValue(META_NAME, file.baseName().toStdString());
}
SceneObject::~SceneObject() {}

vsg::dmat4 SceneObject::world()
{
    return matrix * localToWord;
}
