#ifndef RPROPERTIESEDITOR_H
#define RPROPERTIESEDITOR_H

#include "tool.h"
#include <QItemSelectionModel>
#include <vsg/viewer/EllipsoidModel.h>

namespace Ui {
class RailsPointEditor;
}

class RailsPointEditor : public Tool
{
    Q_OBJECT
public:
    explicit RailsPointEditor(DatabaseManager *database, QWidget *parent = nullptr);
    virtual ~RailsPointEditor();

    //void addWireframe(const QModelIndex &index, const vsg::Node *node, vsg::dmat4 ltw);

    void intersection(const FoundNodes& isection) override;

public slots:
    void updateData();
    void clearSelection();
    void setActive();

private:
    void clear();
    void toggle(route::RailPoint *object);
    void setSpinEanbled(bool enabled);

    Ui::RailsPointEditor *ui;

    QSet<route::RailPoint*> _selectedObjects;
};

#endif //
