#include "AddDialog.h"
#include "ui_AddDialog.h"
#include "SceneModel.h"


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
        auto objectsGroup = new SceneGroup(ui->lineEdit->text().toStdString());
        return new AddNode(group, objectsGroup);
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
