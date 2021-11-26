#include "Manipulator.h"
#include "LambdaVisitor.h"
#include <QSettings>
#include "undo-redo.h"

Manipulator::Manipulator(vsg::ref_ptr<vsg::Camera> camera,
                         vsg::ref_ptr<vsg::EllipsoidModel> ellipsoidModel,
                         vsg::ref_ptr<vsg::Builder> in_builder,
                         vsg::ref_ptr<vsg::Group> in_scenegraph,
                         vsg::ref_ptr<vsg::CopyAndReleaseBuffer> copyBuffer,
                         QUndoStack *stack,
                         SceneModel *model,
                         QObject *parent) :
    QObject(parent),
    vsg::Inherit<vsg::Trackball, Manipulator>(camera, ellipsoidModel),
    copyBufferCmd(copyBuffer),
    builder(in_builder),
    scenegraph(in_scenegraph),
    pointer(vsg::MatrixTransform::create(vsg::translate(_lookAt->center))),
    terrainPoints(vsg::Group::create()),
    active(),
    tilesModel(model),
    undoStack(stack)
{
    rotateButtonMask = vsg::BUTTON_MASK_2;
    supportsThrow = false;
    scenegraph->addChild(pointer);
    addPointer();
}
Manipulator::~Manipulator()
{
    disconnect();
}

template<class T>
bool isCompatible(const vsg::Node* node)
{
    return node->is_compatible(typeid (T));
}

