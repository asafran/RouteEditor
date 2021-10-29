#ifndef SORTER_H
#define SORTER_H

#include <QDialog>
#include "SceneModel.h"
#include "tilessorter.h"

namespace Ui {
class Sorter;
}

class Sorter : public QDialog
{
    Q_OBJECT

public:
    explicit Sorter(SceneModel *tilesmodel, QWidget *parent = nullptr);
    ~Sorter();

signals:
    void selected(const QModelIndex &index);

private:
    Ui::Sorter *ui;

    QScopedPointer<SceneModel> model;
    QScopedPointer<TilesSorter> sorter;
};

#endif // SORTER_H
