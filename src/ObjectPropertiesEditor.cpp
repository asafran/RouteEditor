#include "ObjectPropertiesEditor.h"
#include "ui_ObjectPropertiesEditor.h"
#include "undo-redo.h"
#include <vsg/traversals/ComputeBounds.h>
#include "ParentVisitor.h"
#include <QSignalBlocker>

ObjectPropertiesEditor::ObjectPropertiesEditor(DatabaseManager *database, QWidget *parent) : Tool(database, parent)
    , _ellipsoidModel(database->getDatabase()->getObject<vsg::EllipsoidModel>("EllipsoidModel"))
    , ui(new Ui::ObjectPropertiesEditor)
{
    ui->setupUi(this);

    setEnabled(false);

    auto stack = _database->getUndoStack();

    connect(stack, &QUndoStack::indexChanged, this, &ObjectPropertiesEditor::updateData);

    connect(ui->ecefXspin, &QDoubleSpinBox::valueChanged, this, [stack, this](double d)
    {
        auto newpos = _firstObject->getPosition();
        auto delta = vsg::dvec3(d - newpos.x, 0.0, 0.0);

        if(_selectedObjects.size() != 1)
        {
            stack->beginMacro(tr("Перемещены объекты по x"));
            move(delta);
            stack->endMacro();
        }
        else
            move(delta);
    });
    connect(ui->ecefYspin, &QDoubleSpinBox::valueChanged, this, [stack, this](double d)
    {
        auto newpos = _firstObject->getPosition();
        auto delta = vsg::dvec3(0.0, d - newpos.y, 0.0);

        if(_selectedObjects.size() != 1)
        {
            stack->beginMacro(tr("Перемещены объекты по y"));
            move(delta);
            stack->endMacro();
        }
        else
            move(delta);
    });
    connect(ui->ecefZspin, &QDoubleSpinBox::valueChanged, this, [stack, this](double d)
    {
        auto newpos = _firstObject->getPosition();
        auto delta = vsg::dvec3(0.0, 0.0, d - newpos.z);

        if(_selectedObjects.size() != 1)
        {
            stack->beginMacro(tr("Перемещены объекты по z"));
            move(delta);
            stack->endMacro();
        }
        else
            move(delta);
    });

    connect(ui->latSpin, &QDoubleSpinBox::valueChanged, this, [stack, this](double d)
    {
        auto lla = _ellipsoidModel->convertECEFToLatLongAltitude(_firstObject->getPosition());
        lla.x = d;
        auto ecef = _ellipsoidModel->convertLatLongAltitudeToECEF(lla);
        auto delta = ecef - _firstObject->getPosition();

        if(_selectedObjects.size() != 1)
        {
            stack->beginMacro(tr("Перемещены объекты по широте"));
            move(delta);
            stack->endMacro();
        }
        else
            move(delta);
    });
    connect(ui->lonSpin, &QDoubleSpinBox::valueChanged, this, [stack, this](double d)
    {
        auto lla = _ellipsoidModel->convertECEFToLatLongAltitude(_firstObject->getPosition());
        lla.y = d;
        auto ecef = _ellipsoidModel->convertLatLongAltitudeToECEF(lla);
        auto delta = ecef - _firstObject->getPosition();

        if(_selectedObjects.size() != 1)
        {
            stack->beginMacro(tr("Перемещены объекты по долготе"));
            move(delta);
            stack->endMacro();
        }
        else
            move(delta);
    });
    connect(ui->altSpin, &QDoubleSpinBox::valueChanged, this, [stack, this](double d)
    {
        auto lla = _ellipsoidModel->convertECEFToLatLongAltitude(_firstObject->getPosition());
        lla.z = d;
        auto ecef = _ellipsoidModel->convertLatLongAltitudeToECEF(lla);
        auto delta = ecef - _firstObject->getPosition();

        if(_selectedObjects.size() != 1)
        {
            stack->beginMacro(tr("Перемещены объекты по высоте"));
            move(delta);
            stack->endMacro();
        }
        else
            move(delta);
    });

    connect(ui->rotXspin, &QDoubleSpinBox::valueChanged, this, [stack, this](double d)
    {
        auto rad = qDegreesToRadians(d);
        _database->push(new RotateObject(_firstObject, vsg::dquat(rad - _xrot, vsg::dvec3(1.0, 0.0, 0.0))));
        _xrot = rad;
        if(_selectedObjects.size() != 1)
            emit sendStatusText(tr("Вращение нескольких объектов не поддерживается"), 2000);
    });
    connect(ui->rotYspin, &QDoubleSpinBox::valueChanged, this, [stack, this](double d)
    {
        auto rad = qDegreesToRadians(d);
        _database->push(new RotateObject(_firstObject, vsg::dquat(rad - _yrot, vsg::dvec3(0.0, 1.0, 0.0))));
        _yrot = rad;
        if(_selectedObjects.size() != 1)
            emit sendStatusText(tr("Вращение нескольких объектов не поддерживается"), 2000);
    });
    connect(ui->rotZspin, &QDoubleSpinBox::valueChanged, this, [stack, this](double d)
    {
        auto rad = qDegreesToRadians(d);
        _database->push(new RotateObject(_firstObject, vsg::dquat(rad - _zrot, vsg::dvec3(0.0, 0.0, 1.0))));
        _zrot = rad;
        if(_selectedObjects.size() != 1)
            emit sendStatusText(tr("Вращение нескольких объектов не поддерживается"), 2000);
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

void ObjectPropertiesEditor::move(const vsg::dvec3 &delta)
{
    for(auto &index : _selectedObjects)
    {
        auto object = index.second;
        _database->push(new MoveObject(object, object->getPosition() + delta));
    }
}

void ObjectPropertiesEditor::selectObject(const QItemSelection &selected, const QItemSelection &deselected)
{
    for (const auto &index : deselected.indexes())
        if(!selected.contains(index) && (_selectedObjects.find(index) != _selectedObjects.end()))
        {
            auto deselectObject = _selectedObjects.at(index);
            deselectObject->deselect();
            _selectedObjects.erase(index);
            if(deselectObject == _firstObject)
                _firstObject = _selectedObjects.empty() ? nullptr : _selectedObjects.begin()->second;
            emit sendFirst(_firstObject);
        }

    for (const auto &index : selected.indexes())
    {
        if(_selectedObjects.find(index) != _selectedObjects.end())
            continue;
        auto object = static_cast<vsg::Node*>(index.internalPointer());
        Q_ASSERT(object);

        if(auto sceneobject = object->cast<route::SceneObject>(); sceneobject)
        {
            select(index, sceneobject);
        }
    }
    updateData();
}

void ObjectPropertiesEditor::intersection(const FindNode& isection)
{
    _single = (isection.keyModifier & vsg::MODKEY_Control) == 0;

    if(_single)
        clear();

    if(isection.objects.empty())
        return;

    if((isection.keyModifier & vsg::MODKEY_Shift) == 0)
        toggle(isection.objects.front());
    else
        toggle(isection.objects.back());

    updateData();
}
void ObjectPropertiesEditor::toggle(std::pair<const route::SceneObject *, const vsg::Node *> object)
{
    auto casted = const_cast<route::SceneObject*>(object.first);
    auto index = _database->getTilesModel()->index(object.first, object.second);
    if(auto selectedIt = _selectedObjects.find(index); selectedIt != _selectedObjects.end())
    {
        auto selected = selectedIt->second;
        selected->deselect();

        //_selectedObjects.erase(selectedIt);
        emit deselectItem(selectedIt->first);


        if(selected == _firstObject)
        {
            _firstObject = _selectedObjects.empty() ? nullptr : _selectedObjects.begin()->second;
            emit sendFirst(_firstObject);
        }
        return;
    }
    select(index, casted);
    emit objectClicked(index);
}
void ObjectPropertiesEditor::clear()
{
    _firstObject = nullptr;
    emit sendFirst(_firstObject);
    for (auto &object : _selectedObjects) {
        object.second->deselect();
    }
    _selectedObjects.clear();
    emit deselect();
}
void ObjectPropertiesEditor::select(const QModelIndex &index, route::SceneObject* object)
{
    if(!_firstObject)
    {
        _firstObject = object;
        emit sendFirst(_firstObject);
    }
    object->setWireframe(_database->getBuilder());
    _selectedObjects.insert({index, object});
}

void ObjectPropertiesEditor::updateData()
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

    setEnabled(_firstObject);
    if(!_firstObject)
        return;

    auto position = _firstObject->getPosition();
    ui->ecefXspin->setValue(position.x);
    ui->ecefYspin->setValue(position.y);
    ui->ecefZspin->setValue(position.z);

    ui->latSpin->setValue(_ellipsoidModel->convertECEFToLatLongAltitude(position).x);
    ui->lonSpin->setValue(_ellipsoidModel->convertECEFToLatLongAltitude(position).y);
    ui->altSpin->setValue(_ellipsoidModel->convertECEFToLatLongAltitude(position).z);

    auto quat = _firstObject->getRotation();

    double sinr_cosp = 2 * (quat.w * quat.x + quat.y * quat.z);
    double cosr_cosp = 1 - 2 * (quat.x * quat.x + quat.y * quat.y);
    _xrot = std::atan2(sinr_cosp, cosr_cosp);
    ui->rotXspin->setValue(qRadiansToDegrees(_xrot));

    double sinp = 2 * (quat.w * quat.y - quat.z * quat.x);
    if (std::abs(sinp) >= 1)
        _yrot = std::copysign(vsg::PI / 2, sinp); // use 90 degrees if out of range
    else
        _yrot = std::asin(sinp);
    ui->rotYspin->setValue(qRadiansToDegrees(_yrot));

    double siny_cosp = 2 * (quat.w * quat.z + quat.x * quat.y);
    double cosy_cosp = 1 - 2 * (quat.y * quat.y + quat.z * quat.z);
    _zrot = std::atan2(siny_cosp, cosy_cosp);
    ui->rotZspin->setValue(qRadiansToDegrees(_zrot));
}
