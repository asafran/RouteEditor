#include "Manipulator.h"
//#include "LambdaVisitor.h"
#include <QSettings>

Manipulator::Manipulator(vsg::ref_ptr<vsg::Camera> camera,
                         vsg::ref_ptr<vsg::EllipsoidModel> ellipsoidModel,
                         vsg::ref_ptr<vsg::Builder> in_builder,
                         vsg::ref_ptr<vsg::Group> in_scenegraph,
                         QUndoStack *stack,
                         SceneModel *model,
                         QObject *parent) :
    QObject(parent),
    vsg::Inherit<vsg::Trackball, Manipulator>(camera, ellipsoidModel),
    builder(in_builder),
    scenegraph(in_scenegraph),
    pointer(vsg::MatrixTransform::create(vsg::translate(_lookAt->center))),
    tilesModel(model)
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

    if (buttonPress.mask & vsg::BUTTON_MASK_1){
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
            if(auto tile = lowTile(isection); tile)
                emit addRequest(isection.worldIntersection, tilesModel->index(tile));
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

vsg::ref_ptr<vsg::Node> Manipulator::lowTile(const vsg::LineSegmentIntersector::Intersection &intersection)
{
    auto find = std::find_if(intersection.nodePath.crbegin(), intersection.nodePath.crend(), isCompatible<vsg::PagedLOD>);
    if(find != intersection.nodePath.crend())
    {
        auto plod = (*find)->cast<vsg::PagedLOD>();
        if(plod->highResActive(database->frameCount))
        {
            if(plod->children.front().node.cast<vsg::Group>()->children.front()->is_compatible(typeid (vsg::MatrixTransform)))
                return plod->children.front().node;
        }
    }
    return vsg::ref_ptr<vsg::Node>();
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

}

void Manipulator::addPointer()
{
    pointer->children.erase(pointer->children.begin(), pointer->children.end());

    QSettings settings(ORGANIZATION_NAME, APPLICATION_NAME);

    vsg::GeometryInfo info;

    info.dx.set(10.0f, 0.0f, 0.0f);
    info.dy.set(0.0f, 10.0f, 0.0f);
    info.dz.set(0.0f, 0.0f, 10.0f);
    info.position = {0.0f, 0.0f, -5.0f};
    pointer->addChild(builder->createCone(info));
}

void Manipulator::addAction(bool checked)
{
    mode = checked ? Manipulator::ADD : Manipulator::SELECT;
}

void Manipulator::selectObject(const QModelIndex &index)
{
    if(auto object = static_cast<vsg::Node*>(index.internalPointer()); object)
    {
        if(auto sceneobject = object->cast<SceneObject>(); sceneobject)
        {
            setViewpoint(sceneobject->matrix[3]);
            return;
        }
        vsg::ComputeBounds computeBounds;
        object->accept(computeBounds);
        vsg::dvec3 centre = (computeBounds.bounds.min + computeBounds.bounds.max) * 0.5;
        setViewpoint(centre);
    }
}

void Manipulator::apply(vsg::PointerEvent& pointerEvent)
{
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
    /*
    auto vertarray = lastIntersection.front().arrays.front().cast<vsg::vec3Array>();
    for (auto it = vertarray->begin(); it != vertarray->end(); ++it)
    {
        std::cout << it->x << " " << it->y << " " << it->z << std::endl;
    }*/
    return intersector->intersections.front();
}
