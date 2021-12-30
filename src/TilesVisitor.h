#ifndef TILESVISITOR_H
#define TILESVISITOR_H

#include <vsg/core/Visitor.h>
#include <vsg/maths/transform.h>
#include <vsg/viewer/Camera.h>
#include <QString>

/*
class LoadTiles : public vsg::Visitor
{
public:
    LoadTiles(vsg::ref_ptr<vsg::Options> in_options);

    void apply(vsg::Node& node) override;
    void apply(vsg::PagedLOD& plod) override;

    unsigned int numTiles = 0;

    vsg::ref_ptr<vsg::Group> tiles;

    vsg::ref_ptr<vsg::Options> options
};
*/


#endif // TILESVISITOR_H
