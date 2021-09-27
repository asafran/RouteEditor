#ifndef ADDDIALOG_H
#define ADDDIALOG_H

#include <QDialog>
#include <vsg/nodes/MatrixTransform.h>
#include "metainf.h"
#include "undo-redo.h"

namespace Ui {
class AddDialog;
}

class AddDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddDialog(QWidget *parent = nullptr);
    ~AddDialog();

    QUndoCommand *constructCommand(vsg::Group *group);

private:

    inline vsg::Group *createGroup();
    Ui::AddDialog *ui;
};

#endif // ADDDIALOG_H
