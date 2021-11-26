#include "MainConvWindow.h"
#include <QErrorMessage>

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    MainWindow window;
    window.show();

    return a.exec();
}
