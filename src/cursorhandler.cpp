#include "cursorhandler.h"

CursorHandler::CursorHandler(vsg::ref_ptr<vsg::Builder> in_builder, vsg::ref_ptr<vsg::Camera> in_camera, vsg::ref_ptr<vsg::Group> in_scenegraph, vsg::ref_ptr<vsg::EllipsoidModel> in_ellipsoidModel, double in_scale, vsg::ref_ptr<vsg::Options> in_options) :
    builder(in_builder),
    options(in_options),
    camera(in_camera),
    scenegraph(in_scenegraph),
    ellipsoidModel(in_ellipsoidModel),
    scale(in_scale)
{
    if (scale > 10.0) scale = 10.0;
}

void CursorHandler::apply(vsg::KeyPressEvent &keyPress)
{
    if (lastPointerEvent)
    {
        interesection(*lastPointerEvent);
        if (!lastIntersection) return;

        vsg::GeometryInfo info;
        info.position = vsg::vec3(lastIntersection.worldIntersection);
        info.dx.set(scale, 0.0f, 0.0f);
        info.dy.set(0.0f, scale, 0.0f);
        info.dz.set(0.0f, 0.0f, scale);

        // info.image = vsg::read_cast<vsg::Data>("textures/lz.vsgb", options);

        if (keyPress.keyBase == 'b')
        {
            scenegraph->addChild(builder->createBox(info));
        }
        else if (keyPress.keyBase == 'q')
        {
            scenegraph->addChild(builder->createQuad(info));
        }
        else if (keyPress.keyBase == 'c')
        {
            scenegraph->addChild(builder->createCylinder(info));
        }
        else if (keyPress.keyBase == 'p')
        {
            scenegraph->addChild(builder->createCapsule(info));
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
}

void CursorHandler::apply(vsg::ButtonPressEvent &buttonPressEvent)
{
    lastPointerEvent = &buttonPressEvent;

    if (buttonPressEvent.button == 1)
    {
        interesection(buttonPressEvent);
    }
}

void CursorHandler::apply(vsg::PointerEvent& pointerEvent)
{
    lastPointerEvent = &pointerEvent;
}

void CursorHandler::interesection(vsg::PointerEvent &pointerEvent)
{
    auto intersector = vsg::LineSegmentIntersector::create(*camera, pointerEvent.x, pointerEvent.y);
    scenegraph->accept(*intersector);

    if (verbose) std::cout << "interesection(" << pointerEvent.x << ", " << pointerEvent.y << ") " << intersector->intersections.size() << ")" << std::endl;

    if (intersector->intersections.empty()) return;

    // sort the intersectors front to back
    std::sort(intersector->intersections.begin(), intersector->intersections.end(), [](auto lhs, auto rhs) { return lhs.ratio < rhs.ratio; });

    for (auto& intersection : intersector->intersections)
    {
        if (verbose) std::cout << "intersection = " << intersection.worldIntersection << " ";

        if (ellipsoidModel)
        {
            std::cout.precision(10);
            auto location = ellipsoidModel->convertECEFToLatLongAltitude(intersection.worldIntersection);
            if (verbose) std::cout << " lat = " << location[0] << ", long = " << location[1] << ", height = " << location[2];
        }

        if (lastIntersection)
        {
            if (verbose) std::cout << ", distance from previous intersection = " << vsg::length(intersection.worldIntersection - lastIntersection.worldIntersection);
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

    lastIntersection = intersector->intersections.front();
}
