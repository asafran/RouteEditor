#include "Selector.h"
#include <vsg/traversals/ComputeBounds.h>
#include "ParentVisitor.h"
#include <QSettings>

Selector::Selector(vsg::ref_ptr<vsg::Group> root, vsg::ref_ptr<vsg::Builder> builder, QObject *parent) : QObject(parent)
  , _builder(builder)
  , _root(root)
  , _mergedSelection(route::Selection::create())
{
}
Selector::~Selector() {}
