#ifndef ADDRAILS_H
#define ADDRAILS_H

#include "tool.h"

namespace Ui {
class AddRails;
}

struct attrib_t;

class AddRails : public Tool
{
    Q_OBJECT

public:
    explicit AddRails(DatabaseManager *database, QString root, QWidget *parent = nullptr);
    ~AddRails();

    void intersection(const FoundNodes& isection) override;

signals:
    void sendMovingPoint(route::SceneObject *object);
    void startMoving();

private:
    //tinyobj::ObjReader loadObj(std::string path);

    Ui::AddRails *ui;

    std::vector<vsg::vec3> _geometry;
    vsg::ref_ptr<vsg::Node> _sleeper;

    QFileSystemModel *_fsmodel;
};

#endif // ADDRAILS_H
