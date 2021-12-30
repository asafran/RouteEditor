#include <vsg/io/FileSystem.h>
#include <vsg/io/read.h>
#include <vsg/nodes/CullNode.h>
#include <vsg/nodes/LOD.h>
#include <vsg/nodes/MatrixTransform.h>
#include <vsg/nodes/PagedLOD.h>
#include <vsg/nodes/Switch.h>
#include "TilesVisitor.h"
#include <QFileInfo>

LoadTiles::LoadTiles(vsg::ref_ptr<vsg::Options> in_options) :
    vsg::Visitor()
  , tiles(vsg::Group::create())
  , options(in_options)
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
        if (!child.node)
        {
            child.node = vsg::read_cast<vsg::Node>(plod.filename, options);
            if(child.node->is_compatible(typeid (vsg::Group)))
                if(auto group = child.node.cast<vsg::Group>(); group->children.front()->is_compatible(typeid (vsg::MatrixTransform)))
                {
                    group->setValue(TILE_PATH, plod.filename);
                    group->setValue(META_NAME, plod.filename);
                    tiles->addChild(group);
                    if(group->children.size() < 5)
                    {
                        auto objectLayer = vsg::Switch::create();
                        objectLayer->setValue(META_NAME, "Слой объектов");
                        group->addChild(objectLayer);

                        auto trackLayer = vsg::Group::create();
                        trackLayer->setValue(META_NAME, "Слой путевой инфраструктуры");
                        group->addChild(trackLayer);
                    }
                }
            ++numTiles;
        }

        if (child.node) child.node->accept(*this);
    }
}

/*
void LoadTiles::apply(vsg::PagedLOD& plod)
{
    for (auto& child : plod.children)
    {
        auto find = loaded.find(plod.filename);
        if (!child.node && find != loaded.end())
        {
            child.node = find->second;
            if(child.node->is_compatible(typeid (vsg::Group)))
                if(auto group = child.node.cast<vsg::Group>(); group->children.front()->is_compatible(typeid (vsg::MatrixTransform)))
                {
                    group->setValue(TILE_PATH, plod.filename);
                    group->setValue(META_NAME, plod.filename);
                    tiles->addChild(group);
                    if(group->children.size() < 5)
                    {
                        auto objectLayer = vsg::Switch::create();
                        objectLayer->setValue(META_NAME, "Слой объектов");
                        group->addChild(objectLayer);

                        auto trackLayer = vsg::Group::create();
                        trackLayer->setValue(META_NAME, "Слой путевой инфраструктуры");
                        group->addChild(trackLayer);
                    }
                }
            ++numTiles;
        }

        if (child.node) child.node->accept(*this);
    }
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
*/
