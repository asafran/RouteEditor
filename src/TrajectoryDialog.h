#ifndef TRAJECTORYDIALOG_H
#define TRAJECTORYDIALOG_H

#include <QDialog>

namespace Ui {
class TrajectoryDialog;
}

class TrajectoryDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TrajectoryDialog(QWidget *parent = nullptr);
    ~TrajectoryDialog();

private:
    Ui::TrajectoryDialog *ui;
};

#endif // TRAJECTORYDIALOG_H
