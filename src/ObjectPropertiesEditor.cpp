#include "ObjectPropertiesEditor.h"
#include "ui_ObjectPropertiesEditor.h"
#include "undo-redo.h"
#include <vsg/utils/ComputeBounds.h>
#include "sceneobjectvisitor.h"
#include "tools.h"
#include <QSignalBlocker>

ObjectPropertiesEditor::ObjectPropertiesEditor(DatabaseManager *database, QWidget *parent) : Tool(database, parent)
    , _ellipsoidModel(database->route->ellipsoidModel)
    , ui(new Ui::ObjectPropertiesEditor)
{
    ui->setupUi(this);

    auto stack = _database->undoStack;

    //ui->stationBox->setModel(new StationsModel(_database->topology));

    connect(stack, &QUndoStack::indexChanged, this, &ObjectPropertiesEditor::updateData);

    connect(ui->ecefXspin, &QDoubleSpinBox::valueChanged, this, &ObjectPropertiesEditor::updatePositionECEF);
    connect(ui->ecefYspin, &QDoubleSpinBox::valueChanged, this, &ObjectPropertiesEditor::updatePositionECEF);
    connect(ui->ecefZspin, &QDoubleSpinBox::valueChanged, this, &ObjectPropertiesEditor::updatePositionECEF);

    connect(ui->latSpin, &QDoubleSpinBox::valueChanged, this, &ObjectPropertiesEditor::updatePositionLLA);
    connect(ui->lonSpin, &QDoubleSpinBox::valueChanged, this, &ObjectPropertiesEditor::updatePositionLLA);
    connect(ui->altSpin, &QDoubleSpinBox::valueChanged, this, &ObjectPropertiesEditor::updatePositionLLA);

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
    {/*
        vsg::MatrixTransform *mt = nullptr;
        if(_firstObject->getValue(app::PARENT, mt))
            stack->push(new MoveObjectOnTraj(mt, d));*/
    });

    connect(ui->nameEdit, &QLineEdit::textEdited, this, [stack, this](const QString &text)
    {
        stack->push(new RenameObject(*_selectedObjects.begin(), text));
    });

    connect(ui->stationBox, &QComboBox::currentIndexChanged, this, [this](int idx)
    {
        /*
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
        */
    });
}

ObjectPropertiesEditor::~ObjectPropertiesEditor()
{
    delete ui;
}

void ObjectPropertiesEditor::applyTransform(const vsg::dvec3 &delta)
{
    if(_selectedObjects.size() > 1)
        _database->undoStack->push(new MoveObjects(_selectedObjects, delta));
    else
    {
        auto object = static_cast<route::MVCObject*>(_selectedObjects.begin()->internalPointer());
        auto pos = object->getPosition() + delta;
        _database->undoStack->push(new MoveObject(object, pos));
    }
}

void ObjectPropertiesEditor::selectIndex(const QItemSelection &selected, const QItemSelection &deselected)
{
    for (const auto &index : deselected.indexes())
    {
        if(auto it = _selectedObjects.find(index); it != _selectedObjects.end())
        {
            auto object = static_cast<route::MVCObject*>(index.internalPointer());
            object->setSelection(false);
            _selectedObjects.erase(it);
        }
    }

    for (const auto &index : selected.indexes())
    {
        if(_selectedObjects.find(index) != _selectedObjects.end())
            continue;
        auto object = static_cast<route::MVCObject*>(index.internalPointer());

        object->setSelection(true);
        _selectedObjects.insert(index);
        emit objectClicked(index);
    }
    updateData();
}

void ObjectPropertiesEditor::selectObject(route::SceneObject *object)
{
    clear();
    toggle(object);
    updateData();
}

void ObjectPropertiesEditor::updatePositionECEF(double)
{
    auto object = static_cast<route::MVCObject*>(_selectedObjects.begin()->internalPointer());
    auto x = ui->ecefXspin->value();
    auto y = ui->ecefYspin->value();
    auto z = ui->ecefZspin->value();

    vsg::dvec3 pos{x,y,z};

    if(_selectedObjects.size() > 1)
    {
        pos -= object->getPosition();
        _database->undoStack->push(new MoveObjects(_selectedObjects, pos));
    }
    else
        _database->undoStack->push(new MoveObject(object, pos));
}

