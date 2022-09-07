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
#include "RailsPointEditor.h"
#include "AddRails.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(vsg::ref_ptr<DatabaseManager> dbm, QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void intersection(const FoundNodes& isection);

private:
    QWindow* initilizeVSGwindow();
    QWidget *embedded;

    void constructWidgets();

    void initializeTools();

    Ui::MainWindow *ui;

    ObjectPropertiesEditor *ope;

    AddRails *rm;

    double horizonMountainHeight;
    vsgQt::ViewerWindow *viewerWindow;
    DatabaseManager *database;

    QString pathDB;

    TilesSorter *sorter;

    QToolBox *toolbox;

    QUndoView *undoView;

};
