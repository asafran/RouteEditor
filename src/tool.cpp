#include "tool.h"

Tool::Tool(DatabaseManager *database, QWidget *parent) : QWidget(parent)
  , _database(database)
{

}
Tool::~Tool() {}
/*
void Tool::setCamera(vsg::ref_ptr<vsg::Camera> camera)
{
    _camera = camera;
}*/
