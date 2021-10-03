#ifndef TILESVISITOR_H
#define TILESVISITOR_H

#include <QSet>
#include <QString>
#include <QFileInfo>
#include <vsg/nodes/PagedLOD.h>
#include <vsg/nodes/MatrixTransform.h>


class TilesVisitor : public vsg::ConstVisitor
{
public:
    TilesVisitor(vsg::ref_ptr<vsg::Group> group);

    using vsg::ConstVisitor::apply;

    void apply(const vsg::Object& object) override;

    void apply(const vsg::PagedLOD& plod) override;

    vsg::ref_ptr<vsg::Group> tiles;
    QSet<QString> filenames;
};

#endif // TILESVISITOR_H
