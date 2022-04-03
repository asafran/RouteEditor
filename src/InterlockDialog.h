#ifndef INTERLOCKDIALOG_H
#define INTERLOCKDIALOG_H

#include <QDialog>
#include "DatabaseManager.h"
#include "TilesSorter.h"
#include "stmodels.h"

namespace Ui {
class InterlockDialog;
}

class InterlockDialog : public QDialog
{
    Q_OBJECT

public:
    explicit InterlockDialog(DatabaseManager *db, QWidget *parent = nullptr);
    ~InterlockDialog();

public slots:
    void addTrajs();
    void addJcts();
    void addSignal();
    void addRoute();

private:
    Ui::InterlockDialog *ui;

    DatabaseManager *_database;

    vsg::ref_ptr<route::Station> _station;

    vsg::ref_ptr<route::Routes> _begin;

    vsg::ref_ptr<route::Route> _route;

    TilesSorter *_sorter;
};

#endif // INTERLOCKDIALOG_H
