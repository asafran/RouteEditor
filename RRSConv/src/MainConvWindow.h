#pragma once

#include <QMainWindow>
#include <vsg/all.h>

#include <vsgXchange/all.h>

#include <QtWidgets/QApplication>
#include <QtWidgets/QMainWindow>
#include <vsgQt/ViewerWindow.h>

#include "animated-model.h"
#include "IntersectionHandler.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

/*
public slots:
    void openModel();
    void addObject();
    //void receiveData(vsg::ref_ptr<vsg::Data> buffer, vsg::ref_ptr<vsg::BufferInfo> info);
*/
private:
    QWindow* initilizeVSGwindow();
    QWidget *embedded;

    vsg::ref_ptr<IntersectionHandler> handler;

    void constructWidgets();

    Ui::MainWindow *ui;
    vsg::ref_ptr<vsg::Group> scene;
    vsg::ref_ptr<AnimatedModel> model;
    vsg::ref_ptr<vsg::Options> options;
    vsgQt::ViewerWindow *viewerWindow;

};
