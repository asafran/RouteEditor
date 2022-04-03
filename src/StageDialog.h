#ifndef STAGEDIALOG_H
#define STAGEDIALOG_H

#include "DatabaseManager.h"
#include "stmodels.h"
#include <QDialog>

namespace Ui {
class StageDialog;
}

class StageDialog : public QDialog
{
    Q_OBJECT

public:
    explicit StageDialog(DatabaseManager *db, QWidget *parent = nullptr);
    ~StageDialog();

private:
    Ui::StageDialog *ui;

    StationsModel *model;

    // QDialog interface
public slots:
    void accept();
};

#endif // STAGEDIALOG_H
