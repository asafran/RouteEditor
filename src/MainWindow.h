#pragma once

#include <QMainWindow>
#include <QGridLayout>
#include <QSplitter>
#include <QTreeView>
#include "DatabaseManager.h"
#include "Manipulator.h"
#include <vsgQt/ViewerWindow.h>
#include <QUndoView>
#include <QRegularExpression>
#include "ObjectModel.h"
#include "tilessorter.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QString routePath, QString skybox, QWidget *parent = nullptr);
    ~MainWindow();

public slots:
//    void addToRoot(vsg::ref_ptr<vsg::Node> node);
//    void setTilesModel(SceneModel *model);
//    void openRoute();
    void addObject();
    void pushCommand(QUndoCommand *command);
    //void receiveData(vsg::ref_ptr<vsg::Data> buffer, vsg::ref_ptr<vsg::BufferInfo> info);

private:
    QWindow* initilizeVSGwindow();
    QWidget *embedded;

    void constructWidgets();

    //DatabaseManager *openDialog();

    Ui::MainWindow *ui;
    vsg::ref_ptr<vsg::Group> scene;
    //vsg::ref_ptr<vsg::Options> options;
    vsg::ref_ptr<vsg::Builder> builder;
    double horizonMountainHeight;
    vsgQt::ViewerWindow *viewerWindow;
    DatabaseManager *database;
    //vsg::ref_ptr<Manipulator> manipulator;

    //QFileSystemModel *fsmodel;
    TilesSorter *sorter;

    QUndoStack *undoStack;
    QUndoView *undoView;

};
