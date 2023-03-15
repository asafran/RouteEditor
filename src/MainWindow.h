#pragma once

#include <QMainWindow>
#include <QGridLayout>
#include <QSplitter>
#include <QTreeView>
#include "ContentManager.h"
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
#include "Painter.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(vsg::ref_ptr<DatabaseManager> dbm, QWidget *parent = nullptr);
    ~MainWindow();

private:
    QWindow* initilizeVSGwindow();
    QWidget *_embedded;

    void constructWidgets();

    void initializeTools();

    Ui::MainWindow *ui;

    ObjectPropertiesEditor *_objectsPrpEditor;
    ContentManager *_contentManager;
    RailsPointEditor *_railsPointEditor;
    AddRails *_railsManager;
    Painter *_painter;

    vsgQt::ViewerWindow *_viewerWindow;

    DatabaseManager *_database;

    TilesSorter *_sorter;
    QToolBox *_toolbox;
    QUndoView *_undoView;

};