void ObjectPropertiesEditor::updatePositionLLA(double)
{
    auto object = static_cast<route::MVCObject*>(_selectedObjects.begin()->internalPointer());
    auto lat = ui->latSpin->value();
    auto lon = ui->lonSpin->value();
    auto alt = ui->altSpin->value();
    auto ltw = object->getWorldTransform();
    auto ecef = _ellipsoidModel->convertLatLongAltitudeToECEF({lat, lon, alt});

    if(_selectedObjects.size() > 1)
    {
        ecef -= vsg::dvec3{ltw(3,0), ltw(3,1), ltw(3,2)};
        _database->undoStack->push(new MoveObjects(_selectedObjects, ecef));
    }
    else
    {
        auto pos = vsg::inverse(ltw) * ecef;
        _database->undoStack->push(new MoveObject(object, pos));
    }
}
/*
void ObjectPropertiesEditor::updateRotationX(double val)
{
    auto object = static_cast<route::MVCObject*>(_selectedObjects.begin()->internalPointer());
    auto x = qDegreesToRadians(val);
    auto delta = x - object->getEulerRotation().x;
    _database->undoStack->push(new RotateObjects(_selectedObjects, delta, {1.0, 0.0, 0.0}));
}

void ObjectPropertiesEditor::updateRotationY(double val)
{
    auto object = static_cast<route::MVCObject*>(_selectedObjects.begin()->internalPointer());
    auto y = qDegreesToRadians(val);
    auto delta = y - object->getEulerRotation().y;
    _database->undoStack->push(new RotateObjects(_selectedObjects, delta, {0.0, 1.0, 0.0}));
}

void ObjectPropertiesEditor::updateRotationZ(double val)
{
    auto object = static_cast<route::MVCObject*>(_selectedObjects.begin()->internalPointer());
    auto z = qDegreesToRadians(val);
    auto delta = z - object->getEulerRotation().z;
    _database->undoStack->push(new RotateObjects(_selectedObjects, delta, {0.0, 0.0, 1.0}));
}
*/
void ObjectPropertiesEditor::updateRotation(double)
{
    auto x = qDegreesToRadians(ui->rotXspin->value());
    auto y = qDegreesToRadians(ui->rotYspin->value());
    auto z = qDegreesToRadians(ui->rotZspin->value());
    _database->undoStack->push(new RotateObjects(_selectedObjects, route::toQuaternion(x, y, z)));
}

void ObjectPropertiesEditor::toggle(route::SceneObject *object)
{
    auto index = _database->tilesModel->index(object);
    if(auto selectedIt = _selectedObjects.find(index); selectedIt != _selectedObjects.end())
    {
        auto selected = static_cast<route::MVCObject*>(index.internalPointer());
        selected->setSelection(false);

        //_selectedObjects.erase(selectedIt);
        emit deselectItem(*selectedIt);
    }
    else
    {
        object->setSelection(true);
        _selectedObjects.insert(index);
        emit objectClicked(index);
    }
}
void ObjectPropertiesEditor::clear()
{
    for (auto &index : qAsConst(_selectedObjects)) {
        auto object = static_cast<route::MVCObject*>(index.internalPointer());
        object->setSelection(false);
    }
    _selectedObjects.clear();
    emit deselect();
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

    if(_selectedObjects.empty())
    {
        setEnabled(false);
        return;
    }
    else
        setEnabled(true);
    setSpinEanbled(true);

    auto object = static_cast<route::MVCObject*>(_selectedObjects.begin()->internalPointer());
/*
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
    route::Tile *tile = nullptr;
    if(_firstObject->getValue(app::PARENT, rmt))
    {
        double val = 0.0;
        rmt->getValue(app::PROP, val);
        ui->trjCoordspin->setValue(val);
        ui->trjCoordspin->setEnabled(true);
    }
    else
        ui->trjCoordspin->setEnabled(false);

    if(_firstObject->getValue(app::PARENT, tile); tile)
    {
        ui->latSpin->setEnabled(true);
        ui->lonSpin->setEnabled(true);
        ui->altSpin->setEnabled(true);
    }
    else
    {
        ui->latSpin->setEnabled(false);
        ui->lonSpin->setEnabled(false);
        ui->altSpin->setEnabled(false);
    }


    ui->nameEdit->clear();
    std::string name;
    if(_firstObject->getValue(app::NAME, name))
        ui->nameEdit->setText(name.c_str());
*/
    auto position = object->getPosition();
    ui->ecefXspin->setValue(position.x);
    ui->ecefYspin->setValue(position.y);
    ui->ecefZspin->setValue(position.z);

    auto lla = _ellipsoidModel->convertECEFToLatLongAltitude(object->getPosition());

    ui->latSpin->setValue(lla.x);
    ui->lonSpin->setValue(lla.y);
    ui->altSpin->setValue(lla.z);

    auto quat = object->getRotation();

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


void ObjectPropertiesEditor::apply(vsg::KeyPressEvent &press)
{
    if(press.keyModifier & vsg::MODKEY_Control)
        _single = true;
    if(press.keyModifier & vsg::MODKEY_Shift)
        _shift = true;

    switch (press.keyBase) {
    case vsg::KEY_M:
    {
        break;
    }
    default:
        break;

    }
}

void ObjectPropertiesEditor::apply(vsg::KeyReleaseEvent &release)
{
    if(release.keyModifier & vsg::MODKEY_Control)
        _single = false;
    if(release.keyModifier & vsg::MODKEY_Shift)
        _shift = false;
}

void ObjectPropertiesEditor::apply(vsg::ButtonPressEvent &press)
{
    auto isection = route::testIntersections(press, _database->root, _camera);

     if(_single)
         clear();

     if(isection.empty())
         return;

     auto toogleObject = [this](route::SceneObject *object)
     {
         toggle(object);
         return true;
     };

     route::SceneObjLambdaCast<route::SceneObject> lv(toogleObject);
     auto& nodePath = isection.front()->nodePath;
     if(_shift)
         vsg::visit(lv, nodePath.rbegin(), nodePath.rend());
     else
         vsg::visit(lv, nodePath.begin(), nodePath.end());

     updateData();
}

void ObjectPropertiesEditor::apply(vsg::ButtonReleaseEvent &)
{
}

void ObjectPropertiesEditor::apply(vsg::MoveEvent &)
{
}
