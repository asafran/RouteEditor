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

    void intersection(const FindNode& isection) override;

public slots:
    void activeGroupChanged(const QModelIndex &index);

private:
    QModelIndex findGroup(const FindNode& isection);

    bool addToTrack(vsg::ref_ptr<vsg::Node> node, const FindNode &isection);

    Ui::ContentManager *ui;

    QModelIndex _activeGroup;

    QString _loadedPath;
    vsg::ref_ptr<vsg::Node> _loaded;

    QDir modelsDir;

    QFileSystemModel *_fsmodel;
};

#endif // CONTENTMANAGER_H
