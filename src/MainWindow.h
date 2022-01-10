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
#include <vsg/all.h>
#include <QToolBox>
#include "TilesSorter.h"
#include "ObjectPropertiesEditor.h"

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
    void intersection(const FindNode& isection);

private:
    QWindow* initilizeVSGwindow();
    QWidget *embedded;

    void constructWidgets();

    void initializeTools();

    Ui::MainWindow *ui;
    vsg::ref_ptr<vsg::Builder> builder;
    ObjectPropertiesEditor *ope;

    double horizonMountainHeight;
    vsgQt::ViewerWindow *viewerWindow;
    DatabaseManager *database;

    QString pathDB;

    TilesSorter *sorter;

    QToolBox *toolbox;

    QUndoStack *undoStack;
    QUndoView *undoView;

};
