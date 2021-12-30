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




    void apply(vsg::ButtonPressEvent& buttonPressEvent) override;
    void apply(vsg::MoveEvent& pointerEvent) override;

    void rotate(double angle, const vsg::dvec3& axis) override;
    void zoom(double ratio) override;
    void pan(const vsg::dvec2& delta) override;

    FindNode intersectedObjects(uint32_t mask, const vsg::PointerEvent &pointerEvent);

    FindNode intersectedObjects(vsg::LineSegmentIntersector::Intersections isections);

    vsg::LineSegmentIntersector::Intersections interesection(uint32_t mask, const vsg::PointerEvent& pointerEvent);

    enum Mode
    {
        SELECT,
        MOVE,
        ADD,
        ADDTRACK
    };

public slots:
    void selectObject(const QModelIndex &index);
    void moveToObject(const QModelIndex &index);
    void setViewpoint(const vsg::dvec3 &pos);
    void setLatLongAlt(const vsg::dvec3 &pos);
    void setViewpoint(const vsg::dvec4 &pos_mat);
    void setMode(int index);

signals:
    void expand(const QModelIndex &index);
    void sendData(vsg::ref_ptr<vsg::Data> buffer, vsg::ref_ptr<vsg::BufferInfo> info);
    void sendPos(const vsg::dvec3 &pos);

    void addRequest(const vsg::dvec3& position, const QModelIndex &index);
    void objectClicked(const QModelIndex &index);
    void objectSelected(vsg::ref_ptr<route::SceneObject> object);
    void deselect();
    void sendStatusText(const QString &message, int timeout);

protected:

    inline void addPointer();
    inline void addWireframe();
    inline void moveWireframe(const vsg::Node *node, vsg::dmat4 ltw);

    void handlePress(vsg::ButtonPressEvent& buttonPressEvent);

    vsg::ref_ptr<vsg::MatrixTransform> _pointer;
    vsg::ref_ptr<vsg::MatrixTransform> _wireframe;

    //----------------------Move-----------------------

    DatabaseManager *_database;

    route::SceneObject *_movingObject;

    vsg::dmat4 _oldMatrix;

    int _mode = ADD;

    //FindNode _lastIntersection;
};
/*
template<class T>
bool isCompatible(const vsg::Node* node)
{
    return node->is_compatible(typeid (T));
}
*/


#endif // MANIPULATOR_H
