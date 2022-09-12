#ifndef PAINTER_H
#define PAINTER_H

#include "tool.h"

namespace Ui {
class Painter;
}

class Painter : public Tool
{
    Q_OBJECT

public:
    explicit Painter(DatabaseManager *database, QString root, QWidget *parent = nullptr);
    ~Painter();

    void intersection(const FoundNodes& isection) override;

public slots:
    void activeTextureChanged(const QItemSelection &selected, const QItemSelection &);

private:
    Ui::Painter *ui;

    QImage _image;
    QRadialGradient _alpha;
    int _size = 128;

    QFileSystemModel *_fsmodel;
};

#endif // PAINTER_H
