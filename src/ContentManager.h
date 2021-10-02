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
    explicit ContentManager(vsg::ref_ptr<vsg::Options> options, QUndoStack *stack, QWidget *parent = nullptr);
    ~ContentManager();

public slots:
    void addObject(const vsg::dvec3 &pos);
    void setActiveGroup(const QItemSelection &selected, const QItemSelection &deselected);


private:
    Ui::ContentManager *ui;

    vsg::ref_ptr<vsg::Options> _options;
    vsg::ref_ptr<vsg::Group> _active;
    QFileSystemModel *model;
    QUndoStack *_stack;
};

#endif // CONTENTMANAGER_H
