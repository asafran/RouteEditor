#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "VSGViewer.h"
#include "SceneModel.h"

#include <QFileDialog>
#include <QColorDialog>
#include <QTreeView>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    auto window = new VSGViewer();

    auto widget = QWidget::createWindowContainer(window, this);

    QWidget *centralWidget = new QWidget( this );
    centralWidget->setObjectName("centralWidget");
    setCentralWidget(centralWidget);

    QGridLayout *baseWidgetLayout = new QGridLayout( centralWidget );
    baseWidgetLayout->setSpacing(0);
    baseWidgetLayout->setContentsMargins(0, 0, 0, 0);
    baseWidgetLayout->setObjectName("gridLayout");

    //treeview->setItemDelegateForColumn(0, )
    centralsplitter = new QSplitter( this );
    auto analyzers = new QTabWidget(centralsplitter);
    auto scenetree = new QTreeView(analyzers);
    scenetree->setAllColumnsShowFocus(true);
    auto tilestree = new QTreeView(analyzers);
    tilestree->setAllColumnsShowFocus(true);
    analyzers->addTab(scenetree, "Scene");
    analyzers->addTab(tilestree, "Objects");

    centralsplitter->setObjectName("Centralsplitter");
    centralsplitter->setOrientation( Qt::Horizontal );
    centralsplitter->addWidget( widget );
    centralsplitter->addWidget( analyzers );

    baseWidgetLayout->addWidget( centralsplitter, 0, 0 );

    QList<int> sizes;
    sizes << 350 << 500;
    centralsplitter->setSizes( sizes );

    connect(ui->actionOpen, &QAction::triggered, this, [=]() {
        if (const auto filename = QFileDialog::getOpenFileName(this, tr("Open file"), nullptr, "VSG bin files (*.vsgb);;All files (*.*)"); !filename.isEmpty())
            window->loadToRoot(filename);
    });

    connect(window, &VSGViewer::initialized, this, [=]() {
        auto scenemodel = window->getSceneModel();
        scenetree->setModel(scenemodel);
        scenetree->setRootIndex(scenemodel->index(0,0));
    });

}

MainWindow::~MainWindow()
{
    delete ui;
}

