#ifndef STARTDIALOG_H
#define STARTDIALOG_H

#include <QDialog>
#include <QItemSelectionModel>
#include <QFileSystemModel>
#include <QtConcurrent>
#include <vsg/nodes/Node.h>
#include "DatabaseManager.h"

namespace Ui {
class StartDialog;
}

class StartDialog : public QDialog
{
    Q_OBJECT

public:
    explicit StartDialog(QWidget *parent = nullptr);
    ~StartDialog();

    void updateSettings();

    QFileSystemModel *routeModel;
    QString skyboxPath;

    QFuture<vsg::ref_ptr<DatabaseManager>> database;
    vsg::ref_ptr<vsg::Options> options;

    enum Colors
    {
        Standart,
        Contrast,
        BlackWhite
    };
    Colors color;

    double cursorSize;

public slots:
    void load();

private:
    Ui::StartDialog *ui;
};

#endif // STARTDIALOG_H
