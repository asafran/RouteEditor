#ifndef SCENEOBJECTS_H
#define SCENEOBJECTS_H

#include <QFileInfo>
#include <vsg/all.h>

class SceneObject : public vsg::Inherit<vsg::MatrixTransform, SceneObject>
{
public:
    SceneObject(vsg::ref_ptr<vsg::Node> loaded, const QFileInfo &file, const vsg::dmat4& in_matrix = {});

    ~SceneObject();

    vsg::dmat4 world();

    QFileInfo fileinfo;
    vsg::dmat4 localToWord;
    vsg::Group *parent;
};

#endif // SCENEOBJECTS_H
