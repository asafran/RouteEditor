#include "ObjectPropertiesEditor.h"
#include "ui_ObjectPropertiesEditor.h"
#include "undo-redo.h"
#include <vsg/traversals/ComputeBounds.h>
#include "ParentVisitor.h"
#include <QSignalBlocker>

ObjectPropertiesEditor::ObjectPropertiesEditor(DatabaseManager *database, QWidget *parent) : Tool(database, parent)
    , _ellipsoidModel(database->getDatabase()->getObject<vsg::EllipsoidModel>("EllipsoidModel"))
    , _selectedObjects(route::Selection::create())
    , ui(new Ui::ObjectPropertiesEditor)
{
    ui->setupUi(this);

    setEnabled(false);

    connect(ui->ecefXspin, &QDoubleSpinBox::valueChanged, this, [this](double d)
    {
        auto newpos = _selectedObject->getPosition();
        newpos.x = d;
        _database->push(new MoveObject(_selectedObject, newpos));
    });
    connect(ui->ecefYspin, &QDoubleSpinBox::valueChanged, this, [this](double d)
    {
        auto newpos = _selectedObject->getPosition();
        newpos.y = d;
        _database->push(new MoveObject(_selectedObject, newpos));
    });
    connect(ui->ecefZspin, &QDoubleSpinBox::valueChanged, this, [this](double d)
    {
        auto newpos = _selectedObject->getPosition();
        newpos.z = d;
        _database->push(new MoveObject(_selectedObject, newpos));
    });

    connect(ui->latSpin, &QDoubleSpinBox::valueChanged, this, [this](double d)
    {
        auto lla = _ellipsoidModel->convertECEFToLatLongAltitude(_selectedObject->getPosition());
        lla.x = d;
        _database->push(new MoveObject(_selectedObject, _ellipsoidModel->convertLatLongAltitudeToECEF(lla)));
    });
    connect(ui->lonSpin, &QDoubleSpinBox::valueChanged, this, [this](double d)
    {
        auto lla = _ellipsoidModel->convertECEFToLatLongAltitude(_selectedObject->getPosition());
        lla.y = d;
        _database->push(new MoveObject(_selectedObject, _ellipsoidModel->convertLatLongAltitudeToECEF(lla)));
    });
    connect(ui->altSpin, &QDoubleSpinBox::valueChanged, this, [this](double d)
    {
        auto lla = _ellipsoidModel->convertECEFToLatLongAltitude(_selectedObject->getPosition());
        lla.z = d;
        _database->push(new MoveObject(_selectedObject, _ellipsoidModel->convertLatLongAltitudeToECEF(lla)));
    });

    connect(ui->rotXspin, &QDoubleSpinBox::valueChanged, this, [this](double d)
    {
        auto rad = qDegreesToRadians(d);
        _database->push(new RotateObject(_selectedObject, vsg::dquat(rad - _xrot, vsg::dvec3(1.0, 0.0, 0.0))));
        _xrot = rad;
    });
    connect(ui->rotYspin, &QDoubleSpinBox::valueChanged, this, [this](double d)
    {
        auto rad = qDegreesToRadians(d);
        _database->push(new RotateObject(_selectedObject, vsg::dquat(rad - _yrot, vsg::dvec3(0.0, 1.0, 0.0))));
        _yrot = rad;
    });
    connect(ui->rotZspin, &QDoubleSpinBox::valueChanged, this, [this](double d)
    {
        auto rad = qDegreesToRadians(d);
        _database->push(new RotateObject(_selectedObject, vsg::dquat(rad - _zrot, vsg::dvec3(0.0, 0.0, 1.0))));
        _zrot = rad;
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

void ObjectPropertiesEditor::addWireframe(const QModelIndex &index, const vsg::Node *node, vsg::dmat4 ltw)
{
    vsg::ComputeBounds cb;
    node->accept(cb);

    vsg::dvec3 centre = ltw * ((cb.bounds.min + cb.bounds.max) * 0.5);

    vsg::GeometryInfo info;
    vsg::StateInfo state;

    state.wireframe = true;
    state.lighting = false;

    auto delta = cb.bounds.max - cb.bounds.min;

    info.dx.set(delta.x, 0.0f, 0.0f);
    info.dy.set(0.0f, delta.y, 0.0f);
    info.dz.set(0.0f, 0.0f, delta.z);
    //info.position = centre;

    auto box = _database->getBuilder()->createBox(info, state);

    auto matrix = vsg::translate(centre) * vsg::rotate(vsg::dquat(vsg::dvec3(0.0, 0.0, -1.0), vsg::normalize(centre)));
    auto transform = vsg::MatrixTransform::create(matrix);
    transform->addChild(box);
    _wireframes.insert(std::make_pair(index, transform));
}
/*
void Selector::createWireframe()
{

    vsg::GeometryInfo info;
    vsg::StateInfo state;

    state.wireframe = true;
    state.lighting = false;

    info.dx.set(1.0f, 0.0f, 0.0f);
    info.dy.set(0.0f, 1.0f, 0.0f);
    info.dz.set(0.0f, 0.0f, 1.0f);
    _wireframe = _builder->createBox(info, state);

}
*/
void ObjectPropertiesEditor::selectObject(const QItemSelection &selected, const QItemSelection &deselected)
{
    for (const auto &index : deselected.indexes()) { if(!selected.contains(index)) _wireframes.erase(index); }

    for (const auto &index : selected.indexes())
    {
        if(_wireframes.find(index) != _wireframes.end())
            continue;
        auto object = static_cast<vsg::Node*>(index.internalPointer());
        Q_ASSERT(object);

        if(auto sceneobject = object->cast<route::SceneObject>(); sceneobject)
        {
            auto ltw = vsg::dmat4();
            if(sceneobject->local)
            {
                ParentVisitor pv(sceneobject);
                _database->getRoot()->accept(pv);
                pv.pathToChild.pop_back();
                ltw = vsg::computeTransform(pv.pathToChild);
            }
            merge(sceneobject);
            addWireframe(index, sceneobject, ltw);
        }
    }
    updateData();
}

void ObjectPropertiesEditor::intersection(const FindNode& isection)
{
    _single = (isection.keyModifier & vsg::MODKEY_Control) == 0;

    if(_single)
        clear();

    if((isection.keyModifier & vsg::MODKEY_Shift) == 0)
        select(isection.objects.front(), isection.localToWord);
    else
        select(isection.objects.back(), isection.localToWord);

    updateData();
}
void ObjectPropertiesEditor::select(std::pair<const route::SceneObject *, const vsg::Node *> object, const vsg::dmat4 &ltw)
{
    merge(const_cast<route::SceneObject*>(object.first));
    auto tilesModel = _database->getTilesModel();
    auto index = tilesModel->index(object.first, object.second);
    addWireframe(index, object.first, ltw * vsg::inverse(object.first->transform(vsg::dmat4())));
    emit objectClicked(index);
}
void ObjectPropertiesEditor::clear()
{
    _selectedObject = nullptr;
    _selectedObjects->selected.clear();
    _wireframes.clear();
    emit deselect();
}
void ObjectPropertiesEditor::merge(route::SceneObject *object)
{
    if(!_selectedObject)
    {
        _selectedObject = object;
        _selectedObjects->selected.push_back(_selectedObject);
    }
    else
    {
        _selectedObjects->selected.emplace_back(object);
        _selectedObject = _selectedObjects;
    }
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

    setEnabled(_selectedObject);
    if(!_selectedObject)
        return;

    auto position = _selectedObject->getPosition();
    ui->ecefXspin->setValue(position.x);
    ui->ecefYspin->setValue(position.y);
    ui->ecefZspin->setValue(position.z);

    ui->latSpin->setValue(_ellipsoidModel->convertECEFToLatLongAltitude(position).x);
    ui->lonSpin->setValue(_ellipsoidModel->convertECEFToLatLongAltitude(position).y);
    ui->altSpin->setValue(_ellipsoidModel->convertECEFToLatLongAltitude(position).z);

    auto quat = _selectedObject->getRotation();

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
