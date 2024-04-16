#pragma once

#include <QMainWindow>
#include <QGridLayout>
#include <QSplitter>
#include <QTreeView>
#include "ContentManager.h"
#include "DatabaseManager.h"
#include "Manipulator.h"
#include <vsgQt/Viewer.h>
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
    vsgQt::Window* createWindow();
    QWidget *_embedded;

    void constructWidgets();

    void initializeTools();

    Ui::MainWindow *ui;

    ObjectPropertiesEditor *_objectsPrpEditor;
    ContentManager *_contentManager;
    RailsPointEditor *_railsPointEditor;
    AddRails *_railsManager;
    Painter *_painter;

    vsg::ref_ptr<vsgQt::Viewer> _viewer;

    DatabaseManager *_database;

    TilesSorter *_sorter;
    QToolBox *_toolbox;
    QUndoView *_undoView;

};
