#ifndef TILESVISITOR_H
#define TILESVISITOR_H

#include <vsg/core/Visitor.h>
#include <vsg/maths/transform.h>
#include <vsg/viewer/Camera.h>
#include <QMap>

#include <stack>

class LoadTiles : public vsg::Visitor
{
public:
    LoadTiles(QMap<QString, vsg::ref_ptr<vsg::Node>> in_loaded);

    void apply(vsg::Node& node) override;
    void apply(vsg::PagedLOD& plod) override;

    unsigned int numTiles = 0;

    vsg::ref_ptr<vsg::Group> tiles;

    QMap<QString, vsg::ref_ptr<vsg::Node>> loaded;
};



#endif // TILESVISITOR_H
