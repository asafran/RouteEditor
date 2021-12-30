#include "ObjectPropertiesEditor.h"
#include "ui_ObjectPropertiesEditor.h"
#include "undo-redo.h"
#include <QSignalBlocker>

ObjectPropertiesEditor::ObjectPropertiesEditor(vsg::ref_ptr<vsg::EllipsoidModel> model, QUndoStack *stack, QWidget *parent) :
    QWidget(parent),
    ellipsoidModel(model),
    undoStack(stack),
    ui(new Ui::ObjectPropertiesEditor)
{
    ui->setupUi(this);

    connect(ui->ecefXspin, &QDoubleSpinBox::valueChanged, this, [this](double d)
    {
        auto newpos = selectedObject->getPosition();
        newpos.x = d;
        undoStack->push(new MoveObject(selectedObject, newpos));
    });
    connect(ui->ecefYspin, &QDoubleSpinBox::valueChanged, this, [this](double d)
    {
        auto newpos = selectedObject->getPosition();
        newpos.y = d;
        undoStack->push(new MoveObject(selectedObject, newpos));
    });
    connect(ui->ecefZspin, &QDoubleSpinBox::valueChanged, this, [this](double d)
    {
        auto newpos = selectedObject->getPosition();
        newpos.z = d;
        undoStack->push(new MoveObject(selectedObject, newpos));
    });

    connect(ui->latSpin, &QDoubleSpinBox::valueChanged, this, [this](double d)
    {
        auto lla = ellipsoidModel->convertECEFToLatLongAltitude(selectedObject->getPosition());
        lla.x = d;
        undoStack->push(new MoveObject(selectedObject, ellipsoidModel->convertLatLongAltitudeToECEF(lla)));
    });
    connect(ui->lonSpin, &QDoubleSpinBox::valueChanged, this, [this](double d)
    {
        auto lla = ellipsoidModel->convertECEFToLatLongAltitude(selectedObject->getPosition());
        lla.y = d;
        undoStack->push(new MoveObject(selectedObject, ellipsoidModel->convertLatLongAltitudeToECEF(lla)));
    });
    connect(ui->altSpin, &QDoubleSpinBox::valueChanged, this, [this](double d)
    {
        auto lla = ellipsoidModel->convertECEFToLatLongAltitude(selectedObject->getPosition());
        lla.z = d;
        undoStack->push(new MoveObject(selectedObject, ellipsoidModel->convertLatLongAltitudeToECEF(lla)));
    });

    connect(ui->rotXspin, &QDoubleSpinBox::valueChanged, this, [this](double d)
    {
        auto rad = qDegreesToRadians(d);
        undoStack->push(new RotateObject(selectedObject, vsg::dquat(rad - xrot, vsg::dvec3(1.0, 0.0, 0.0))));
        xrot = rad;
    });
    connect(ui->rotYspin, &QDoubleSpinBox::valueChanged, this, [this](double d)
    {
        auto rad = qDegreesToRadians(d);
        undoStack->push(new RotateObject(selectedObject, vsg::dquat(rad - yrot, vsg::dvec3(0.0, 1.0, 0.0))));
        yrot = rad;
    });
    connect(ui->rotZspin, &QDoubleSpinBox::valueChanged, this, [this](double d)
    {
        auto rad = qDegreesToRadians(d);
        undoStack->push(new RotateObject(selectedObject, vsg::dquat(rad - zrot, vsg::dvec3(0.0, 0.0, 1.0))));
        zrot = rad;
    });
    connect(ui->llaCombo, &QComboBox::currentIndexChanged, this, [this](int index)
    {
        auto step = 0.0001 * std::pow(100, index);
        ui->latSpin->setSingleStep(step);
        ui->lonSpin->setSingleStep(step);
    });
    connect(ui->ecefCombo, &QComboBox::currentIndexChanged, this, [this](int index)
    {
        auto step = 0.01 * std::pow(10, index);
        ui->ecefXspin->setSingleStep(step);
        ui->ecefYspin->setSingleStep(step);
        ui->ecefZspin->setSingleStep(step);
    });
    connect(ui->rotCombo, &QComboBox::currentIndexChanged, this, [this](int index)
    {
        auto step = 0.0001 * std::pow(100, index);
        ui->rotXspin->setSingleStep(step);
        ui->rotYspin->setSingleStep(step);
        ui->rotZspin->setSingleStep(step);
    });
}

