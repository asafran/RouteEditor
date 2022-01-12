#ifndef MANIPULATOR_H
#define MANIPULATOR_H

#include <vsg/viewer/Trackball.h>
#include <QObject>
#include <vsg/nodes/Switch.h>
#include "SceneObjectVisitor.h"
#include <optional>

class DatabaseManager;

class Manipulator : public QObject, public vsg::Inherit<vsg::Trackball, Manipulator>
{
    Q_OBJECT
public:
    Manipulator(vsg::ref_ptr<vsg::Camera> camera,
                vsg::ref_ptr<vsg::EllipsoidModel> ellipsoidModel,
                DatabaseManager *manager,
                QObject *parent = nullptr);
    ~Manipulator();

    void apply(vsg::KeyPressEvent& keyPress) override;
    void apply(vsg::KeyReleaseEvent& keyPress) override;
    void apply(vsg::ButtonPressEvent& buttonPressEvent) override;
    void apply(vsg::MoveEvent& pointerEvent) override;

    void rotate(double angle, const vsg::dvec3& axis) override;
    void zoom(double ratio) override;
    void pan(const vsg::dvec2& delta) override;

    FindNode intersectedObjects(uint32_t mask, const vsg::PointerEvent &pointerEvent);

    FindNode intersectedObjects(vsg::LineSegmentIntersector::Intersections isections);

    vsg::LineSegmentIntersector::Intersections intersections(uint32_t mask, const vsg::PointerEvent& pointerEvent);

public slots:
    void moveToObject(const QModelIndex &index);
    void setFirst(vsg::ref_ptr<route::SceneObject> firstObject);
    void startMoving();
    void setViewpoint(const vsg::dvec3 &pos);
    void setLatLongAlt(const vsg::dvec3 &pos);
    void setViewpoint(const vsg::dvec4 &pos_mat);
    void setMask(uint32_t mask);

signals:
    void sendPos(const vsg::dvec3 &pos);
    void sendMovingDelta(const vsg::dvec3 &delta);
    void sendIntersection(const FindNode& isection);
    //void objectClicked(const QModelIndex &index);
    void sendStatusText(const QString &message, int timeout);

protected:
    inline void createPointer();

    //void handlePress(vsg::ButtonPressEvent& buttonPressEvent);

    enum MovingAxis
    {
        X,
        Y,
        Z,
        TERRAIN
    };

    MovingAxis _axis = TERRAIN;

    DatabaseManager *_database;

    vsg::ref_ptr<vsg::MatrixTransform> _pointer;

    bool _isMoving;
    vsg::ref_ptr<route::SceneObject> _movingObject;

    uint32_t _mask = 0xFFFFFF;

    uint16_t _keyModifier = 0x0;

    vsg::LineSegmentIntersector::Intersection _lastIntersection;
};
/*
template<class T>
bool isCompatible(const vsg::Node* node)
{
    return node->is_compatible(typeid (T));
}
*/


#endif // MANIPULATOR_H
