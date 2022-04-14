#include "ObjectPropertiesEditor.h"
#include "ui_ObjectPropertiesEditor.h"
#include "undo-redo.h"
#include <vsg/traversals/ComputeBounds.h>
#include "ParentVisitor.h"
#include "tools.h"
#include <QSignalBlocker>

ObjectPropertiesEditor::ObjectPropertiesEditor(DatabaseManager *database, QWidget *parent) : Tool(database, parent)
    , _ellipsoidModel(database->getDatabase()->getObject<vsg::EllipsoidModel>("EllipsoidModel"))
    , ui(new Ui::ObjectPropertiesEditor)
{
    ui->setupUi(this);

    auto stack = _database->undoStack;

    ui->stationBox->setModel(new StationsModel(_database->topology));

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
        auto lla = _ellipsoidModel->convertECEFToLatLongAltitude(_firstObject->getWorldPosition());
        lla.x = d;
        auto wtl = vsg::inverse(_firstObject->localToWorld);
        auto ecef = wtl * _ellipsoidModel->convertLatLongAltitudeToECEF(lla);
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
        auto lla = _ellipsoidModel->convertECEFToLatLongAltitude(_firstObject->getWorldPosition());
        lla.y = d;
        auto wtl = vsg::inverse(_firstObject->localToWorld);
        auto ecef = wtl * _ellipsoidModel->convertLatLongAltitudeToECEF(lla);
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
        auto lla = _ellipsoidModel->convertECEFToLatLongAltitude(_firstObject->getWorldPosition());
        lla.z = d;
        auto wtl = vsg::inverse(_firstObject->localToWorld);
        auto ecef = wtl * _ellipsoidModel->convertLatLongAltitudeToECEF(lla);
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

    connect(ui->rotXspin, &QDoubleSpinBox::valueChanged, this, &ObjectPropertiesEditor::updateRotation);
    connect(ui->rotYspin, &QDoubleSpinBox::valueChanged, this, &ObjectPropertiesEditor::updateRotation);
    connect(ui->rotZspin, &QDoubleSpinBox::valueChanged, this, &ObjectPropertiesEditor::updateRotation);

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
        ui->altSpin->setSingleStep(step);
    });
    connect(ui->rotCombo, &QComboBox::currentIndexChanged, this, [this](int index)
    {
        auto step = 0.0001 * std::pow(100, index);
        ui->rotXspin->setSingleStep(step);
        ui->rotYspin->setSingleStep(step);
        ui->rotZspin->setSingleStep(step);
    });

    connect(ui->trjCoordspin, &QDoubleSpinBox::valueChanged, this, [stack, this](double d)
    {
        vsg::MatrixTransform *mt = nullptr;
        if(_firstObject->getValue(app::PARENT, mt))
            stack->push(new MoveObjectOnTraj(mt, d));
    });

    connect(ui->nameEdit, &QLineEdit::textEdited, this, [stack, this](const QString &text)
    {
        stack->push(new RenameObject(_firstObject.get(), text));
    });

    connect(ui->stationBox, &QComboBox::currentIndexChanged, this, [this](int idx)
    {
        auto sig = _firstObject.cast<signalling::Signal>();
        Q_ASSERT(sig);
        if(_idx != _database->topology->stations.end())
            _idx->second->rsignals.erase(sig);
        if(idx == -1)
        {
            sig->station.clear();
            return;
        }
        _idx = std::next(_database->topology->stations.begin(), idx);
        _idx->second->rsignals.insert({sig, signalling::Routes::create()});
        sig->station = _idx->first;

    });
}

ObjectPropertiesEditor::~ObjectPropertiesEditor()
{
    delete ui;
}

void ObjectPropertiesEditor::updateRotation(double)
{
    auto x = qDegreesToRadians(ui->rotXspin->value());
    auto y = qDegreesToRadians(ui->rotYspin->value());
    auto z = qDegreesToRadians(ui->rotZspin->value());
    _database->undoStack->push(new RotateObject(_firstObject, route::toQuaternion(x, y, z)));
}

void ObjectPropertiesEditor::move(const vsg::dvec3 &delta)
{
    for(auto &index : _selectedObjects)
    {
        auto object = index.second;
        _database->undoStack->push(new MoveObject(object, object->getPosition() + delta));
    }
}

