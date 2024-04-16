#include "IntersectionHandler.h"
#include "ui_IntersectionHandler.h"

#include <vsg/ui/KeyEvent.h>
#include <vsg/ui/PointerEvent.h>
#include <vsg/ui/ScrollWheelEvent.h>

#include <vsg/nodes/StateGroup.h>
#include <vsg/nodes/CullNode.h>
#include <vsg/nodes/DepthSorted.h>

#include <vsg/utils/ComputeBounds.h>
#include <vsg/maths/transform.h>
#include <vsg/nodes/MatrixTransform.h>

#include <vsg/app/Viewer.h>

#include "animation-common.h"

#include "LambdaVisitor.h"

#include "filesystem.h"

IntersectionHandler::IntersectionHandler(vsg::ref_ptr<vsg::Group> scenegraph, vsg::ref_ptr<route::AnimatedObject> model, vsg::ref_ptr<vsg::Viewer> viewer, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::IntersectionHandler),
    _scenegraph(scenegraph),
    _selected(vsg::MatrixTransform::create()),
    _viewer(viewer)
{
    ui->setupUi(this);

    _model = new AnimationModel(model, this);
    ui->listView->setModel(_model);

    connect(ui->addButt, &QPushButton::pressed, this, &IntersectionHandler::add);
    connect(ui->startButt, &QPushButton::pressed, this, &IntersectionHandler::start);
    connect(ui->baseButt, &QPushButton::pressed, this, &IntersectionHandler::addBase);
    connect(ui->removeButt, &QPushButton::pressed, this, &IntersectionHandler::remove);
    connect(ui->upButt, &QPushButton::pressed, this, &IntersectionHandler::down);
    connect(ui->downButt, &QPushButton::pressed, this, &IntersectionHandler::up);
    connect(ui->moveButt, &QPushButton::pressed, this, [this]{ _type = Move; });
    connect(ui->alphaButt, &QPushButton::pressed, this, [this]{ _type = Alpha; });
    connect(ui->rotButt, &QPushButton::pressed, this, [this]{ _type = Rot; });
    connect(ui->illumButt, &QPushButton::pressed, this, [this]{ _type = Light; });

    connect(ui->listView, &QListView::clicked, this, [this](const QModelIndex &index)
    {
        _animation = _model->animationIndex(index);
    });
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

    auto duration = ui->durationSpin->value();

    vsg::ref_ptr<Animation> animation;

    switch (_type) {
    case IntersectionHandler::Move:
    {
        vsg::dvec3 min{ui->x1->value(), ui->y1->value(), ui->z1->value()};
        vsg::dvec3 max{ui->x2->value(), ui->y2->value(), ui->z2->value()};
        //animation = MoveAnimation<Animation>::create(node, path, min, max, duration);
        break;
    }
    case IntersectionHandler::Rot:
    {
        auto quat1 = route::toQuaternion(qDegreesToRadians(ui->xr1->value()), qDegreesToRadians(ui->yr1->value()), qDegreesToRadians(ui->zr1->value()));
        auto quat2 = route::toQuaternion(qDegreesToRadians(ui->xr2->value()), qDegreesToRadians(ui->yr2->value()), qDegreesToRadians(ui->zr2->value()));
        //animation = RotateAnimation<Animation>::create(node, path, quat1, quat2, duration);
        break;
    }
    case IntersectionHandler::Light:
    {
        vsg::ref_ptr<vsg::Light> found;
        auto findLight = [name=ui->illumEdit->text().toStdString(), &found](vsg::Light& light)
        {
            if(name == light.name)
                found = &light;
        };
        LambdaVisitor<decltype (findLight), vsg::Light> lv(findLight);
        _scenegraph->accept(lv);

        auto min = static_cast<float>(ui->illum1->value());
        auto max = static_cast<float>(ui->illum2->value());

        if(found)
            //animation = LightAnimation<Animation>::create(found, min, max, duration);
        break;
    }
    case IntersectionHandler::Alpha:
    {
        auto min = static_cast<float>(ui->alpha1->value());
        auto max = static_cast<float>(ui->alpha2->value());

        auto fdi = vsg::visit<route::FindTexture>(node);

        auto data = fdi.imageInfo->imageView->image->data;
        data->properties.dataVariance = vsg::DYNAMIC_DATA;

        vsg::StateInfo si;
        si.blending = true;
        si.image = data;
        si.lighting = false;

        //route::assignStateGroup(fdi.imageStateGroup, si, builder->options);

        auto cb = vsg::visit<vsg::ComputeBounds>(node);

        auto depthSorted = vsg::DepthSorted::create();
        depthSorted->binNumber = 10;
        depthSorted->bound.center = (cb.bounds.min + cb.bounds.max) * 0.5;
        depthSorted->bound.radius = vsg::length(cb.bounds.max - cb.bounds.min) * 0.6;
        depthSorted->child = node;

        //animation = AlphaAnimation<Animation>::create(depthSorted, data, min, max, duration);
        break;
    }


    }
    auto result = _viewer->compileManager->compile(animation);
    if(result.result == VK_SUCCESS)
        vsg::updateViewer(*_viewer, result);

    ui->listView->selectionModel()->clearSelection();

    animation->matrix = _selected->matrix;
    _model->addAnimation(ui->nameEdit->text(), animation);
}

void IntersectionHandler::start()
{
    if(_animation)
    {
        _animation->resetReverse();
        _animation->resetRun();
    }
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

void IntersectionHandler::remove()
{
    const auto &selected = ui->listView->selectionModel()->selectedIndexes();
    if(selected.isEmpty())
        return;
    _model->remove(selected.front());
}

void IntersectionHandler::processSelection()
{
    auto cb = vsg::visit<vsg::ComputeBounds>(*_curr);
    auto ct = vsg::visit<vsg::ComputeTransform>(lastIntersection->nodePath.begin(), _curr);
    vsg::GeometryInfo gi(static_cast<vsg::box>(cb.bounds));

    gi.transform = ct.matrix;
    gi.color = vsg::vec4{1.0, 1.0, 1.0, 1.0};

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
