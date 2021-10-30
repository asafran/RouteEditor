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
    ContentManager(vsg::ref_ptr<vsg::Builder> builder, vsg::ref_ptr<vsg::Options> options, QWidget *parent = nullptr);
    ~ContentManager();

    std::optional<vsg::ref_ptr<vsg::Node>> getSelectedObject();
    //void setActiveGroup(const QItemSelection &selected, const QItemSelection &);

signals:
    void compile();

private:
    void loadModels(QStringList tileFiles);
    std::pair<QString, vsg::ref_ptr<vsg::Node>> read(const QString &path);

    Ui::ContentManager *ui;
    QMap<QString, vsg::ref_ptr<vsg::Node>> preloaded;

    QModelIndex active;
    QFileSystemModel *model;
};

#endif // CONTENTMANAGER_H
