#include "Manipulator.h"
#include "LambdaVisitor.h"
#include <QSettings>
#include "undo-redo.h"
#include "DatabaseManager.h"
#include "ParentVisitor.h"
#include <vsg/nodes/MatrixTransform.h>
#include <vsg/utils/Builder.h>
#include <vsg/utils/ComputeBounds.h>
#include <QInputDialog>

Manipulator::Manipulator(vsg::ref_ptr<vsg::Camera> camera,
                         vsg::ref_ptr<vsg::EllipsoidModel> ellipsoidModel,
                         DatabaseManager *manager,
                         QObject *parent)
    : vsg::Inherit<vsg::Trackball, Manipulator>(camera, ellipsoidModel)
    , _database(manager)
    , _pointer(vsg::MatrixTransform::create(vsg::translate(_lookAt->center)))
{
    rotateButtonMask = vsg::BUTTON_MASK_2;
    supportsThrow = false;

    createPointer();
    manager->root->addChild(_pointer);
}
Manipulator::~Manipulator()
{
}

void Manipulator::createPointer()
{
    QSettings settings(app::ORGANIZATION_NAME, app::APPLICATION_NAME);

    auto size = static_cast<float>(settings.value("CURSORSIZE", 1).toInt());
    vsg::GeometryInfo info;

    info.dx.set(1.0f * size, 0.0f, 0.0f);
    info.dy.set(0.0f, 1.0f * size, 0.0f);
    info.dz.set(0.0f, 0.0f, 1.0f * size);
    info.position = {0.0f, 0.0f, -1.0f * size / 2};
    auto cone = _database->builder->createCone(info);
    _pointer->addChild(cone);
}

void Manipulator::apply(vsg::KeyPressEvent& keyPress)
{
     _keyModifier = keyPress.keyModifier;

     switch (keyPress.keyBase) {
     case vsg::KEY_M:
     {
         startMoving();
         break;
     }
     default:
         break;

     }
}

void Manipulator::apply(vsg::KeyReleaseEvent& keyRelease)
{
     _keyModifier &= !keyRelease.keyModifier;
}

void Manipulator::apply(vsg::ButtonPressEvent& buttonPress)
{
    if (buttonPress.handled) return;

    _hasFocus = withinRenderArea(buttonPress);
    _lastPointerEventWithinRenderArea = _hasFocus;

    if (buttonPress.mask & vsg::BUTTON_MASK_1)
    {
        _updateMode = INACTIVE;
        if(_isMoving)
        {
            _database->undoStack->endMacro();
            _isMoving = false;
        }
        else
            emit sendIntersection(intersectedObjects(buttonPress));
    } else if (buttonPress.mask & vsg::BUTTON_MASK_2)
        _updateMode = ROTATE;
    else if (buttonPress.mask & vsg::BUTTON_MASK_3 && _ellipsoidModel)
    {
        _updateMode = INACTIVE;

        auto isection = intersectedObjects(buttonPress);
        if(isection.tile == nullptr)
            return;
        setViewpoint(isection.intersection->worldIntersection);
        //FindPositionVisitor fpv(isection.tile.first);
        //auto model = _database->getTilesModel();
        //emit objectClicked(model->index(fpv(model->getRoot()), 0, QModelIndex()));
    } else
        _updateMode = INACTIVE;

    if (_hasFocus) buttonPress.handled = true;

    _zoomPreviousRatio = 0.0;
    _pan.set(0.0, 0.0);
    _rotateAngle = 0.0;

    _previousPointerEvent = &buttonPress;
}

void Manipulator::rotate(double angle, const vsg::dvec3& axis)
{
    vsg::dmat4 rotation = vsg::rotate(angle, axis);
    vsg::dmat4 lv = lookAt(_lookAt->eye, _lookAt->center, _lookAt->up);
    vsg::dvec3 centerEyeSpace = (lv * _lookAt->center);

    vsg::dmat4 matrix = inverse(lv) * translate(centerEyeSpace) * rotation * translate(-centerEyeSpace) * lv;

    _lookAt->up = normalize(matrix * (_lookAt->eye + _lookAt->up) - matrix * _lookAt->eye);
    _lookAt->center = matrix * _lookAt->center;
    _lookAt->eye = matrix * _lookAt->eye;
}

void Manipulator::zoom(double ratio)
{
    vsg::dvec3 lookVector = _lookAt->center - _lookAt->eye;
    _lookAt->eye = _lookAt->eye + lookVector * ratio;
}

void Manipulator::pan(const vsg::dvec2& delta)
{
}


