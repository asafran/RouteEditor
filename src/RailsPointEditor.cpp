#include "RailsPointEditor.h"
#include "ui_RailsPointEditor.h"
#include "undo-redo.h"
#include <vsg/traversals/ComputeBounds.h>
#include "ParentVisitor.h"
#include "sceneobjects.h"
#include <QSignalBlocker>

RailsPointEditor::RailsPointEditor(DatabaseManager *database, QWidget *parent) : Tool(database, parent)
    , ui(new Ui::RailsPointEditor)
{
    ui->setupUi(this);

    setSpinEanbled(false);

    auto stack = _database->undoStack;

    connect(stack, &QUndoStack::indexChanged, this, &RailsPointEditor::updateData);

    connect(ui->inclSpin, &QDoubleSpinBox::valueChanged, this, [stack, this](double d)
    {
        auto parent = new QUndoCommand(tr("Изменен уклон"));

        for(auto object : qAsConst(_selectedObjects))
        {
            vsg::ref_ptr<route::RailPoint> ref(object);
            auto fn = [ref](double val){ ref->setInclination(val); };

            auto quat = ref->_quat;
            double sinr_cosp = 2 * (quat.w * quat.x + quat.y * quat.z);
            double cosr_cosp = 1 - 2 * (quat.x * quat.x + quat.y * quat.y);
            auto rot = std::atan2(sinr_cosp, cosr_cosp) * 1000.0;

            new ExecuteLambda<decltype (fn), double>(fn, rot, d, parent);
        }

        stack->push(parent);
    });
    connect(ui->tangSpin, &QDoubleSpinBox::valueChanged, this, [stack, this](double d)
    {
        auto parent = new QUndoCommand(tr("Изменен вес производной"));

        for(auto object : qAsConst(_selectedObjects))
        {
            vsg::ref_ptr<route::RailPoint> ref(object);
            auto fn = [ref](double val){ ref->setTangent(val); };
            new ExecuteLambda<decltype (fn), double>(fn, ref->_tangent, d, parent);
        }

        stack->push(parent);
    });
    connect(ui->tiltSpin, &QDoubleSpinBox::valueChanged, this, [stack, this](double d)
    {
        auto parent = new QUndoCommand(tr("Изменен наклон"));

        for(auto object : qAsConst(_selectedObjects))
        {
            vsg::ref_ptr<route::RailPoint> ref(object);
            auto fn = [ref](double val){ ref->setTilt(val); };
            new ExecuteLambda<decltype (fn), double>(fn, ref->_tilt, d, parent);
        }

        stack->push(parent);
    });

    connect(ui->cheightSpin, &QDoubleSpinBox::valueChanged, this, [stack, this](double d)
    {
        auto parent = new QUndoCommand(tr("Изменена высота КС"));

        for(auto object : qAsConst(_selectedObjects))
        {
            vsg::ref_ptr<route::RailPoint> ref(object);
            auto fn = [ref](double val){ ref->setCHeight(val); };
            new ExecuteLambda<decltype (fn), double>(fn, ref->_cheight, d, parent);
        }

        stack->push(parent);
    });
}

RailsPointEditor::~RailsPointEditor()
{
    delete ui;
}

void RailsPointEditor::intersection(const FindNode& isection)
{
    auto single = (isection.keyModifier & vsg::MODKEY_Control) == 0;

    if(single || ui->trajRemPButt->isChecked() || ui->trajRemPButt->isChecked())
        clear();

    if(isection.connector)
    {
        toggle(isection.connector);
    }
    else if(isection.trackpoint)
    {
        if(ui->trajRemPButt->isChecked())
            _database->undoStack->push(new RemoveRailPoint(isection.strajectory, isection.trackpoint));
        else
            toggle(isection.trackpoint);
    }
    else if (ui->trajAddPButt->isChecked() && isection.strajectory)
    {
        auto point = route::RailPoint::create(_database->getStdAxis(), _database->getStdWireBox(), isection.worldIntersection);
        _database->undoStack->push(new AddRailPoint(isection.strajectory, point));
    }

    updateData();
}

void RailsPointEditor::toggle(route::RailPoint *object)
{
    if(auto selectedIt = _selectedObjects.find(object); selectedIt != _selectedObjects.end())
    {
        auto selected = *selectedIt;
        selected->setSelection(false);

        _selectedObjects.erase(selectedIt);
    }
    else
    {
        object->setSelection(true);
        _selectedObjects.insert(object);
    }
}
void RailsPointEditor::clear()
{
    for (auto &object : _selectedObjects)
    {
        object->setSelection(false);
    }
    _selectedObjects.clear();
}

void RailsPointEditor::setSpinEanbled(bool enabled)
{
    ui->inclSpin->setEnabled(enabled);
    ui->tangSpin->setEnabled(enabled);
    ui->tiltSpin->setEnabled(enabled);
    ui->cheightSpin->setEnabled(enabled);
}

void RailsPointEditor::updateData()
{
    QSignalBlocker l1(ui->inclSpin);
    QSignalBlocker l2(ui->tangSpin);
    QSignalBlocker l3(ui->tiltSpin);
    QSignalBlocker l4(ui->cheightSpin);

    setSpinEanbled(!_selectedObjects.isEmpty());

    if(_selectedObjects.isEmpty())
        return;

    auto point = *_selectedObjects.begin();

    auto quat = point->_quat;
    double sinr_cosp = 2 * (quat.w * quat.x + quat.y * quat.z);
    double cosr_cosp = 1 - 2 * (quat.x * quat.x + quat.y * quat.y);
    auto rot = std::atan2(sinr_cosp, cosr_cosp) * 1000.0;

    ui->inclSpin->setValue(rot);
    ui->tangSpin->setValue(point->_tangent);
    ui->tiltSpin->setValue(point->_tilt);
    ui->cheightSpin->setValue(point->_cheight);
}

void RailsPointEditor::clearSelection()
{
    clear();
    updateData();
}
