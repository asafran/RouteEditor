#ifndef SIGMANAGER_H
#define SIGMANAGER_H

#include "tool.h"
#include <QFileSystemModel>
#include <QItemSelectionModel>

namespace Ui {
class SignalManager;
}

class SignalManager : public Tool
{
    Q_OBJECT
public:
    SignalManager(DatabaseManager *database, QString root, QWidget *parent = nullptr);
    virtual ~SignalManager();

    void intersection(const FoundNodes& isection) override;

private:
    //bool addToTrack(vsg::ref_ptr<vsg::Node> node, const FindNode &isection);
    enum Types
    {
        Auto,
        StRepeater,
        Routing,
        RoutingV2,
        Sh,
        Sh2
    };

    Ui::SignalManager *ui;

    QDir modelsDir;

    QFileSystemModel *_fsmodel;
};

#endif // SIGMANAGER_H
