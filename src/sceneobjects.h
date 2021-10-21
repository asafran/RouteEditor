#ifndef SCENEOBJECTS_H
#define SCENEOBJECTS_H

#include <QFileInfo>
#include <vsg/nodes/MatrixTransform.h>

class SceneObject : public vsg::Inherit<vsg::MatrixTransform, SceneObject>
{
public:
    SceneObject(vsg::ref_ptr<vsg::Node> loaded, const QFileInfo &file, const vsg::dmat4& in_matrix = {});

    ~SceneObject();

    QFileInfo fileinfo;
};

#endif // SCENEOBJECTS_H
