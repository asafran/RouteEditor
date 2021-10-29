#ifndef CONTENTMANAGER_H
#define CONTENTMANAGER_H

#include <QWidget>
#include <QFileSystemModel>
#include <QItemSelectionModel>
#include "undo-redo.h"

namespace Ui {
class ContentManager;
}

class ContentManager : public QWidget
{
    Q_OBJECT

public:
    ContentManager(vsg::ref_ptr<vsg::Builder> builder, vsg::ref_ptr<vsg::Options> options, SceneModel *model, QWidget *parent = nullptr);
    ~ContentManager();

public slots:
    void addObject(const vsg::LineSegmentIntersector::Intersection &isection, const QModelIndex &group);
    void setActiveGroup(const QItemSelection &selected, const QItemSelection &);

signals:
    void compile();

private:
    Ui::ContentManager *ui;

    vsg::ref_ptr<vsg::Options> _options;
    vsg::ref_ptr<vsg::Builder> _builder;
    QModelIndex _active;
    vsg::ref_ptr<vsg::Viewer> _viewer;
    QFileSystemModel *_model;
    SceneModel *_tilesModel;
};

#endif // CONTENTMANAGER_H
