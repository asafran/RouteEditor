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

public slots:
    void activeGroupChanged(const QModelIndex &index) noexcept;
    void activeFileChanged(const QItemSelection &selected, const QItemSelection &) noexcept;
    void loaderButton(bool checked) noexcept;

private:
    Ui::ContentManager *ui;
    QMap<QString, vsg::ref_ptr<vsg::Node>> _preloaded;

    QModelIndex _activeGroup;

    QString _loadedPath;
    vsg::ref_ptr<vsg::Node> _loaded;

    QFileSystemModel *_fsmodel;
};

#endif // CONTENTMANAGER_H
