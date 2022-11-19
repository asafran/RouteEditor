#include "IntersectionHandler.h"
#include "ui_IntersectionHandler.h"

#include <vsg/ui/KeyEvent.h>
#include <vsg/ui/PointerEvent.h>
#include <vsg/ui/ScrollWheelEvent.h>

#include <vsg/nodes/StateGroup.h>
#include <vsg/nodes/CullNode.h>

#include <vsg/traversals/ComputeBounds.h>
#include <vsg/maths/transform.h>
#include <vsg/nodes/MatrixTransform.h>

#include "move-animation.h"
#include "rotate-animation.h"
#include "alpha-animation.h"
#include "light-animation.h"
#include "animation-path-handler.h"

#include "filesystem.h"

IntersectionHandler::IntersectionHandler(vsg::ref_ptr<vsg::Group> scenegraph, vsg::ref_ptr<AnimatedModel> model, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::IntersectionHandler),
    _scenegraph(scenegraph),
    _selected(vsg::MatrixTransform::create())
{
    ui->setupUi(this);

    _model = new AnimationModel(model, this);

    connect(ui->addButt, &QPushButton::pressed, this, &IntersectionHandler::add);
    connect(ui->startButt, &QPushButton::pressed, this, &IntersectionHandler::start);
    connect(ui->baseButt, &QPushButton::pressed, this, &IntersectionHandler::addBase);
    connect(ui->resetButt, &QPushButton::pressed, this, &IntersectionHandler::reset);
    connect(ui->upButt, &QPushButton::pressed, this, &IntersectionHandler::up);
    connect(ui->downButt, &QPushButton::pressed, this, &IntersectionHandler::down);
    connect(ui->moveButt, &QPushButton::pressed, this, [this]{ _type = Move; });
    connect(ui->alphaButt, &QPushButton::pressed, this, [this]{ _type = Alpha; });
    connect(ui->rotButt, &QPushButton::pressed, this, [this]{ _type = Rot; });
    connect(ui->illumButt, &QPushButton::pressed, this, [this]{ _type = Light; });
}


void IntersectionHandler::apply(vsg::ButtonPressEvent &buttonPressEvent)
{
    lastPointerEvent = &buttonPressEvent;

    if (buttonPressEvent.button == 1)
    {
        intersection(buttonPressEvent);
    }
}

void IntersectionHandler::apply(vsg::PointerEvent &pointerEvent)
{
    lastPointerEvent = &pointerEvent;
}

void IntersectionHandler::intersection(vsg::PointerEvent& pointerEvent)
{
    auto intersector = vsg::LineSegmentIntersector::create(*camera, pointerEvent.x, pointerEvent.y);
    _scenegraph->accept(*intersector);

    if (intersector->intersections.empty()) return;

    // sort the intersectors front to back
    std::sort(intersector->intersections.begin(), intersector->intersections.end(), [](auto& lhs, auto& rhs) { return lhs->ratio < rhs->ratio; });

    auto& front = intersector->intersections.front();

    if(front->nodePath.empty())
        return;

    _curr = std::find_if(front->nodePath.begin(), front->nodePath.end(), [](const vsg::Node* obj){ return obj->is_compatible(typeid(vsg::StateGroup)); });

    lastIntersection = intersector->intersections.front();

    processSelection();
}

void IntersectionHandler::add()
{
    if(_selected->children.empty() || ui->modelEdit->text().isEmpty() || ui->nameEdit->text().isEmpty())
        return;
    auto node = _selected->children.front();
    vsg::Path contentRoot(qgetenv("RRS2_ROOT").toStdString());
    if(contentRoot.empty())
        return;
    vsg::Path model(ui->modelEdit->text().toStdString());
    vsg::Path name(ui->nameEdit->text().toStdString() + ".vsgt");
    auto path = model.append(name);
    auto out = contentRoot.append("data").append("animations").append(path);

    switch (_type) {
    case IntersectionHandler::Move:
    {
        _animation = MoveAnimation<Animation>::create(node, path,
                                                      vsg::dvec3(ui->x1->value(), ui->y1->value(), ui->z1->value()),
                                                      vsg::dvec3(ui->x2->value(), ui->y2->value(), ui->z2->value()), ui->durationSpin->value());
        break;
    }
    case IntersectionHandler::Rot:
        break;
    case IntersectionHandler::Light:
        break;
    case IntersectionHandler::Alpha:
        break;


    }
    _animation->matrix = _selected->matrix;
    _model->addAnimation(ui->nameEdit->text(), _animation);
}

void IntersectionHandler::start()
{
    if(_animation)
        _animation->run();
}

void IntersectionHandler::up()
{
    if(lastIntersection->nodePath.empty())
        return;
    _curr++;
    if(_curr > lastIntersection->nodePath.end() - 1)
        _curr = lastIntersection->nodePath.end() - 1;

    processSelection();
}

void IntersectionHandler::down()
{
    if(lastIntersection->nodePath.empty())
        return;
    _curr--;
    if(_curr < lastIntersection->nodePath.begin())
        _curr = lastIntersection->nodePath.begin();

    processSelection();
}

void IntersectionHandler::addBase()
{
    _model->addBase(_selected);
}

void IntersectionHandler::reset()
{
    //_model->children.clear();
}

void IntersectionHandler::processSelection()
{
    auto cb = vsg::visit<vsg::ComputeBounds>(*_curr);
    auto ct = vsg::visit<vsg::ComputeTransform>(lastIntersection->nodePath.begin(), _curr);
    vsg::GeometryInfo gi(static_cast<vsg::box>(cb.bounds));

    gi.transform = ct.matrix;
    gi.color = {1.0, 1.0, 1.0, 1.0};

    vsg::StateInfo si;
    si.lighting = false;
    si.wireframe = true;

    if(_selected->matrix == ct.matrix)
        _selected->children.emplace_back(const_cast<vsg::Node*>(*_curr));
    _selected = vsg::MatrixTransform::create(ct.matrix);
    _selected->children.emplace_back(const_cast<vsg::Node*>(*_curr));

    builder->compileTraversal->compile(_selected);

    if(_scenegraph->children.size() > 1)
        _scenegraph->children.pop_back();
    _scenegraph->children.push_back(builder->createBox(gi, si));
}
