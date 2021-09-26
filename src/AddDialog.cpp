#include "AddDialog.h"
#include "ui_AddDialog.h"


AddDialog::AddDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddDialog)
{
    ui->setupUi(this);
}

vsg::Node* AddDialog::constructNode()
{
    switch (ui->comboBox->currentIndex()) {
    case ObjectGroup:
    {
        return createGroup();
    }
    case RailsGroup:
    {
        return createGroup();
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
    group->setValue("MetaName", ui->lineEdit->text().toStdString());
    std::string name;
    return group.release();
}

AddDialog::~AddDialog()
{
    delete ui;
}
