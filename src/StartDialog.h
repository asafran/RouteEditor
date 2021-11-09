#ifndef STARTDIALOG_H
#define STARTDIALOG_H

#include <QDialog>
#include <QItemSelectionModel>

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

    QString routePath;
    QString skyboxPath;

    enum Colors
    {
        Standart,
        Contrast,
        BlackWhite
    };
    Colors color;

    double cursorSize;

private:
    Ui::StartDialog *ui;
};

#endif // STARTDIALOG_H
