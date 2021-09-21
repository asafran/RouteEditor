#pragma once

#include <QMainWindow>
#include <QGridLayout>
#include <QSplitter>
#include <vsg/all.h>
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
    void loadToRoot(QString &filename);

private:
    QWindow* initilizeVSGwindow();



    Ui::MainWindow *ui;
    QSplitter *centralsplitter;
    vsg::ref_ptr<vsg::Group> scene;
    vsg::ref_ptr<vsg::Options> options;
    vsg::ref_ptr<vsg::Builder> builder;
    vsg::ref_ptr<vsg::Trackball> trackball;
    double horizonMountainHeight;
    vsgQt::ViewerWindow *viewerWindow;

};
