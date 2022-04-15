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
    void addRouteCommand();
    void removeCmd();
    void assemble();
    void removeTrajs();
    void removeRoute();

private:
    Ui::InterlockDialog *ui;

    DatabaseManager *_database;

    RouteBeginModel *_beginModel;
    RouteEndModel *_endModel;
    RouteCmdModel *_cmdModel;
    RouteTrjModel *_trjModel;

    vsg::ref_ptr<signalling::Station> _station;
    vsg::ref_ptr<signalling::Routes> _begin;
    vsg::ref_ptr<signalling::Route> _route;

    TilesSorter *_sorter;
};

#endif // INTERLOCKDIALOG_H