ObjectPropertiesEditor::~ObjectPropertiesEditor()
{
    delete ui;
}
/*
void ObjectPropertiesEditor::selectObject(const QModelIndex &modelindex)
{
    QSignalBlocker l1(ui->ecefXspin);
    QSignalBlocker l2(ui->ecefYspin);
    QSignalBlocker l3(ui->ecefZspin);
    QSignalBlocker l4(ui->latSpin);
    QSignalBlocker l5(ui->lonSpin);
    QSignalBlocker l6(ui->altSpin);
    QSignalBlocker l7(ui->rotXspin);
    QSignalBlocker l8(ui->rotYspin);
    QSignalBlocker l9(ui->rotZspin);

    if(auto object = static_cast<vsg::Node*>(modelindex.internalPointer())->cast<route::SceneObject>(); object)
        selectedObject = object;
    else
        selectedObject = nullptr;
    updateData();
}*/

void ObjectPropertiesEditor::receiveObject(vsg::ref_ptr<route::SceneObject> object)
{
    QSignalBlocker l1(ui->ecefXspin);
    QSignalBlocker l2(ui->ecefYspin);
    QSignalBlocker l3(ui->ecefZspin);
    QSignalBlocker l4(ui->latSpin);
    QSignalBlocker l5(ui->lonSpin);
    QSignalBlocker l6(ui->altSpin);
    QSignalBlocker l7(ui->rotXspin);
    QSignalBlocker l8(ui->rotYspin);
    QSignalBlocker l9(ui->rotZspin);

    selectedObject = object;
    updateData();
}

void ObjectPropertiesEditor::updateData()
{
    setEnabled(selectedObject);
    if(!selectedObject)
        return;

    auto position = selectedObject->getPosition();
    ui->ecefXspin->setValue(position.x);
    ui->ecefYspin->setValue(position.y);
    ui->ecefZspin->setValue(position.z);

    ui->latSpin->setValue(ellipsoidModel->convertECEFToLatLongAltitude(position).x);
    ui->lonSpin->setValue(ellipsoidModel->convertECEFToLatLongAltitude(position).y);
    ui->altSpin->setValue(ellipsoidModel->convertECEFToLatLongAltitude(position).z);

    double sinr_cosp = 2 * (selectedObject->quat.w * selectedObject->quat.x + selectedObject->quat.y * selectedObject->quat.z);
    double cosr_cosp = 1 - 2 * (selectedObject->quat.x * selectedObject->quat.x + selectedObject->quat.y * selectedObject->quat.y);
    xrot = std::atan2(sinr_cosp, cosr_cosp);
    ui->rotXspin->setValue(qRadiansToDegrees(xrot));

    double sinp = 2 * (selectedObject->quat.w * selectedObject->quat.y - selectedObject->quat.z * selectedObject->quat.x);
    if (std::abs(sinp) >= 1)
        yrot = std::copysign(vsg::PI / 2, sinp); // use 90 degrees if out of range
    else
        yrot = std::asin(sinp);
    ui->rotYspin->setValue(qRadiansToDegrees(yrot));

    double siny_cosp = 2 * (selectedObject->quat.w * selectedObject->quat.z + selectedObject->quat.x * selectedObject->quat.y);
    double cosy_cosp = 1 - 2 * (selectedObject->quat.y * selectedObject->quat.y + selectedObject->quat.z * selectedObject->quat.z);
    zrot = std::atan2(siny_cosp, cosy_cosp);
    ui->rotZspin->setValue(qRadiansToDegrees(zrot));
}
