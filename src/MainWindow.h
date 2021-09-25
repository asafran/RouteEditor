#pragma once

#include <QMainWindow>
#include <QGridLayout>
#include <QSplitter>
#include <QTreeView>
#include "databasemanager.h"
#include <vsgQt/ViewerWindow.h>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void addToRoot(vsg::Node *node);
//    void setTilesModel(SceneModel *model);
    void openRoute();

private:
    QWindow* initilizeVSGwindow();

    void constructWidgets();

    Ui::MainWindow *ui;
    vsg::ref_ptr<vsg::Group> scene;
    vsg::ref_ptr<vsg::Options> options;
    vsg::ref_ptr<vsg::Builder> builder;
    double horizonMountainHeight;
    vsgQt::ViewerWindow *viewerWindow;
    QScopedPointer<DatabaseManager> database;

};
