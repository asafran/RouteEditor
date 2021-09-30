#ifndef SCENEOBJECT_H
#define SCENEOBJECT_H

#include <vsg/nodes/Group.h>
#include <vsg/io/read.h>
#include <QFileInfo>

class SceneObject : public vsg::Inherit<vsg::Group, SceneObject>
{
public:
    SceneObject(size_t numChildren = 1);
    SceneObject(vsg::ref_ptr<vsg::Node> loaded, const QFileInfo &file);
    SceneObject(vsg::Allocator* allocator, size_t numChildren = 1);

    template<typename Iterator>
    SceneObject(Iterator begin, Iterator end)
    {
        for (Iterator itr = begin; itr != end; ++itr) addChild(*itr);
    }
    ~SceneObject();

    QString path;
};

#endif // SCENEOBJECT_H
