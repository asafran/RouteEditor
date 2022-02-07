#include "tool.h"

Tool::Tool(DatabaseManager *database, QWidget *parent) : QWidget(parent)
  , _database(database)
{

}
Tool::~Tool() {}
