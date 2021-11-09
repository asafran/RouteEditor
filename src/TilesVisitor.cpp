#include <vsg/io/FileSystem.h>
#include <vsg/io/read.h>
#include <vsg/nodes/CullNode.h>
#include <vsg/nodes/LOD.h>
#include <vsg/nodes/MatrixTransform.h>
#include <vsg/nodes/PagedLOD.h>
#include "TilesVisitor.h"
#include <QFileInfo>

LoadTiles::LoadTiles() :
    vsg::Visitor()
  , tiles(vsg::Group::create())
{
}

void LoadTiles::apply(vsg::Node& node)
{
    node.traverse(*this);
}

void LoadTiles::apply(vsg::PagedLOD& plod)
{
    for (auto& child : plod.children)
    {
        QFileInfo filepath(plod.filename.c_str());

        if (!child.node)
        {
            child.node = vsg::read_cast<vsg::Node>(filepath.canonicalFilePath().toStdString(), plod.options);
            if(child.node->is_compatible(typeid (vsg::Group)))
                if(child.node.cast<vsg::Group>()->children.front()->is_compatible(typeid (vsg::MatrixTransform)))
                {
                    child.node->setValue(META_NAME, plod.filename);
                    tiles->addChild(child.node);
                }
            ++numTiles;
        }

        if (child.node) child.node->accept(*this);
    }
}
