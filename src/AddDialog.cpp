#include "AddDialog.h"
#include "ui_AddDialog.h"


AddDialog::AddDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddDialog)
{
    ui->setupUi(this);
}

QUndoCommand *AddDialog::constructCommand(vsg::Group *group)
{
    switch (ui->comboBox->currentIndex()) {
    case ObjectLayer:
    {
        return new AddNode(group, createGroup());
    }
    case ObjectGroup:
    {
        return new AddObject(group, createGroup());
    }
    case BinGroup:
    {
        break;
    }
    case ShaderGroup:
    {
        break;
    }
    default:
        break;

    }
    return nullptr;
}
vsg::Group* AddDialog::createGroup()
{
    auto group = vsg::Group::create();
    group->setValue(META_NAME, ui->lineEdit->text().toStdString());
    return group.release();
}

AddDialog::~AddDialog()
{
    delete ui;
}