void ObjectPropertiesEditor::selectIndex(const QItemSelection &selected, const QItemSelection &deselected)
{
    for (const auto &index : deselected.indexes())
        if(!selected.contains(index) && (_selectedObjects.find(index) != _selectedObjects.end()))
        {
            auto deselectObject = _selectedObjects.at(index);
            deselectObject->setSelection(false);
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

void ObjectPropertiesEditor::intersection(const FoundNodes &isection)
{
    _single = (isection.keyModifier & vsg::MODKEY_Control) == 0;

    if(_single)
        clear();

    if(!isection.objects.empty())
    {
        if((isection.keyModifier & vsg::MODKEY_Shift) == 0)
            toggle(isection.objects.back());
        else
            toggle(isection.objects.front());
    }

    updateData();
}

void ObjectPropertiesEditor::selectObject(route::SceneObject *object)
{
    clear();
    toggle(object);
    updateData();
}

void ObjectPropertiesEditor::toggle(route::SceneObject *object)
{
    auto index = _database->tilesModel->index(object);
    if(auto selectedIt = _selectedObjects.find(index); selectedIt != _selectedObjects.end())
    {
        auto selected = selectedIt->second;
        selected->setSelection(false);

        //_selectedObjects.erase(selectedIt);
        emit deselectItem(selectedIt->first);


        if(selected == _firstObject)
        {
            _firstObject = _selectedObjects.empty() ? nullptr : _selectedObjects.begin()->second;
            emit sendFirst(_firstObject);
        }
        return;
    }
    select(index, object);
    emit objectClicked(index);
}
void ObjectPropertiesEditor::clear()
{
    _firstObject = nullptr;
    emit sendFirst(_firstObject);
    for (auto &object : _selectedObjects) {
        object.second->setSelection(false);
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
    object->setSelection(true);
    _selectedObjects.insert({index, object});
}

void ObjectPropertiesEditor::setSpinEanbled(bool enabled)
{
    ui->nameEdit->setEnabled(enabled);
    ui->ecefXspin->setEnabled(enabled);
    ui->ecefYspin->setEnabled(enabled);
    ui->ecefZspin->setEnabled(enabled);
    ui->latSpin->setEnabled(enabled);
    ui->lonSpin->setEnabled(enabled);
    ui->altSpin->setEnabled(enabled);
    ui->rotXspin->setEnabled(enabled);
    ui->rotYspin->setEnabled(enabled);
    ui->rotZspin->setEnabled(enabled);
    ui->trjCoordspin->setEnabled(enabled);
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
    QSignalBlocker l10(ui->trjCoordspin);

    setEnabled(_firstObject && !_firstObject->is_compatible(typeid (route::SplineTrajectory)));
    //setSpinEanbled(_firstObject && !_firstObject->is_compatible(typeid (route::SplineTrajectory)));
    if(!_firstObject)
        return;

    if(_firstObject->is_compatible(typeid (route::RailPoint)))
    {
        //ui->rotXspin->setEnabled(false);
        ui->rotYspin->setEnabled(false);
        ui->stationBox->setEnabled(false);
    }
    else if(_firstObject->is_compatible(typeid (signalling::Signal)))
    {
        ui->stationBox->setEnabled(true);
        _idx = _database->topology->stations.find(_firstObject.cast<signalling::Signal>()->station);
        if(_idx != _database->topology->stations.end())
            ui->stationBox->setCurrentIndex(std::distance(_database->topology->stations.begin(), _idx));
        else
            ui->stationBox->setCurrentIndex(-1);
    }
    else
        ui->stationBox->setEnabled(false);

    vsg::MatrixTransform *rmt = nullptr;
    if(_firstObject->getValue(app::PARENT, rmt))
    {
        double val = 0.0;
        rmt->getValue(app::PROP, val);
        ui->trjCoordspin->setValue(val);
        ui->trjCoordspin->setEnabled(true);
    }
    else
        ui->trjCoordspin->setEnabled(false);

    ui->nameEdit->clear();
    std::string name;
    if(_firstObject->getValue(app::NAME, name))
        ui->nameEdit->setText(name.c_str());

    auto position = _firstObject->getPosition();
    ui->ecefXspin->setValue(position.x);
    ui->ecefYspin->setValue(position.y);
    ui->ecefZspin->setValue(position.z);

    auto lla = _ellipsoidModel->convertECEFToLatLongAltitude(_firstObject->getWorldPosition());

    ui->latSpin->setValue(lla.x);
    ui->lonSpin->setValue(lla.y);
    ui->altSpin->setValue(lla.z);

    auto quat = _firstObject->getRotation();

    double sinr_cosp = 2 * (quat.w * quat.x + quat.y * quat.z);
    double cosr_cosp = 1 - 2 * (quat.x * quat.x + quat.y * quat.y);
    auto xrot = std::atan2(sinr_cosp, cosr_cosp);
    ui->rotXspin->setValue(qRadiansToDegrees(xrot));

    double sinp = 2 * (quat.w * quat.y - quat.z * quat.x);
    double yrot;
    if (std::abs(sinp) >= 1)
        yrot = std::copysign(vsg::PI / 2, sinp); // use 90 degrees if out of range
    else
        yrot = std::asin(sinp);
    ui->rotYspin->setValue(qRadiansToDegrees(yrot));

    double siny_cosp = 2 * (quat.w * quat.z + quat.x * quat.y);
    double cosy_cosp = 1 - 2 * (quat.y * quat.y + quat.z * quat.z);
    auto _zrot = std::atan2(siny_cosp, cosy_cosp);
    ui->rotZspin->setValue(qRadiansToDegrees(_zrot));
}

void ObjectPropertiesEditor::clearSelection()
{
    clear();
    updateData();
}
