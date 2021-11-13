#include "AddDialog.h"
#include "ui_AddDialog.h"
#include "SceneModel.h"


AddDialog::AddDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddDialog)
{
    ui->setupUi(this);
}

vsg::ref_ptr<vsg::Node> AddDialog::constructNode()
{
    switch (ui->comboBox->currentIndex()) {
    case ObjectLayer:
    {
        auto layer = vsg::Group::create();
        layer->setValue(META_NAME, ui->lineEdit->text().toStdString());
        return layer;
    }
    case ObjectGroup:
    {
        auto group = SceneObject::create();
        group->setValue(META_NAME, ui->lineEdit->text().toStdString());
        return group;
    }
    case ProcedurePositionGroup:
    {
        break;
    }
    case AutoShaderGroup:
    {
        break;
    }
    default:
        break;

    }
    return vsg::ref_ptr<vsg::Node>();
}

AddDialog::~AddDialog()
{
    delete ui;
}
