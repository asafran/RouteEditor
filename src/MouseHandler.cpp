#include "MouseHandler.h"
#include "LambdaVisitor.h"
#include <QSettings>
#include "undo-redo.h"
#include <QInputDialog>

MouseHandler::MouseHandler(vsg::ref_ptr<vsg::Builder> in_builder,
                         vsg::ref_ptr<vsg::CopyAndReleaseBuffer> copyBuffer,
                         QUndoStack *stack,
                         SceneModel *model,
                         QObject *parent) :
    QObject(parent),
    copyBufferCmd(copyBuffer),
    builder(in_builder),
    terrainPoints(vsg::Group::create()),
    active(),
    tilesModel(model),
    undoStack(stack)
{
}
MouseHandler::~MouseHandler()
{
    disconnect();
}

template<class T>
bool isCompatible(const vsg::Node* node)
{
    return node->is_compatible(typeid (T));
}

void MouseHandler::handleIntersection(vsg::LineSegmentIntersector::Intersections intersections)
{

    if(intersections.empty())
        switch (mode) {
        case TERRAIN:
        {
            if(isMovingTerrain)
               movingPoint->children.pop_back();
            isMovingTerrain = false;
            return;
        }
        case MOVE:
        {
            if(isMovingObject)
            {
                undoStack->endMacro();
                isMovingObject = false;
            }
            return;
        }
        default:
            return;
        }

    auto front = intersections.front();

    switch (mode) {
    case SELECT:
    {
        auto find = std::find_if(front.nodePath.crbegin(), front.nodePath.crend(), isCompatible<SceneObject>);
        if(find != front.nodePath.crend())
            emit objectClicked(tilesModel->index(*find));
        break;
    }
    case ADD:
    {
        if(auto tile = lowTile(front, database->frameCount); tile)
            emit addRequest(front.worldIntersection, tilesModel->index(tile->children.at(4)));
        //check children!!!
        break;
    }
    case TERRAIN:
    {
        if(points.isEmpty())
        {
            if(auto vid = front.nodePath.back()->cast<vsg::VertexIndexDraw>(); vid && lowTile(front, database->frameCount))
            {
                auto vertarray = vid->arrays.front()->data.cast<vsg::vec3Array>();
                for (auto it = vertarray->begin(); it != vertarray->end(); ++it)
                {
                    auto point = addTerrainPoint(*it);
                    points.insert(point, it);
                    terrainPoints->addChild(point);
                }
                active = const_cast<vsg::MatrixTransform *>((*(front.nodePath.crbegin() + 4))->cast<vsg::MatrixTransform>());
                active->addChild(terrainPoints);
                info = front.nodePath.back()->cast<vsg::VertexIndexDraw>()->arrays.front();
            }
        }
        else if(points.contains(*(front.nodePath.rbegin() + 2)) && !isMovingTerrain)
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

vsg::ref_ptr<vsg::MatrixTransform> MouseHandler::addTerrainPoint(vsg::vec3 pos)
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

void MouseHandler::setMode(int index)
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
        undoStack->endMacro();
        isMovingObject = false;
    }
    mode = index;
}


