#ifndef ADDRAILS_H
#define ADDRAILS_H

#include "tool.h"

namespace Ui {
class AddRails;
}

class AddRails : public Tool
{
    Q_OBJECT

public:
    explicit AddRails(DatabaseManager *database, QString root, QWidget *parent = nullptr);
    ~AddRails();

    void intersection(const FindNode& isection) override;

private:
    Ui::AddRails *ui;

    std::vector<vsg::vec3> _geometry;
    vsg::ref_ptr<vsg::Node> _sleeper;

    QFileSystemModel *_fsmodel;
};

#endif // ADDRAILS_H