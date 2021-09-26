#ifndef ADDDIALOG_H
#define ADDDIALOG_H

#include <QDialog>
#include <vsg/nodes/MatrixTransform.h>
#include "metainf.h"

namespace Ui {
class AddDialog;
}

class AddDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddDialog(QWidget *parent = nullptr);
    ~AddDialog();

    vsg::Node* constructNode();

private:

    inline vsg::Group *createGroup();
    Ui::AddDialog *ui;
};

#endif // ADDDIALOG_H
