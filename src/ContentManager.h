#ifndef CONTENTMANAGER_H
#define CONTENTMANAGER_H

#include "tool.h"
#include <QFileSystemModel>
#include <QItemSelectionModel>
#include <vsg/app/CompileManager.h>
#include <vsg/app/Viewer.h>
#include <vsg/threading/OperationQueue.h>
#include <vsg/io/read.h>

namespace Ui {
class ContentManager;
}

template<typename F>
struct Merge : public vsg::Inherit<vsg::Operation, Merge<F>>
{
    Merge(vsg::observer_ptr<vsg::Viewer> in_viewer, vsg::ref_ptr<route::SceneObject> in_object, F merge, const vsg::CompileResult& in_compileResult):
        viewer(in_viewer),
        object(in_object),
        mergeFunction(merge),
        compileResult(in_compileResult) {}

    vsg::observer_ptr<vsg::Viewer> viewer;
    vsg::ref_ptr<route::SceneObject> object;
    vsg::CompileResult compileResult;

    F mergeFunction;

    void run() override
    {
        vsg::ref_ptr<vsg::Viewer> ref_viewer = viewer;
        if (ref_viewer)
        {
            vsg::updateViewer(*ref_viewer, compileResult);
        }

        mergeFunction(object);
    }
};

template<typename F1, typename F2>
struct LoadOperation : public vsg::Inherit<vsg::Operation, LoadOperation<F1, F2>>
{

    LoadOperation(vsg::ref_ptr<vsg::Viewer> in_viewer, F1 load, F2 merge) :
        viewer(in_viewer),
        loadFunction(load),
        mergeFunction(merge) {}

    vsg::observer_ptr<vsg::Viewer> viewer;

    F1 loadFunction;
    F2 mergeFunction;

    void run() override
    {
        vsg::ref_ptr<vsg::Viewer > ref_viewer = viewer;

        auto node = loadFunction();

        auto result = ref_viewer->compileManager->compile(node);
        if(!result)
            return;

        ref_viewer->addUpdateOperation(Merge<F2>::create(viewer, node, mergeFunction, result));
    }
};

class ContentManager : public Tool
{
    Q_OBJECT
public:
    ContentManager(DatabaseManager *database, QString root, QWidget *parent = nullptr);
    virtual ~ContentManager();

public slots:
    void activeGroupChanged(const QModelIndex &index);

signals:
    void sendObject(route::SceneObject *object);

private:

    Ui::ContentManager *ui;

    QModelIndex _activeGroup;

    QDir modelsDir;

    QFileSystemModel *_fsmodel;

    // Visitor interface
public:
    void apply(vsg::ButtonPressEvent &buttonPress) override;
};

#endif // CONTENTMANAGER_H
