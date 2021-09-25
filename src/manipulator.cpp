#include "manipulator.h"

Manipulator::Manipulator(vsg::ref_ptr<vsg::Camera> camera, vsg::ref_ptr<vsg::EllipsoidModel> ellipsoidModel, vsg::ref_ptr<vsg::Builder> in_builder, vsg::ref_ptr<vsg::Group> in_scenegraph, double in_scale, vsg::ref_ptr<vsg::Options> in_options, QObject *parent) :
    QObject(parent),
    vsg::Inherit<vsg::Trackball, Manipulator>(camera, ellipsoidModel),
    builder(in_builder),
    options(in_options),
    scenegraph(in_scenegraph)
{
    rotateButtonMask = vsg::BUTTON_MASK_2;
    supportsThrow = false;
}

void Manipulator::apply(vsg::KeyPressEvent& keyPress)
{
    /*
    if (_previousPointerEvent)
    {
        interesection(*_previousPointerEvent);
        if (!lastIntersection.front()) return;

        vsg::GeometryInfo info;
        info.position = vsg::vec3(lastIntersection.front().localIntersection);

        vsg::Intersector::NodePath path = lastIntersection.front().nodePath;

        info.dx.set(1.0f, 0.0f, 0.0f);
        info.dy.set(0.0f, 1.0f, 0.0f);
        info.dz.set(0.0f, 0.0f, 1.0f);

        for (auto it = path.crbegin(); it != path.crend(); ++it)
        {
            if (auto plod = (*it)->cast<vsg::PagedLOD>(); plod)
            {
            }
        }

        if (keyPress.keyBase == 'b')
        {
            lastIntersection.front() ->addChild(builder->createBox(info));
        }
        else if (keyPress.keyBase == 'c')
        {
            scenegraph->addChild(builder->createCylinder(info));
        }
        else if (keyPress.keyBase == 's')
        {
            scenegraph->addChild(builder->createSphere(info));
        }
        else if (keyPress.keyBase == 'n')
        {
            scenegraph->addChild(builder->createCone(info));
        }
    }
    */
}

void Manipulator::apply(vsg::ButtonPressEvent& buttonPress)
{
    if (buttonPress.handled) return;

    _hasFocus = withinRenderArea(buttonPress.x, buttonPress.y);
    _lastPointerEventWithinRenderArea = _hasFocus;

    if (buttonPress.mask & vsg::BUTTON_MASK_1){
        _updateMode = INACTIVE;
        auto intersection = interesection(buttonPress);

    } else if (buttonPress.mask & vsg::BUTTON_MASK_2)
        _updateMode = ROTATE;
    else if (buttonPress.mask & vsg::BUTTON_MASK_3 && _ellipsoidModel){
        _updateMode = INACTIVE;
        auto isection = interesection(buttonPress);
        auto lookAt = vsg::LookAt::create();
        lookAt->eye = isection.worldIntersection + vsg::dvec3(0.0, 0.0, 2000.0);
        lookAt->center = isection.worldIntersection;
        lookAt->up = normalize(cross(lookAt->center, vsg::dvec3(-lookAt->center.y, lookAt->center.x, lookAt->center.z)));

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
