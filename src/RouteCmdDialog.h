#ifndef ROUTECMDDIALOG_H
#define ROUTECMDDIALOG_H

#include <QDialog>
#include "DatabaseManager.h"
#include "stmodels.h"

namespace Ui {
class RouteCmdDialog;
}

class RouteCmdDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RouteCmdDialog(DatabaseManager *db, QWidget *parent = nullptr);
    ~RouteCmdDialog();

    vsg::ref_ptr<route::Station> station;

    vsg::ref_ptr<route::Routes> begin;

    vsg::ref_ptr<route::Route> route;

private:
    Ui::RouteCmdDialog *ui;
};

#endif // ROUTECMDDIALOG_H
