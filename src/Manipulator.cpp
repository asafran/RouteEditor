#include "Manipulator.h"
#include <QSettings>
#include "DatabaseManager.h"
#include "tools.h"
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
    QSettings settings(app::ORGANIZATION_NAME, app::APP_NAME);

    auto size = static_cast<float>(settings.value("CURSORSIZE", 1).toInt());
    vsg::GeometryInfo info;

    info.dx.set(1.0f * size, 0.0f, 0.0f);
    info.dy.set(0.0f, 1.0f * size, 0.0f);
    info.dz.set(0.0f, 0.0f, 1.0f * size);
    info.position = {0.0f, 0.0f, -1.0f * size / 2};
    auto cone = _database->builder->createCone(info);
    _pointer->addChild(cone);
}
/*
void Manipulator::apply(vsg::KeyPressEvent& keyPress)
{
     //_keyModifier = keyPress.keyModifier;

     mswitch (keyPress.keyBase) {
     case vsg::KEY_M:
     {
         //startMoving();
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
*/
void Manipulator::apply(vsg::ButtonPressEvent& buttonPress)
{
    if (buttonPress.handled) return;

    _hasFocus = withinRenderArea(buttonPress);
    _lastPointerEventWithinRenderArea = _hasFocus;

    if (buttonPress.mask & vsg::BUTTON_MASK_2)
        _updateMode = ROTATE;
    else if (buttonPress.mask & vsg::BUTTON_MASK_3)
    {
        _updateMode = INACTIVE;

        auto isections = route::testIntersections(buttonPress, _database->root, _camera);
        if(isections.empty())
            return;

        setViewpoint(isections.front()->worldIntersection);
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

    auto object = static_cast<route::MVCObject*>(index.internalPointer());
    auto pos = object->getWorldTransform()[3];
    setViewpoint(pos);
}
/*
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
*/
/*
void Manipulator::apply(vsg::MoveEvent &pointerEvent)
{
    if(!_isMoving || !_movingObject)
    {
        Trackball::apply(pointerEvent);
        _previousPointerEvent = &pointerEvent;
        return;
    }

    auto isections = route::testIntersections(pointerEvent, _database->root, _camera);
    if(isections.empty())
        return;
    auto isectionPrev = isections.front();

    isections = route::testIntersections(pointerEvent, _database->root, _camera);
    if(isections.empty())
        return;
    auto isection = isections.front();

    vsg::dvec3 delta;

    delta = isection->worldIntersection - isectionPrev->worldIntersection;

    emit sendMovingDelta(delta);

    _previousPointerEvent = &pointerEvent;

}
*/
