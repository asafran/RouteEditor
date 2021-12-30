#ifndef MOUSEHANDLER_H
#define MOUSEHANDLER_H

#include <QObject>
#include <vsg/all.h>
#include "SceneModel.h"
#include <QItemSelectionModel>
#include <QSortFilterProxyModel>

class MouseHandler : public QObject
{
    Q_OBJECT
public:
    MouseHandler(vsg::ref_ptr<vsg::Builder> in_builder,
                vsg::ref_ptr<vsg::CopyAndReleaseBuffer> copyBuffer,
                QUndoStack *stack,
                SceneModel *model,
                QObject *parent = nullptr);
    ~MouseHandler();



    //void setPager(vsg::ref_ptr<vsg::DatabasePager> pager) { database = pager; }

public slots:

    //void handleIntersection(vsg::LineSegmentIntersector::Intersections intersections);

signals:



};

#endif // MOUSEHANDLER_H
