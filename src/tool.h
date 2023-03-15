#ifndef TOOL_H
#define TOOL_H

#include <QWidget>
#include "DatabaseManager.h"
#include <vsg/app/Camera.h>

class Tool : public QWidget, public vsg::Visitor
{
    Q_OBJECT
public:
    explicit Tool(DatabaseManager *database, QWidget *parent = nullptr);
    virtual ~Tool();

    void setCamera(vsg::ref_ptr<vsg::Camera> camera);

signals:
    void sendStatusText(const QString &message, int timeout);

protected:
    DatabaseManager *_database;
    vsg::ref_ptr<vsg::Camera> _camera;
};

#endif // TOOL_H
