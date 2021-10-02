#include "manipulator.h"

Manipulator::Manipulator(vsg::ref_ptr<vsg::Camera> camera, vsg::ref_ptr<vsg::EllipsoidModel> ellipsoidModel, vsg::ref_ptr<vsg::Builder> in_builder, vsg::ref_ptr<vsg::Group> in_scenegraph, double in_scale, vsg::ref_ptr<vsg::Options> in_options, QObject *parent) :
    QObject(parent),
    vsg::Inherit<vsg::Trackball, Manipulator>(camera, ellipsoidModel),
    builder(in_builder),
    options(in_options),
    scenegraph(in_scenegraph),
    pointer(vsg::MatrixTransform::create(vsg::translate(_lookAt->center)))
{
    rotateButtonMask = vsg::BUTTON_MASK_2;
    supportsThrow = false;
    scenegraph->addChild(pointer);
    addPointer();
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
        auto intersection = interesection(buttonPress);
        switch (mode) {
        case SELECT:
        {
            auto find = std::find_if(intersection.nodePath.crbegin(), intersection.nodePath.crend(), isCompatible<SceneObject>);
            if(find == intersection.nodePath.crend())
                break;
            if(auto object = (*find)->cast<SceneObject>(); object)
                emit objectClicked(object->index, QItemSelectionModel::SelectCurrent);
            break;
        }
        case ADD:
        {
            auto find = std::find_if(intersection.nodePath.crbegin(), intersection.nodePath.crend(), isCompatible<vsg::PagedLOD>);
            if(find == intersection.nodePath.crend())
                break;
            auto plod = (*find)->cast<vsg::PagedLOD>();
            if(!plod->children.front().node || !plod->children.front().node->is_compatible(typeid (vsg::MatrixTransform)))
                break;
            emit addRequest(intersection.localIntersection);
        }
        }


    } else if (buttonPress.mask & vsg::BUTTON_MASK_2)
        _updateMode = ROTATE;
    else if (buttonPress.mask & vsg::BUTTON_MASK_3 && _ellipsoidModel){
        _updateMode = INACTIVE;
        auto isection = interesection(buttonPress);
        auto lookAt = vsg::LookAt::create(*_lookAt);
        lookAt->eye += (isection.worldIntersection - _lookAt->center);
        lookAt->center = isection.worldIntersection;
        pointer->matrix = vsg::translate(isection.worldIntersection);

        emit tileClicked();
        setViewpoint(lookAt);

    } else
        _updateMode = INACTIVE;

    if (_hasFocus) buttonPress.handled = true;

    _zoomPreviousRatio = 0.0;
    _pan.set(0.0, 0.0);
    _rotateAngle = 0.0;

    _previousPointerEvent = &buttonPress;
}

void Manipulator::addPointer()
{
    pointer->children.erase(pointer->children.begin(), pointer->children.end());
    vsg::GeometryInfo info;

    info.dx.set(1000.0f, 0.0f, 0.0f);
    info.dy.set(0.0f, 1000.0f, 0.0f);
    info.dz.set(0.0f, 0.0f, 1000.0f);
    pointer->addChild(builder->createCone(info));
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
/*
    for (auto& intersection : intersector->intersections)
    {
        if (ellipsoidModel)
        {
            std::cout.precision(10);
            auto location = ellipsoidModel->convertECEFToLatLongAltitude(intersection.worldIntersection);
        }

        if (lastIntersection)
        {
            //if (verbose) std::cout << ", distance from previous intersection = " << vsg::length(intersection.worldIntersection - lastIntersection.worldIntersection);
        }

        if (verbose)
        {
            for (auto& node : intersection.nodePath)
            {
                std::cout << ", " << node->className();
            }

            std::cout << ", Arrays[ ";
            for (auto& array : intersection.arrays)
            {
                std::cout << array << " ";
            }
            std::cout << "] [";
            for (auto& ir : intersection.indexRatios)
            {
                std::cout << "{" << ir.index << ", " << ir.ratio << "} ";
            }
            std::cout << "]";

            std::cout << std::endl;
        }
    }
*/
    lastIntersection = intersector->intersections;
    return intersector->intersections.front();
}
