#pragma once

#include <QMainWindow>
#include <vsg/all.h>

#include <vsgXchange/all.h>

#include <QtWidgets/QApplication>
#include <QtWidgets/QMainWindow>
#include <vsgQt/ViewerWindow.h>

#include "sceneobjects.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void generateTrackFile(vsg::ref_ptr<Track> track);
/*
public slots:
    void openModel();
    void addObject();
    //void receiveData(vsg::ref_ptr<vsg::Data> buffer, vsg::ref_ptr<vsg::BufferInfo> info);
*/
private:
    QWindow* initilizeVSGwindow();
    QWidget *embedded;

    void constructWidgets();

    Ui::MainWindow *ui;
    vsg::ref_ptr<vsg::Node> scene;
    vsg::ref_ptr<vsg::Options> options;
    //vsg::ref_ptr<vsg::Builder> builder;
    vsgQt::ViewerWindow *viewerWindow;

};
