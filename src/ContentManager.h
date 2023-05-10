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
