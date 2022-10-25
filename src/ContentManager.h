#ifndef CONTENTMANAGER_H
#define CONTENTMANAGER_H

#include "tool.h"
#include <QFileSystemModel>
#include <QItemSelectionModel>

namespace Ui {
class ContentManager;
}

class ContentManager : public Tool
{
    Q_OBJECT
public:
    ContentManager(DatabaseManager *database, QString root, QWidget *parent = nullptr);
    virtual ~ContentManager();

    void intersection(const FoundNodes& isection) override;

public slots:
    void activeGroupChanged(const QModelIndex &index);

signals:
    void sendObject(route::SceneObject *object);

private:
    bool addToTrack(vsg::ref_ptr<route::SceneObject> obj, const FoundNodes &isection);
    bool addSignal(vsg::ref_ptr<route::SceneObject> obj, const FoundNodes& isection);

    Ui::ContentManager *ui;

    QModelIndex _activeGroup;

    QDir modelsDir;

    QFileSystemModel *_fsmodel;
};

#endif // CONTENTMANAGER_H
