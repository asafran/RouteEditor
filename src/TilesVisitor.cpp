#include "TilesVisitor.h"

TilesVisitor::TilesVisitor(vsg::ref_ptr<vsg::Group> group) : tiles(group)
{

}

void TilesVisitor::apply(const vsg::Object& object)
{
        object.traverse(*this);
}
void TilesVisitor::apply(const vsg::PagedLOD& plod)
{
    if(auto group = plod.children.front().node.cast<vsg::Group>(); group && group->children.front()->is_compatible(typeid (vsg::MatrixTransform)))
    {
        QFileInfo file(plod.filename.c_str());
        group->setValue(META_NAME, file.absoluteFilePath().toStdString());
        tiles->addChild(group);
        filenames.insert(file.absoluteFilePath());
    }
    plod.traverse(*this);
}
