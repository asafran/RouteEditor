#include "AddRails.h"
#include "ui_AddRails.h"

AddRails::AddRails(DatabaseManager *database, QWidget *parent) : Tool(database, parent)
    , ui(new Ui::AddRails)
{
    ui->setupUi(this);
}

AddRails::~AddRails()
{
    delete ui;
}
/*
void AddRails::intersection(const FindNode &isection)
{
    auto path = _fsmodel->filePath(activeFile).toStdString();
    auto node = vsg::read_cast<vsg::Node>(path);
    _database->compile(node);

    if(addToTrack(node, isection))
        return;

    QModelIndex activeGroup = findGroup(isection);
    vsg::dmat4 wtl;
    auto world = isection.worldIntersection;

    if(loadToSelected || (_activeGroup.isValid() && !activeGroup.isValid()))
    {
        activeGroup = _activeGroup;
        auto group = static_cast<vsg::Node*>(_activeGroup.internalPointer());

        ParentVisitor pv(group);
        _database->getRoot()->accept(pv);
        pv.pathToChild.pop_back();
        wtl = vsg::inverse(vsg::computeTransform(pv.pathToChild));
    } else if(!_activeGroup.isValid() && !activeGroup.isValid())
        return;

    vsg::ref_ptr<route::SceneObject> obj;
    auto norm = vsg::normalize(world);
    vsg::dquat quat(vsg::dvec3(0.0, 0.0, 1.0), norm);

    if(ui->useLinks->isChecked())
        obj = route::SingleLoader::create(node, path, wtl * world, quat, wtl);
    else
        obj = route::SceneObject::create(node, wtl * world, quat, wtl);
    _database->push(new AddSceneObject(_database->getTilesModel(), activeGroup, obj));
    emit sendStatusText(tr("Добавлен объект %1").arg(path.c_str()), 2000);
}*/