void Manipulator::setLatLongAlt(const vsg::dvec3 &pos)
{
    setViewpoint(_ellipsoidModel->convertLatLongAltitudeToECEF(pos));
}
void Manipulator::setViewpoint(const vsg::dvec4 &pos_mat)
{
    setViewpoint(vsg::dvec3(pos_mat.x, pos_mat.y, pos_mat.z));
}
void Manipulator::setViewpoint(const vsg::dvec3 &pos)
{
    auto lookAt = vsg::LookAt::create(*_lookAt);
    lookAt->eye += (pos - _lookAt->center);
    lookAt->center = pos;
    auto up = lookAt->up;
    auto translate = vsg::translate(pos);
    Trackball::setViewpoint(lookAt);

    _pointer->matrix = vsg::rotate(vsg::dquat(vsg::dvec3(0.0, 0.0, -1.0), vsg::normalize(pos)));
    _pointer->matrix[3][0] = translate[3][0];
    _pointer->matrix[3][1] = translate[3][1];
    _pointer->matrix[3][2] = translate[3][2];

    emit sendPos(_ellipsoidModel->convertECEFToLatLongAltitude(pos));
}

void Manipulator::moveToObject(const QModelIndex &index)
{
    if(!index.isValid())
        return;

    auto object = static_cast<vsg::Node*>(index.internalPointer());
    Q_ASSERT(object);

    if(auto sceneobject = object->cast<route::SceneObject>(); sceneobject)
    {
        setViewpoint(sceneobject->getWorldPosition());
        return;
    }
    ParentTracer pt;
    object->accept(pt);
    auto ltw = vsg::computeTransform(pt.nodePath);

    vsg::ComputeBounds computeBounds;
    object->accept(computeBounds);
    vsg::dvec3 centre = (computeBounds.bounds.min + computeBounds.bounds.max) * 0.5;
    setViewpoint(ltw * centre);
}

void Manipulator::setFirst(vsg::ref_ptr<route::SceneObject> firstObject)
{
    _movingObject = firstObject;
}
void Manipulator::startMoving()
{
    if(!_isMoving)
    {
        _isMoving = true;
        _database->undoStack->beginMacro(tr("Перемещены объекты"));
    }
}

void Manipulator::apply(vsg::MoveEvent &pointerEvent)
{
    Trackball::apply(pointerEvent);

    _previousPointerEvent = &pointerEvent;

    if(!_isMoving || !_movingObject)
        return;


        auto isections = intersections(pointerEvent);
        if(isections.empty())
            return;
        auto isection = isections.front();

        vsg::dvec3 delta;

        delta = isection->worldIntersection - _movingObject->getWorldPosition();

        emit sendMovingDelta(delta);
        /*
    case MovingAxis::X:
    {
        auto delta = (pointerEvent.y - _previousPointerEvent->y) / 4;
        auto quat = _movingObject->getWorldRotation();

        auto rotated = vsg::inverse(vsg::rotate(quat)) * vsg::dvec3(delta, 0.0, 0.0);

        emit sendMovingDelta(rotated);
    }
    case MovingAxis::Y:
    {
        auto delta = (pointerEvent.y - _previousPointerEvent->y) / 4;
        auto quat = _movingObject->getWorldRotation();

        auto rotated = route::mult(quat, vsg::dvec3(0.0, delta, 0.0));

        emit sendMovingDelta(rotated);
    }
    case MovingAxis::Z:
    {
        auto delta = (pointerEvent.y - _previousPointerEvent->y) / 4;
        auto quat = _movingObject->getWorldRotation();

        auto rotated = route::mult(quat, vsg::dvec3(0.0, 0.0, delta));

        emit sendMovingDelta(rotated);
    }

    }
    */

}

FindNode Manipulator::intersectedObjects(vsg::LineSegmentIntersector::Intersections isections)
{
    if(isections.empty())
        return FindNode();
    FindNode fn(isections.front());
    fn.keyModifier = _keyModifier;
    return fn;
}

FindNode Manipulator::intersectedObjects(const vsg::PointerEvent &pointerEvent)
{
    return intersectedObjects(intersections(pointerEvent));
}

vsg::LineSegmentIntersector::Intersections Manipulator::intersections(const vsg::PointerEvent& pointerEvent)
{
    auto intersector = vsg::LineSegmentIntersector::create(*_camera, pointerEvent.x, pointerEvent.y);
    _database->root->accept(*intersector);

    if (intersector->intersections.empty()) return vsg::LineSegmentIntersector::Intersections();

    // sort the intersectors front to back
    std::sort(intersector->intersections.begin(), intersector->intersections.end(), [](auto lhs, auto rhs) { return lhs->ratio < rhs->ratio; });

    //_lastIntersection = fn;

    return intersector->intersections;
}
