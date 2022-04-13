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
    explicit Painter(DatabaseManager *database, QWidget *parent = nullptr);
    ~Painter();

    void intersection(const FoundNodes& isection) override;

private:
    Ui::Painter *ui;
};

#endif // PAINTER_H