void Manipulator::apply(vsg::ButtonPressEvent& buttonPress)
{
    if (buttonPress.handled) return;

    _hasFocus = withinRenderArea(buttonPress.x, buttonPress.y);
    _lastPointerEventWithinRenderArea = _hasFocus;

    if (buttonPress.mask & vsg::BUTTON_MASK_1)
    {
        _updateMode = INACTIVE;
        auto isection = interesection(buttonPress);
        switch (mode) {
        case SELECT:
        {
            auto find = std::find_if(isection.nodePath.crbegin(), isection.nodePath.crend(), isCompatible<SceneObject>);
            if(find != isection.nodePath.crend())
                emit objectClicked(tilesModel->index(*find));
            break;
        }
        case ADD:
        {
            if(auto traj = std::find_if(isection.nodePath.begin(), isection.nodePath.end(), isCompatible<Trajectory>); traj != isection.nodePath.end())
            {
                emit addRequest(isection.worldIntersection, tilesModel->index(*traj));
            }
            else if(auto tile = lowTile(isection); tile)
                emit addRequest(isection.worldIntersection, tilesModel->index(tile->children.at(4)));
            //check children!!!
            break;
        }
        case TERRAIN:
        {
            if(isection.nodePath.empty())
            {
                if(isMovingTerrain)
                   movingPoint->children.pop_back();
                isMovingTerrain = false;
                break;
            }
            if(points.isEmpty())
            {
                if(auto vid = isection.nodePath.back()->cast<vsg::VertexIndexDraw>(); vid && lowTile(isection))
                {
                    auto vertarray = vid->arrays.front()->data.cast<vsg::vec3Array>();
                    for (auto it = vertarray->begin(); it != vertarray->end(); ++it)
                    {
                        auto point = addTerrainPoint(*it);
                        points.insert(point, it);
                        terrainPoints->addChild(point);   
                    }
                    active = const_cast<vsg::MatrixTransform *>((*(isection.nodePath.crbegin() + 4))->cast<vsg::MatrixTransform>());
                    active->addChild(terrainPoints);
                    info = isection.nodePath.back()->cast<vsg::VertexIndexDraw>()->arrays.front();
                }
            }
            else if(points.contains(*(isection.nodePath.rbegin() + 2)) && !isMovingTerrain)
            {
                isMovingTerrain = true;
                movingPoint = const_cast<vsg::MatrixTransform *>((*(isection.nodePath.rbegin() + 2))->cast<vsg::MatrixTransform>());

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
                auto find = std::find_if(isection.nodePath.cbegin(), isection.nodePath.cend(), isCompatible<SceneObject>);
                if(find != isection.nodePath.cend())
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
            if(auto traj = std::find_if(isection.nodePath.begin(), isection.nodePath.end(), isCompatible<Trajectory>); traj != isection.nodePath.end())
                emit addTrackRequest(isection.worldIntersection, const_cast<Trajectory*>((*traj)->cast<Trajectory>()));
            else if(auto tile = lowTile(isection); tile)
                emit addRequest(isection.worldIntersection, tilesModel->index(tile->children.at(5)));
            break;
        }
        }


    } else if (buttonPress.mask & vsg::BUTTON_MASK_2)
        _updateMode = ROTATE;
    else if (buttonPress.mask & vsg::BUTTON_MASK_3 && _ellipsoidModel){
        _updateMode = INACTIVE;

        auto isection = interesection(buttonPress);
        if(isection.nodePath.empty() || isection.nodePath.back() == pointer)
            return;
        setViewpoint(isection.worldIntersection);

        if(auto tile = lowTile(isection); tile)
        {
            //emit objectClicked(cachedTilesModel->index(tile.get()), QItemSelectionModel::Select);
            emit expand(tilesModel->index(tile));
        }

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
            _lookAt->eye = matrix * _lookAt->eye;

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

vsg::ref_ptr<vsg::Group> Manipulator::lowTile(const vsg::LineSegmentIntersector::Intersection &intersection)
{
    /*
    auto find = std::find_if(intersection.nodePath.crbegin(), intersection.nodePath.crend(), isCompatible<vsg::PagedLOD>);
    if(find != intersection.nodePath.crend())
    {*/
    if(intersection.nodePath.size() > 6)
    {
        auto plod = (*(intersection.nodePath.crbegin() + 6))->cast<vsg::PagedLOD>();
        if(plod)
            if(plod->highResActive(database->frameCount))
                if(plod->children.front().node.cast<vsg::Group>()->children.front()->is_compatible(typeid (vsg::MatrixTransform)))
                    return plod->children.front().node.cast<vsg::Group>();
    }
    return vsg::ref_ptr<vsg::Group>();
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
    pointer->matrix = vsg::rotate(vsg::dquat(vsg::dvec3(0.0, 0.0, -1.0), vsg::normalize(pos)));
    pointer->matrix[3][0] = translate[3][0];
    pointer->matrix[3][1] = translate[3][1];
    pointer->matrix[3][2] = translate[3][2];
    emit sendPos(_ellipsoidModel->convertECEFToLatLongAltitude(pos));

}

void Manipulator::addPointer()
{
    pointer->children.erase(pointer->children.begin(), pointer->children.end());

    QSettings settings(ORGANIZATION_NAME, APPLICATION_NAME);

    auto size = static_cast<float>(settings.value("CURSORSIZE", 1).toInt());
    vsg::GeometryInfo info;

    info.dx.set(1.0f * size, 0.0f, 0.0f);
    info.dy.set(0.0f, 1.0f * size, 0.0f);
    info.dz.set(0.0f, 0.0f, 1.0f * size);
    info.position = {0.0f, 0.0f, -1.0f * size / 2};
    pointer->addChild(builder->createCone(info));
}

vsg::ref_ptr<vsg::MatrixTransform> Manipulator::addTerrainPoint(vsg::vec3 pos)
{
/*
    QSettings settings(ORGANIZATION_NAME, APPLICATION_NAME);
    auto size = static_cast<float>(settings.value("CURSORSIZE", 1).toInt());
*/
    vsg::GeometryInfo info;

    info.dx.set(3.0f, 0.0f, 0.0f);
    info.dy.set(0.0f, 3.0f, 0.0f);
    info.dz.set(0.0f, 0.0f, 3.0f);

    auto transform = vsg::MatrixTransform::create(vsg::translate(vsg::dvec3(pos)));
    transform->addChild(builder->createSphere(info));
    return transform;
}

void Manipulator::setMode(int index)
{
    if(index == mode)
        return;
    if(mode == TERRAIN)
    {
        if(isMovingTerrain)
            movingPoint->children.pop_back();
        isMovingTerrain = false;
        if(active)
            active->children.erase(std::find(active->children.begin(), active->children.end(), terrainPoints));
        terrainPoints->children.clear();
        points.clear();
    } else if(mode == MOVE)
    {
        isMovingObject = false;
    }
    mode = index;
}

void Manipulator::selectObject(const QModelIndex &index)
{
    if(auto object = static_cast<vsg::Node*>(index.internalPointer()); object)
    {
        if(mode == SELECT)
        {
            if(auto sceneobject = object->cast<SceneObject>(); sceneobject)
            {
                setViewpoint(sceneobject->position);
                return;
            }
            vsg::ComputeBounds computeBounds;
            object->accept(computeBounds);
            vsg::dvec3 centre = (computeBounds.bounds.min + computeBounds.bounds.max) * 0.5;
            setViewpoint(centre);
        }
        else if(mode == MOVE)
        {
            if(isMovingObject)
            {
                undoStack->endMacro();
                isMovingObject = false;
            } else {
                if(auto sceneobject = object->cast<SceneObject>(); sceneobject)
                {
                    isMovingObject = true;
                    movingObject = sceneobject;
                    undoStack->beginMacro("Перемещен объект");
                }
            }
        }
    }
}

void Manipulator::apply(vsg::MoveEvent &pointerEvent)
{
    Trackball::apply(pointerEvent);
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
        auto isection = interesection(pointerEvent);
        auto find = std::find_if(isection.nodePath.crbegin(), isection.nodePath.crend(), isCompatible<SceneObject>);
        if(find == isection.nodePath.crend())
            undoStack->push(new MoveObject(movingObject, isection.worldIntersection));

    }
    _previousPointerEvent = &pointerEvent;

}

vsg::LineSegmentIntersector::Intersection Manipulator::interesection(vsg::PointerEvent& pointerEvent)
{
    auto intersector = vsg::LineSegmentIntersector::create(*_camera, pointerEvent.x, pointerEvent.y);
    scenegraph->accept(*intersector);

    if (intersector->intersections.empty()) return vsg::LineSegmentIntersector::Intersection();

    // sort the intersectors front to back
    std::sort(intersector->intersections.begin(), intersector->intersections.end(), [](auto lhs, auto rhs) { return lhs.ratio < rhs.ratio; });

    lastIntersection = intersector->intersections;

    return intersector->intersections.front();
}
