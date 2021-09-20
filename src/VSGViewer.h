#pragma once

#include "SceneModel.h"
#include <QWindow>
#include <vsg/nodes/Node.h>

namespace vsg {
class Viewer;
class Window;
class StateGroup;
}

namespace vsgQt {
class Window;

}

namespace vsg {
class Instance;
}

class VSGViewer : public QWindow
{
    Q_OBJECT
public:

    VSGViewer();
    virtual ~VSGViewer() override;

    bool loadFile(const QString &filename, vsg::ref_ptr<vsg::Group> parent);
    SceneModel* getSceneModel();

    vsg::Instance* instance();

public slots:
    void loadToRoot(const QString &filename);

signals:
    void initialized();
    void tilesLoaded(const SceneModel& tiles);

protected:

    void render();

    void exposeEvent(QExposeEvent *e) override;
    bool event(QEvent *e) override;
    void keyPressEvent(QKeyEvent *) override;
    void keyReleaseEvent(QKeyEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void resizeEvent(QResizeEvent *) override;
    void moveEvent(QMoveEvent *) override;
    void wheelEvent(QWheelEvent *) override;

private:
    struct Private;
    QScopedPointer<Private> p;
    vsg::ref_ptr<vsg::Node> cursorTile;
};
