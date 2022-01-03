#include "Manipulator.h"
#include "LambdaVisitor.h"
#include <QSettings>
#include "undo-redo.h"
#include "DatabaseManager.h"
#include "ParentVisitor.h"
#include <vsg/nodes/MatrixTransform.h>
#include <vsg/utils/Builder.h>
#include <vsg/traversals/ComputeBounds.h>
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
    manager->getRoot()->addChild(_pointer);
}
Manipulator::~Manipulator()
{
}

void Manipulator::createPointer()
{
    QSettings settings(ORGANIZATION_NAME, APPLICATION_NAME);

    auto size = static_cast<float>(settings.value("CURSORSIZE", 1).toInt());
    vsg::GeometryInfo info;

    info.dx.set(1.0f * size, 0.0f, 0.0f);
    info.dy.set(0.0f, 1.0f * size, 0.0f);
    info.dz.set(0.0f, 0.0f, 1.0f * size);
    info.position = {0.0f, 0.0f, -1.0f * size / 2};
    auto cone = _database->getBuilder()->createCone(info);
    _pointer->addChild(cone);
}
/*
void Manipulator::handlePress(vsg::ButtonPressEvent& buttonPressEvent)
{
    switch (_database->mode) {
    case DatabaseManager::SELECT:
    {
        auto isection = intersectedObjects(route::SceneObjects | route::Points | route::Tiles, buttonPressEvent);
        if(!isection.objects.empty())
        {
            auto node = isection.objects.front().first;
            auto parent = isection.objects.front().second;
            auto index = _database->tilesModel->index(node, parent);

            _selector->addWireframe(index, node, isection.localToWord * vsg::inverse(node->transform(vsg::dmat4())));
            emit objectClicked(index);
        }
        else
            emit deselect();
        break;
    }
    case DatabaseManager::ADD:
    {
        auto mask = route::Tiles | route::Tracks;
        auto isection = intersectedObjects(mask, buttonPressEvent);

        if(isection.tile.first)
        {
            FindPositionVisitor fpv(isection.tile.first);
            emit objectClicked(_database->tilesModel->index(fpv(_database->tilesModel->getRoot()), 0, QModelIndex()));
            _database->addToSelected(isection.worldIntersection);
        }

        break;
    }

    case TERRAIN:
    {
        if(points.contains(*(front.nodePath.rbegin() + 2)) && !isMovingTerrain)
        {
            isMovingTerrain = true;
            movingPoint = const_cast<vsg::MatrixTransform *>((*(front.nodePath.rbegin() + 2))->cast<vsg::MatrixTransform>());

            vsg::GeometryInfo info;
            info.dx.set(4.0f, 0.0f, 0.0f);
            info.dy.set(0.0f, 4.0f, 0.0f);
            info.dz.set(0.0f, 0.0f, 4.0f);
            info.color = {1.0f, 0.5f, 0.0f, 1.0f};

            movingPoint->addChild(builder->createSphere(info));
        } else
        {
            if(isMovingTerrain)
               movingPoint->children.pop_back();
            isMovingTerrain = false;
        }
        break;
    }
    case MOVE:
    {
        if(isMovingObject)
        {
            undoStack->endMacro();
            isMovingObject = false;
        } else {
            auto find = std::find_if(front.nodePath.cbegin(), front.nodePath.cend(), isCompatible<SceneObject>);
            if(find != front.nodePath.cend())
            {
                isMovingObject = true;
                movingObject = const_cast<SceneObject *>((*find)->cast<SceneObject>());
                undoStack->beginMacro("Перемещен объект");
            }
        }
        break;
    }
    case ADDTRACK:
    {
        if(auto traj = std::find_if(front.nodePath.begin(), front.nodePath.end(), isCompatible<SceneTrajectory>); traj != front.nodePath.end())
            emit addTrackRequest(const_cast<SceneTrajectory*>((*traj)->cast<SceneTrajectory>()), 0.0);
        else if(auto tile = lowTile(front, database->frameCount); tile)
            emit addRequest(front.worldIntersection, tilesModel->index(tile->children.at(5)));
        break;
    }
    }


}
*/

void Manipulator::setMask(uint32_t mask)
{
    _mask = mask;
}

void Manipulator::apply(vsg::ButtonPressEvent& buttonPress)
{
    if (buttonPress.handled) return;

    _hasFocus = withinRenderArea(buttonPress);
    _lastPointerEventWithinRenderArea = _hasFocus;

    if (buttonPress.mask & vsg::BUTTON_MASK_1)
    {
        _updateMode = INACTIVE;
        emit sendIntersection(intersectedObjects(_mask, buttonPress));

    } else if (buttonPress.mask & vsg::BUTTON_MASK_2)
        _updateMode = ROTATE;
    else if (buttonPress.mask & vsg::BUTTON_MASK_3 && _ellipsoidModel)
    {
        _updateMode = INACTIVE;

        auto isection = intersectedObjects(route::Tiles, buttonPress);
        if(isection.tile.first == nullptr)
            return;
        setViewpoint(isection.worldIntersection);
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

    //clampToGlobe();
}

void Manipulator::zoom(double ratio)
{
    vsg::dvec3 lookVector = _lookAt->center - _lookAt->eye;
    _lookAt->eye = _lookAt->eye + lookVector * ratio;

    //clampToGlobe();
}

void Manipulator::pan(const vsg::dvec2& delta)
{
    /*
    vsg::dvec3 lookVector = _lookAt->center - _lookAt->eye;
    vsg::dvec3 lookNormal = vsg::normalize(lookVector);
    vsg::dvec3 upNormal = _lookAt->up;
    vsg::dvec3 sideNormal = vsg::cross(lookNormal, upNormal);

    double distance = length(lookVector);
    distance *= 0.25; // TODO use Camera project matrix to guide how much to scale

    if (_ellipsoidModel)
    {
        double scale = distance;
        double angle = (length(delta) * scale) / _ellipsoidModel->radiusEquator();

        if (angle != 0.0)
        {
            vsg::dvec3 globeNormal = normalize(_lookAt->center);
            vsg::dvec3 m = upNormal * (-delta.y) + sideNormal * (delta.x); // compute the position relative to the center in the eye plane
            vsg::dvec3 v = m + lookNormal * dot(m, globeNormal);           // compensate for any tile relative to the globenormal
            vsg::dvec3 axis = normalize(cross(globeNormal, v));            // compute the axis of rotation to map the mouse pan

            vsg::dmat4 matrix = vsg::rotate(-angle, axis);

            _lookAt->up = normalize(matrix * (_lookAt->eye + _lookAt->up) - matrix * _lookAt->eye);
            _lookAt->center = matrix * _lookAt->center;
            _lookAt->eye = matrix * _lookAt->eye;void Manipulator::addWireframe(const QModelIndex &index, const vsg::Node *node, vsg::dmat4 ltw)
{
    vsg::ComputeBounds cb;
    node->accept(cb);

    vsg::dvec3 centre = ltw * ((cb.bounds.min + cb.bounds.max) * 0.5);
    //setViewpoint(centre);

    auto matrix = vsg::translate(centre) * vsg::scale(cb.bounds.max - cb.bounds.min) *
            vsg::rotate(vsg::dquat(vsg::dvec3(0.0, 0.0, -1.0), vsg::normalize(centre)));
    auto transform = vsg::MatrixTransform::create(matrix);
    transform->addChild(_compiledWireframe);
    _wireframes->children.push_back(transform);
    _selectedObjects.insert(std::make_pair(index, _wireframes->children.end()));
}

            //clampToGlobe();
        }
    }
    else
    {
        vsg::dvec3 translation = sideNormal * (-delta.x * distance) + upNormal * (delta.y * distance);

        _lookAt->eye = _lookAt->eye + translation;
        _lookAt->center = _lookAt->center + translation;
    }
    */
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

    if(auto sceneobject = object->cast<route::SceneObject>(); sceneobject && !sceneobject->local)
    {
        setViewpoint(sceneobject->getPosition());
        return;
    }
    ParentVisitor pv(object);
    pv.traversalMask = route::SceneObjects | route::Tiles;
    _database->getRoot()->accept(pv);
    pv.pathToChild.pop_back();
    auto ltw = vsg::computeTransform(pv.pathToChild);

    vsg::ComputeBounds computeBounds;
    object->accept(computeBounds);
    vsg::dvec3 centre = (computeBounds.bounds.min + computeBounds.bounds.max) * 0.5;
    setViewpoint(ltw * centre);
}


void Manipulator::apply(vsg::MoveEvent &pointerEvent)
{

    Trackball::apply(pointerEvent);/*
    if(isMovingTerrain)
    {
        auto delta = (pointerEvent.y - _previousPointerEvent->y);
        auto fmat = vsg::mat4(active->matrix);
        vsg::vec3 mvec;
        mvec.x = fmat[3].x;
        mvec.y = fmat[3].y;
        mvec.z = fmat[3].z;
        auto norm = vsg::normalize(mvec);

        vsg::quat quat(vsg::vec3(0.0f, 0.0f, 1.0f), norm);
        vsg::quat vec(0.0f, 0.0f, delta, 0.0f);
        auto rotated = mult(mult(quat, vec), vsg::quat(-quat.x, -quat.y, -quat.z, quat.w));
        auto point = points.value(movingPoint);
        point->x += rotated.x;
        point->y += rotated.y;
        point->z += rotated.z;
        movingPoint->matrix = vsg::translate(*point);

        copyBufferCmd->copy(info->data, info);
    }
    else if(isMovingObject)
    {
        auto isection = interesection(pointerEvent).front();
        auto find = std::find_if(isection.nodePath.crbegin(), isection.nodePath.crend(), isCompatible<SceneObject>);
        if(find == isection.nodePath.crend())
            undoStack->push(new MoveObject(movingObject, isection.worldIntersection));

    }
    _previousPointerEvent = &pointerEvent;
*/
}

FindNode Manipulator::intersectedObjects(vsg::LineSegmentIntersector::Intersections isections)
{
    if(isections.empty())
        return FindNode();
    FindNode fn(isections.front());
    std::for_each(fn.nodePath.begin(), fn.nodePath.end(), [&fn](const vsg::Node *node){ node->accept(fn); });
    return fn;
}

FindNode Manipulator::intersectedObjects(uint32_t mask, const vsg::PointerEvent &pointerEvent)
{
    return intersectedObjects(intersections(mask, pointerEvent));
}

vsg::LineSegmentIntersector::Intersections Manipulator::intersections(uint32_t mask, const vsg::PointerEvent& pointerEvent)
{
    auto intersector = vsg::LineSegmentIntersector::create(*_camera, pointerEvent.x, pointerEvent.y);
    intersector->traversalMask = mask;
    _database->getRoot()->accept(*intersector);

    if (intersector->intersections.empty()) return vsg::LineSegmentIntersector::Intersections();

    // sort the intersectors front to back
    std::sort(intersector->intersections.begin(), intersector->intersections.end(), [](auto lhs, auto rhs) { return lhs.ratio < rhs.ratio; });

    //_lastIntersection = fn;

    return intersector->intersections;
}
